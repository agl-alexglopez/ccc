/* Author: Alexander Lopez
   File: maze.c
   ============
   This file provides a simple maze builder that implements Prim's algorithm
   to randomly generate a maze. I chose this algorithm because it can use
   both a set and a priority queue to acheive its purpose. Such data structures
   are provided by the library offering a perfect sample program opportunity. */
#include "cli.h"
#include "pqueue.h"
#include "random.h"
#include "set.h"
#include "str_view.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*=======================   Maze Helper Types   =============================*/

enum speed
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
    enum speed speed;
    uint16_t *maze;
};

/*===================  Prim's Algorithm Helper Types   ======================*/

struct priority_cell
{
    struct point cell;
    int priority;
    struct depq_elem elem;
};

struct point_cost
{
    struct point p;
    int cost;
    struct set_elem elem;
};

/*======================   Maze Constants   =================================*/

const char *walls[] = {
    "■", "╵", "╶", "└", "╷", "│", "┌", "├",
    "╴", "┘", "─", "┴", "┐", "┤", "┬", "┼",
};

const int speeds[] = {
    0, 5000000, 2500000, 1000000, 500000, 250000, 100000, 1000,
};

const struct point build_dirs[] = {{-2, 0}, {0, 2}, {2, 0}, {0, -2}};
const size_t build_dirs_size = sizeof(build_dirs) / sizeof(build_dirs[0]);

const str_view rows = SV("-r=");
const str_view cols = SV("-c=");
const str_view speed = SV("-s=");
const str_view help_flag = SV("-h");
const str_view escape = SV("\033[");
const str_view semi_colon = SV(";");
const str_view cursor_pos_specifier = SV("f");
const int default_rows = 33;
const int default_cols = 111;
const int default_speed = 4;
const int row_col_min = 7;
const int speed_max = 7;

const uint16_t path_bit = 0b0010000000000000;
const uint16_t wall_mask = 0b1111;
const uint16_t floating_wall = 0b0000;
const uint16_t north_wall = 0b0001;
const uint16_t east_wall = 0b0010;
const uint16_t south_wall = 0b0100;
const uint16_t west_wall = 0b1000;
const uint16_t cached_bit = 0b0001000000000000;

/*==========================   Prototypes  ================================= */

static void animate_maze(struct maze *);
static void fill_maze_with_walls(struct maze *);
static void build_wall(struct maze *, struct point);
static void print_square(const struct maze *, struct point);
static uint16_t *maze_at_mut(const struct maze *, struct point);
static uint16_t maze_at(const struct maze *, struct point);
static void clear_and_flush_maze(const struct maze *);
static void carve_path_walls_animated(struct maze *, struct point, int);
static void join_squares_animated(struct maze *, struct point, struct point,
                                  int);
static void flush_cursor_maze_coordinate(const struct maze *, struct point);
static bool can_build_new_square(const struct maze *, struct point);
static void *valid_malloc(size_t);
static void help(void);
static struct point pick_rand_point(const struct maze *);
static node_threeway_cmp cmp_priority_cells(const struct depq_elem *,
                                            const struct depq_elem *, void *);
static node_threeway_cmp cmp_points(const struct set_elem *,
                                    const struct set_elem *, void *);
static void set_destructor(struct set_elem *);
static struct int_conversion parse_digits(str_view);

/*======================  Main Arg Handling  ===============================*/

