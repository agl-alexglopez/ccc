/** The graph program runs Dijkstra's shortest path algorithm over randomly
generated graphs displayed in the terminal.

Builds weighted graphs for Dijkstra's Algorithm to demonstrate usage of the
priority queue and map provided by this library.
Usage:
-r=N The row flag lets you specify area for grid rows > 7.
-c=N The col flag lets you specify area for grid cols > 7.
-v=N specify 1-26 vertices for the randomly generated and connected graph.
-s=N specify 1-7 animation speed for Dijkstra's algorithm.
Example:
./build/[debug/]bin/graph -c=111 -r=33 -v=4 -s=3
Once the graph is built seek the shortest path between two uppercase vertices.
Examples:
AB
A->B
CtoD
Enter 'q' to quit. */
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_USING_NAMESPACE_CCC
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC

#include "ccc/flat_double_ended_queue.h"
#include "ccc/flat_hash_map.h"
#include "ccc/priority_queue.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "util/alloc.h"
#include "util/cli.h"
#include "util/random.h"
#include "util/str_view.h"

#define CYN "\033[38;5;14m"
#define RED "\033[38;5;9m"
#define MAG "\033[38;5;13m"
#define NIL "\033[0m"

/** 16 bits in the grid shall be reserved for the edge id if the square is a
path. An edge ID is a concatenation of two vertex names. Vertex names are 8 bit
characters, so two vertices can fit into a uint16_t which we have room for in a
Cell. The concatenation shall always be sorted alphabetically so an edge
connecting vertex A and Z will be uint16_t=AZ. Here is the breakdown of bits
currently.

path shape bits───────────────────────────────────┬──┐
path_bit────────────────────────────────────────┐ │  │
vertex bit────────────────────────────────────┐ │ │  │
paint bit───────────────────────────────────┐ │ │ │  │
digit bit─────────────────────────────────┐ │ │ │ │  │
edge cost digit──────────────────────↓──↓ │ │ │ │ │  │
vertex title────────────────────┬───────┐ │ │ │ │ │  │
edge id───────┬───────────────┐ │       │ │ │ │ │ │  │
unused─────┬─┐↓               ↓ ↓       ↓ ↓ ↓ ↓ ↓ ↓  ↓
        0b...00000000 00000000 0000 0000 0 0 0 0 0000

If various signal bits such as paint or digit are turned on we know
which bits to look at and how to interpret them.

  - path shape bits determine how edges join as they run and turn.
  - path bit marks a cell as a path
  - vertex bit marks a cell as a vertex meaning it holds a vertex title.
  - paint bit is a single bit to mark a path should have a color.
  - digit bit marks that one digit of an edge cost is stored in a edge cell.
  - edge cost digit is stored in at most four bits. 9 is highest digit
    in base 10.
  - unused is not currently used but could be in the future.
  - edge id is the concatenation of two vertex titles in an edge to signify
    which vertices are connected. The edge id is sorted lexicographical
    with the lower value in the leftmost bits.
  - the vertex title is stored in 8 bits if the cell is a vertex.

This way of organizing bits comes from maze building practices which are
technically an extension of graph building and searching algorithms. */
typedef uint32_t cell;

enum
{
    DIRS_SIZE = 4,
    MAX_VERTICES = 'Z' - 'A' + 1,
    MAX_DEGREE = 4,
    MAX_SPEED = 7,
};

static_assert(
    MAX_VERTICES == 26,
    "maximum number of graph vertices should equal letters of the alphabet");

struct point
{
    int r;
    int c;
};

struct path_backtrack_cell
{
    struct point current;
    struct point parent;
};

/** A cost optimizes the problem so that we can store the path
rebuilding map implicitly in an array of cost[A-Z]. Then the pq
element can run the efficient push and decrease key operations with pointer
stability guaranteed. Finally these can be allocated on the stack because
there will be at most 26 of them which is very small. */
struct cost
{
    pq_elem pq_elem;
    int cost;
    char name;
    char from;
};

struct path_request
{
    char src;
    char dst;
};

/* Helper type for labelling costs for edges between vertices in the graph. */
enum label_orientation
{
    NORTH,
    SOUTH,
    EAST,
    WEST,
    DIAGONAL,
};

struct digit_encoding
{
    struct point start;
    int cost;
    size_t spaces_needed;
    enum label_orientation orientation;
};

struct node
{
    char name;
    int cost;
};

/* Each vertex in the map/graph will hold it's key name and edges to other
   vertices that it is connected to. This is displayed in a CLI so there
   is a maximum out degree of 4. Terminals only display cells in a grid.
   Vertex has 4 edge limit on a terminal grid. */
struct vertex
{
    char name;
    struct point pos;
    struct node edges[MAX_DEGREE];
};

struct graph
{
    int rows;
    int cols;
    int vertices;
    struct timespec speed;
    cell *grid;
    struct vertex *graph;
};

struct edge
{
    struct node n;
    struct point pos;
};

