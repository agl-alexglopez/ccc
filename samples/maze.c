/* Author: Alexander Lopez, File: maze.c
This file provides a simple maze builder that implements Prim's algorithm
to randomly generate a maze. I chose this algorithm because it can use
both an ordered and a priority queue to achieve its purpose. Such data
structures are provided by the library offering a perfect sample program
opportunity. Also there are some interesting ways to combine allocating and
non-allocating interfaces. */
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#define BUFFER_USING_NAMESPACE_CCC
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define ORDERED_MAP_USING_NAMESPACE_CCC

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "alloc.h"
#include "ccc/buffer.h"
#include "ccc/ordered_map.h"
#include "ccc/priority_queue.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "cli.h"
#include "random.h"
#include "str_view/str_view.h"

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
    pq_elem pq_elem;
    omap_elem map_elem;
};

/*======================   Maze Constants   =================================*/

char const *const walls[] = {
    "■", "╵", "╶", "└", "╷", "│", "┌", "├",
    "╴", "┘", "─", "┴", "┐", "┤", "┬", "┼",
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
int const default_rows = 33;
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

static void animate_maze(struct maze *);
static void fill_maze_with_walls(struct maze *);
static void build_wall(struct maze *, struct point);
static void print_square(struct maze const *, struct point);
static uint16_t *maze_at_r(struct maze const *, struct point);
static uint16_t maze_at(struct maze const *, struct point);
static void clear_and_flush_maze(struct maze const *);
static void carve_path_walls_animated(struct maze *, struct point, int);
static void join_squares_animated(struct maze *, struct point, struct point,
                                  int);
static void flush_cursor_maze_coordinate(struct maze const *, struct point);
static bool can_build_new_square(struct maze const *, struct point);
static void help(void);
static struct point rand_point(struct maze const *);
static threeway_cmp cmp_priority_cells(cmp);
static threeway_cmp cmp_priority_pos(key_cmp);
static struct int_conversion parse_digits(str_view);
static void *arena_bump_alloc(void *ptr, size_t size, void *arena);

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
    set_cursor_position(maze.rows + 1, maze.cols + 1);
    printf("\n");
}

/*======================      Maze Animation      ===========================*/

/* This function aims to demonstrate how to compose various containers when
   taking advantage of the flexible allocating or non-allocating options.
   There are better ways to run Prim's algorithm, but this is just to show
   how we can tailor allocations to the scope of this function by combining
   a buffer and priority queue that do not allocate, with a map that does.
   In fact, the composition allows us to present the buffer as if it is an
   allocator to the map; all the while we benefit from one contiguous arena
   of our data type. The priority queue and map also share the same memory
   as intrusive struct fields, further simplifying locality of reference. */