int
main(int argc, char **argv)
{
    /* Randomness will be used throughout the program but it need not be
       perfect. It only helps build the maze.
       NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp) */
    srand(time(NULL));
    struct maze maze = {
        .rows = default_rows,
        .cols = default_cols,
        .speed = default_speed,
        .maze = NULL,
    };
    for (int i = 1; i < argc; ++i)
    {
        const str_view arg = sv(argv[i]);
        if (sv_starts_with(arg, rows))
        {
            const struct int_conversion row_arg = parse_digits(arg);
            if (row_arg.status == CONV_ER || row_arg.conversion < row_col_min)
            {
                quit("rows below required minimum or negative.\n", 1);
            }
            maze.rows = row_arg.conversion;
        }
        else if (sv_starts_with(arg, cols))
        {
            const struct int_conversion col_arg = parse_digits(arg);
            if (col_arg.status == CONV_ER || col_arg.conversion < row_col_min)
            {
                quit("cols below required minimum or negative.\n", 1);
            }
            maze.cols = col_arg.conversion;
        }
        else if (sv_starts_with(arg, speed))
        {
            const struct int_conversion speed_arg = parse_digits(arg);
            if (speed_arg.status == CONV_ER || speed_arg.conversion > speed_max
                || speed_arg.conversion < 0)
            {
                quit("speed outside of valid range.\n", 1);
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

static void
animate_maze(struct maze *maze)
{
    /* Setting up the data structures needed should look similar to C++.
       A set could be replaced by a 2D vector copy of the maze with costs
       mapped but the purpose of this program is to test both the set
       and priority queue data structures. Also a 2D vector wastes space. */
    struct depqueue cells;
    struct set cell_costs;
    set_init(&cell_costs, cmp_points, NULL);
    depq_init(&cells, cmp_priority_cells, NULL);
    struct point_cost *odd_point = valid_malloc(sizeof(struct point_cost));
    *odd_point = (struct point_cost){
        .p = pick_rand_point(maze),
        .cost = rand_range(0, 100),
    };
    (void)set_insert(&cell_costs, &odd_point->elem);
    struct priority_cell *start = valid_malloc(sizeof(struct priority_cell));
    *start = (struct priority_cell){
        .cell = odd_point->p,
        .priority = odd_point->cost,
    };
    (void)depq_push(&cells, &start->elem);

    const int animation_speed = speeds[maze->speed];
    fill_maze_with_walls(maze);
    clear_and_flush_maze(maze);
    while (!depq_empty(&cells))
    {
        const struct priority_cell *const cur
            = depq_entry(depq_max(&cells), struct priority_cell, elem);
        *maze_at_mut(maze, cur->cell) |= cached_bit;
        struct point min_neighbor = {0};
        int min_weight = INT_MAX;
        for (size_t i = 0; i < build_dirs_size; ++i)
        {
            const struct point next = {
                .r = cur->cell.r + build_dirs[i].r,
                .c = cur->cell.c + build_dirs[i].c,
            };
            if (!can_build_new_square(maze, next))
            {
                continue;
            }
            int cur_weight = 0;
            struct point_cost key = {.p = next};
            const struct set_elem *const found
                = set_find(&cell_costs, &key.elem);
            if (found == set_end(&cell_costs))
            {
                struct point_cost *new_cost
                    = valid_malloc(sizeof(struct point_cost));
                *new_cost = (struct point_cost){
                    .p = next,
                    .cost = rand_range(0, 100),
                };
                cur_weight = new_cost->cost;
                assert(set_insert(&cell_costs, &new_cost->elem));
            }
            else
            {
                cur_weight = set_entry(found, struct point_cost, elem)->cost;
            }
            if (cur_weight < min_weight)
            {
                min_weight = cur_weight;
                min_neighbor = next;
            }
        }
        if (min_neighbor.r)
        {
            join_squares_animated(maze, cur->cell, min_neighbor,
                                  animation_speed);
            struct priority_cell *new_cell
                = valid_malloc(sizeof(struct priority_cell));
            *new_cell = (struct priority_cell){
                .cell = min_neighbor,
                .priority = min_weight,
            };
            depq_push(&cells, &new_cell->elem);
        }
        else
        {
            struct priority_cell *pc
                = depq_entry(depq_pop_max(&cells), struct priority_cell, elem);
            free(pc);
        }
    }
    /* The priority queue does not need to be cleared because it's emptiness
       determined the course of the maze building. It has no hidden allocations
       either so no more work is needed if we know it's empty and the data
       structure metadata is on the stack. */
    set_clear(&cell_costs, set_destructor);
}

static struct point
pick_rand_point(const struct maze *const maze)
{
    return (struct point){
        .r = 2 * rand_range(1, (maze->rows - 2) / 2) + 1,
        .c = 2 * rand_range(1, (maze->cols - 2) / 2) + 1,
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
clear_and_flush_maze(const struct maze *const maze)
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
join_squares_animated(struct maze *maze, const struct point cur,
                      const struct point next, int s)
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
carve_path_walls_animated(struct maze *maze, const struct point p, int s)
{
    *maze_at_mut(maze, p) |= path_bit;
    flush_cursor_maze_coordinate(maze, p);
    struct timespec ts = {.tv_sec = 0, .tv_nsec = s};
    nanosleep(&ts, NULL);
    if (p.r - 1 >= 0
        && !(maze_at(maze, (struct point){.r = p.r - 1, .c = p.c}) & path_bit))
    {
        *maze_at_mut(maze, (struct point){.r = p.r - 1, .c = p.c})
            &= ~south_wall;
        flush_cursor_maze_coordinate(maze, (struct point){p.r - 1, p.c});
        nanosleep(&ts, NULL);
    }
    if (p.r + 1 < maze->rows
        && !(maze_at(maze, (struct point){.r = p.r + 1, .c = p.c}) & path_bit))
    {
        *maze_at_mut(maze, (struct point){.r = p.r + 1, .c = p.c})
            &= ~north_wall;
        flush_cursor_maze_coordinate(maze, (struct point){p.r + 1, p.c});
        nanosleep(&ts, NULL);
    }
    if (p.c - 1 >= 0
        && !(maze_at(maze, (struct point){.r = p.r, .c = p.c - 1}) & path_bit))
    {
        *maze_at_mut(maze, (struct point){.r = p.r, .c = p.c - 1})
            &= ~east_wall;
        flush_cursor_maze_coordinate(maze,
                                     (struct point){.r = p.r, .c = p.c - 1});
        nanosleep(&ts, NULL);
    }
    if (p.c + 1 < maze->cols
        && !(maze_at(maze, (struct point){.r = p.r, .c = p.c + 1}) & path_bit))
    {
        *maze_at_mut(maze, (struct point){.r = p.r, .c = p.c + 1})
            &= ~west_wall;
        flush_cursor_maze_coordinate(maze,
                                     (struct point){.r = p.r, .c = p.c + 1});
        nanosleep(&ts, NULL);
    }
    *maze_at_mut(maze, (struct point){.r = p.r, .c = p.c}) |= cached_bit;
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
    *maze_at_mut(m, p) |= wall;
    *maze_at_mut(m, p) &= ~path_bit;
}

static void
flush_cursor_maze_coordinate(const struct maze *maze, const struct point p)
{
    set_cursor_position(p.r, p.c);
    print_square(maze, p);
    (void)fflush(stdout);
}

static void
print_square(const struct maze *m, struct point p)
{
    const uint16_t square = maze_at(m, p);
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
maze_at_mut(const struct maze *const maze, struct point p)
{
    return &maze->maze[p.r * maze->cols + p.c];
}

static uint16_t
maze_at(const struct maze *const maze, struct point p)
{
    return maze->maze[p.r * maze->cols + p.c];
}

static bool
can_build_new_square(const struct maze *const maze, const struct point next)
{
    return next.r > 0 && next.r < maze->rows - 1 && next.c > 0
           && next.c < maze->cols - 1 && !(maze_at(maze, next) & cached_bit);
}

/*===================   Data Structure Comparators   ========================*/

static node_threeway_cmp
cmp_priority_cells(const struct depq_elem *const key, const struct depq_elem *n,
                   void *const aux)
{
    (void)aux;
    const struct priority_cell *const a
        = depq_entry(key, struct priority_cell, elem);
    const struct priority_cell *const b
        = depq_entry(n, struct priority_cell, elem);
    return (a->priority > b->priority) - (a->priority < b->priority);
}

static node_threeway_cmp
cmp_points(const struct set_elem *key, const struct set_elem *n, void *aux)
{
    (void)aux;
    const struct point_cost *const a = set_entry(key, struct point_cost, elem);
    const struct point_cost *const b = set_entry(n, struct point_cost, elem);
    if (a->p.r == b->p.r && a->p.c == b->p.c)
    {
        return NODE_EQL;
    }
    if (a->p.r == b->p.r)
    {
        return (a->p.c > b->p.c) - (a->p.c < b->p.c);
    }
    return (a->p.r > b->p.r) - (a->p.r < b->p.r);
}

static void
set_destructor(struct set_elem *e)
{
    struct point_cost *pc = set_entry(e, struct point_cost, elem);
    free(pc);
}

/*===========================    Misc    ====================================*/

static struct int_conversion
parse_digits(str_view arg)
{
    const size_t eql = sv_rfind(arg, sv_npos(arg), SV("="));
    if (eql == sv_npos(arg))
    {
        return (struct int_conversion){.status = CONV_ER};
    }
    str_view row_count = sv_substr(arg, eql, ULLONG_MAX);
    if (sv_empty(row_count))
    {
        (void)fprintf(stderr, "please specify element to convert.\n");
        return (struct int_conversion){.status = CONV_ER};
    }
    row_count = sv_remove_prefix(row_count, 1);
    return convert_to_int(sv_begin(arg));
}

/* Promises valid memory or exits the program if the heap has an error. */
static void *
valid_malloc(size_t n)
{
    void *mem = malloc(n);
    if (!mem)
    {
        (void)fprintf(stderr, "heap is exhausted, exiting program.\n");
        exit(1);
    }
    return mem;
}

static void
help(void)
{
    (void)fprintf(stdout,
                  "Maze Builder:\nBuilds a Perfect Maze with Prim's "
                  "Algorithm to demonstrate usage of the priority "
                  "queue and set provided by this library.\nUsage:\n-r=N The "
                  "row flag lets you specify maze rows > 7.\n-c=N The col flag "
                  "lets you specify maze cols > 7.\n-s=N The speed flag lets "
                  "you specify the speed of the animation "
                  "0-7.\nExample:\n./build/rel/maze -c=111 -r=33 -s=4\n");
}