/*======================   Graph Constants   ================================*/

/* Because the maximum out degree that is easy to display on a terminal is 4,
   it is easy to pack all the vertices and fixed length edge arrays into one
   static buffer. This gives nice default initializations and provides easy
   access to vertices. */
static struct vertex network[MAX_VERTICES];

/* Go to the box drawing Unicode character Wikipedia page to change styles. */
static char const *paths[] = {
    "●", "╵", "╶", "╰", "╷", "│", "╭", "├",
    "╴", "╯", "─", "┴", "╮", "┤", "┬", "┼",
};

/* Animation speed for edge coloring during solving phase. */
static int const speeds[8] = {
    0, 500000000, 250000000, 100000000, 50000000, 25000000, 10000000, 1000000,
};

/* North, East, South, West */
static struct point const dirs[DIRS_SIZE] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};

/* Graphs are limited to 26 vertices. */
enum : char
{
    BEGIN_VERTICES = 'A',
    END_VERTICES = 'Z',
};

enum : int
{
    DEFAULT_ROWS = 33,
    DEFAULT_COLS = 111,
    DEFAULT_VERTICES = 4,
    ROW_COL_MIN = 7,
    VERTEX_PLACEMENT_PADDING = 3,
};

enum : cell
{
    VERTEX_CELL_TITLE_SHIFT = 8,
    VERTEX_TITLE_MASK = 0xFF00,
    EDGE_ID_SHIFT = 16,
    EDGE_ID_MASK = 0xFFFF0000,
    L_EDGE_ID_MASK = 0xFF000000,
    L_EDGE_ID_SHIFT = 24,
    R_EDGE_ID_MASK = 0x00FF0000,
    R_EDGE_ID_SHIFT = 16,
    PATH_MASK = 0b1111,
    NORTH_PATH = 0b0001,
    EAST_PATH = 0b0010,
    SOUTH_PATH = 0b0100,
    WEST_PATH = 0b1000,
    PATH_BIT = 0b10000,
    VERTEXT_BIT = 0b100000,
    PAINT_BIT = 0b1000000,
    DIGIT_BIT = 0b10000000,
    DIGIT_SHIFT = 8,
    DIGIT_MASK = 0xF00,
};

static str_view const prompt_msg
    = SV("Enter two vertices to find the shortest path between them (i.e. "
         "A-Z). Enter q to quit:");
static str_view const quit_cmd = SV("q");

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

static void build_graph(struct graph *);
static void find_shortest_paths(struct graph *);
static bool found_dst(struct graph *, struct vertex *);
static void edge_construct(struct graph *g, flat_hash_map *parent_map,
                           struct vertex *src, struct vertex *dst);
static int dijkstra_shortest_path(struct graph *, char src, char dst);
static void paint_edge(struct graph *, char, char, char const *edge_color);
static void add_edge_cost_label(struct graph *, struct vertex *,
                                struct edge const *);
static cell make_edge(char src, char dst);
static void flush_at(struct graph const *g, int r, int c,
                     char const *edge_color);

static struct point random_vertex_placement(struct graph const *);
static bool is_valid_vertex_pos(struct graph const *, int r, int c);
static cell *grid_at_mut(struct graph const *, int r, int c);
static cell grid_at(struct graph const *, int r, int c);
static uint16_t sort_vertices(char, char);
static int vertex_degree(struct vertex const *);
static void build_path_outline(struct graph *);
static void build_path_cell(struct graph *, int r, int c, cell);
static void clear_and_flush_graph(struct graph const *, char const *edge_color);
static void print_cell(cell, char const *edge_color);
static char get_cell_vertex_title(cell);
static bool has_edge_with(struct vertex const *, char);
static bool add_edge(struct vertex *, struct edge const *);
static bool is_edge_vertex(cell, cell);
static bool is_valid_edge_cell(cell, cell);
static void clear_paint(struct graph *);
static bool is_vertex(cell);
static bool is_path_cell(cell);
static struct vertex *vertex_at(struct graph const *g, char name);
static struct cost *map_pq_at(struct cost const *dj_arr, char vertex);
static int paint_shortest_path(struct graph *, struct cost const *,
                               struct cost const *);

static void encode_digits(struct graph const *, struct digit_encoding *);
static enum label_orientation get_direction(struct point const *,
                                            struct point const *);
static struct int_conversion parse_digits(str_view, int lower_bound,
                                          int upper_bound, char const *err_msg);
static struct path_request parse_path_request(struct graph *, str_view);
static void help(void);

static threeway_cmp cmp_pq_costs(any_type_cmp);
static ccc_tribool eq_parent_cells(any_key_cmp);
static uint64_t hash_parent_cells(any_key point_struct);
static uint64_t hash_64_bits(uint64_t);

static unsigned count_digits(uintmax_t n);

/*======================  Main Arg Handling  ===============================*/

