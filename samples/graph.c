#include "cli.h"
#include "pqueue.h"
#include "set.h"
#include "str_view.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

struct parent_cell
{
    struct point key;
    struct point parent;
    struct set_elem elem;
};

struct prev_vertex
{
    struct vertex *v;
    struct vertex *prev;
    struct set_elem elem;
};

struct bfs_point
{
    struct point p;
    /* We will use a priority queue because that is the data structure
       handy in this library. However, we will make a comparator function
       that returns EQL every time. The priority queue promises round
       robin fairness so this is an overly complicated standard queue under
       such circumstances. */
    struct pq_elem elem;
};

struct dist_point
{
    struct vertex *v;
    int dist;
    struct pq_elem pq_elem;
    struct set_elem set_elem;
};

struct path_request
{
    struct vertex *src;
    struct vertex *dst;
};

/* A vertex is connected via an edge to another vertex. Each vertex
   shall store a bounded list of edges representing the other vertices
   to which it is connected. The cost of this connection is the distance
   in cells on the screen to reach that vertex. Edges are undirected and
   unique between two vertices. */
struct edge
{
    struct vertex *to; /* This vertex shares an edge TO the specified vertex. */
    int cost;          /* Distance in cells to the other vertex. */
};

/* Each vertex in the set/graph will hold it's key name and edges to other
   vertices that it is connected to. This is displayed in a CLI so there
   is a maximum out degree of 4. Terminals only display cells in a grid. */
struct vertex
{
    char name;            /* Names are bounded to 26 [A-Z] maximum nodes. */
    struct point pos;     /* Position of this vertex in the grid. */
    struct edge edges[4]; /* The other vertices to which a vertex connects. */
    struct set_elem elem; /* Vertices are in an adjacency map. */
};

struct graph
{
    int rows;
    int cols;
    int vertices;
    enum speed speed;
    /* This is mostly needed for the visual representation of the graph on
       the screen. However, as we build the paths the result of that building
       will play a role in the cost of each edge between vertices. */
    uint32_t *grid;
    /* Yes we could rep this differently, but we want to test the set here. */
    struct set adjacency_list;
};

/*======================   Graph Constants   ================================*/

const char *paths[] = {
    "■", "╵", "╶", "└", "╷", "│", "┌", "├",
    "╴", "┘", "─", "┴", "┐", "┤", "┬", "┼",
};

const int speeds[] = {
    0, 5000000, 2500000, 1000000, 500000, 250000, 100000, 1000,
};

/* North, East, South, West */
const struct point dirs[] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
const size_t dirs_size = sizeof(dirs) / sizeof(dirs[0]);

const str_view rows = SV("-r=");
const str_view cols = SV("-c=");
const str_view speed = SV("-s=");
const str_view vertices_flag = SV("-v=");
const str_view help_flag = SV("-h");
const str_view escape = SV("\033[");
const str_view semi_colon = SV(";");
const str_view cursor_pos_specifier = SV("f");
const int default_rows = 33;
const int default_cols = 111;
const int default_vertices = 4;
const int default_speed = 4;
const int row_col_min = 7;
const int speed_max = 7;
const int max_vertices = 26;
const int max_degree = 4;
const int vertex_placement_padding = 3;
const char start_vertex_title = 'A';
const size_t vertex_cell_title_shift = 8;
const size_t edge_id_shift = 16;
const size_t vertex_title_mask = 0xFF00;
const uint32_t edge_id_mask = 0xFFFF0000;
const uint32_t l_edge_id_mask = 0xFF000000;
const uint32_t l_edge_id_shift = 24;
const uint32_t r_edge_id_mask = 0x00FF0000;
const uint32_t r_edge_id_shift = 16;
/* This comes immediately after 'Z' on the ASCII table. */
const char exclusive_end_vertex_title = '[';

/* The highest order 16 bits in the grid shall be reserved for the edge
   id if the square is a path. An edge ID is a concatenation of two
   vertex names. Vertex names are 8 bit characters, so two vertices can
   fit into a uint16_t which we have room for in a uint32_t. The concatenation
   shall always be sorted alphabetically so an edge connecting vertex A and
   Z will be uint16_t=AZ. */