static void
animate_maze(struct maze *maze)
{
    int const speed = speeds[maze->speed];
    fill_maze_with_walls(maze);
    clear_and_flush_maze(maze);

    /* The buffer holds enough space for use to bump all cells into it,
       eliminating the need for a non-contiguous allocator. In this type of
       maze only odd squares can be paths, which is what we are building here
       so that is half of the squares. */
    size_t const cap = (((size_t)maze->rows * maze->cols) / 2) + 1;
    size_t bytes = cap * sizeof(struct prim_cell);
    /* Calling malloc like this is kind of nice to get rid of a dangling ref
       but we need to have some kind of sanity check. */
    buffer bump_arena
        = buf_init(((struct prim_cell *)malloc(bytes)), NULL, NULL, cap);
    assert(buf_begin(&bump_arena) != NULL);
    /* Priority queue elements will not participate in allocation. They piggy
       back on the ordered map memory because the relationship is 1-to-1. */
    priority_queue cells = pq_init(struct prim_cell, pq_elem, CCC_LES, NULL,
                                   cmp_priority_cells, NULL);
    /* The ordered map uses a wrapper around the bump allocator as its backing
       store making allocation speed optimal for the given problem. */
    ordered_map costs
        = om_init(costs, struct prim_cell, map_elem, cell, arena_bump_alloc,
                  cmp_priority_pos, &bump_arena);
    struct point s = rand_point(maze);
    struct prim_cell *const first = om_insert_entry_w(
        entry_r(&costs, &s),
        (struct prim_cell){.cell = s, .cost = rand_range(0, 100)});
    assert(first);
    (void)push(&cells, &first->pq_elem);
    assert(ccc_buf_size(&bump_arena) == 1);
    while (!is_empty(&cells))
    {
        struct prim_cell const *const c = front(&cells);
        *maze_at_r(maze, c->cell) |= cached_bit;
        struct prim_cell *min_cell = NULL;
        int min = INT_MAX;
        for (size_t i = 0; i < dir_offsets_size; ++i)
        {
            struct point const next = {.r = c->cell.r + dir_offsets[i].r,
                                       .c = c->cell.c + dir_offsets[i].c};
            if (!can_build_new_square(maze, next))
            {
                continue;
            }
            /* The Entry Interface helps make what would be an if else branch
               a simple lazily evaluated insertion. If the entry is Occupied
               rand_range is never called. This technique also means cells
               can be given weights lazily as we go rather than all at once
               before the main algorithm starts. */
            struct prim_cell *const cell = om_or_insert_w(
                entry_r(&costs, &next),
                (struct prim_cell){.cell = next, .cost = rand_range(0, 100)});
            assert(cell);
            if (cell->cost < min)
            {
                min = cell->cost;
                min_cell = cell;
            }
        }
        if (min_cell)
        {
            join_squares_animated(maze, c->cell, min_cell->cell, speed);
            (void)push(&cells, &min_cell->pq_elem);
        }
        else
        {
            (void)pop(&cells);
        }
    }
    /* Thanks to how the containers worked together there was only a single
       allocation and free from the heap's perspective. */
    result l = ccc_buf_alloc(&bump_arena, 0, std_alloc);
    assert(l == CCC_OK);
}

static struct point
rand_point(struct maze const *const maze)
{
    return (struct point){
        .r = (2 * rand_range(1, (maze->rows - 2) / 2)) + 1,
        .c = (2 * rand_range(1, (maze->cols - 2) / 2)) + 1,
    };
}

/*=========================   Maze Support Code   ===========================*/

static void
fill_maze_with_walls(struct maze *maze)
{
    for (int row = 0; row < maze->rows; ++row)
    {
        for (int col = 0; col < maze->cols; ++col)
        {
            build_wall(maze, (struct point){.r = row, .c = col});
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
            print_square(maze, (struct point){.r = row, .c = col});
        }
        printf("\n");
    }
    (void)fflush(stdout);
}

static void
join_squares_animated(struct maze *maze, struct point const cur,
                      struct point const next, int s)
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
    carve_path_walls_animated(maze, cur, s);
    carve_path_walls_animated(maze, wall, s);
    carve_path_walls_animated(maze, next, s);
}

static void
carve_path_walls_animated(struct maze *maze, struct point const p, int s)
{
    *maze_at_r(maze, p) |= path_bit;
    flush_cursor_maze_coordinate(maze, p);
    struct timespec ts = {.tv_sec = 0, .tv_nsec = s};
    nanosleep(&ts, NULL);
    if (p.r - 1 >= 0
        && !(maze_at(maze, (struct point){.r = p.r - 1, .c = p.c}) & path_bit))
    {
        *maze_at_r(maze, (struct point){.r = p.r - 1, .c = p.c}) &= ~south_wall;
        flush_cursor_maze_coordinate(maze, (struct point){p.r - 1, p.c});
        nanosleep(&ts, NULL);
    }
    if (p.r + 1 < maze->rows
        && !(maze_at(maze, (struct point){.r = p.r + 1, .c = p.c}) & path_bit))
    {
        *maze_at_r(maze, (struct point){.r = p.r + 1, .c = p.c}) &= ~north_wall;
        flush_cursor_maze_coordinate(maze, (struct point){p.r + 1, p.c});
        nanosleep(&ts, NULL);
    }
    if (p.c - 1 >= 0
        && !(maze_at(maze, (struct point){.r = p.r, .c = p.c - 1}) & path_bit))
    {
        *maze_at_r(maze, (struct point){.r = p.r, .c = p.c - 1}) &= ~east_wall;
        flush_cursor_maze_coordinate(maze,
                                     (struct point){.r = p.r, .c = p.c - 1});
        nanosleep(&ts, NULL);
    }
    if (p.c + 1 < maze->cols
        && !(maze_at(maze, (struct point){.r = p.r, .c = p.c + 1}) & path_bit))
    {
        *maze_at_r(maze, (struct point){.r = p.r, .c = p.c + 1}) &= ~west_wall;
        flush_cursor_maze_coordinate(maze,
                                     (struct point){.r = p.r, .c = p.c + 1});
        nanosleep(&ts, NULL);
    }
    *maze_at_r(maze, (struct point){.r = p.r, .c = p.c}) |= cached_bit;
}

