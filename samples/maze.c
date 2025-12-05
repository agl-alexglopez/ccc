/* Author: Alexander Lopez
This file provides a simple maze builder that implements Prim's algorithm
to randomly generate a maze. I chose this algorithm because it can use
both a map and a priority queue to achieve its purpose. Such data structures are
provided by the library offering a perfect sample program opportunity.

Maze Builder:
Builds a Perfect Maze with Prim's Algorithm to demonstrate usage of the priority
queue and map provided by this library.
Usage:
-r=N The row flag lets you specify maze rows > 7.
-c=N The col flag lets you specify maze cols > 7.
-s=N The speed flag lets you specify the speed of the animation 0-7.
Example:
./build/[debug/]bin/maze -c=111 -r=33 -s=4 */
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC

#include "ccc/flat_hash_map.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "utility/allocate.h"
#include "utility/cli.h"
#include "utility/random.h"
#include "utility/string_view/string_view.h"

/*=======================   Maze Helper Types   =============================*/

enum Animation_speed
{
    INSTANT = 0,
    SPEED_1,
    SPEED_2,
    SPEED_3,
    SPEED_4,
    SPEED_5,
    SPEED_6,
    SPEED_7,
};

struct Point
{
    int r;
    int c;
};

struct Maze
{
    int rows;
    int cols;
    enum Animation_speed speed;
    uint16_t *maze;
};

/*===================  Prim's Algorithm Helper Types   ======================*/

struct Prim_cell
{
    struct Point cell;
    int cost;
};

/*======================   Maze Constants   =================================*/

/** Using these half Unicode squares for now because they create symmetrical
mazes. A terminal cell is about 2x as tall as it is wide so many ways to
produce a maze result in vertically stretched paths/walls. These are
symmetrical but require a little more logic to print to the screen because
there are two for every cell and the terminal cannot help us with that. Notably
when printing to a position on the screen you need to halve the position because
there are two of these mini squares per cell of the terminal. */
static char const *const walls[16] = {
    "▀", "▀", "▀", "▀", "█", "█", "█", "█",
    "▀", "▀", "▀", "▀", "█", "█", "█", "█",
};

static int const speeds[] = {
    0, 5000000, 2500000, 1000000, 500000, 250000, 100000, 1000,
};

struct Point const dir_offsets[] = {{-2, 0}, {0, 2}, {2, 0}, {0, -2}};

enum : size_t
{
    DIR_OFFSETS_SIZE = sizeof(dir_offsets) / sizeof(dir_offsets[0]),
};

enum : int
{
    DEFAULT_ROWS = 66,
    DEFAULT_COLS = 111,
    ROW_COL_MIN = 7,
};

enum : uint16_t
{
    PATH_BIT = 0b0010000000000000,
    WALL_MASK = 0b1111,
    FLOAT_WALL = 0b0000,
    NORTH_WALL = 0b0001,
    EAST_WALL = 0b0010,
    SOUTH_WALL = 0b0100,
    WEST_WALL = 0b1000,
    CACHE_BIT = 0b0001000000000000,
};

SV_String_view const row_flag = SV("-r=");
SV_String_view const col_flag = SV("-c=");
SV_String_view const speed_flag = SV("-s=");
SV_String_view const help_flag = SV("-h");
SV_String_view const escape = SV("\033[");
SV_String_view const semi_colon = SV(";");
SV_String_view const cursor_pos_specifier = SV("f");

/*==========================   Prototypes  ================================= */

#define check(cond, ...)                                                       \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            __VA_OPT__(__VA_ARGS__)                                            \
            printf("%s, %d, condition is false: %s\n", __FILE__, __LINE__,     \
                   #cond);                                                     \
            exit(1);                                                           \
        }                                                                      \
    }                                                                          \
    while (0)

static void animate_maze(struct Maze *);
static void fill_maze_with_walls(struct Maze *);
static void build_wall(struct Maze *, int r, int c);
static void print_square(struct Maze const *, int r, int c);
static uint16_t *maze_at_wrap(struct Maze const *, int r, int c);
static uint16_t maze_at(struct Maze const *, int r, int c);
static void clear_and_flush_maze(struct Maze const *);
static void carve_path_walls_animated(struct Maze *, struct Point, int);
static void join_squares_animated(struct Maze *, struct Point, struct Point,
                                  int);
static void flush_cursor_maze_coordinate(struct Maze const *, int r, int c);
static bool can_build_new_square(struct Maze const *, int r, int c);
static void help(void);
static struct Point rand_point(struct Maze const *);
static Order order_prim_cells(Type_comparator_context);
static struct Int_conversion parse_digits(SV_String_view);
static CCC_Order prim_cell_order(Key_comparator_context);
static uint64_t prim_cell_hash_fn(Key_context);
static uint64_t hash_64_bits(uint64_t);