const uint32_t path_mask = 0b1111;
const uint32_t floating_path = 0b0000;
const uint32_t north_path = 0b0001;
const uint32_t east_path = 0b0010;
const uint32_t south_path = 0b0100;
const uint32_t west_path = 0b1000;
const uint32_t cached_bit = 0b10000;
const uint32_t path_bit = 0b100000;
const uint32_t vertex_bit = 0b1000000;
const uint32_t paint_bit = 0b10000000;

const str_view prompt_msg
    = SV("Enter two vertices to find the shortest path between them (i.e. "
         "A-Z). Enter quit to quit:");
const str_view quit_cmd = SV("quit");

/*==========================   Prototypes  ================================= */

static void build_graph(struct graph *);
static void find_shortest_paths(struct graph *);
static bool find_grid_vertex_bfs(struct graph *, struct vertex *,
                                 struct vertex *);
static bool dijkstra_shortest_path(struct graph *, struct vertex *,
                                   struct vertex *);
static void paint_edge(struct graph *, const struct vertex *,
                       const struct vertex *);

static struct point random_vertex_placement(const struct graph *);
static bool is_valid_vertex_pos(const struct graph *, struct point);
static int rand_range(int, int);
static uint32_t *grid_at_mut(const struct graph *, struct point);
static uint32_t grid_at(const struct graph *, struct point);
static uint16_t sort_vertices(char, char);
static int vertex_degree(const struct vertex *);
static bool connect_random_edge(struct graph *, struct vertex *);
static void build_path_outline(struct graph *);
static void build_path_cell(struct graph *, struct point, uint32_t);
static void flush_cursor_grid_coordinate(const struct graph *, struct point);
static void clear_and_flush_graph(const struct graph *);
static void print_cell(uint32_t);
static char get_cell_vertex_title(uint32_t);
static bool has_edge_with(const struct vertex *, char);
static bool add_edge(struct vertex *, const struct edge *);
static bool is_edge_vertex(uint32_t, uint32_t);
static bool is_valid_edge_cell(uint32_t, uint32_t);
static void clear_paint(struct graph *);

static struct int_conversion parse_digits(str_view);
static struct path_request parse_path_request(struct graph *, str_view);
static void *valid_malloc(size_t);
static void help(void);

static threeway_cmp cmp_vertices(const struct set_elem *,
                                 const struct set_elem *, void *);
static threeway_cmp cmp_queue_points(const struct pq_elem *,
                                     const struct pq_elem *, void *);
static threeway_cmp cmp_parent_cells(const struct set_elem *,
                                     const struct set_elem *, void *);
static threeway_cmp cmp_pq_dist_points(const struct pq_elem *,
                                       const struct pq_elem *, void *);
static threeway_cmp cmp_set_dist_points(const struct set_elem *,
                                        const struct set_elem *, void *);
static threeway_cmp cmp_set_prev_vertices(const struct set_elem *,
                                          const struct set_elem *, void *);
static void pq_update_dist(struct pq_elem *, void *);
static void print_vertex(const struct set_elem *);
static void set_vertex_destructor(struct set_elem *);
static void set_parent_destructor(struct set_elem *);
static void pq_dist_point_destructor(struct pq_elem *);

/*======================  Main Arg Handling  ===============================*/

