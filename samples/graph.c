/** The graph program runs Dijkstra's shortest path algorithm over randomly
generated graphs displayed in the terminal.

Builds weighted graphs for Dijkstra's Algorithm to demonstrate usage of the
priority queue and map provided by this library.
Usage:
-r=N The row flag lets you specify area for grid rows > 7.
-c=N The col flag lets you specify area for grid cols > 7.
-v=N specify 1 to 26 vertices for the randomly generated and connected graph.
Example:
./build/[debug/]bin/graph -c=111 -r=33 -v=4
Once the graph is built seek the shortest path between two uppercase vertices.
Examples:
AB
A->B
CtoD
Enter 'q' to quit. */
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
#include "alloc.h"
#include "ccc/flat_double_ended_queue.h"
#include "ccc/flat_hash_map.h"
#include "ccc/priority_queue.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "cli.h"
#include "random.h"
#include "str_view.h"

enum
{
    DIRS_SIZE = 4,
    MAX_VERTICES = 26,
    MAX_DEGREE = 4,
};

/* The highest order 16 bits in the grid shall be reserved for the edge
   id if the square is a path. An edge ID is a concatenation of two
   vertex names. Vertex names are 8 bit characters, so two vertices can
   fit into a uint16_t which we have room for in a Cell. The concatenation
   shall always be sorted alphabetically so an edge connecting vertex A and
   Z will be uint16_t=AZ. Here is the breakdown of bits currently.

   path shape bits───────────────────────────────────┬──┐
   path_bit────────────────────────────────────────┐ │  │
   vertex bit────────────────────────────────────┐ │ │  │
   paint bit───────────────────────────────────┐ │ │ │  │
   digit bit─────────────────────────────────┐ │ │ │ │  │
   vertex title────────────────────┬───────┐ │ │ │ │ │  │
   edge cost digit─────────────────┼────┬──┤ │ │ │ │ │  │
   edge id───────┬──────┬─┬──────┐ │    │  │ │ │ │ │ │  │
               0b00000000 00000000 0000 0000 0 0 0 0 0000
   If various signal bits such as paint or digit are turned on we know
   which bits to look at and how to interpret them.
     - path shape bits determine how edges join as they run and turn.
     - path bit marks a cell as a path
     - vertex bit marks a cell as a vertex with a title in the highest eight
       bits
     - paint bit is a single bit to mark a path should be lit up.
     - digit bit marks that one digit of an edge cost is stored in a edge cell.
     - edge cost digit is stored in at most four bits. 9 is highest digit
       in base 10.
     - unused is not currently used but could be in the future.
     - edge id is the concatenation of two vertex titles in an edge to signify
       which vertices are connected. The edge id is sorted lexicographically
       with the lower value in the leftmost bits.
     - the vertex title is stored in 8 bits if the cell is a vertex. */
typedef uint32_t cell;

struct point
{
    int r;
    int c;
};

struct path_backtrack_cell
{
    fhmap_elem elem;
    struct point current;
    struct point parent;
};

/** A dijkstra_vertex optimizes the problem so that we can store the path
rebuilding map implicitly in an array of dijkstra_vertex[A-Z]. Then the pq
element can run the efficient push and decrease key operations with pointer
stability guaranteed. Finally these can be allocated on the stack because
there will be at most 26 of them which is very small. */
struct dijkstra_vertex
{
    pq_elem pq_elem;
    int dist;
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

/* North, East, South, West */
static struct point const dirs[DIRS_SIZE] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
static char const vertex_titles[MAX_VERTICES] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
};

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
    } while (0)

static str_view const rows = SV("-r=");
static str_view const cols = SV("-c=");
static str_view const vertices_flag = SV("-v=");
static str_view const help_flag = SV("-h");
static int const default_rows = 33;
static int const default_cols = 111;
static int const default_vertices = 4;
static int const row_col_min = 7;
static int const vertex_placement_padding = 3;
static char const start_vertex_title = 'A';
static char const end_vertex_title = 'Z';