/*======================  Main Arg Handling  ===============================*/

int
main(int argc, char **argv)
{
    /* Randomness will be used throughout the program but it need not be
       perfect. It only helps build the maze.
       NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp) */
    srand(time(NULL));
    struct Maze maze = {
        .rows = DEFAULT_ROWS,
        .cols = DEFAULT_COLS,
        .speed = SPEED_4,
        .maze = NULL,
    };
    for (int i = 1; i < argc; ++i)
    {
        SV_String_view const arg = SV_sv(argv[i]);
        if (SV_starts_with(arg, row_flag))
        {
            struct Int_conversion const row_arg = parse_digits(arg);
            if (row_arg.status == CONV_ER || row_arg.conversion < ROW_COL_MIN)
            {
                quit("rows below required minimum or negative.\n", 1);
                return 1;
            }
            maze.rows = row_arg.conversion;
        }
        else if (SV_starts_with(arg, col_flag))
        {
            struct Int_conversion const col_arg = parse_digits(arg);
            if (col_arg.status == CONV_ER || col_arg.conversion < ROW_COL_MIN)
            {
                quit("cols below required minimum or negative.\n", 1);
                return 1;
            }
            maze.cols = col_arg.conversion;
        }
        else if (SV_starts_with(arg, speed_flag))
        {
            struct Int_conversion const speed_arg = parse_digits(arg);
            if (speed_arg.status == CONV_ER || speed_arg.conversion > SPEED_7
                || speed_arg.conversion < 0)
            {
                quit("speed outside of valid range.\n", 1);
                return 1;
            }
            maze.speed = speed_arg.conversion;
        }
        else if (SV_starts_with(arg, help_flag))
        {
            help();
            return 0;
        }
        else
        {
            quit("can only specify rows, columns, or speed "
                 "for now (-r=N, -c=N, -s=N)\n",
                 1);
        }
    }
    /* This type of maze generation requires odd rows and cols. */
    maze.rows = maze.rows + (maze.rows % 2 == 0);
    maze.cols = maze.cols + (maze.cols % 2 == 0);
    maze.maze = calloc((size_t)maze.rows * maze.cols, sizeof(uint16_t));
    if (!maze.maze)
    {
        (void)fprintf(stderr, "allocation failure for specified maze size.\n");
        return 1;
    }
    animate_maze(&maze);
    /* The squares are mini so there are 2 per row. Halve the position. */
    set_cursor_position((maze.rows / 2) + 1, maze.cols + 1);
    printf("\n");
}

/*======================      Maze Animation      ===========================*/

/* This function presents a very traditional implementation of an algorithm
that requires multiple containers. This is implemented in a manner that one
would normally see in Rust or C++, where each container manages its own memory.
For examples of non-owning container use and composition see other samples.
This style of data structure use can be comfortable and convenient in some
cases.

However, we can still be "C" about it by thinking about reserving exactly
the memory we need dynamically and giving no ability to the container to re-size
later. In this way we exercise exact control over memory use of our program. */
static void
animate_maze(struct Maze *maze)
{
    int const speed = speeds[maze->speed];
    fill_maze_with_walls(maze);
    clear_and_flush_maze(maze);
    size_t const capacity = ((maze->rows * maze->cols) / 2) + 1;
    struct Prim_cell const start = (struct Prim_cell){
        .cell = rand_point(maze),
        .cost = rand_range(0, 100),
    };
    Flat_hash_map cost_map = CCC_flat_hash_map_from(
        cell, prim_cell_hash_fn, prim_cell_order, std_allocate, NULL, capacity,
        (struct Prim_cell[]){
            start,
        });
    /* Priority queue gets same reserve interface. */
    Flat_priority_queue cell_priority_queue = flat_priority_queue_from(
        CCC_ORDER_LESSER, order_prim_cells, std_allocate, NULL, capacity,
        (struct Prim_cell[]){
            start,
        });
    check(!is_empty(&cell_priority_queue) && !is_empty(&cost_map));
    while (!is_empty(&cell_priority_queue))
    {
        struct Prim_cell const *const c = front(&cell_priority_queue);
        *maze_at_wrap(maze, c->cell.r, c->cell.c) |= CACHE_BIT;
        /* 0 is an invalid row and column in any maze. */
        struct Prim_cell min_cell = {};
        int min = INT_MAX;
        for (size_t i = 0; i < DIR_OFFSETS_SIZE; ++i)
        {
            struct Point const n = {
                .r = c->cell.r + dir_offsets[i].r,
                .c = c->cell.c + dir_offsets[i].c,
            };
            if (can_build_new_square(maze, n.r, n.c))
            {
                struct Prim_cell const *const cell
                    = flat_hash_map_or_insert_with(
                        entry_wrap(&cost_map, &n),
                        (struct Prim_cell){
                            .cell = n,
                            .cost = rand_range(0, 100),
                        });
                check(cell);
                if (cell->cost < min)
                {
                    min = cell->cost;
                    min_cell = *cell;
                }
            }
        }
        if (min != INT_MAX)
        {
            join_squares_animated(maze, c->cell, min_cell.cell, speed);
            (void)push(&cell_priority_queue, &min_cell, &(struct Prim_cell){});
        }
        else
        {
            (void)pop(&cell_priority_queue, &(struct Prim_cell){});
        }
    }
    (void)clear_and_free(&cost_map, NULL);
    (void)clear_and_free(&cell_priority_queue, NULL);
}