int
main(int argc, char **argv)
{
    /* Randomness will be used throughout the program but it need not be
       perfect. It only helps build graphs.
       NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp) */
    srand(1);
    struct graph graph = {
        .rows = default_rows,
        .cols = default_cols,
        .vertices = default_vertices,
        .speed = default_speed,
        .grid = NULL,
    };
    set_init(&graph.adjacency_list);
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
            graph.rows = row_arg.conversion;
        }
        else if (sv_starts_with(arg, cols))
        {
            const struct int_conversion col_arg = parse_digits(arg);
            if (col_arg.status == CONV_ER || col_arg.conversion < row_col_min)
            {
                quit("cols below required minimum or negative.\n", 1);
            }
            graph.cols = col_arg.conversion;
        }
        else if (sv_starts_with(arg, speed))
        {
            const struct int_conversion speed_arg = parse_digits(arg);
            if (speed_arg.status == CONV_ER || speed_arg.conversion > speed_max
                || speed_arg.conversion < 0)
            {
                quit("speed outside of valid range.\n", 1);
            }
            graph.speed = speed_arg.conversion;
        }
        else if (sv_starts_with(arg, vertices_flag))
        {
            const struct int_conversion vert_arg = parse_digits(arg);
            if (vert_arg.status == CONV_ER || vert_arg.conversion > max_vertices
                || vert_arg.conversion < 1)
            {
                quit("vertices outside of valid range.\n", 1);
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
    /* This type of maze generation requires odd rows and cols. */
    graph.grid = calloc((size_t)graph.rows * graph.cols, sizeof(uint32_t));
    if (!graph.grid)
    {
        (void)fprintf(stderr, "allocation failure for specified maze size.\n");
        return 1;
    }
    build_graph(&graph);
    find_shortest_paths(&graph);
    set_cursor_position(graph.rows + 1, graph.cols + 1);
    set_print(&graph.adjacency_list, set_root(&graph.adjacency_list),
              print_vertex);
    printf("\n");
    set_clear(&graph.adjacency_list, set_vertex_destructor);
}

/*========================   Graph Building    ==============================*/

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
            = vertex_bit | ((uint32_t)vertex_title << vertex_cell_title_shift);
        struct vertex *new_vertex = valid_malloc(sizeof(struct vertex));
        *new_vertex = (struct vertex){
            .name = (char)vertex_title,
            .pos = rand_point,
            .edges = {{0}, {0}, {0}, {0}},
        };
        if (!set_insert(&graph->adjacency_list, &new_vertex->elem, cmp_vertices,
                        NULL))
        {
            quit("Error building the graph. New vertex is already present.\n",
                 1);
        }
    }
    struct vertex key = {0};
    for (int vertex = 0, vertex_title = start_vertex_title;
         vertex < graph->vertices; ++vertex, ++vertex_title)
    {
        key.name = (char)vertex_title;
        const struct set_elem *const e
            = set_find(&graph->adjacency_list, &key.elem, cmp_vertices, NULL);
        if (e == set_end(&graph->adjacency_list))
        {
            quit("Vertex that should be present in the set is absent.\n", 1);
        }
        struct vertex *const src = set_entry(e, struct vertex, elem);
        const int degree = vertex_degree(src);
        if (degree == max_degree)
        {
            continue;
        }
        const int new_edges = rand_range(1, max_degree - degree);
        for (int i = 0; i < new_edges && connect_random_edge(graph, src); ++i)
        {}
    }
    clear_and_flush_graph(graph);
}

static bool
connect_random_edge(struct graph *const graph, struct vertex *const src_vertex)
{
    const char last_title
        = (char)(start_vertex_title + set_size(&graph->adjacency_list));
    struct vertex key = {
        .name = (char)rand_range(start_vertex_title, last_title - 1),
    };
    const struct set_elem *e = NULL;
    struct vertex *dst = NULL;
    const size_t size = set_size(&graph->adjacency_list) - 1;
    for (size_t i = 0; i < size; ++i, ++key.name)
    {
        key.name = (char)(key.name + (key.name == src_vertex->name));
        if (key.name >= last_title)
        {
            key.name = start_vertex_title;
        }
        e = set_find(&graph->adjacency_list, &key.elem, cmp_vertices, NULL);
        if (e == set_end(&graph->adjacency_list))
        {
            quit("Looping through possible vertices yielded error.\n", 1);
        }
        dst = set_entry(e, struct vertex, elem);
        if (!has_edge_with(src_vertex, dst->name)
            && vertex_degree(dst) < max_degree
            && find_grid_vertex_bfs(graph, src_vertex, dst))
        {

            return true;
        }
    }
    return false;
}