static void
build_wall(struct maze *m, struct point p)
{
    uint16_t wall = 0;
    if (p.r - 1 >= 0)
    {
        wall |= north_wall;
    }
    if (p.r + 1 < m->rows)
    {
        wall |= south_wall;
    }
    if (p.c - 1 >= 0)
    {
        wall |= west_wall;
    }
    if (p.c + 1 < m->cols)
    {
        wall |= east_wall;
    }
    *maze_at_r(m, p) |= wall;
    *maze_at_r(m, p) &= ~path_bit;
}

static void
flush_cursor_maze_coordinate(struct maze const *maze, struct point const p)
{
    set_cursor_position(p.r, p.c);
    print_square(maze, p);
    (void)fflush(stdout);
}

static void
print_square(struct maze const *m, struct point p)
{
    uint16_t const square = maze_at(m, p);
    if (!(square & path_bit))
    {
        printf("%s", walls[square & wall_mask]);
    }
    else if (square & path_bit)
    {
        printf(" ");
    }
    else
    {
        (void)fprintf(stderr, "uncategorizable square.\n");
    }
}

static uint16_t *
maze_at_r(struct maze const *const maze, struct point p)
{
    return &maze->maze[(p.r * maze->cols) + p.c];
}

static uint16_t
maze_at(struct maze const *const maze, struct point p)
{
    return maze->maze[(p.r * maze->cols) + p.c];
}

static bool
can_build_new_square(struct maze const *const maze, struct point const next)
{
    return next.r > 0 && next.r < maze->rows - 1 && next.c > 0
           && next.c < maze->cols - 1 && !(maze_at(maze, next) & cached_bit);
}

/*===================   Data Structure Comparators   ========================*/

static threeway_cmp
cmp_priority_cells(cmp const cmp_cells)
{
    struct prim_cell const *const lhs = cmp_cells.user_type_lhs;
    struct prim_cell const *const rhs = cmp_cells.user_type_rhs;
    return (lhs->cost > rhs->cost) - (lhs->cost < rhs->cost);
}

static threeway_cmp
cmp_priority_pos(key_cmp const cmp_points)
{
    struct prim_cell const *const a_rhs = cmp_points.user_type_rhs;
    struct point const *const key_lhs = cmp_points.key_lhs;
    if (a_rhs->cell.r == key_lhs->r && a_rhs->cell.c == key_lhs->c)
    {
        return CCC_EQL;
    }
    if (a_rhs->cell.r == key_lhs->r)
    {
        return (key_lhs->c > a_rhs->cell.c) - (key_lhs->c < a_rhs->cell.c);
    }
    return (key_lhs->r > a_rhs->cell.r) - (key_lhs->r < a_rhs->cell.r);
}

/*===========================    Misc    ====================================*/

static void *
arena_bump_alloc(void *const ptr, size_t const size, void *const arena)
{
    if (!ptr && !size)
    {
        return NULL;
    }
    if (!ptr)
    {
        return ccc_buf_alloc_back(arena);
    }
    /* Freeing and reallocation are not supported in a bump allocator. */
    assert(!"You shouldn't be here this is a bump allocator!");
    return NULL;
}

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
        "0-7.\nExample:\n./build/rel/maze -c=111 -r=33 -s=4\n");
}