/*===================     Container Support Code     ========================*/

static CCC_Order
prim_cell_order(Key_comparator_context const c)
{
    struct Point const *const left = c.key_left;
    struct Prim_cell const *const right = c.type_right;
    CCC_Order const order
        = (left->r < right->cell.r) - (left->r > right->cell.r);
    if (order != CCC_ORDER_EQUAL)
    {
        return order;
    }
    return (left->c > right->cell.c) - (left->c < right->cell.c);
}

static uint64_t
prim_cell_hash_fn(Key_context const k)
{
    struct Prim_cell const *const p = k.key;
    uint64_t const wr = p->cell.r;
    static_assert(sizeof((struct Point){}.r) * CHAR_BIT == 32,
                  "hashing a row and column requires shifting half the size of "
                  "the hash code");
    return hash_64_bits((wr << 31) | p->cell.c);
}

static inline uint64_t
hash_64_bits(uint64_t x)
{
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

static Order
order_prim_cells(Type_comparator_context const order_cells)
{
    struct Prim_cell const *const left = order_cells.type_left;
    struct Prim_cell const *const right = order_cells.type_right;
    return (left->cost > right->cost) - (left->cost < right->cost);
}

/*=========================   Maze Support Code   ===========================*/

static struct Point
rand_point(struct Maze const *const maze)
{
    return (struct Point){
        .r = (2 * rand_range(1, (maze->rows - 2) / 2)) + 1,
        .c = (2 * rand_range(1, (maze->cols - 2) / 2)) + 1,
    };
}

static void
fill_maze_with_walls(struct Maze *maze)
{
    for (int row = 0; row < maze->rows; ++row)
    {
        for (int col = 0; col < maze->cols; ++col)
        {
            build_wall(maze, row, col);
        }
    }
}

static void
clear_and_flush_maze(struct Maze const *const maze)
{
    clear_screen();
    for (int row = 0; row < maze->rows; ++row)
    {
        for (int col = 0; col < maze->cols; ++col)
        {
            flush_cursor_maze_coordinate(maze, row, col);
        }
        printf("\n");
    }
    (void)fflush(stdout);
}

static void
join_squares_animated(struct Maze *maze, struct Point const cur,
                      struct Point const next, int const time)
{
    struct Point wall = cur;
    if (next.r < cur.r)
    {
        wall.r--;
    }
    else if (next.r > cur.r)
    {
        wall.r++;
    }
    else if (next.c < cur.c)
    {
        wall.c--;
    }
    else if (next.c > cur.c)
    {
        wall.c++;
    }
    else
    {
        printf("Wall break error. Step through wall didn't work\n");
    }
    carve_path_walls_animated(maze, cur, time);
    carve_path_walls_animated(maze, wall, time);
    carve_path_walls_animated(maze, next, time);
}

static void
carve_path_walls_animated(struct Maze *maze, struct Point const p,
                          int const time)
{
    *maze_at_wrap(maze, p.r, p.c) |= PATH_BIT;
    flush_cursor_maze_coordinate(maze, p.r, p.c);
    struct timespec ts = {.tv_sec = 0, .tv_nsec = time};
    nanosleep(&ts, NULL);
    if (p.r - 1 >= 0 && !(maze_at(maze, p.r - 1, p.c) & PATH_BIT))
    {
        *maze_at_wrap(maze, p.r - 1, p.c) &= ~SOUTH_WALL;
        flush_cursor_maze_coordinate(maze, p.r - 1, p.c);
        nanosleep(&ts, NULL);
    }
    if (p.r + 1 < maze->rows && !(maze_at(maze, p.r + 1, p.c) & PATH_BIT))
    {
        *maze_at_wrap(maze, p.r + 1, p.c) &= ~NORTH_WALL;
        flush_cursor_maze_coordinate(maze, p.r + 1, p.c);
        nanosleep(&ts, NULL);
    }
    if (p.c - 1 >= 0 && !(maze_at(maze, p.r, p.c - 1) & PATH_BIT))
    {
        *maze_at_wrap(maze, p.r, p.c - 1) &= ~EAST_WALL;
        flush_cursor_maze_coordinate(maze, p.r, p.c - 1);
        nanosleep(&ts, NULL);
    }
    if (p.c + 1 < maze->cols && !(maze_at(maze, p.r, p.c + 1) & PATH_BIT))
    {
        *maze_at_wrap(maze, p.r, p.c + 1) &= ~WEST_WALL;
        flush_cursor_maze_coordinate(maze, p.r, p.c + 1);
        nanosleep(&ts, NULL);
    }
    *maze_at_wrap(maze, p.r, p.c) |= CACHE_BIT;
}

static void
build_wall(struct Maze *m, int const r, int const c)
{
    uint16_t wall = 0;
    if (r - 1 >= 0)
    {
        wall |= NORTH_WALL;
    }
    if (r + 1 < m->rows)
    {
        wall |= SOUTH_WALL;
    }
    if (c - 1 >= 0)
    {
        wall |= WEST_WALL;
    }
    if (c + 1 < m->cols)
    {
        wall |= EAST_WALL;
    }
    *maze_at_wrap(m, r, c) |= wall;
    *maze_at_wrap(m, r, c) &= ~PATH_BIT;
}

static void
flush_cursor_maze_coordinate(struct Maze const *maze, int const r, int const c)
{
    set_cursor_position(r / 2, c);
    print_square(maze, r, c);
    (void)fflush(stdout);
}

static void
print_square(struct Maze const *m, int const r, int const c)
{
    uint16_t const square = maze_at(m, r, c);
    if (!(square & PATH_BIT))
    {
        if (r % 2)
        {
            printf("%s", walls[maze_at(m, r - 1, c) & WALL_MASK]);
        }
        else
        {
            printf("%s", walls[square & WALL_MASK]);
        }
    }
    else if (square & PATH_BIT)
    {
        if (r % 2)
        {
            if (r > 0 && !(maze_at(m, r - 1, c) & PATH_BIT))
            {
                printf("▀");
            }
            else
            {
                printf(" ");
            }
        }
        else
        {
            if (!(maze_at(m, r + 1, c) & PATH_BIT))
            {
                printf("▄");
            }
            else
            {
                printf(" ");
            }
        }
    }
    else
    {
        (void)fprintf(stderr, "uncategorizable square.\n");
    }
}

/** Square by mutable reference. */
static uint16_t *
maze_at_wrap(struct Maze const *const maze, int const r, int const c)
{
    return &maze->maze[(r * maze->cols) + c];
}

/** Square by value. */
static uint16_t
maze_at(struct Maze const *const maze, int const r, int const c)
{
    return maze->maze[(r * maze->cols) + c];
}

/** Can't build if square has been seen/cached. */
static bool
can_build_new_square(struct Maze const *const maze, int const r, int const c)
{
    return r > 0 && r < maze->rows - 1 && c > 0 && c < maze->cols - 1
        && !(maze_at(maze, r, c) & CACHE_BIT);
}

/*===========================    Misc    ====================================*/

static struct Int_conversion
parse_digits(SV_String_view arg)
{
    size_t const eql = SV_rfind(arg, SV_npos(arg), SV("="));
    if (eql == SV_npos(arg))
    {
        return (struct Int_conversion){.status = CONV_ER};
    }
    arg = SV_substr(arg, eql, ULLONG_MAX);
    if (SV_empty(arg))
    {
        (void)fprintf(stderr, "please specify element to convert.\n");
        return (struct Int_conversion){.status = CONV_ER};
    }
    arg = SV_remove_prefix(arg, 1);
    return convert_to_int(SV_begin(arg));
}

static void
help(void)
{
    (void)fprintf(
        stdout,
        "Maze Builder:\nBuilds a Perfect Maze with Prim's "
        "Algorithm to demonstrate usage of the priority "
        "queue and adaptive_map provided by this library.\nUsage:\n-r=N The "
        "row flag lets you specify maze rows > 7.\n-c=N The col flag "
        "lets you specify maze cols > 7.\n-s=N The speed flag lets "
        "you specify the speed of the animation "
        "0-7.\nExample:\n./build/[debug/]bin/maze -c=111 -r=33 -s=4\n");
}