static bool
find_grid_vertex_bfs(struct graph *const graph, struct vertex *const src,
                     struct vertex *const dst)
{
    const uint32_t edge_id = sort_vertices(src->name, dst->name)
                             << edge_id_shift;
    struct set parent_map;
    set_init(&parent_map);
    struct pqueue bfs;
    pq_init(&bfs);
    struct parent_cell *const start = valid_malloc(sizeof(struct parent_cell));
    *start = (struct parent_cell){
        .key = src->pos,
        .parent = (struct point){-1, -1},
    };
    (void)set_insert(&parent_map, &start->elem, cmp_parent_cells, NULL);
    struct bfs_point *start_point = valid_malloc(sizeof(struct bfs_point));
    *start_point = (struct bfs_point){.p = src->pos};
    (void)pq_insert(&bfs, &start_point->elem, cmp_queue_points, NULL);
    bool success = false;
    struct point cur = {0};
    while (!pq_empty(&bfs))
    {
        struct bfs_point *bp
            = pq_entry(pq_pop_max(&bfs), struct bfs_point, elem);
        cur = bp->p;
        for (size_t i = 0; i < dirs_size; ++i)
        {
            struct point next = {
                .r = cur.r + dirs[i].r,
                .c = cur.c + dirs[i].c,
            };
            struct parent_cell push = {.key = next, .parent = cur};
            const uint32_t next_cell = grid_at(graph, next);
            if ((next_cell & vertex_bit))
            {
                if (get_cell_vertex_title(next_cell) != dst->name)
                {
                    continue;
                }
                free(bp);
                struct parent_cell *new_lineage
                    = valid_malloc(sizeof(struct parent_cell));
                *new_lineage = push;
                (void)set_insert(&parent_map, &new_lineage->elem,
                                 cmp_parent_cells, NULL);
                cur = next;
                success = true;
                break;
            }
            if (!(next_cell & path_bit)
                && !set_contains(&parent_map, &push.elem, cmp_parent_cells,
                                 NULL))
            {
                struct parent_cell *new_lineage
                    = valid_malloc(sizeof(struct parent_cell));
                *new_lineage = push;
                (void)set_insert(&parent_map, &new_lineage->elem,
                                 cmp_parent_cells, NULL);
                struct bfs_point *qp = valid_malloc(sizeof(struct bfs_point));
                *qp = (struct bfs_point){.p = next};
                (void)pq_insert(&bfs, &qp->elem, cmp_queue_points, NULL);
            }
        }
        if (success)
        {
            break;
        }
        free(bp);
    }
    if (success)
    {
        struct parent_cell c = {.key = cur};
        const struct set_elem *e
            = set_find(&parent_map, &c.elem, cmp_parent_cells, NULL);
        if (e == set_end(&parent_map))
        {
            quit("Cannot find cell parent to rebuild path.\n", 1);
        }
        struct parent_cell *cell = set_entry(e, struct parent_cell, elem);
        c.key = cell->parent;
        struct edge edge = {.to = dst, .cost = 1};
        while (cell->parent.r > 0)
        {
            e = set_find(&parent_map, &c.elem, cmp_parent_cells, NULL);
            if (e == set_end(&parent_map))
            {
                quit("Cannot find cell parent to rebuild path.\n", 1);
            }
            ++edge.cost;
            cell = set_entry(e, struct parent_cell, elem);
            *grid_at_mut(graph, cell->key) |= edge_id;
            build_path_cell(graph, cell->key, edge_id);
            c.key = cell->parent;
        }
        (void)add_edge(src, &edge);
        edge.to = src;
        (void)add_edge(dst, &edge);
    }
    set_clear(&parent_map, set_parent_destructor);
    return success;
}

static void
find_shortest_paths(struct graph *const graph)
{
    char *lineptr = NULL;
    for (;;)
    {
        clear_paint(graph);
        set_cursor_position(graph->rows + 1, graph->cols);
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
            }
            dijkstra_shortest_path(graph, pr.src, pr.dst);
            break;
        }
    }
    free(lineptr);
}

