#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "cli.h"
#include "flat_double_ended_queue.h"
#include "flat_hash_map.h"
#include "priority_queue.h"
#include "random.h"
#include "str_view/str_view.h"
#include "traits.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
    DIRS_SIZE = 4,
    MAX_VERTICES = 26,
    MAX_DEGREE = 4,
};

typedef uint32_t Cell;

struct point
{
    int r;
    int c;
};

struct parent_cell
{
    struct point key;
    struct point parent;
    ccc_fh_map_elem elem;
};

struct prev_vertex
{
    struct vertex *v;
    struct vertex *prev;
    ccc_fh_map_elem elem;
    /* A pointer to the corresponding pq_entry for this element. */
    struct dist_point *dist_point;
};

struct dist_point
{
    struct vertex *v;
    int dist;
    ccc_pq_elem pq_elem;
};

struct path_request
{
    struct vertex *src;
    struct vertex *dst;
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
    Cell *grid;
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

/* Go to the box drawing unicode character wikipedia page to change styles. */
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

static size_t const vertex_cell_title_shift = 8;
static Cell const vertex_title_mask = 0xFF00;
static size_t const edge_id_shift = 16;
static Cell const edge_id_mask = 0xFFFF0000;
static Cell const l_edge_id_mask = 0xFF000000;
static Cell const l_edge_id_shift = 24;
static Cell const r_edge_id_mask = 0x00FF0000;
static Cell const r_edge_id_shift = 16;
static Cell const path_mask = 0b1111;
static Cell const north_path = 0b0001;
static Cell const east_path = 0b0010;
static Cell const south_path = 0b0100;
static Cell const west_path = 0b1000;
static Cell const path_bit = 0b10000;
static Cell const vertex_bit = 0b100000;
static Cell const paint_bit = 0b1000000;
static Cell const digit_bit = 0b10000000;
static size_t const digit_shift = 8;
static Cell const digit_mask = 0xF00;

static str_view const prompt_msg
    = SV("Enter two vertices to find the shortest path between them (i.e. "
         "A-Z). Enter q to quit:");
static str_view const quit_cmd = SV("q");

/*==========================   Prototypes  ================================= */

static void build_graph(struct graph *);
static void find_shortest_paths(struct graph *);
static bool has_built_edge(struct graph *, struct vertex *, struct vertex *);
static bool dijkstra_shortest_path(struct graph *, struct path_request);
static void prepare_vertices(struct graph *, ccc_priority_queue *,
                             ccc_flat_hash_map *, struct path_request const *);
static void paint_edge(struct graph *, struct vertex const *,
                       struct vertex const *);
static void add_edge_cost_label(struct graph *, struct vertex *,
                                struct edge const *);
static bool is_dst(Cell, char);

static struct point random_vertex_placement(struct graph const *);
static bool is_valid_vertex_pos(struct graph const *, struct point);
static Cell *grid_at_mut(struct graph const *, struct point);
static Cell grid_at(struct graph const *, struct point);
static uint16_t sort_vertices(char, char);
static int vertex_degree(struct vertex const *);
static bool connect_random_edge(struct graph *, struct vertex *);
static void build_path_outline(struct graph *);
static void build_path_cell(struct graph *, struct point, Cell);
static void clear_and_flush_graph(struct graph const *);
static void print_cell(Cell);
static char get_cell_vertex_title(Cell);
static bool has_edge_with(struct vertex const *, char);
static bool add_edge(struct vertex *, struct edge const *);
static bool is_edge_vertex(Cell, Cell);
static bool is_valid_edge_cell(Cell, Cell);
static void clear_paint(struct graph *);
static bool is_vertex(Cell);
static bool is_path(Cell);
static struct vertex *vertex_at(struct graph const *g, char name);

static void encode_digits(struct graph *, struct digit_encoding);
static enum label_orientation get_direction(struct point, struct point);
static struct int_conversion parse_digits(str_view);
static struct path_request parse_path_request(struct graph *, str_view);
static void *valid_malloc(size_t);
static void help(void);

static ccc_threeway_cmp cmp_pq_dist_points(ccc_cmp);

static bool eq_parent_cells(ccc_key_cmp);
static uint64_t hash_parent_cells(void const *point_struct);
static bool eq_prev_vertices(ccc_key_cmp);
static uint64_t hash_vertex_addr(void const *pointer_to_vertex);
static uint64_t hash_64_bits(uint64_t);

static void pq_update_dist(ccc_update);
static void map_pq_prev_vertex_dist_point_destructor(void *);
static void map_parent_point_destructor(void *);
static unsigned count_digits(uintmax_t n);

/*======================  Main Arg Handling  ===============================*/

int
main(int argc, char **argv)
{
    /* Randomness will be used throughout the program but it need not be
       perfect. It only helps build graphs.
       NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp) */
    srand(time(NULL));
    struct graph graph = {
        .rows = default_rows,
        .cols = default_cols,
        .vertices = default_vertices,
        .grid = NULL,
        .graph = network,
    };
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
    graph.grid = calloc((size_t)graph.rows * graph.cols, sizeof(Cell));
    if (!graph.grid)
    {
        (void)fprintf(stderr, "allocation failure for specified maze size.\n");
        return 1;
    }
    build_graph(&graph);
    find_shortest_paths(&graph);
    set_cursor_position(graph.rows + 1, graph.cols + 1);
    printf("\n");
}

/*========================   Graph Building    ==============================*/

/* The undirected graphs produced are randomly generated graphs where each
   vertex is placed on the grid of terminal cells. Each vertex then tries
   to connect a random number of out edges to vertices that can accept an
   in edge. The search for another vertex is a simple bfs search and thus
   connects the edge via shortest path in the grid. The number of cells taken
   to connect the edge to the other vertex is the cost/weight of that edge. */
static void
build_graph(struct graph *const graph)
{
    build_path_outline(graph);
    clear_and_flush_graph(graph);
    for (int vertex = 0, vertex_title = start_vertex_title;
         vertex < graph->vertices; ++vertex, ++vertex_title)
    {
        struct point rand_point = random_vertex_placement(graph);
        *grid_at_mut(graph, rand_point)
            = vertex_bit | path_bit
              | ((Cell)vertex_title << vertex_cell_title_shift);
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
        if (!src)
        {
            quit("Vertex that should be present in the map is absent.\n", 1);
        }
        int const degree = vertex_degree(src);
        if (degree == MAX_DEGREE)
        {
            continue;
        }
        int const out_edges = rand_range(1, MAX_DEGREE - degree);
        for (int i = 0; i < out_edges && connect_random_edge(graph, src); ++i)
        {}
    }
    clear_and_flush_graph(graph);
}

static bool
connect_random_edge(struct graph *const graph, struct vertex *const src_vertex)
{
    size_t const graph_size = graph->vertices;
    /* Bounded at size of the alphabet A-Z so alloca is fine here. */
    size_t vertex_title_indices[sizeof(size_t) * graph_size];
    for (size_t i = 0; i < graph_size; ++i)
    {
        vertex_title_indices[i] = i;
    }
    /* Cycle through all vertices with which to join an edge randomly. */
    rand_shuffle(sizeof(size_t), vertex_title_indices, graph_size);
    struct vertex *dst = NULL;
    for (size_t i = 0; i < graph_size; ++i)
    {
        char key = vertex_titles[vertex_title_indices[i]];
        if (key == src_vertex->name)
        {
            continue;
        }
        dst = vertex_at(graph, key);
        if (!dst)
        {
            quit("Broken or corrupted adjacency list.\n", 1);
        }
        if (!has_edge_with(src_vertex, dst->name)
            && vertex_degree(dst) < MAX_DEGREE
            && has_built_edge(graph, src_vertex, dst))
        {
            return true;
        }
    }
    return false;
}

/* This function assumes that the destination is valid. Valid means that
   source is not already connected to destination and that destination
   has less than the maximum allowable in degree. However, edge formation
   still may fail if no path exists from source to destination. */
static bool
has_built_edge(struct graph *const graph, struct vertex *const src,
               struct vertex *const dst)
{
    Cell const edge_id = sort_vertices(src->name, dst->name) << edge_id_shift;
    ccc_flat_hash_map parent_map;
    [[maybe_unused]] ccc_result res
        = FHM_INIT(&parent_map, NULL, 0, struct parent_cell, key, elem, realloc,
                   hash_parent_cells, eq_parent_cells, NULL);
    assert(res == CCC_OK);
    ccc_flat_double_ended_queue bfs
        = CCC_FDEQ_INIT(NULL, 0, struct point, realloc);
    [[maybe_unused]] struct parent_cell *pc = FHM_INSERT_ENTRY(
        FHM_ENTRY(&parent_map, src->pos), (struct parent_cell){
                                              .key = src->pos,
                                              .parent = (struct point){-1, -1},
                                          });
    assert(pc);
    ccc_fdeq_push_back(&bfs, &src->pos);
    bool success = false;
    struct point cur = {0};
    while (!ccc_fdeq_empty(&bfs) && !success)
    {
        cur = *((struct point *)ccc_fdeq_front(&bfs));
        ccc_fdeq_pop_front(&bfs);
        for (size_t i = 0; i < DIRS_SIZE; ++i)
        {
            struct point next = {
                .r = cur.r + dirs[i].r,
                .c = cur.c + dirs[i].c,
            };
            struct parent_cell push = {.key = next, .parent = cur};
            Cell const next_cell = grid_at(graph, next);
            if (is_dst(next_cell, dst->name))
            {
                [[maybe_unused]] struct parent_cell *inserted
                    = insert_entry(entry_vr(&parent_map, &next), &push.elem);
                assert(inserted);
                cur = next;
                success = true;
                break;
            }
            if (!is_path(next_cell) && !fhm_contains(&parent_map, &next))
            {
                [[maybe_unused]] struct parent_cell *inserted
                    = insert_entry(entry_vr(&parent_map, &next), &push.elem);
                assert(inserted != NULL);
                (void)ccc_fdeq_push_back(&bfs, &next);
            }
        }
    }
    if (success)
    {
        struct parent_cell const *cell = fhm_get(&parent_map, &cur);
        assert(cell);
        struct edge edge = {
            .n = {.name = dst->name, .cost = 0},
            .pos = dst->pos,
        };
        while (cell->parent.r > 0)
        {
            cell = fhm_get(&parent_map, &cell->parent);
            if (!cell)
            {
                quit("Cannot find cell parent to rebuild path.\n", 1);
            }
            ++edge.n.cost;
            *grid_at_mut(graph, cell->key) |= edge_id;
            build_path_cell(graph, cell->key, edge_id);
        }
        (void)add_edge(src, &edge);
        edge.n.name = src->name;
        edge.pos = src->pos;
        (void)add_edge(dst, &edge);
        add_edge_cost_label(graph, dst, &edge);
    }
    fhm_clear_and_free(&parent_map, map_parent_point_destructor);
    ccc_fdeq_clear_and_free(&bfs, NULL);
    return success;
}

/* A edge cost lable will only be added if there is sufficient space. For
   edges that are too small to fit a digit or two the line length can be
   easily counted with the mouse or by eye. */
static void
add_edge_cost_label(struct graph *const g, struct vertex *const src,
                    struct edge const *const e)
{
    struct point cur = src->pos;
    Cell const edge_id = sort_vertices(src->name, e->n.name) << edge_id_shift;
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
            encode_digits(g, (struct digit_encoding){
                                 .start = cur,
                                 .cost = e->n.cost,
                                 .spaces_needed = spaces_needed_for_cost,
                                 .orientation = direction,
                             });
            return;
        }
        for (size_t i = 0; i < DIRS_SIZE; ++i)
        {
            struct point next = {
                .r = cur.r + dirs[i].r,
                .c = cur.c + dirs[i].c,
            };
            Cell const next_cell = grid_at(g, next);
            if ((next_cell & vertex_bit)
                && get_cell_vertex_title(next_cell) == e->n.name)
            {
                return;
            }
            /* Always make forward progress, no backtracking. */
            if ((grid_at(g, next) & edge_id_mask) == edge_id
                && (prev.r != next.r || prev.c != next.c))
            {
                direction = get_direction(prev, next);
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
encode_digits(struct graph *g, struct digit_encoding e)
{
    uintmax_t digits = e.cost;
    if (e.orientation == NORTH || e.orientation == SOUTH)
    {
        e.start.r = e.orientation == NORTH
                        ? e.start.r + (int)e.spaces_needed - 2
                        : e.start.r - 1;
        for (; digits; digits /= 10, --e.start.r)
        {
            *grid_at_mut(g, e.start) |= digit_bit;
            *grid_at_mut(g, e.start) |= ((digits % 10) << digit_shift);
        }
    }
    else
    {
        e.start.c = e.orientation == WEST ? e.start.c + (int)e.spaces_needed - 2
                                          : e.start.c - 1;
        for (; digits; digits /= 10, --e.start.c)
        {
            *grid_at_mut(g, e.start) |= digit_bit;
            *grid_at_mut(g, e.start) |= ((digits % 10) << digit_shift);
        }
    }
}

static enum label_orientation
get_direction(struct point prev, struct point next)
{
    struct point const diff = {.r = next.r - prev.r, .c = next.c - prev.c};
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
            if (is_valid_vertex_pos(graph, cur))
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

static inline bool
is_dst(Cell const c, char const dst)
{
    return is_vertex(c) && get_cell_vertex_title(c) == dst;
}

/*========================    Graph Solving    ==============================*/

static void
find_shortest_paths(struct graph *const graph)
{
    char *lineptr = NULL;
    for (;;)
    {
        clear_paint(graph);
        set_cursor_position(graph->rows, 0);
        clear_line();
        sv_print(stdout, prompt_msg);
        size_t len = 0;
        ssize_t read = 0;
        while ((read = getline(&lineptr, &len, stdin)) != -1)
        {
            str_view line = (str_view){
                .s = lineptr,
                .sz = read - 1,
            };
            struct path_request pr = parse_path_request(graph, line);
            if (!pr.src)
            {
                clear_line();
                quit("Please provide any source and destination vertex "
                     "represented in the grid\nExamples: AB, A B, B-C, X->Y, "
                     "DtoF\nMost formats work but two capital vertices are "
                     "needed.\n",
                     1);
                return;
            }
            if (!dijkstra_shortest_path(graph, pr))
            {
                set_cursor_position(pr.src->pos.r, pr.src->pos.c);
                printf("\033[38;5;9m%c\033[0m", pr.src->name);
                set_cursor_position(pr.dst->pos.r, pr.dst->pos.c);
                printf("\033[38;5;9m%c\033[0m", pr.dst->name);
            }
            break;
        }
    }
    free(lineptr);
}

static bool
dijkstra_shortest_path(struct graph *const graph, struct path_request const pr)
{
    ccc_priority_queue dist_q = CCC_PQ_INIT(struct dist_point, pq_elem, CCC_LES,
                                            NULL, cmp_pq_dist_points, NULL);
    ccc_flat_hash_map prev_map;
    [[maybe_unused]] ccc_result res
        = FHM_INIT(&prev_map, NULL, 0, struct prev_vertex, v, elem, realloc,
                   hash_vertex_addr, eq_prev_vertices, NULL);
    assert(res == CCC_OK);
    prepare_vertices(graph, &dist_q, &prev_map, &pr);
    bool success = false;
    struct dist_point *cur = NULL;
    while (!ccc_pq_empty(&dist_q))
    {
        /* PQ entries are popped but the map will free the memory at
           the end because it always holds a reference to its pq_elem. */
        cur = ccc_pq_front(&dist_q);
        ccc_pq_pop(&dist_q);
        if (cur->v == pr.dst || cur->dist == INT_MAX)
        {
            success = cur->dist != INT_MAX;
            break;
        }
        for (int i = 0; i < MAX_DEGREE && cur->v->edges[i].name; ++i)
        {
            struct prev_vertex *next = FHM_GET_MUT(
                &prev_map, vertex_at(graph, cur->v->edges[i].name));
            assert(next);
            /* The seen map also holds a pointer to the corresponding
               priority queue element so that this update is easier. */
            struct dist_point *dist = next->dist_point;
            int alt = cur->dist + cur->v->edges[i].cost;
            if (alt < dist->dist)
            {
                /* Build the map with the appropriate best candidate parent. */
                next->prev = cur->v;
                /* Dijkstra with update technique tests the pq abilities. */
                if (!ccc_pq_decrease(&dist_q, &dist->pq_elem, pq_update_dist,
                                     &alt))
                {
                    quit("Updating vertex that is not in queue.\n", 1);
                }
            }
        }
    }
    if (success)
    {
        struct vertex *v = cur->v;
        struct prev_vertex const *prev = FHM_GET(&prev_map, v);
        assert(prev);
        while (prev->prev)
        {
            paint_edge(graph, v, prev->prev);
            v = prev->prev;
            prev = FHM_GET(&prev_map, prev->prev);
            assert(prev);
        }
    }
    /* Choosing when to free gets tricky during the algorithm. So, the
       prev map is the last allocation with access to the priority queue
       elements that have been popped but not freed. It will free its
       own map and its references to priority queue elements. */
    fhm_clear_and_free(&prev_map, map_pq_prev_vertex_dist_point_destructor);
    clear_and_flush_graph(graph);
    return success;
}

static void
prepare_vertices(struct graph *const graph, ccc_priority_queue *dist_q,
                 ccc_flat_hash_map *prev_map, struct path_request const *pr)
{
    for (int count = 0, name = start_vertex_title; count < graph->vertices;
         ++count, ++name)
    {
        struct vertex *v = vertex_at(graph, (char)name);
        struct dist_point *p = valid_malloc(sizeof(struct dist_point));
        *p = (struct dist_point){
            .v = v,
            .dist = v == pr->src ? 0 : INT_MAX,
        };
        struct prev_vertex const *const inserted
            = FHM_INSERT_ENTRY(FHM_ENTRY(prev_map, p->v), (struct prev_vertex){
                                                              .v = p->v,
                                                              .prev = NULL,
                                                              .dist_point = p,
                                                          });
        if (!inserted)
        {
            quit("inserting into map in in loading phase failed.\n", 1);
        }
        ccc_pq_push(dist_q, &p->pq_elem);
    }
}

static void
paint_edge(struct graph *const g, struct vertex const *const src,
           struct vertex const *const dst)
{
    struct point cur = src->pos;
    Cell const edge_id = sort_vertices(src->name, dst->name) << edge_id_shift;
    struct point prev = cur;
    while (cur.r != dst->pos.r || cur.c != dst->pos.c)
    {
        *grid_at_mut(g, cur) |= paint_bit;
        for (size_t i = 0; i < DIRS_SIZE; ++i)
        {
            struct point next = {
                .r = cur.r + dirs[i].r,
                .c = cur.c + dirs[i].c,
            };
            Cell const next_cell = grid_at(g, next);
            if ((next_cell & vertex_bit)
                && get_cell_vertex_title(next_cell) == dst->name)
            {
                return;
            }
            /* Always make forward progress, no backtracking. */
            if ((grid_at(g, next) & edge_id_mask) == edge_id
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
    return &((struct vertex *)g->graph)[((size_t)name - start_vertex_title)];
}

/* This function assumes that checking one cell in any direction is safe
   and within bounds. The vertices are only placed with padding around the
   full grid space so this assumption should be safe. */
static inline bool
is_valid_vertex_pos(struct graph const *graph, struct point p)
{
    return !(grid_at(graph, (struct point){.r = p.r, .c = p.c}) & vertex_bit)
           && !(grid_at(graph, (struct point){(p.r + 1), p.c}) & vertex_bit)
           && !(grid_at(graph, (struct point){(p.r - 1), p.c}) & vertex_bit)
           && !(grid_at(graph, (struct point){p.r, (p.c - 1)}) & vertex_bit)
           && !(grid_at(graph, (struct point){p.r, (p.c + 1)}) & vertex_bit);
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

static inline Cell *
grid_at_mut(struct graph const *const graph, struct point p)
{
    return &graph->grid[p.r * graph->cols + p.c];
}

static inline Cell
grid_at(struct graph const *const graph, struct point p)
{
    return graph->grid[p.r * graph->cols + p.c];
}

static inline uint16_t
sort_vertices(char a, char b)
{
    return a < b ? (a << 8) | b : (b << 8) | a;
}

static char
get_cell_vertex_title(Cell const cell)
{
    return (char)((cell & vertex_title_mask) >> vertex_cell_title_shift);
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
is_vertex(Cell c)
{
    return (c & vertex_bit) != 0;
}

static bool
is_path(Cell c)
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
            print_cell(grid_at(g, (struct point){.r = row, .c = col}));
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
            *grid_at_mut(graph, (struct point){.r = r, .c = c}) &= ~paint_bit;
        }
    }
}

static void
print_cell(Cell const cell)
{

    if (cell & vertex_bit)
    {
        printf("\033[38;5;14m%c\033[0m",
               (char)(cell >> vertex_cell_title_shift));
    }
    else if (cell & digit_bit)
    {
        printf("%d", (cell & digit_mask) >> digit_shift);
    }
    else if (cell & path_bit)
    {
        (cell & paint_bit)
            ? printf("\033[38;5;13m%s\033[0m", paths[cell & path_mask])
            : printf("%s", paths[cell & path_mask]);
    }
    else if (!(cell & path_bit))
    {
        printf(" ");
    }
    else
    {
        (void)fprintf(stderr, "uncategorizable square.\n");
    }
}

static bool
is_edge_vertex(Cell const square, Cell edge_id)
{
    char const vertex_name = get_cell_vertex_title(square);
    char const edge_vertex1
        = (char)((edge_id & l_edge_id_mask) >> l_edge_id_shift);
    char const edge_vertex2
        = (char)((edge_id & r_edge_id_mask) >> r_edge_id_shift);
    return vertex_name == edge_vertex1 || vertex_name == edge_vertex2;
}

static bool
is_valid_edge_cell(Cell const square, Cell const edge_id)
{
    return ((square & vertex_bit) && is_edge_vertex(square, edge_id))
           || ((square & path_bit) && (square & edge_id_mask) == edge_id);
}

static void
build_path_cell(struct graph *g, struct point p, Cell const edge_id)
{
    Cell path = path_bit;
    if (p.r - 1 >= 0
        && is_valid_edge_cell(
            grid_at(g, (struct point){.r = p.r - 1, .c = p.c}), edge_id))
    {
        path |= north_path;
        *grid_at_mut(g, (struct point){p.r - 1, p.c}) |= south_path;
    }
    if (p.r + 1 < g->rows
        && is_valid_edge_cell(
            grid_at(g, (struct point){.r = p.r + 1, .c = p.c}), edge_id))
    {
        path |= south_path;
        *grid_at_mut(g, (struct point){p.r + 1, p.c}) |= north_path;
    }
    if (p.c - 1 >= 0
        && is_valid_edge_cell(
            grid_at(g, (struct point){.r = p.r, .c = p.c - 1}), edge_id))
    {
        path |= west_path;
        *grid_at_mut(g, (struct point){p.r, p.c - 1}) |= east_path;
    }
    if (p.c + 1 < g->cols
        && is_valid_edge_cell(
            grid_at(g, (struct point){.r = p.r, .c = p.c + 1}), edge_id))
    {
        path |= east_path;
        *grid_at_mut(g, (struct point){p.r, p.c + 1}) |= west_path;
    }
    *grid_at_mut(g, p) |= path;
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
                struct point cur = {.r = row, .c = col};
                build_path_cell(graph, cur, 0);
            }
        }
    }
}

/*====================    Data Structure Helpers    =========================*/

static bool
eq_parent_cells(ccc_key_cmp const c)
{
    struct parent_cell const *const pc = c.container;
    struct point const *const p = c.key;
    return pc->key.r == p->r && pc->key.c == p->c;
}

static uint64_t
hash_parent_cells(void const *const point_struct)
{
    struct point const *const p = point_struct;
    return hash_64_bits((p->r << 31) | p->c);
}

static ccc_threeway_cmp
cmp_pq_dist_points(ccc_cmp const cmp)
{
    struct dist_point const *const a = cmp.container_a;
    struct dist_point const *const b = cmp.container_b;
    return (a->dist > b->dist) - (a->dist < b->dist);
}

static bool
eq_prev_vertices(ccc_key_cmp const cmp)
{
    struct prev_vertex const *const a = cmp.container;
    struct vertex const *const *const v = cmp.key;
    return a->v == *v;
}

static uint64_t
hash_vertex_addr(void const *pointer_to_vertex)
{
    return hash_64_bits((uintptr_t) * ((struct vertex **)pointer_to_vertex));
}

static uint64_t
hash_64_bits(uint64_t x)
{
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

static void
pq_update_dist(ccc_update const u)
{
    ((struct dist_point *)u.container)->dist = *((int *)u.aux);
}

static void
map_pq_prev_vertex_dist_point_destructor(void *const e)
{
    struct prev_vertex *pv = e;
    free(pv->dist_point);
}

static void
map_parent_point_destructor(void *const e)
{
    (void)e;
}

/*===========================    Misc    ====================================*/

static struct path_request
parse_path_request(struct graph *const g, str_view r)
{
    if (sv_contains(r, quit_cmd))
    {
        quit("Exiting now.\n", 0);
    }
    struct path_request res = {0};
    char const end_title = (char)(start_vertex_title + g->vertices - 1);
    for (char const *c = sv_begin(r); c != sv_end(r); c = sv_next(c))
    {
        if (*c >= start_vertex_title && *c <= end_title)
        {
            struct vertex *v = vertex_at(g, *c);
            assert(v);
            res.src ? (res.dst = v) : (res.src = v);
        }
    }
    return res.src && res.dst ? res : (struct path_request){0};
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
    (void)fprintf(
        stdout,
        "Graph Builder:\nBuilds weighted graphs for Dijkstra's"
        "Algorithm to demonstrate usage of the priority "
        "queue and map provided by this library.\nUsage:\n-r=N The "
        "row flag lets you specify area for grid rows > 7.\n-c=N The col flag "
        "lets you specify area for grid cols > 7.\n-s=N The speed flag lets "
        "you specify the speed of the animation while building the grid"
        "0-7.\nExample:\n./build/rel/graph -c=111 -r=33 -v=4\n");
}