int
main(int argc, char **argv)
{
    /* Randomness will be used throughout the program but it need not be
       perfect. It only helps build graphs. */
    srand(time(NULL)); /* NOLINT */
    struct graph graph = {
        .rows = DEFAULT_ROWS,
        .cols = DEFAULT_COLS,
        .vertices = DEFAULT_VERTICES,
        .speed = {},
        .grid = NULL,
        .graph = network,
    };
    for (int i = 1; i < argc; ++i)
    {
        str_view const arg = sv(argv[i]);
        if (sv_starts_with(arg, SV("-r=")))
        {
            struct int_conversion const row_arg
                = parse_digits(arg, ROW_COL_MIN, INT_MAX,
                               "rows_below required minimum or negative\n");
            graph.rows = row_arg.conversion;
        }
        else if (sv_starts_with(arg, SV("-c=")))
        {
            struct int_conversion const col_arg
                = parse_digits(arg, ROW_COL_MIN, INT_MAX,
                               "cols below required minimum or negative.\n");
            graph.cols = col_arg.conversion;
        }
        else if (sv_starts_with(arg, SV("-v=")))
        {
            struct int_conversion const vert_arg
                = parse_digits(arg, 1, MAX_VERTICES,
                               "vertices outside of valid range (1-26).\n");
            graph.vertices = vert_arg.conversion;
        }
        else if (sv_starts_with(arg, SV("-s=")))
        {
            struct int_conversion const vert_arg = parse_digits(
                arg, 0, MAX_SPEED,
                "animation speed outside of valid range (1-7).\n");
            graph.speed.tv_nsec = speeds[vert_arg.conversion];
        }
        else if (sv_starts_with(arg, SV("-h")))
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
    if (!graph.rows || !graph.cols)
    {
        quit("graph rows or cols is 0.\n", 1);
        return 1;
    }
    graph.grid = calloc((size_t)graph.rows * graph.cols, sizeof(cell));
    if (!graph.grid)
    {
        quit("allocation failure for specified graph size.\n", 1);
        return 1;
    }
    build_graph(&graph);
    find_shortest_paths(&graph);
    set_cursor_position(graph.rows + 1, graph.cols + 1);
    printf("\n");
    free(graph.grid);
}

/*========================   Graph Building    ==============================*/

/* The undirected graphs produced are randomly generated graphs where each
   vertex is placed on the grid of terminal cells. Each vertex then tries to
   connect a random number of out edges to vertices that can accept an in edge.
   The number of cells taken to connect the edge to another other vertex is the
   cost/weight of that edge. */
static void
build_graph(struct graph *const graph)
{
    build_path_outline(graph);
    clear_and_flush_graph(graph, MAG);
    for (int vertex = 0, vertex_title = BEGIN_VERTICES;
         vertex < graph->vertices; ++vertex, ++vertex_title)
    {
        struct point rand_point = random_vertex_placement(graph);
        *grid_at_mut(graph, rand_point.r, rand_point.c)
            = VERTEXT_BIT | PATH_BIT
            | ((cell)vertex_title << VERTEX_CELL_TITLE_SHIFT);
        *vertex_at(graph, (char)vertex_title) = (struct vertex){
            .name = (char)vertex_title,
            .pos = rand_point,
        };
    }
    for (int vertex = 0, vertex_title = BEGIN_VERTICES;
         vertex < graph->vertices; ++vertex, ++vertex_title)
    {
        char key = (char)vertex_title;
        struct vertex *const src = vertex_at(graph, key);
        while (vertex_degree(src) < MAX_DEGREE && found_dst(graph, src))
        {}
    }
    clear_and_flush_graph(graph, MAG);
}

/* This function uses a breadth first search to find the closest vertex it has
   not already formed an edge with. If the dst has an available out degree it
   will form an edge with src. This creates graphs that require the later
   Dijkstra's algorithm to be correct because there will likely be multiple
   paths between vertices that may have small differences in distance. The
   graphs also are more visually pleasing and make some sense because routes
   are formed efficiently due to the bfs. */
static bool
found_dst(struct graph *const graph, struct vertex *const src)
{
    flat_hash_map parent_map
        = fhm_init(NULL, struct path_backtrack_cell, current, hash_parent_cells,
                   eq_parent_cells, std_alloc, NULL, 0);
    flat_double_ended_queue bfs
        = fdeq_init(NULL, struct point, std_alloc, NULL, 0);
    entry *e = fhm_insert_or_assign_w(
        &parent_map, src->pos,
        (struct path_backtrack_cell){.parent = {-1, -1}});
    check(!insert_error(e));
    (void)push_back(&bfs, &src->pos);
    bool dst_connection = false;
    while (!is_empty(&bfs))
    {
        struct point cur = *((struct point *)front(&bfs));
        (void)pop_front(&bfs);
        for (size_t i = 0; i < DIRS_SIZE; ++i)
        {
            struct point next
                = {.r = cur.r + dirs[i].r, .c = cur.c + dirs[i].c};
            struct path_backtrack_cell push = {.current = next, .parent = cur};
            cell const next_cell = grid_at(graph, next.r, next.c);
            if (is_vertex(next_cell))
            {
                struct vertex *const nv
                    = vertex_at(graph, get_cell_vertex_title(next_cell));
                if (src->name != nv->name && vertex_degree(nv) < MAX_DEGREE
                    && !has_edge_with(src, nv->name))
                {
                    entry const in = insert_or_assign(&parent_map, &push);
                    check(!insert_error(&in));
                    edge_construct(graph, &parent_map, src, nv);
                    dst_connection = true;
                    goto done;
                }
            }
            if (!is_path_cell(next_cell)
                && !occupied(try_insert_r(&parent_map, &push)))
            {
                struct point const *const n = push_back(&bfs, &next);
                check(n);
            }
        }
    }
done:
    (void)clear_and_free(&bfs, NULL);
    (void)clear_and_free(&parent_map, NULL);
    return dst_connection;
}

/* Assumes that src and dst have not already been connected in the graph or in
   the terminal cells via edge ids. Creates the appropriate edge and updates the
   edge lists of src and dst. */
static void
edge_construct(struct graph *const g, flat_hash_map *const parent_map,
               struct vertex *const src, struct vertex *const dst)
{
    cell const edge_id = make_edge(src->name, dst->name);
    struct point cur = dst->pos;
    struct path_backtrack_cell const *c = get_key_val(parent_map, &cur);
    check(c);
    struct edge edge = {.n = {.name = dst->name, .cost = 0}, .pos = dst->pos};
    while (c->parent.r > 0)
    {
        c = get_key_val(parent_map, &c->parent);
        check(c, printf("Cannot find cell parent to rebuild path.\n"););
        ++edge.n.cost;
        *grid_at_mut(g, c->current.r, c->current.c) |= edge_id;
        build_path_cell(g, c->current.r, c->current.c, edge_id);
    }
    (void)add_edge(src, &edge);
    edge.n.name = src->name;
    edge.pos = src->pos;
    (void)add_edge(dst, &edge);
    add_edge_cost_label(g, dst, &edge);
}

/* A edge cost label will only be added if there is sufficient space. For
   edges that are too small to fit a digit or two the line length can be
   easily counted with the mouse or by eye. */
static void
add_edge_cost_label(struct graph *const g, struct vertex *const src,
                    struct edge const *const e)
{
    struct point cur = src->pos;
    cell const edge_id = make_edge(src->name, e->n.name);
    struct point prev = cur;
    /* Add a two space buffer to either side of the label so direction of lines
       is not lost to writing of digits. Otherwise it would be unclear which
       edge a cost is associated with if close to other costs. */
    size_t const spaces_needed_for_cost = count_digits(e->n.cost) + 2;
    size_t consecutive_spaces_found = 0;
    enum label_orientation direction = NORTH;
    while (cur.r != e->pos.r || cur.c != e->pos.c)
    {
        if (consecutive_spaces_found == spaces_needed_for_cost)
        {
            encode_digits(g, &(struct digit_encoding){
                                 .start = cur,
                                 .cost = e->n.cost,
                                 .spaces_needed = spaces_needed_for_cost,
                                 .orientation = direction});
            return;
        }
        for (size_t i = 0; i < DIRS_SIZE; ++i)
        {
            struct point next
                = {.r = cur.r + dirs[i].r, .c = cur.c + dirs[i].c};
            cell const next_cell = grid_at(g, next.r, next.c);
            if ((next_cell & VERTEXT_BIT)
                && get_cell_vertex_title(next_cell) == e->n.name)
            {
                return;
            }
            /* Always make forward progress, no backtracking. */
            if ((grid_at(g, next.r, next.c) & EDGE_ID_MASK) == edge_id
                && (prev.r != next.r || prev.c != next.c))
            {
                direction = get_direction(&prev, &next);
                direction == DIAGONAL ? consecutive_spaces_found = 0
                                      : ++consecutive_spaces_found;
                prev = cur;
                cur = next;
                break;
            }
        }
    }
}

/* Digits will be encoded so that they are readable by English language
   standards. That means digits will either appear on a line to be
   read left to right or top to bottom. This means that digits of the
   numbers must either be handled left to right or right to left as they
   are laid down such that they are read correctly. */
static void
encode_digits(struct graph const *const g, struct digit_encoding *const e)
{
    if (e->orientation == NORTH || e->orientation == SOUTH)
    {
        e->start.r = e->orientation == NORTH
                       ? e->start.r + (int)e->spaces_needed - 2
                       : e->start.r - 1;
        for (uintmax_t digits = e->cost; digits; digits /= 10, --e->start.r)
        {
            *grid_at_mut(g, e->start.r, e->start.c) |= DIGIT_BIT;
            *grid_at_mut(g, e->start.r, e->start.c)
                |= ((digits % 10) << DIGIT_SHIFT);
        }
    }
    else
    {
        e->start.c = e->orientation == WEST
                       ? e->start.c + (int)e->spaces_needed - 2
                       : e->start.c - 1;
        for (uintmax_t digits = e->cost; digits; digits /= 10, --e->start.c)
        {
            *grid_at_mut(g, e->start.r, e->start.c) |= DIGIT_BIT;
            *grid_at_mut(g, e->start.r, e->start.c)
                |= ((digits % 10) << DIGIT_SHIFT);
        }
    }
}

static enum label_orientation
get_direction(struct point const *const prev, struct point const *const next)
{
    struct point const diff = {.r = next->r - prev->r, .c = next->c - prev->c};
    if (diff.c && !diff.r && diff.c > 0)
    {
        return EAST;
    }
    if (diff.c && !diff.r && diff.c < 0)
    {
        return WEST;
    }
    if (diff.r && !diff.c && diff.r > 0)
    {
        return SOUTH;
    }
    if (diff.r && !diff.c && diff.r < 0)
    {
        return NORTH;
    }
    return DIAGONAL;
}

static struct point
random_vertex_placement(struct graph const *const graph)
{
    int const row_end = graph->rows - 2;
    int const col_end = graph->cols - 2;
    if (row_end < ROW_COL_MIN || col_end < ROW_COL_MIN)
    {
        quit("rows and cols are below minimum quitting now.\n", 1);
        exit(1);
    }
    /* No vertices should be close to the edge of the map. */
    int const row_start = rand_range(VERTEX_PLACEMENT_PADDING,
                                     graph->rows - VERTEX_PLACEMENT_PADDING);
    int const col_start = rand_range(VERTEX_PLACEMENT_PADDING,
                                     graph->cols - VERTEX_PLACEMENT_PADDING);
    bool exhausted = false;
    for (int row = row_start; !exhausted && row < row_end;
         row = (row + 1) % row_end)
    {
        if (!row)
        {
            row = VERTEX_PLACEMENT_PADDING;
        }
        for (int col = col_start; !exhausted && col < col_end;
             col = (col + 1) % col_end)
        {
            if (!col)
            {
                col = VERTEX_PLACEMENT_PADDING;
            }
            struct point const cur = {.r = row, .c = col};
            if (is_valid_vertex_pos(graph, cur.r, cur.c))
            {
                return cur;
            }
            exhausted = ((row + 1) % graph->rows) == row_start
                     && ((col + 1) % graph->cols) == col_start;
        }
    }
    quit("cannot find a place for another vertex on this grid, quitting now.\n",
         1);
    exit(1);
}

/*========================    Graph Solving    ==============================*/

static void
find_shortest_paths(struct graph *const graph)
{
    char *lineptr = NULL;
    size_t len = 0;
    int total_cost = 0;
    do
    {
        set_cursor_position(graph->rows, 0);
        clear_line();
        if (total_cost == INT_MAX)
        {
            (void)fprintf(stdout, "Total Cost: INFINITY\n");
        }
        else
        {
            (void)fprintf(stdout, "Total Cost: %d\n", total_cost);
        }
        sv_print(stdout, prompt_msg);
        ssize_t read = 0;
        while ((read = getline(&lineptr, &len, stdin)) > 0)
        {
            struct path_request pr = parse_path_request(
                graph, (str_view){.s = lineptr, .len = read - 1});
            if (pr.src == 'q')
            {
                free(lineptr);
                printf("Exiting now.\n");
                return;
            }
            if (!pr.src)
            {
                clear_line();
                free(lineptr);
                printf("Please provide any source and destination vertex "
                       "represented in the grid\nExamples: AB, A B, B-C, X->Y, "
                       "DtoF\nMost formats work but two capital vertices are "
                       "needed.\n");
                return;
            }
            total_cost = dijkstra_shortest_path(graph, pr.src, pr.dst);
            if (total_cost == INT_MAX)
            {
                struct point const *const src = &vertex_at(graph, pr.src)->pos;
                struct point const *const dst = &vertex_at(graph, pr.dst)->pos;
                set_cursor_position(src->r, src->c);
                printf("%s%c", RED, pr.src);
                set_cursor_position(dst->r, dst->c);
                printf("%c%s", pr.dst, NIL);
                (void)fflush(stdout);
            }
            break;
        }
    }
    while (1);
}

/** Returns the distance of the shortest path from src to dst in the graph. If
no route exists INT_MAX is returned which can be interpreted as INFINITY in this
context. Assumes the graph is well formed without negative distances. */
static int
dijkstra_shortest_path(struct graph *const graph, char const src,
                       char const dst)
{
    clear_paint(graph);
    clear_and_flush_graph(graph, NIL);
    /* One struct cost will represent the path rebuilding map and the
       priority queue. The intrusive pq elem will give us an O(1) (technically
       o(lg N)) decrease key. The pq element is not given allocation permissions
       so that push and pop from the pq only affects the priority queue data
       structure not the memory that is used to store the elements; the path
       rebuild map remains accessible. Best of all, maximum pq/map size is known
       to be small [A-Z] so provide memory on the stack for speed and safety. */
    struct cost map_pq[MAX_VERTICES] = {};
    priority_queue costs
        = pq_init(struct cost, pq_elem, CCC_LES, cmp_pq_costs, NULL, NULL);
    for (int i = 0, vx = BEGIN_VERTICES; i < graph->vertices; ++i, ++vx)
    {
        *map_pq_at(map_pq, (char)vx) = (struct cost){
            .name = (char)vx,
            .from = '\0',
            .cost = (char)vx == src ? 0 : INT_MAX,
        };
        struct cost const *const v
            = push(&costs, &map_pq_at(map_pq, (char)vx)->pq_elem);
        check(v);
    }
    while (!is_empty(&costs))
    {
        /* The reference to u is valid after the pop because the pop does not
           deallocate any memory. The pq has no allocation permissions. */
        struct cost const *u = front(&costs);
        (void)pop(&costs);
        if (u->cost == INT_MAX)
        {
            return INT_MAX;
        }
        if (u->name == dst)
        {
            return paint_shortest_path(graph, map_pq, u);
        }
        struct node const *const edges = vertex_at(graph, u->name)->edges;
        for (int i = 0; i < MAX_DEGREE && edges[i].name; ++i)
        {
            struct cost *const v = map_pq_at(map_pq, edges[i].name);
            int const alt = u->cost + edges[i].cost;
            if (alt < v->cost)
            {
                /* Build the map with the appropriate best candidate parent. */
                struct cost const *const relax
                    = pq_decrease_w(&costs, v,
                                    {
                                        T->cost = alt;
                                        T->from = u->name;
                                    });
                check(relax == v);
                paint_edge(graph, u->name, v->name, MAG);
                nanosleep(&graph->speed, NULL);
            }
        }
    }
    return INT_MAX;
}

/** Returns the shortest path total distance while painting the lines as a side
effect of this process. The edges will be painted a color different than the
color used while considering paths to clearly indicate it is the shortest. */
static int
paint_shortest_path(struct graph *const graph, struct cost const *const map_pq,
                    struct cost const *u)
{
    int total = 0;
    for (; u->from; u = map_pq_at(map_pq, u->from))
    {
        struct node const *const edges = vertex_at(graph, u->name)->edges;
        int i = 0;
        for (; i < MAX_DEGREE && edges[i].name != u->from; ++i)
        {}
        check(i < MAX_DEGREE && edges[i].name);
        total += edges[i].cost;
        paint_edge(graph, u->name, u->from, CYN);
        nanosleep(&graph->speed, NULL);
    }
    return total;
}

static inline struct cost *
map_pq_at(struct cost const *const dj_arr, char const vertex)
{
    check(vertex >= BEGIN_VERTICES && vertex <= END_VERTICES);
    return (struct cost *)&dj_arr[vertex - BEGIN_VERTICES];
}

/* Paints the edge and flushes the specified color at that position. If NULL
   is passed as the edge color then the paint bit is removed and default path
   color will be flushed at the patch square location. */
static void
paint_edge(struct graph *const g, char const src_name, char const dst_name,
           char const *const edge_color)
{
    struct vertex const *const src = vertex_at(g, src_name);
    struct vertex const *const dst = vertex_at(g, dst_name);
    struct point cur = src->pos;
    cell const edge_id = make_edge(src->name, dst->name);
    struct point prev = cur;
    while (cur.r != dst->pos.r || cur.c != dst->pos.c)
    {
        if (edge_color)
        {
            *grid_at_mut(g, cur.r, cur.c) |= PAINT_BIT;
        }
        else
        {
            *grid_at_mut(g, cur.r, cur.c) &= ~PAINT_BIT;
        }
        flush_at(g, cur.r, cur.c, edge_color);
        for (size_t i = 0; i < DIRS_SIZE; ++i)
        {
            struct point next = {
                .r = cur.r + dirs[i].r,
                .c = cur.c + dirs[i].c,
            };
            cell const next_cell = grid_at(g, next.r, next.c);
            if ((next_cell & VERTEXT_BIT)
                && get_cell_vertex_title(next_cell) == dst->name)
            {
                return;
            }
            /* Always make forward progress, no backtracking. */
            if ((grid_at(g, next.r, next.c) & EDGE_ID_MASK) == edge_id
                && (prev.r != next.r || prev.c != next.c))
            {
                prev = cur;
                cur = next;
                break;
            }
        }
    }
}

/*========================  Graph/Grid Helpers  =============================*/

static struct vertex *
vertex_at(struct graph const *const g, char const name)
{
    check(name >= BEGIN_VERTICES && name < BEGIN_VERTICES + g->vertices);
    return &((struct vertex *)g->graph)[((size_t)name - BEGIN_VERTICES)];
}

/* This function assumes that checking one cell in any direction is safe
   and within bounds. The vertices are only placed with padding around the
   full grid space so this assumption should be safe. */
static inline bool
is_valid_vertex_pos(struct graph const *graph, int const r, int const c)
{
    return !(grid_at(graph, r, c) & VERTEXT_BIT)
        && !(grid_at(graph, r + 1, c) & VERTEXT_BIT)
        && !(grid_at(graph, r - 1, c) & VERTEXT_BIT)
        && !(grid_at(graph, r, c - 1) & VERTEXT_BIT)
        && !(grid_at(graph, r, c + 1) & VERTEXT_BIT);
}

static int
vertex_degree(struct vertex const *const v)
{
    int n = 0;
    /* Vertexes are initialized with zeroed out edge array. Falsey. */
    for (int i = 0; i < MAX_DEGREE && v->edges[i].name; ++i, ++n)
    {}
    return n;
}

static inline cell
make_edge(char const src, char const dst)
{
    return sort_vertices(src, dst) << EDGE_ID_SHIFT;
}

static inline cell *
grid_at_mut(struct graph const *const graph, int const r, int const c)
{
    return &graph->grid[(r * graph->cols) + c];
}

static inline cell
grid_at(struct graph const *const graph, int const r, int const c)
{
    return graph->grid[(r * graph->cols) + c];
}

static inline uint16_t
sort_vertices(char a, char b)
{
    return a < b ? (a << 8) | b : (b << 8) | a;
}

static char
get_cell_vertex_title(cell const c)
{
    return (char)((c & VERTEX_TITLE_MASK) >> VERTEX_CELL_TITLE_SHIFT);
}

static bool
has_edge_with(struct vertex const *const v, char vertex)
{
    for (int i = 0; i < MAX_DEGREE && v->edges[i].name; ++i)
    {
        if (v->edges[i].name == vertex)
        {
            return true;
        }
    }
    return false;
}

static bool
add_edge(struct vertex *const v, struct edge const *const e)
{
    for (int i = 0; i < MAX_DEGREE; ++i)
    {
        if (!v->edges[i].name)
        {
            v->edges[i] = (struct node){
                .name = e->n.name,
                .cost = e->n.cost,
            };
            return true;
        }
    }
    return false;
}

static inline bool
is_vertex(cell c)
{
    return (c & VERTEXT_BIT) != 0;
}

static bool
is_path_cell(cell c)
{
    return (c & PATH_BIT) != 0;
}

static void
clear_and_flush_graph(struct graph const *const g, char const *const edge_color)
{
    clear_screen();
    for (int row = 0; row < g->rows; ++row)
    {
        for (int col = 0; col < g->cols; ++col)
        {
            set_cursor_position(row, col);
            print_cell(grid_at(g, row, col), edge_color);
        }
        printf("\n");
    }
    (void)fflush(stdout);
}

static void
clear_paint(struct graph *const graph)
{
    for (int r = 0; r < graph->rows; ++r)
    {
        for (int c = 0; c < graph->cols; ++c)
        {
            *grid_at_mut(graph, r, c) &= ~PAINT_BIT;
        }
    }
}

static inline void
flush_at(struct graph const *const g, int const r, int const c,
         char const *const edge_color)
{
    set_cursor_position(r, c);
    print_cell(grid_at(g, r, c), edge_color);
    (void)fflush(stdout);
}

static inline void
print_cell(cell const c, char const *const edge_color)
{

    if (c & VERTEXT_BIT)
    {
        printf("%s%c%s", CYN, (char)(c >> VERTEX_CELL_TITLE_SHIFT), NIL);
    }
    else if (c & DIGIT_BIT)
    {
        printf("%d", (c & DIGIT_MASK) >> DIGIT_SHIFT);
    }
    else if (c & PATH_BIT)
    {
        (c & PAINT_BIT) ? printf("%s%s%s", edge_color ? edge_color : NIL,
                                 paths[c & PATH_MASK], NIL)
                        : printf("%s", paths[c & PATH_MASK]);
    }
    else if (!(c & PATH_BIT))
    {
        printf(" ");
    }
    else
    {
        (void)fprintf(stderr, "uncategorizable square.\n");
    }
}

static bool
is_edge_vertex(cell const square, cell edge_id)
{
    char const vertex_name = get_cell_vertex_title(square);
    char const edge_vertex1
        = (char)((edge_id & L_EDGE_ID_MASK) >> L_EDGE_ID_SHIFT);
    char const edge_vertex2
        = (char)((edge_id & R_EDGE_ID_MASK) >> R_EDGE_ID_SHIFT);
    return vertex_name == edge_vertex1 || vertex_name == edge_vertex2;
}

static bool
is_valid_edge_cell(cell const square, cell const edge_id)
{
    return ((square & VERTEXT_BIT) && is_edge_vertex(square, edge_id))
        || ((square & PATH_BIT) && (square & EDGE_ID_MASK) == edge_id);
}

static void
build_path_cell(struct graph *g, int const r, int const c, cell const edge_id)
{
    cell path = PATH_BIT;
    if (r - 1 >= 0 && is_valid_edge_cell(grid_at(g, r - 1, c), edge_id))
    {
        path |= NORTH_PATH;
        *grid_at_mut(g, r - 1, c) |= SOUTH_PATH;
    }
    if (r + 1 < g->rows && is_valid_edge_cell(grid_at(g, r + 1, c), edge_id))
    {
        path |= SOUTH_PATH;
        *grid_at_mut(g, r + 1, c) |= NORTH_PATH;
    }
    if (c - 1 >= 0 && is_valid_edge_cell(grid_at(g, r, c - 1), edge_id))
    {
        path |= WEST_PATH;
        *grid_at_mut(g, r, c - 1) |= EAST_PATH;
    }
    if (c + 1 < g->cols && is_valid_edge_cell(grid_at(g, r, c + 1), edge_id))
    {
        path |= EAST_PATH;
        *grid_at_mut(g, r, c + 1) |= WEST_PATH;
    }
    *grid_at_mut(g, r, c) |= path;
}

static void
build_path_outline(struct graph *graph)
{
    for (int row = 0; row < graph->rows; row++)
    {
        for (int col = 0; col < graph->cols; col++)
        {
            if (col == 0 || col == graph->cols - 1 || row == 0
                || row == graph->rows - 1)
            {
                build_path_cell(graph, row, col, 0);
            }
        }
    }
}

/*====================    Data Structure Helpers    =========================*/

static ccc_tribool
eq_parent_cells(any_key_cmp const c)
{
    struct point const *const p = c.any_key_lhs;
    struct path_backtrack_cell const *const pc = c.any_type_rhs;
    return pc->current.r == p->r && pc->current.c == p->c;
}

static uint64_t
hash_parent_cells(any_key const point_struct)
{
    struct point const *const p = point_struct.any_key;
    uint64_t const wr = p->r;
    return hash_64_bits((wr << 31) | p->c);
}

static threeway_cmp
cmp_pq_costs(any_type_cmp const cost_cmp)
{
    struct cost const *const a = cost_cmp.any_type_lhs;
    struct cost const *const b = cost_cmp.any_type_rhs;
    return (a->cost > b->cost) - (a->cost < b->cost);
}

static uint64_t
hash_64_bits(uint64_t x)
{
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

/*===========================    Misc    ====================================*/

/* Parses the path request. Returns the character 'q' as the src and dst if
   the user has requested to quit. Otherwise tries to parse. If parsing
   cannot be completed an empty path request with the null terminator is
   returned. */
static struct path_request
parse_path_request(struct graph *const g, str_view r)
{
    if (sv_contains(r, quit_cmd))
    {
        return (struct path_request){'q', 'q'};
    }
    struct path_request res = {};
    char const end_title = (char)(BEGIN_VERTICES + g->vertices - 1);
    for (char const *c = sv_begin(r); c != sv_end(r); c = sv_next(c))
    {
        if (*c >= BEGIN_VERTICES && *c <= end_title)
        {
            struct vertex *v = vertex_at(g, *c);
            check(v);
            res.src ? (res.dst = v->name) : (res.src = v->name);
        }
    }
    return res.src && res.dst ? res : (struct path_request){};
}

static struct int_conversion
parse_digits(str_view arg, int const lower_bound, int const upper_bound,
             char const *const err_msg)
{
    size_t const eql = sv_rfind(arg, sv_npos(arg), SV("="));
    if (eql == sv_npos(arg))
    {
        return (struct int_conversion){.status = CONV_ER};
    }
    arg = sv_substr(arg, eql + 1, ULLONG_MAX);
    if (sv_empty(arg))
    {
        (void)fprintf(stderr, "please specify element to convert.\n");
        return (struct int_conversion){.status = CONV_ER};
    }
    struct int_conversion res = convert_to_int(sv_begin(arg));
    if (res.status == CONV_ER || res.conversion > upper_bound
        || res.conversion < lower_bound)
    {
        printf("flag argument outside of valid range (%d-%d).\n", lower_bound,
               upper_bound);
        quit(err_msg, 1);
    }
    return res;
}

static unsigned
count_digits(uintmax_t n)
{
    unsigned res = 0;
    for (; n; ++res, n /= 10)
    {}
    return res;
}

static void
help(void)
{
    (void)fprintf(
        stdout,
        "graph.c\nBuilds weighted graphs for Dijkstra's Algorithm to "
        "demonstrate usage of the priority queue and map provided by this "
        "library.\nUsage:\n-r=N The row flag lets you specify area for grid "
        "rows > 7.\n-c=N The col flag lets you specify area for grid cols > "
        "7.\n-v=N specify 1-26 vertices for the randomly generated and "
        "connected graph.\n-s=N specify 1-7 animation speed for Dijkstra's "
        "algorithm.\nExample:\n./build/[debug/]bin/graph -c=111 -r=33 "
        "-v=19 -s=3\nOnce the graph is built seek the shortest path between "
        "two "
        "uppercase vertices. Examples:\nAB\nA->B\nCtoD\nEnter 'q' to quit.\n");
    exit(0);
}