static bool
dijkstra_shortest_path(struct graph *const graph, struct vertex *const src,
                       struct vertex *const dst)
{
    struct pqueue distances;
    pq_init(&distances);
    struct set vertices;
    set_init(&vertices);
    struct set parent_map;
    set_init(&parent_map);
    for (struct set_elem *e = set_begin(&graph->adjacency_list);
         e != set_end(&graph->adjacency_list);
         e = set_next(&graph->adjacency_list, e))
    {
        struct dist_point *p = valid_malloc(sizeof(struct dist_point));
        *p = (struct dist_point){
            .v = set_entry(e, struct vertex, elem),
            .dist = INT_MAX,
        };
        if (p->v == src)
        {
            p->dist = 0;
            struct prev_vertex *pv = valid_malloc(sizeof(struct parent_cell));
            *pv = (struct prev_vertex){
                .v = p->v,
                .prev = NULL,
            };
            (void)set_insert(&parent_map, &pv->elem, cmp_set_prev_vertices,
                             NULL);
        }
        (void)pq_insert(&distances, &p->pq_elem, cmp_pq_dist_points, NULL);
        (void)set_insert(&vertices, &p->set_elem, cmp_set_dist_points, NULL);
    }
    bool success = false;
    struct dist_point *cur = NULL;
    while (!pq_empty(&distances))
    {
        cur = pq_entry(pq_pop_min(&distances), struct dist_point, pq_elem);
        if (cur->v == dst)
        {
            success = true;
            break;
        }
        (void)set_erase(&vertices, &cur->set_elem, cmp_set_dist_points, NULL);
        for (int i = 0; i < max_degree && cur->v->edges[i].to; ++i)
        {
            struct dist_point d = {.v = cur->v->edges[i].to};
            const struct set_elem *e
                = set_find(&vertices, &d.set_elem, cmp_set_dist_points, NULL);
            if (e == set_end(&vertices))
            {
                continue;
            }
            struct dist_point *next = set_entry(e, struct dist_point, set_elem);
            int alt = cur->dist + cur->v->edges[i].cost;
            if (alt < next->dist)
            {
                struct prev_vertex *pv
                    = valid_malloc(sizeof(struct parent_cell));
                *pv = (struct prev_vertex){
                    .v = next->v,
                    .prev = cur->v,
                };
                (void)set_insert(&parent_map, &pv->elem, cmp_set_prev_vertices,
                                 NULL);
                pq_update(&distances, &next->pq_elem, cmp_pq_dist_points,
                          pq_update_dist, &alt);
            }
        }
    }
    if (success)
    {
        struct prev_vertex key = {.v = cur->v};
        struct prev_vertex *prev = set_entry(
            set_find(&parent_map, &key.elem, cmp_set_prev_vertices, NULL),
            struct prev_vertex, elem);
        while (prev->prev)
        {
            paint_edge(graph, key.v, prev->prev);
            key.v = prev->prev;
            prev = set_entry(
                set_find(&parent_map, &key.elem, cmp_set_prev_vertices, NULL),
                struct prev_vertex, elem);
        }
    }
    pq_clear(&distances, pq_dist_point_destructor);
    set_clear(&parent_map, set_parent_destructor);
    clear_and_flush_graph(graph);
    return success;
}

