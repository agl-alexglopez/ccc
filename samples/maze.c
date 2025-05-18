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
#include "alloc.h"
#include "ccc/flat_hash_map.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "cli.h"
#include "random.h"
#include "str_view.h"

/*=======================   Maze Helper Types   =============================*/

enum animation_speed
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

struct point
{
    int r;
    int c;
};

struct maze
{
    int rows;
    int cols;
    enum animation_speed speed;
    uint16_t *maze;
};

/*===================  Prim's Algorithm Helper Types   ======================*/

struct prim_cell
{
    struct point cell;
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
char const *const walls[16] = {
    "▀", "▀", "▀", "▀", "█", "█", "█", "█",
    "▀", "▀", "▀", "▀", "█", "█", "█", "█",
};

int const speeds[] = {
    0, 5000000, 2500000, 1000000, 500000, 250000, 100000, 1000,
};

struct point const dir_offsets[] = {{-2, 0}, {0, 2}, {2, 0}, {0, -2}};
size_t const dir_offsets_size = sizeof(dir_offsets) / sizeof(dir_offsets[0]);

str_view const row_flag = SV("-r=");
str_view const col_flag = SV("-c=");
str_view const speed_flag = SV("-s=");
str_view const help_flag = SV("-h");
str_view const escape = SV("\033[");
str_view const semi_colon = SV(";");
str_view const cursor_pos_specifier = SV("f");
int const default_rows = 66;
int const default_cols = 111;
int const default_speed = 4;
int const row_col_min = 7;
int const speed_max = 7;

uint16_t const path_bit = 0b0010000000000000;
uint16_t const wall_mask = 0b1111;
uint16_t const floating_wall = 0b0000;
uint16_t const north_wall = 0b0001;
uint16_t const east_wall = 0b0010;
uint16_t const south_wall = 0b0100;
uint16_t const west_wall = 0b1000;
uint16_t const cached_bit = 0b0001000000000000;

/*==========================   Prototypes  ================================= */

#define prog_assert(cond, ...)                                                 \
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

static void animate_maze(struct maze *);
static void fill_maze_with_walls(struct maze *);
static void build_wall(struct maze *, int r, int c);
static void print_square(struct maze const *, int r, int c);
static uint16_t *maze_at_r(struct maze const *, int r, int c);
static uint16_t maze_at(struct maze const *, int r, int c);
static void clear_and_flush_maze(struct maze const *);
static void carve_path_walls_animated(struct maze *, struct point, int);
static void join_squares_animated(struct maze *, struct point, struct point,
                                  int);
static void flush_cursor_maze_coordinate(struct maze const *, int r, int c);
static bool can_build_new_square(struct maze const *, int r, int c);
static void help(void);
static struct point rand_point(struct maze const *);
static threeway_cmp cmp_prim_cells(any_type_cmp);
static struct int_conversion parse_digits(str_view);
static ccc_tribool prim_cell_eq(any_key_cmp);
static uint64_t prim_cell_hash_fn(any_key);
static uint64_t hash_64_bits(uint64_t);

/*======================  Main Arg Handling  ===============================*/

int
main(int argc, char **argv)
{
    /* Randomness will be used throughout the program but it need not be
       perfect. It only helps build the maze.
       NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp) */
    srand(time(NULL));
    struct maze maze = {.rows = default_rows,
                        .cols = default_cols,
                        .speed = default_speed,
                        .maze = NULL};
    for (int i = 1; i < argc; ++i)
    {
        str_view const arg = sv(argv[i]);
        if (sv_starts_with(arg, row_flag))
        {
            struct int_conversion const row_arg = parse_digits(arg);
            if (row_arg.status == CONV_ER || row_arg.conversion < row_col_min)
            {
                quit("rows below required minimum or negative.\n", 1);
                return 1;
            }
            maze.rows = row_arg.conversion;
        }
        else if (sv_starts_with(arg, col_flag))
        {
            struct int_conversion const col_arg = parse_digits(arg);
            if (col_arg.status == CONV_ER || col_arg.conversion < row_col_min)
            {
                quit("cols below required minimum or negative.\n", 1);
                return 1;
            }
            maze.cols = col_arg.conversion;
        }
        else if (sv_starts_with(arg, speed_flag))
        {
            struct int_conversion const speed_arg = parse_digits(arg);
            if (speed_arg.status == CONV_ER || speed_arg.conversion > speed_max
                || speed_arg.conversion < 0)
            {
                quit("speed outside of valid range.\n", 1);
                return 1;
            }
            maze.speed = speed_arg.conversion;
        }
        else if (sv_starts_with(arg, help_flag))
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
animate_maze(struct maze *maze)
{
    int const speed = speeds[maze->speed];
    fill_maze_with_walls(maze);
    clear_and_flush_maze(maze);
    /* Test use case of reserve without reallocation permission. Guarantees
       exactly the needed memory and no more over lifetime of program. */
    flat_hash_map cost_map
        = fhm_init(NULL, struct prim_cell, cell, prim_cell_hash_fn,
                   prim_cell_eq, NULL, NULL, 0);
    result r = fhm_reserve(&cost_map, ((maze->rows * maze->cols) / 2) + 1,
                           std_alloc);
    prog_assert(r == CCC_RESULT_OK);
    /* Priority queue gets same reserve interface. */
    flat_priority_queue cell_pq = fpq_init((struct prim_cell *)NULL, CCC_LES,
                                           cmp_prim_cells, NULL, NULL, 0);
    r = fpq_reserve(&cell_pq, ((maze->rows * maze->cols) / 2) + 1, std_alloc);
    prog_assert(r == CCC_RESULT_OK);
    struct point s = rand_point(maze);
    struct prim_cell const *const first = fhm_insert_entry_w(
        entry_r(&cost_map, &s),
        (struct prim_cell){.cell = s, .cost = rand_range(0, 100)});
    prog_assert(first);
    (void)push(&cell_pq, first);
    while (!is_empty(&cell_pq))
    {
        struct prim_cell const *const c = front(&cell_pq);
        *maze_at_r(maze, c->cell.r, c->cell.c) |= cached_bit;
        /* 0 is an invalid row and column in any maze. */
        struct prim_cell min_cell = {};
        int min = INT_MAX;
        for (size_t i = 0; i < dir_offsets_size; ++i)
        {
            struct point const n = {.r = c->cell.r + dir_offsets[i].r,
                                    .c = c->cell.c + dir_offsets[i].c};
            if (can_build_new_square(maze, n.r, n.c))
            {
                struct prim_cell const *const cell = fhm_or_insert_w(
                    entry_r(&cost_map, &n),
                    (struct prim_cell){.cell = n, .cost = rand_range(0, 100)});
                prog_assert(cell);
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
            (void)push(&cell_pq, &min_cell);
        }
        else
        {
            (void)pop(&cell_pq);
        }
    }
    /* If a container is reserved without allocation permission it has no way
       to free itself. Give it the same allocation function used to reserve. */
    (void)fhm_clear_and_free_reserve(&cost_map, NULL, std_alloc);
    (void)fpq_clear_and_free_reserve(&cell_pq, NULL, std_alloc);
}

/*===================     Container Support Code     ========================*/

static ccc_tribool
prim_cell_eq(any_key_cmp const c)
{
    struct point const *const lhs = c.any_key_lhs;
    struct prim_cell const *const rhs = c.any_type_rhs;
    return lhs->r == rhs->cell.r && lhs->c == rhs->cell.c;
}

static uint64_t
prim_cell_hash_fn(any_key const k)
{
    struct prim_cell const *const p = k.any_key;
    uint64_t const wr = p->cell.r;
    static_assert(sizeof((struct point){}.r) * CHAR_BIT == 32,
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

static threeway_cmp
cmp_prim_cells(any_type_cmp const cmp_cells)
{
    struct prim_cell const *const lhs = cmp_cells.any_type_lhs;
    struct prim_cell const *const rhs = cmp_cells.any_type_rhs;
    return (lhs->cost > rhs->cost) - (lhs->cost < rhs->cost);
}

/*=========================   Maze Support Code   ===========================*/

static struct point
rand_point(struct maze const *const maze)
{
    return (struct point){
        .r = (2 * rand_range(1, (maze->rows - 2) / 2)) + 1,
        .c = (2 * rand_range(1, (maze->cols - 2) / 2)) + 1,
    };
}

static void
fill_maze_with_walls(struct maze *maze)
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
clear_and_flush_maze(struct maze const *const maze)
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
join_squares_animated(struct maze *maze, struct point const cur,
                      struct point const next, int const time)
{
    struct point wall = cur;
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
carve_path_walls_animated(struct maze *maze, struct point const p,
                          int const time)
{
    *maze_at_r(maze, p.r, p.c) |= path_bit;
    flush_cursor_maze_coordinate(maze, p.r, p.c);
    struct timespec ts = {.tv_sec = 0, .tv_nsec = time};
    nanosleep(&ts, NULL);
    if (p.r - 1 >= 0 && !(maze_at(maze, p.r - 1, p.c) & path_bit))
    {
        *maze_at_r(maze, p.r - 1, p.c) &= ~south_wall;
        flush_cursor_maze_coordinate(maze, p.r - 1, p.c);
        nanosleep(&ts, NULL);
    }
    if (p.r + 1 < maze->rows && !(maze_at(maze, p.r + 1, p.c) & path_bit))
    {
        *maze_at_r(maze, p.r + 1, p.c) &= ~north_wall;
        flush_cursor_maze_coordinate(maze, p.r + 1, p.c);
        nanosleep(&ts, NULL);
    }
    if (p.c - 1 >= 0 && !(maze_at(maze, p.r, p.c - 1) & path_bit))
    {
        *maze_at_r(maze, p.r, p.c - 1) &= ~east_wall;
        flush_cursor_maze_coordinate(maze, p.r, p.c - 1);
        nanosleep(&ts, NULL);
    }
    if (p.c + 1 < maze->cols && !(maze_at(maze, p.r, p.c + 1) & path_bit))
    {
        *maze_at_r(maze, p.r, p.c + 1) &= ~west_wall;
        flush_cursor_maze_coordinate(maze, p.r, p.c + 1);
        nanosleep(&ts, NULL);
    }
    *maze_at_r(maze, p.r, p.c) |= cached_bit;
}

static void
build_wall(struct maze *m, int const r, int const c)
{
    uint16_t wall = 0;
    if (r - 1 >= 0)
    {
        wall |= north_wall;
    }
    if (r + 1 < m->rows)
    {
        wall |= south_wall;
    }
    if (c - 1 >= 0)
    {
        wall |= west_wall;
    }
    if (c + 1 < m->cols)
    {
        wall |= east_wall;
    }
    *maze_at_r(m, r, c) |= wall;
    *maze_at_r(m, r, c) &= ~path_bit;
}

static void
flush_cursor_maze_coordinate(struct maze const *maze, int const r, int const c)
{
    set_cursor_position(r / 2, c);
    print_square(maze, r, c);
    (void)fflush(stdout);
}

static void
print_square(struct maze const *m, int const r, int const c)
{
    uint16_t const square = maze_at(m, r, c);
    if (!(square & path_bit))
    {
        if (r % 2)
        {
            printf("%s", walls[maze_at(m, r - 1, c) & wall_mask]);
        }
        else
        {
            printf("%s", walls[square & wall_mask]);
        }
    }
    else if (square & path_bit)
    {
        if (r % 2)
        {
            if (r > 0 && !(maze_at(m, r - 1, c) & path_bit))
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
            if (!(maze_at(m, r + 1, c) & path_bit))
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
maze_at_r(struct maze const *const maze, int const r, int const c)
{
    return &maze->maze[(r * maze->cols) + c];
}

/** Square by value. */
static uint16_t
maze_at(struct maze const *const maze, int const r, int const c)
{
    return maze->maze[(r * maze->cols) + c];
}

/** Can't build if square has been seen/cached. */
static bool
can_build_new_square(struct maze const *const maze, int const r, int const c)
{
    return r > 0 && r < maze->rows - 1 && c > 0 && c < maze->cols - 1
        && !(maze_at(maze, r, c) & cached_bit);
}

/*===========================    Misc    ====================================*/

static struct int_conversion
parse_digits(str_view arg)
{
    size_t const eql = sv_rfind(arg, sv_npos(arg), SV("="));
    if (eql == sv_npos(arg))
    {
        return (struct int_conversion){.status = CONV_ER};
    }
    arg = sv_substr(arg, eql, ULLONG_MAX);
    if (sv_empty(arg))
    {
        (void)fprintf(stderr, "please specify element to convert.\n");
        return (struct int_conversion){.status = CONV_ER};
    }
    arg = sv_remove_prefix(arg, 1);
    return convert_to_int(sv_begin(arg));
}

static void
help(void)
{
    (void)fprintf(
        stdout,
        "Maze Builder:\nBuilds a Perfect Maze with Prim's "
        "Algorithm to demonstrate usage of the priority "
        "queue and ordered_map provided by this library.\nUsage:\n-r=N The "
        "row flag lets you specify maze rows > 7.\n-c=N The col flag "
        "lets you specify maze cols > 7.\n-s=N The speed flag lets "
        "you specify the speed of the animation "
        "0-7.\nExample:\n./build/[debug/]bin/maze -c=111 -r=33 -s=4\n");
}
