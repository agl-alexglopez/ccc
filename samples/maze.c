#include "pqueue.h"
#include "set.h"
#include "str_view.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

struct priority_cell
{
    struct point cell;
    int priority;
    struct pq_elem elem;
};

struct point_cost
{
    struct point p;
    int cost;
    struct set_elem elem;
};

const char *walls[] = {
    "■", "╵", "╶", "└", "╷", "│", "┌", "├",
    "╴", "┘", "─", "┴", "┐", "┤", "┬", "┼",
};

int speeds[] = {
    0, 5000000, 2500000, 1000000, 500000, 250000, 100000, 1000,
};

const struct point build_dirs[] = {{-2, 0}, {0, 2}, {2, 0}, {0, -2}};
const size_t build_dirs_size = sizeof(build_dirs) / sizeof(build_dirs[0]);

const str_view rows = SV("-r=");
const str_view cols = SV("-c=");
const str_view speed = SV("-s=");
const str_view escape = SV("\033[");
const str_view semi_colon = SV(";");
const str_view cursor_pos_specifier = SV("f");
const int default_rows = 33;
const int default_cols = 111;
const int row_col_min = 7;
const int speed_max = 7;

const uint16_t path_bit = 0b0010000000000000;
const uint16_t wall_mask = 0b1111;
const uint16_t floating_wall = 0b0000;
const uint16_t north_wall = 0b0001;
const uint16_t east_wall = 0b0010;
const uint16_t south_wall = 0b0100;
const uint16_t west_wall = 0b1000;
const uint16_t builder_bit = 0b0001000000000000;

int convert_to_int(str_view, const char *);
void animate_maze(struct maze *);
void initialize_cells(struct maze *, struct pqueue *, struct set *);
void fill_maze_with_walls(struct maze *);
void build_wall(struct maze *, struct point);
void print_square(const struct maze *, struct point);
uint16_t *maze_at_mut(const struct maze *, struct point);
uint16_t maze_at(const struct maze *, struct point);
void clear_screen(void);
void clear_and_flush_maze(const struct maze *);
void carve_path_walls_animated(struct maze *, struct point, int);
void set_cursor_position(struct point);
void join_squares_animated(struct maze *, struct point, struct point, int);
void flush_cursor_maze_coordinate(const struct maze *, struct point);
bool can_build_new_square(const struct maze *, struct point);
void *checked_allocation(size_t);

struct point pick_rand_point(const struct maze *);
int rand_range(int, int);

threeway_cmp cmp_priority_cells(const struct pq_elem *, const struct pq_elem *,
                                void *);
threeway_cmp cmp_points(const struct set_elem *, const struct set_elem *,
                        void *);
void set_destructor(struct set_elem *);

int
main(int argc, char **argv)
{
    struct maze maze = {
        .rows = default_rows,
        .cols = default_cols,
        .maze = NULL,
    };
    for (int i = 1; i < argc; ++i)
    {
        const str_view arg = sv(argv[i]);
        if (sv_starts_with(arg, rows))
        {
            const int row_arg = convert_to_int(arg, "rows");
            if (row_arg < row_col_min)
            {
                (void)fprintf(stderr,
                              "rows below required minimum or negative.\n");
                return 1;
            }
            maze.rows = row_arg;
        }
        else if (sv_starts_with(arg, cols))
        {
            const int col_arg = convert_to_int(arg, "cols");
            if (col_arg < row_col_min)
            {
                (void)fprintf(stderr,
                              "cols below required minimum or negative.\n");
                return 1;
            }
            maze.cols = col_arg;
        }
        else if (sv_starts_with(arg, speed))
        {
            const int speed_arg = convert_to_int(arg, "speeds");
            if (speed_arg > speed_max || speed_arg < 0)
            {
                (void)fprintf(stderr, "speed outside of valid range.\n");
                return 1;
            }
            maze.speed = speed_arg;
        }
        else
        {
            (void)fprintf(
                stderr,
                "can only specify rows or columns for now (-r=N, -c=N)\n");
            return 1;
        }
    }
    maze.maze = calloc((size_t)maze.rows * maze.cols, sizeof(uint16_t));
    if (!maze.maze)
    {
        (void)fprintf(stderr, "allocation failure for specified maze size.\n");
        return 1;
    }
    animate_maze(&maze);
    set_cursor_position((struct point){.r = maze.rows + 1, .c = maze.cols + 1});
    printf("\n");
}