static void
paint_edge(struct graph *const g, const struct vertex *const src,
           const struct vertex *const dst)
{
    struct point cur = src->pos;
    const uint32_t edge_id = sort_vertices(src->name, dst->name)
                             << edge_id_shift;
    struct point prev = cur;
    while (cur.r != dst->pos.r || cur.c != dst->pos.c)
    {
        *grid_at_mut(g, cur) |= paint_bit;
        for (size_t i = 0; i < dirs_size; ++i)
        {
            struct point next = {
                .r = cur.r + dirs[i].r,
                .c = cur.c + dirs[i].c,
            };
            const uint32_t next_cell = grid_at(g, next);
            if ((next_cell & vertex_bit)
                && get_cell_vertex_title(next_cell) == dst->name)
            {
                return;
            }
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

static struct point
random_vertex_placement(const struct graph *const graph)
{
    const int row_end = graph->rows - 2;
    const int col_end = graph->cols - 2;
    /* No vertices should be close to the edge of the map. */
    const int row_start = rand_range(vertex_placement_padding,
                                     graph->rows - vertex_placement_padding);
    const int col_start = rand_range(vertex_placement_padding,
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
            const struct point cur = {.r = row, .c = col};
            if (is_valid_vertex_pos(graph, cur))
            {
                return cur;
            }
            exhausted = ((row + 1) % graph->rows) == row_start
                        && ((col + 1) % graph->cols) == col_start;
        }
    }
    (void)fprintf(stderr, "cannot find a place for another vertex "
                          "on this grid, quitting now.\n");
    exit(1);
}

/* This function assumes that checking one cell in any direction is safe
   and within bounds. The vertices are only placed with padding around the
   full grid space so this assumption should be safe. */
static inline bool
is_valid_vertex_pos(const struct graph *graph, struct point p)
{
    return !(grid_at(graph, (struct point){.r = p.r, .c = p.c}) & vertex_bit)
           && !(grid_at(graph, (struct point){(p.r + 1), p.c}) & vertex_bit)
           && !(grid_at(graph, (struct point){(p.r - 1), p.c}) & vertex_bit)
           && !(grid_at(graph, (struct point){p.r, (p.c - 1)}) & vertex_bit)
           && !(grid_at(graph, (struct point){p.r, (p.c + 1)}) & vertex_bit);
}

static int
vertex_degree(const struct vertex *const v)
{
    int n = 0;
    /* Vertexes are initialized with zeroed out edge array. Falsey. */
    for (int i = 0; i < max_degree && v->edges[i].to; ++i, ++n)
    {}
    return n;
}

static inline int
rand_range(const int min, const int max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

static inline uint32_t *
grid_at_mut(const struct graph *const graph, struct point p)
{
    return &graph->grid[p.r * graph->cols + p.c];
}

static inline uint32_t
grid_at(const struct graph *const graph, struct point p)
{
    return graph->grid[p.r * graph->cols + p.c];
}

static inline uint16_t
sort_vertices(char a, char b)
{
    return a < b ? a << 8 | b : b << 8 | a;
}

static char
get_cell_vertex_title(const uint32_t cell)
{
    return (char)((cell & vertex_title_mask) >> vertex_cell_title_shift);
}

static bool
has_edge_with(const struct vertex *const v, char vertex)
{
    for (int i = 0; i < max_degree && v->edges[i].to; ++i)
    {
        if (v->edges[i].to->name == vertex)
        {
            return true;
        }
    }
    return false;
}

static bool
add_edge(struct vertex *const v, const struct edge *const e)
{
    for (int i = 0; i < max_degree; ++i)
    {
        if (!v->edges[i].to)
        {
            v->edges[i] = *e;
            return true;
        }
    }
    return false;
}

/*======================       Grid Helpers      ============================*/

static void
clear_and_flush_graph(const struct graph *const g)
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
flush_cursor_grid_coordinate(const struct graph *g, struct point p)
{
    set_cursor_position(p.r, p.c);
    print_cell(grid_at(g, p));
    (void)fflush(stdout);
}

static void
print_cell(const uint32_t cell)
{

    if (cell & vertex_bit)
    {
        printf("\033[38;5;14m%c\033[0m",
               (char)(cell >> vertex_cell_title_shift));
    }
    else if (cell & path_bit)
    {
        if (cell & paint_bit)
        {
            printf("\033[38;5;14m%s\033[0m", paths[cell & path_mask]);
        }
        else
        {
            printf("%s", paths[cell & path_mask]);
        }
    }
    else if (!(cell & path_bit))
    {
        if (cell & paint_bit)
        {
            printf("\033[38;5;14m█\033[0m");
        }
        else
        {
            printf(" ");
        }
    }
    else
    {
        (void)fprintf(stderr, "uncategorizable square.\n");
    }
}

static bool
is_edge_vertex(const uint32_t square, uint32_t edge_id)
{
    const char vertex_name = get_cell_vertex_title(square);
    const char edge_vertex1
        = (char)((edge_id & l_edge_id_mask) >> l_edge_id_shift);
    const char edge_vertex2
        = (char)((edge_id & r_edge_id_mask) >> r_edge_id_shift);
    return vertex_name == edge_vertex1 || vertex_name == edge_vertex2;
}

static bool
is_valid_edge_cell(const uint32_t square, const uint32_t edge_id)
{
    return ((square & vertex_bit) && is_edge_vertex(square, edge_id))
           || ((square & path_bit) && (square & edge_id_mask) == edge_id);
}

static void
build_path_cell(struct graph *g, struct point p, const uint32_t edge_id)
{
    uint32_t path = path_bit;
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

static threeway_cmp
cmp_vertices(const struct set_elem *const a, const struct set_elem *const b,
             void *aux)
{
    (void)aux;
    const struct vertex *const v_a = set_entry(a, struct vertex, elem);
    const struct vertex *const v_b = set_entry(b, struct vertex, elem);
    return (v_a->name > v_b->name) - (v_a->name < v_b->name);
}

static threeway_cmp
cmp_parent_cells(const struct set_elem *x, const struct set_elem *y, void *aux)
{
    (void)aux;
    const struct parent_cell *const a = set_entry(x, struct parent_cell, elem);
    const struct parent_cell *const b = set_entry(y, struct parent_cell, elem);
    if (a->key.r == b->key.r && a->key.c == b->key.c)
    {
        return EQL;
    }
    if (a->key.r == b->key.r)
    {
        return (a->key.c > b->key.c) - (a->key.c < b->key.c);
    }
    return (a->key.r > b->key.r) - (a->key.r < b->key.r);
}

static threeway_cmp
cmp_pq_dist_points(const struct pq_elem *const x, const struct pq_elem *const y,
                   void *aux)
{
    (void)aux;
    const struct dist_point *const a = pq_entry(x, struct dist_point, pq_elem);
    const struct dist_point *const b = pq_entry(y, struct dist_point, pq_elem);
    return (a->dist > b->dist) - (a->dist < b->dist);
}

static threeway_cmp
cmp_set_dist_points(const struct set_elem *const x,
                    const struct set_elem *const y, void *aux)
{
    (void)aux;
    const struct dist_point *const a
        = set_entry(x, struct dist_point, set_elem);
    const struct dist_point *const b
        = set_entry(y, struct dist_point, set_elem);
    if (a->v > b->v)
    {
        return GRT;
    }
    if (a->v < b->v)
    {
        return LES;
    }
    return EQL;
}

static threeway_cmp
cmp_set_prev_vertices(const struct set_elem *const x,
                      const struct set_elem *const y, void *aux)
{
    (void)aux;
    const struct prev_vertex *const a = set_entry(x, struct prev_vertex, elem);
    const struct prev_vertex *const b = set_entry(y, struct prev_vertex, elem);
    if (a->v > b->v)
    {
        return GRT;
    }
    if (a->v < b->v)
    {
        return LES;
    }
    return EQL;
}

static void
pq_update_dist(struct pq_elem *e, void *aux)
{
    pq_entry(e, struct dist_point, pq_elem)->dist = *((int *)aux);
}

/* By returning EQL every time while using a priority queue, it becomes
   a standard queue. This is because the priority queue provided promises
   round robin fairness. While a simple linked list or array could be used
   to provide standard queue functionality, there is no need to make these
   data structures when a working queue is possible with what is provided. */
static threeway_cmp
cmp_queue_points(const struct pq_elem *const a, const struct pq_elem *b,
                 void *aux)
{
    (void)a;
    (void)b;
    (void)aux;
    return EQL;
}

static void
print_vertex(const struct set_elem *const x)
{
    const struct vertex *v = set_entry(x, struct vertex, elem);
    printf("{%c,pos{%d,%d},edges{", v->name, v->pos.r, v->pos.c);
    for (int i = 0; i < max_degree && v->edges[i].to; ++i)
    {
        printf("{%c,%d}", v->edges[i].to->name, v->edges[i].cost);
    }
    printf("}");
    if (!v->edges[0].to)
    {
        printf("}");
    }
}

static void
set_vertex_destructor(struct set_elem *const e)
{
    free(set_entry(e, struct vertex, elem));
}

static void
set_parent_destructor(struct set_elem *const e)
{
    free(set_entry(e, struct parent_cell, elem));
}

static void
pq_dist_point_destructor(struct pq_elem *const e)
{
    free(pq_entry(e, struct dist_point, pq_elem));
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
    for (const char *c = sv_begin(r); c != sv_end(r); c = sv_next(c))
    {
        if (*c >= start_vertex_title
            && *c <= (char)(start_vertex_title + set_size(&g->adjacency_list)
                            - 1))
        {
            struct vertex key = {.name = *c};
            struct vertex *v = set_entry(
                set_find(&g->adjacency_list, &key.elem, cmp_vertices, NULL),
                struct vertex, elem);
            res.src ? (res.dst = v) : (res.src = v);
        }
    }
    return res.src && res.dst ? res : (struct path_request){0};
}

static struct int_conversion
parse_digits(str_view arg)
{
    const size_t eql = sv_rfind(arg, sv_npos(arg), SV("="));
    if (eql == sv_npos(arg))
    {
        return (struct int_conversion){.status = CONV_ER};
    }
    str_view num = sv_substr(arg, eql + 1, ULLONG_MAX);
    if (sv_empty(num))
    {
        (void)fprintf(stderr, "please specify element to convert.\n");
        return (struct int_conversion){.status = CONV_ER};
    }
    return convert_to_int(sv_begin(num));
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
        "queue and set provided by this library.\nUsage:\n-r=N The "
        "row flag lets you specify area for grid rows > 7.\n-c=N The col flag "
        "lets you specify area for grid cols > 7.\n-s=N The speed flag lets "
        "you specify the speed of the animation while building the grid"
        "0-7.\nExample:\n./build/rel/graph -c=111 -r=33 -v=4\n");
}