static size_t const vertex_cell_title_shift = 8;
static cell const vertex_title_mask = 0xFF00;
static size_t const edge_id_shift = 16;
static cell const edge_id_mask = 0xFFFF0000;
static cell const l_edge_id_mask = 0xFF000000;
static cell const l_edge_id_shift = 24;
static cell const r_edge_id_mask = 0x00FF0000;
static cell const r_edge_id_shift = 16;
static cell const path_mask = 0b1111;
static cell const north_path = 0b0001;
static cell const east_path = 0b0010;
static cell const south_path = 0b0100;
static cell const west_path = 0b1000;
static cell const path_bit = 0b10000;
static cell const vertex_bit = 0b100000;
static cell const paint_bit = 0b1000000;
static cell const digit_bit = 0b10000000;
static size_t const digit_shift = 8;
static cell const digit_mask = 0xF00;

static str_view const prompt_msg
    = SV("Enter two vertices to find the shortest path between them (i.e. "
         "A-Z). Enter q to quit:");
static str_view const quit_cmd = SV("q");

/*==========================   Prototypes  ================================= */

static void build_graph(struct graph *);
static void find_shortest_paths(struct graph *);
static int edge_construction_cost(struct graph *, flat_hash_map *,
                                  struct vertex *, struct vertex *);
static void edge_construct(struct graph *g, flat_hash_map *parent_map,
                           struct vertex *src, struct vertex *dst);
static int dijkstra_shortest_path(struct graph *,
                                  struct dijkstra_vertex *map_pq, char src,
                                  char dst);
static void paint_edge(struct graph *, char, char);
static void add_edge_cost_label(struct graph *, struct vertex *,
                                struct edge const *);
static cell make_edge(char src, char dst);

static struct point random_vertex_placement(struct graph const *);
static bool is_valid_vertex_pos(struct graph const *, int r, int c);
static cell *grid_at_mut(struct graph const *, int r, int c);
static cell grid_at(struct graph const *, int r, int c);
static uint16_t sort_vertices(char, char);
static int vertex_degree(struct vertex const *);
static bool connect_rand_dst(struct graph *, struct vertex *);
static void build_path_outline(struct graph *);
static void build_path_cell(struct graph *, int r, int c, cell);
static void clear_and_flush_graph(struct graph const *);
static void print_cell(cell);
static char get_cell_vertex_title(cell);
static bool has_edge_with(struct vertex const *, char);
static bool add_edge(struct vertex *, struct edge const *);
static bool is_edge_vertex(cell, cell);
static bool is_valid_edge_cell(cell, cell);
static void clear_paint(struct graph *);
static bool is_vertex(cell);
static bool is_path_cell(cell);
static inline bool is_dst(cell c, char dst);
static struct vertex *vertex_at(struct graph const *g, char name);
static struct dijkstra_vertex *map_pq_at(struct dijkstra_vertex const *dj_arr,
                                         char vertex);

static void encode_digits(struct graph const *, struct digit_encoding *);
static enum label_orientation get_direction(struct point const *,
                                            struct point const *);
static struct int_conversion parse_digits(str_view);
static struct path_request parse_path_request(struct graph *, str_view);
static void help(void);

static threeway_cmp cmp_pq_costs(cmp);
static bool eq_parent_cells(key_cmp);
static uint64_t hash_parent_cells(user_key point_struct);
static uint64_t hash_64_bits(uint64_t);

static unsigned count_digits(uintmax_t n);

/*======================  Main Arg Handling  ===============================*/