void
animate_maze(struct maze *maze)
{
    if (!maze)
    {
        (void)fprintf(stderr, "No maze provided.\n");
        return;
    }
    /* Setting up the data structures needed should look similar to C++.
       A set could be replaced by a 2D vector copy of the maze with costs
       mapped but the purpose of this program is to test both the set
       and priority queue data structures. */
    struct pqueue cells;
    struct set cell_costs;
    set_init(&cell_costs);
    pq_init(&cells);
    struct point_cost *odd_point
        = checked_allocation(sizeof(struct point_cost));
    *odd_point = (struct point_cost){
        .p = pick_rand_point(maze),
        .cost = rand_range(0, 100),
    };
    (void)set_insert(&cell_costs, &odd_point->elem, cmp_points, NULL);
    struct priority_cell *start
        = checked_allocation(sizeof(struct priority_cell));
    *start = (struct priority_cell){
        .cell = odd_point->p,
        .priority = odd_point->cost,
    };
    (void)pq_insert(&cells, &start->elem, cmp_priority_cells, NULL);

    const int animation_speed = speeds[maze->speed];
    fill_maze_with_walls(maze);
    clear_and_flush_maze(maze);
    while (!pq_empty(&cells))
    {
        const struct priority_cell *const cur
            = pq_entry(pq_max(&cells), struct priority_cell, elem);
        *maze_at_mut(maze, cur->cell) |= builder_bit;
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
                = set_find(&cell_costs, &key.elem, cmp_points, NULL);
            if (found == set_end(&cell_costs))
            {
                struct point_cost *new_cost
                    = checked_allocation(sizeof(struct point_cost));
                *new_cost = (struct point_cost){
                    .p = next,
                    .cost = rand_range(0, 100),
                };
                cur_weight = new_cost->cost;
                (void)set_insert(&cell_costs, &new_cost->elem, cmp_points,
                                 NULL);
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
                = checked_allocation(sizeof(struct priority_cell));
            *new_cell = (struct priority_cell){
                .cell = min_neighbor,
                .priority = min_weight,
            };
            pq_insert(&cells, &new_cell->elem, cmp_priority_cells, NULL);
        }
        else
        {
            struct priority_cell *pc
                = pq_entry(pq_pop_max(&cells), struct priority_cell, elem);
            free(pc);
        }
    }
    set_clear(&cell_costs, set_destructor);
}

void
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

void
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
    *maze_at_mut(maze, (struct point){.r = p.r, .c = p.c}) |= builder_bit;
}

void
flush_cursor_maze_coordinate(const struct maze *maze, const struct point p)
{
    set_cursor_position(p);
    print_square(maze, p);
    (void)fflush(stdout);
}

void
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

void
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

void
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

void
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

void
clear_screen(void)
{
    printf("\033[2J\033[1;1H");
}

void
set_cursor_position(const struct point p)
{
    printf("\033[%d;%df", p.r + 1, p.c + 1);
}

uint16_t *
maze_at_mut(const struct maze *const maze, struct point p)
{
    return &maze->maze[p.r * maze->cols + p.c];
}

uint16_t
maze_at(const struct maze *const maze, struct point p)
{
    return maze->maze[p.r * maze->cols + p.c];
}

bool
can_build_new_square(const struct maze *const maze, const struct point next)
{
    return next.r > 0 && next.r < maze->rows - 1 && next.c > 0
           && next.c < maze->cols - 1 && !(maze_at(maze, next) & builder_bit);
}

int
convert_to_int(const str_view arg, const char *conversion)
{
    const size_t eql = sv_rfind(arg, sv_npos(arg), SV("="));
    str_view row_count = sv_substr(arg, eql, ULLONG_MAX);
    if (sv_empty(row_count))
    {
        (void)fprintf(stderr, "please specify row count.\n");
        return 1;
    }
    row_count = sv_remove_prefix(row_count, 1);
    errno = 0;
    char *end;
    const long row_conversion = strtol(sv_begin(row_count), &end, 10);
    if (errno == ERANGE)
    {
        (void)fprintf(stderr, "%s count could not convert to int.\n",
                      conversion);
        return -1;
    }
    if (row_conversion < 0)
    {
        (void)fprintf(stderr, "%s count cannot be negative.\n", conversion);
        return -1;
    }
    if (row_conversion > INT_MAX)
    {
        (void)fprintf(stderr, "%s count cannot exceed INT_MAX.\n", conversion);
        return -1;
    }
    return (int)row_conversion;
}

threeway_cmp
cmp_priority_cells(const struct pq_elem *const key, const struct pq_elem *n,
                   void *const aux)
{
    (void)aux;
    const struct priority_cell *const a
        = pq_entry(key, struct priority_cell, elem);
    const struct priority_cell *const b
        = pq_entry(n, struct priority_cell, elem);
    return (a->priority > b->priority) - (a->priority < b->priority);
}

threeway_cmp
cmp_points(const struct set_elem *key, const struct set_elem *n, void *aux)
{
    (void)aux;
    const struct point_cost *const a = set_entry(key, struct point_cost, elem);
    const struct point_cost *const b = set_entry(n, struct point_cost, elem);
    if (a->p.r == b->p.r && a->p.c == b->p.c)
    {
        return EQL;
    }
    if (a->p.r == b->p.r)
    {
        return (a->p.c > b->p.c) - (a->p.c < b->p.c);
    }
    return (a->p.r > b->p.r) - (a->p.r < b->p.r);
}

void
set_destructor(struct set_elem *e)
{
    struct point_cost *pc = set_entry(e, struct point_cost, elem);
    free(pc);
}

struct point
pick_rand_point(const struct maze *const maze)
{
    return (struct point){
        .r = 2 * rand_range(1, (maze->rows - 2) / 2) + 1,
        .c = 2 * rand_range(1, (maze->cols - 2) / 2) + 1,
    };
}

int
rand_range(const int min, const int max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

void *
checked_allocation(size_t n)
{
    void *mem = malloc(n);
    if (!mem)
    {
        (void)fprintf(stderr, "heap is exhausted, exiting program.\n");
        exit(1);
    }
    return mem;
}