int
main(int argc, char **argv)
{
    /* Randomness will be used throughout the program but it need not be
       perfect. It only helps build graphs.
       NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp) */
    srand(time(NULL));
    struct graph graph = {.rows = default_rows,
                          .cols = default_cols,
                          .vertices = default_vertices,
                          .grid = NULL,
                          .graph = network};
    for (int i = 1; i < argc; ++i)
    {
        str_view const arg = sv(argv[i]);
        if (sv_starts_with(arg, rows))
        {
            struct int_conversion const row_arg = parse_digits(arg);
            if (row_arg.status == CONV_ER || row_arg.conversion < row_col_min)
            {
                quit("rows below required minimum or negative.\n", 1);
            }
            graph.rows = row_arg.conversion;
        }
        else if (sv_starts_with(arg, cols))
        {
            struct int_conversion const col_arg = parse_digits(arg);
            if (col_arg.status == CONV_ER || col_arg.conversion < row_col_min)
            {
                quit("cols below required minimum or negative.\n", 1);
            }
            graph.cols = col_arg.conversion;
        }
        else if (sv_starts_with(arg, vertices_flag))
        {
            struct int_conversion const vert_arg = parse_digits(arg);
            if (vert_arg.status == CONV_ER || vert_arg.conversion > MAX_VERTICES
                || vert_arg.conversion < 1)
            {
                quit("vertices outside of valid range (1-26).\n", 1);
            }
            graph.vertices = vert_arg.conversion;
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
   vertex is placed on the grid of terminal cells. Each vertex then tries
   to connect a random number of out edges to vertices that can accept an
   in edge. The search for another vertex chooses the shortest path from two
   options: the shortest existing route between src and dst among vertices or
   the cost of building a new edge in the terminal. The number of cells taken to
   connect the edge to another other vertex is the cost/weight of that edge. */
static void
build_graph(struct graph *const graph)
{
    build_path_outline(graph);
    clear_and_flush_graph(graph);
    for (int vertex = 0, vertex_title = start_vertex_title;
         vertex < graph->vertices; ++vertex, ++vertex_title)
    {
        struct point rand_point = random_vertex_placement(graph);
        *grid_at_mut(graph, rand_point.r, rand_point.c)
            = vertex_bit | path_bit
              | ((cell)vertex_title << vertex_cell_title_shift);
        *vertex_at(graph, (char)vertex_title) = (struct vertex){
            .name = (char)vertex_title,
            .pos = rand_point,
        };
    }
    for (int vertex = 0, vertex_title = start_vertex_title;
         vertex < graph->vertices; ++vertex, ++vertex_title)
    {
        char key = (char)vertex_title;
        struct vertex *const src = vertex_at(graph, key);
        prog_assert(src != NULL);
        int const degree = vertex_degree(src);
        if (degree == MAX_DEGREE)
        {
            continue;
        }
        int const out_edges = rand_range(1, MAX_DEGREE - degree);
        for (int i = 0; i < out_edges && connect_rand_dst(graph, src); ++i)
        {}
    }
    clear_and_flush_graph(graph);
}

/* Connects source to random destination vertex. A connection can mean
   confirming an efficient path already exists between source and destination
   across connected vertices or that we build a new edge between source and
   destination. The more efficient (less distance) option is always chosen. */
static bool
connect_rand_dst(struct graph *const graph, struct vertex *const src_vertex)
{
    size_t const graph_size = graph->vertices;
    size_t vertex_title_indices[MAX_VERTICES];
    for (size_t i = 0; i < graph_size; ++i)
    {
        vertex_title_indices[i] = i;
    }
    /* Cycle through all vertices with which to join an edge randomly. */
    rand_shuffle(sizeof(size_t), vertex_title_indices, graph_size,
                 &(size_t){0});
    bool connected = false;
    for (size_t i = 0; i < graph_size && !connected; ++i)
    {
        struct vertex *const dst
            = vertex_at(graph, vertex_titles[vertex_title_indices[i]]);
        if (src_vertex->name == dst->name
            || has_edge_with(src_vertex, dst->name)
            || vertex_degree(dst) == MAX_DEGREE)
        {
            continue;
        }
        /* A graph will look more visually pleasing if it does not make
           excessively dumb choices. Before building a new edge check if a
           good route between source and destination already exists even if
           it is not direct. */
        int const path_cost = dijkstra_shortest_path(
            graph, (struct dijkstra_vertex[MAX_VERTICES]){}, src_vertex->name,
            dst->name);

        flat_hash_map parent_map
            = fhm_init((struct path_backtrack_cell *)NULL, 0, current, elem,
                       std_alloc, hash_parent_cells, eq_parent_cells, NULL);
        int const construction_cost
            = edge_construction_cost(graph, &parent_map, src_vertex, dst);
        if (path_cost < construction_cost)
        {
            connected = true;
        }
        else if (construction_cost != INT_MAX)
        {
            connected = true;
            edge_construct(graph, &parent_map, src_vertex, dst);
        }
        (void)fhm_clear_and_free(&parent_map, NULL);
    }
    return connected;
}

/* This function assumes that the destination is valid. Valid means that
   source is not already connected to destination and that destination
   has less than the maximum allowable in degree. However, edge formation
   still may fail if no path exists from source to destination. The cost
   reported is the distance in cells in the terminal screen it would take
   to complete this path. The parent map will hold the shortest path from dst
   back to src on the terminal screen if it exists. */
static int
edge_construction_cost(struct graph *const graph,
                       flat_hash_map *const parent_map,
                       struct vertex *const src, struct vertex *const dst)
{
    if (src->name == dst->name)
    {
        return 0;
    }
    flat_double_ended_queue bfs
        = fdeq_init((struct point *)NULL, std_alloc, NULL, 0);
    entry *e = fhm_insert_or_assign_w(
        parent_map, src->pos, (struct path_backtrack_cell){.parent = {-1, -1}});
    prog_assert(!insert_error(e));
    (void)push_back(&bfs, &src->pos);
    int cost = -1;
    while (!is_empty(&bfs))
    {
        ++cost;
        struct point cur = *((struct point *)front(&bfs));
        (void)pop_front(&bfs);
        for (size_t i = 0; i < DIRS_SIZE; ++i)
        {
            struct point next
                = {.r = cur.r + dirs[i].r, .c = cur.c + dirs[i].c};
            struct path_backtrack_cell push = {.current = next, .parent = cur};
            cell const next_cell = grid_at(graph, next.r, next.c);
            if (is_dst(next_cell, dst->name))
            {
                ++cost;
                entry const in = insert_or_assign(parent_map, &push.elem);
                prog_assert(!insert_error(&in));
                cur = next;
                (void)fdeq_clear_and_free(&bfs, NULL);
                return cost;
            }
            if (!is_path_cell(next_cell)
                && !occupied(try_insert_r(parent_map, &push.elem)))
            {
                struct point const *const n = push_back(&bfs, &next);
                prog_assert(n);
            }
        }
    }
    (void)fdeq_clear_and_free(&bfs, NULL);
    return INT_MAX;
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
    prog_assert(c);
    struct edge edge = {.n = {.name = dst->name, .cost = 0}, .pos = dst->pos};
    while (c->parent.r > 0)
    {
        c = get_key_val(parent_map, &c->parent);
        prog_assert(c,
                    { printf("Cannot find cell parent to rebuild path.\n"); });
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
            if ((next_cell & vertex_bit)
                && get_cell_vertex_title(next_cell) == e->n.name)
            {
                return;
            }
            /* Always make forward progress, no backtracking. */
            if ((grid_at(g, next.r, next.c) & edge_id_mask) == edge_id
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
    uintmax_t digits = e->cost;
    if (e->orientation == NORTH || e->orientation == SOUTH)
    {
        e->start.r = e->orientation == NORTH
                         ? e->start.r + (int)e->spaces_needed - 2
                         : e->start.r - 1;
        for (; digits; digits /= 10, --e->start.r)
        {
            *grid_at_mut(g, e->start.r, e->start.c) |= digit_bit;
            *grid_at_mut(g, e->start.r, e->start.c)
                |= ((digits % 10) << digit_shift);
        }
    }
    else
    {
        e->start.c = e->orientation == WEST
                         ? e->start.c + (int)e->spaces_needed - 2
                         : e->start.c - 1;
        for (; digits; digits /= 10, --e->start.c)
        {
            *grid_at_mut(g, e->start.r, e->start.c) |= digit_bit;
            *grid_at_mut(g, e->start.r, e->start.c)
                |= ((digits % 10) << digit_shift);
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
    if (row_end < row_col_min || col_end < row_col_min)
    {
        quit("rows and cols are below minimum quitting now.\n", 1);
        exit(1);
    }
    /* No vertices should be close to the edge of the map. */
    int const row_start = rand_range(vertex_placement_padding,
                                     graph->rows - vertex_placement_padding);
    int const col_start = rand_range(vertex_placement_padding,
                                     graph->cols - vertex_placement_padding);
    bool exhausted = false;
    for (int row = row_start; !exhausted && row < row_end;
         row = (row + 1) % row_end)
    {
        if (!row)
        {
            row = vertex_placement_padding;
        }
        for (int col = col_start; !exhausted && col < col_end;
             col = (col + 1) % col_end)
        {
            if (!col)
            {
                col = vertex_placement_padding;
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
    for (;;)
    {
        clear_paint(graph);
        set_cursor_position(graph->rows, 0);
        clear_line();
        sv_print(stdout, prompt_msg);
        size_t len = 0;
        ssize_t read = 0;
        char *lineptr = NULL;
        while ((read = getline(&lineptr, &len, stdin)) > 0)
        {
            struct path_request pr = parse_path_request(
                graph, (str_view){.s = lineptr, .len = read - 1});
            if (!pr.src)
            {
                clear_line();
                free(lineptr);
                quit("Please provide any source and destination vertex "
                     "represented in the grid\nExamples: AB, A B, B-C, X->Y, "
                     "DtoF\nMost formats work but two capital vertices are "
                     "needed.\n",
                     1);
                return;
            }
            struct dijkstra_vertex map_pq[MAX_VERTICES] = {};
            int const cost
                = dijkstra_shortest_path(graph, map_pq, pr.src, pr.dst);
            if (cost != INT_MAX)
            {
                struct dijkstra_vertex const *u = map_pq_at(map_pq, pr.dst);
                for (; u->from; u = map_pq_at(map_pq, u->from))
                {
                    paint_edge(graph, u->name, u->from);
                }
                clear_and_flush_graph(graph);
            }
            else
            {
                clear_paint(graph);
                clear_and_flush_graph(graph);
                struct point const *const src = &vertex_at(graph, pr.src)->pos;
                struct point const *const dst = &vertex_at(graph, pr.dst)->pos;
                set_cursor_position(src->r, src->c);
                printf("\033[38;5;9m%c\033[0m", pr.src);
                set_cursor_position(dst->r, dst->c);
                printf("\033[38;5;9m%c\033[0m", pr.dst);
                (void)fflush(stdout);
            }
            break;
        }
        /* Allocating and freeing in a loop is bad but getline is nice. */
        free(lineptr);
    }
}

static int
dijkstra_shortest_path(struct graph *const graph,
                       struct dijkstra_vertex *const map_pq, char const src,
                       char const dst)
{
    /* One struct dijkstra_vertex will represent the path rebuilding map and the
       priority queue. The intrusive pq elem will give us an O(1) (technically
       o(lg N)) decrease key. The pq element is not given allocation permissions
       so that push and pop from the pq only affects the priority queue data
       structure not the memory that is used to store the elements; the path
       rebuild map remains accessible. Best of all, maximum pq/map size is known
       to be small [A-Z] so provide memory on the stack for speed and safety. */
    priority_queue distances = pq_init(struct dijkstra_vertex, pq_elem, CCC_LES,
                                       NULL, cmp_pq_costs, NULL);
    for (int i = 0, vx = start_vertex_title; i < graph->vertices; ++i, ++vx)
    {
        *map_pq_at(map_pq, (char)vx) = (struct dijkstra_vertex){
            .name = (char)vx,
            .from = '\0',
            .dist = (char)vx == src ? 0 : INT_MAX,
        };
        prog_assert(push(&distances, &map_pq_at(map_pq, vx)->pq_elem));
    }
    struct dijkstra_vertex const *u = NULL;
    while (!is_empty(&distances))
    {
        /* The reference to u is valid after the pop because the pop does not
           deallocate any memory. The pq has no allocation permissions. */
        u = front(&distances);
        (void)pop(&distances);
        if (u->name == dst || u->dist == INT_MAX)
        {
            return u->dist;
        }
        struct node const *const edges = vertex_at(graph, u->name)->edges;
        for (int i = 0; i < MAX_DEGREE && edges[i].name; ++i)
        {
            struct dijkstra_vertex *const v = map_pq_at(map_pq, edges[i].name);
            int const alt = u->dist + edges[i].cost;
            if (alt < v->dist)
            {
                /* Build the map with the appropriate best candidate parent. */
                bool const relax = pq_decrease_w(&distances, &v->pq_elem, {
                    v->dist = alt;
                    v->from = u->name;
                });
                prog_assert(relax == true);
            }
        }
    }
    return INT_MAX;
}

static inline struct dijkstra_vertex *
map_pq_at(struct dijkstra_vertex const *const dj_arr, char const vertex)
{
    prog_assert(vertex >= start_vertex_title && vertex <= end_vertex_title);
    return (struct dijkstra_vertex *)&dj_arr[vertex - start_vertex_title];
}

static void
paint_edge(struct graph *const g, char const src_name, char const dst_name)
{
    struct vertex const *const src = vertex_at(g, src_name);
    struct vertex const *const dst = vertex_at(g, dst_name);
    struct point cur = src->pos;
    cell const edge_id = make_edge(src->name, dst->name);
    struct point prev = cur;
    while (cur.r != dst->pos.r || cur.c != dst->pos.c)
    {
        *grid_at_mut(g, cur.r, cur.c) |= paint_bit;
        for (size_t i = 0; i < DIRS_SIZE; ++i)
        {
            struct point next = {
                .r = cur.r + dirs[i].r,
                .c = cur.c + dirs[i].c,
            };
            cell const next_cell = grid_at(g, next.r, next.c);
            if ((next_cell & vertex_bit)
                && get_cell_vertex_title(next_cell) == dst->name)
            {
                return;
            }
            /* Always make forward progress, no backtracking. */
            if ((grid_at(g, next.r, next.c) & edge_id_mask) == edge_id
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

static inline bool
is_dst(cell const c, char const dst)
{
    return is_vertex(c) && get_cell_vertex_title(c) == dst;
}

static struct vertex *
vertex_at(struct graph const *const g, char const name)
{
    prog_assert(name >= start_vertex_title
                && name < start_vertex_title + g->vertices);
    return &((struct vertex *)g->graph)[((size_t)name - start_vertex_title)];
}

/* This function assumes that checking one cell in any direction is safe
   and within bounds. The vertices are only placed with padding around the
   full grid space so this assumption should be safe. */
static inline bool
is_valid_vertex_pos(struct graph const *graph, int const r, int const c)
{
    return !(grid_at(graph, r, c) & vertex_bit)
           && !(grid_at(graph, r + 1, c) & vertex_bit)
           && !(grid_at(graph, r - 1, c) & vertex_bit)
           && !(grid_at(graph, r, c - 1) & vertex_bit)
           && !(grid_at(graph, r, c + 1) & vertex_bit);
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
    return sort_vertices(src, dst) << edge_id_shift;
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
    return (char)((c & vertex_title_mask) >> vertex_cell_title_shift);
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
    return (c & vertex_bit) != 0;
}

static bool
is_path_cell(cell c)
{
    return (c & path_bit) != 0;
}

static void
clear_and_flush_graph(struct graph const *const g)
{
    clear_screen();
    for (int row = 0; row < g->rows; ++row)
    {
        for (int col = 0; col < g->cols; ++col)
        {
            set_cursor_position(row, col);
            print_cell(grid_at(g, row, col));
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
            *grid_at_mut(graph, r, c) &= ~paint_bit;
        }
    }
}

static void
print_cell(cell const c)
{

    if (c & vertex_bit)
    {
        printf("\033[38;5;14m%c\033[0m", (char)(c >> vertex_cell_title_shift));
    }
    else if (c & digit_bit)
    {
        printf("%d", (c & digit_mask) >> digit_shift);
    }
    else if (c & path_bit)
    {
        (c & paint_bit) ? printf("\033[38;5;13m%s\033[0m", paths[c & path_mask])
                        : printf("%s", paths[c & path_mask]);
    }
    else if (!(c & path_bit))
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
        = (char)((edge_id & l_edge_id_mask) >> l_edge_id_shift);
    char const edge_vertex2
        = (char)((edge_id & r_edge_id_mask) >> r_edge_id_shift);
    return vertex_name == edge_vertex1 || vertex_name == edge_vertex2;
}

static bool
is_valid_edge_cell(cell const square, cell const edge_id)
{
    return ((square & vertex_bit) && is_edge_vertex(square, edge_id))
           || ((square & path_bit) && (square & edge_id_mask) == edge_id);
}

static void
build_path_cell(struct graph *g, int const r, int const c, cell const edge_id)
{
    cell path = path_bit;
    if (r - 1 >= 0 && is_valid_edge_cell(grid_at(g, r - 1, c), edge_id))
    {
        path |= north_path;
        *grid_at_mut(g, r - 1, c) |= south_path;
    }
    if (r + 1 < g->rows && is_valid_edge_cell(grid_at(g, r + 1, c), edge_id))
    {
        path |= south_path;
        *grid_at_mut(g, r + 1, c) |= north_path;
    }
    if (c - 1 >= 0 && is_valid_edge_cell(grid_at(g, r, c - 1), edge_id))
    {
        path |= west_path;
        *grid_at_mut(g, r, c - 1) |= east_path;
    }
    if (c + 1 < g->cols && is_valid_edge_cell(grid_at(g, r, c + 1), edge_id))
    {
        path |= east_path;
        *grid_at_mut(g, r, c + 1) |= west_path;
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

static bool
eq_parent_cells(key_cmp const c)
{
    struct path_backtrack_cell const *const pc = c.user_type_rhs;
    struct point const *const p = c.key_lhs;
    return pc->current.r == p->r && pc->current.c == p->c;
}

static uint64_t
hash_parent_cells(user_key const point_struct)
{
    struct point const *const p = point_struct.user_key;
    uint64_t const wr = p->r;
    return hash_64_bits((wr << 31) | p->c);
}

static threeway_cmp
cmp_pq_costs(cmp const cost_cmp)
{
    struct dijkstra_vertex const *const a = cost_cmp.user_type_lhs;
    struct dijkstra_vertex const *const b = cost_cmp.user_type_rhs;
    return (a->dist > b->dist) - (a->dist < b->dist);
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

static struct path_request
parse_path_request(struct graph *const g, str_view r)
{
    if (sv_contains(r, quit_cmd))
    {
        quit("Exiting now.\n", 0);
    }
    struct path_request res = {};
    char const end_title = (char)(start_vertex_title + g->vertices - 1);
    for (char const *c = sv_begin(r); c != sv_end(r); c = sv_next(c))
    {
        if (*c >= start_vertex_title && *c <= end_title)
        {
            struct vertex *v = vertex_at(g, *c);
            prog_assert(v);
            res.src ? (res.dst = v->name) : (res.src = v->name);
        }
    }
    return res.src && res.dst ? res : (struct path_request){};
}

static struct int_conversion
parse_digits(str_view arg)
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
    return convert_to_int(sv_begin(arg));
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
        "7.\n-v=N specify 1 to 26 vertices for the randomly generated and "
        "connected graph.\nExample:\n./build/[debug/]bin/graph -c=111 -r=33 "
        "-v=4\n"
        "Once the graph is built seek the shortest path between two uppercase "
        "vertices. Examples:\nAB\nA->B\nCtoD\nEnter 'q' to quit.\n");
    exit(0);
}
