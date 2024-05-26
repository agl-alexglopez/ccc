#include "cli.h"
#include "pqueue.h"
#include "set.h"
#include "str_view.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    struct set_elem elem;
};

struct prev_vertex
{
    struct vertex *v;
    struct vertex *prev;
    struct set_elem elem;
    /* A pointer to the corresponding pq_entry for this element. */
    struct pq_elem *pq_elem;
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
    /* This is mostly needed for the visual representation of the graph on
       the screen. However, as we build the paths the result of that building
       will play a role in the cost of each edge between vertices. */
    Cell *grid;
    /* Yes we could rep this differently, but we want to test the set here. */
    struct set adjacency_list;
};

/*======================   Graph Constants   ================================*/

const char *paths[] = {
    // "■", "╵", "╶", "└", "╷", "│", "┌", "├",
    // "╴", "┘", "─", "┴", "┐", "┤", "┬", "┼",
    "●", "╵", "╶", "╰", "╷", "│", "╭", "├", // rounded
    "╴", "╯", "─", "┴", "╮", "┤", "┬", "┼", // rounded
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
const Cell edge_id_mask = 0xFFFF0000;
const Cell l_edge_id_mask = 0xFF000000;
const Cell l_edge_id_shift = 24;
const Cell r_edge_id_mask = 0x00FF0000;
const Cell r_edge_id_shift = 16;
/* This comes immediately after 'Z' on the ASCII table. */
const char exclusive_end_vertex_title = '[';

/* The highest order 16 bits in the grid shall be reserved for the edge
   id if the square is a path. An edge ID is a concatenation of two
   vertex names. Vertex names are 8 bit characters, so two vertices can
   fit into a uint16_t which we have room for in a Cell. The concatenation
   shall always be sorted alphabetically so an edge connecting vertex A and
   Z will be uint16_t=AZ. */

const Cell path_mask = 0b1111;
const Cell floating_path = 0b0000;
const Cell north_path = 0b0001;
const Cell east_path = 0b0010;
const Cell south_path = 0b0100;
const Cell west_path = 0b1000;
const Cell cached_bit = 0b10000;
const Cell path_bit = 0b100000;
const Cell vertex_bit = 0b1000000;
const Cell paint_bit = 0b10000000;

const str_view prompt_msg
    = SV("Enter two vertices to find the shortest path between them (i.e. "
         "A-Z). Enter q to quit:");
const str_view quit_cmd = SV("q");

/*==========================   Prototypes  ================================= */

static void build_graph(struct graph *);
static void find_shortest_paths(struct graph *);
static bool find_grid_vertex_bfs(struct graph *, struct vertex *,
                                 struct vertex *);
static bool dijkstra_shortest_path(struct graph *, struct path_request);
static void paint_edge(struct graph *, const struct vertex *,
                       const struct vertex *);
static bool is_dst(Cell, char);

static struct point random_vertex_placement(const struct graph *);
static bool is_valid_vertex_pos(const struct graph *, struct point);
static int rand_range(int, int);
static Cell *grid_at_mut(const struct graph *, struct point);
static Cell grid_at(const struct graph *, struct point);
static uint16_t sort_vertices(char, char);
static int vertex_degree(const struct vertex *);
static bool connect_random_edge(struct graph *, struct vertex *);
static void build_path_outline(struct graph *);
static void build_path_cell(struct graph *, struct point, Cell);
static void clear_and_flush_graph(const struct graph *);
static void print_cell(Cell);
static char get_cell_vertex_title(Cell);
static bool has_edge_with(const struct vertex *, char);
static bool add_edge(struct vertex *, const struct edge *);
static bool is_edge_vertex(Cell, Cell);
static bool is_valid_edge_cell(Cell, Cell);
static void clear_paint(struct graph *);
static bool is_vertex(Cell);
static bool is_path(Cell);

static struct int_conversion parse_digits(str_view);
static struct path_request parse_path_request(struct graph *, str_view);
static void *valid_malloc(size_t);
static void help(void);

static bool insert_prev_vertex(struct set *, struct prev_vertex);
static bool insert_parent_cell(struct set *, struct parent_cell);
static void insert_bfs_point(struct pqueue *, struct bfs_point);
static threeway_cmp cmp_vertices(const struct set_elem *,
                                 const struct set_elem *, void *);
static threeway_cmp cmp_queue_points(const struct pq_elem *,
                                     const struct pq_elem *, void *);
static threeway_cmp cmp_parent_cells(const struct set_elem *,
                                     const struct set_elem *, void *);
static threeway_cmp cmp_pq_dist_points(const struct pq_elem *,
                                       const struct pq_elem *, void *);
static threeway_cmp cmp_set_prev_vertices(const struct set_elem *,
                                          const struct set_elem *, void *);
static void pq_update_dist(struct pq_elem *, void *);
static void print_vertex(const struct set_elem *);
static void set_vertex_destructor(struct set_elem *);
static void set_pq_prev_vertex_dist_point_destructor(struct set_elem *);
static void set_parent_point_destructor(struct set_elem *);
static void pq_bfs_point_destructor(struct pq_elem *);

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
        else if (sv_starts_with(arg, vertices_flag))
        {
            const struct int_conversion vert_arg = parse_digits(arg);
            if (vert_arg.status == CONV_ER || vert_arg.conversion > max_vertices
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
    /* This type of maze generation requires odd rows and cols. */
    graph.grid = calloc((size_t)graph.rows * graph.cols, sizeof(Cell));
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
        const int out_edges = rand_range(1, max_degree - degree);
        for (int i = 0; i < out_edges && connect_random_edge(graph, src); ++i)
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
            quit("Broken or corrupted adjacency list.\n", 1);
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
    const Cell edge_id = sort_vertices(src->name, dst->name) << edge_id_shift;
    struct set parent_map;
    set_init(&parent_map);
    struct pqueue bfs;
    pq_init(&bfs);
    (void)insert_parent_cell(&parent_map, (struct parent_cell){
                                              .key = src->pos,
                                              .parent = (struct point){-1, -1},
                                          });
    insert_bfs_point(&bfs, (struct bfs_point){.p = src->pos});
    bool success = false;
    struct point cur = {0};
    while (!pq_empty(&bfs))
    {
        struct bfs_point *bp
            = pq_entry(pq_pop_max(&bfs), struct bfs_point, elem);
        cur = bp->p;
        const Cell cur_cell = grid_at(graph, cur);
        if (is_dst(cur_cell, dst->name))
        {
            free(bp);
            success = true;
            break;
        }
        for (size_t i = 0; i < dirs_size; ++i)
        {
            struct point next = {
                .r = cur.r + dirs[i].r,
                .c = cur.c + dirs[i].c,
            };
            struct parent_cell push = {.key = next, .parent = cur};
            const Cell next_cell = grid_at(graph, next);
            if (is_dst(next_cell, dst->name)
                || (!is_path(next_cell)
                    && !set_contains(&parent_map, &push.elem, cmp_parent_cells,
                                     NULL)))
            {
                (void)insert_parent_cell(&parent_map, push);
                insert_bfs_point(&bfs, (struct bfs_point){.p = next});
            }
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
    set_clear(&parent_map, set_parent_point_destructor);
    pq_clear(&bfs, pq_bfs_point_destructor);
    return success;
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

static inline bool
is_dst(const Cell c, const char dst)
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
dijkstra_shortest_path(struct graph *const graph, const struct path_request pr)
{
    struct pqueue dist_q;
    pq_init(&dist_q);
    struct set prev_map;
    set_init(&prev_map);
    for (struct set_elem *e = set_begin(&graph->adjacency_list);
         e != set_end(&graph->adjacency_list);
         e = set_next(&graph->adjacency_list, e))
    {
        struct dist_point *p = valid_malloc(sizeof(struct dist_point));
        *p = (struct dist_point){
            .v = set_entry(e, struct vertex, elem),
            .dist = INT_MAX,
        };
        if (p->v == pr.src)
        {
            p->dist = 0;
        }
        /* All vertices will have undefined parents until we have
           encountered the parent leading to them during the algorithm.
           This doubles as a caching mechanism and helper to find the
           vertex we need to update in the priority queue. */
        (void)insert_prev_vertex(&prev_map, (struct prev_vertex){
                                                .v = p->v,
                                                .prev = NULL,
                                                .pq_elem = &p->pq_elem,
                                            });
        (void)pq_insert(&dist_q, &p->pq_elem, cmp_pq_dist_points, NULL);
    }
    bool success = false;
    struct dist_point *cur = NULL;
    while (!pq_empty(&dist_q))
    {
        /* PQ entries are popped but the set will free the memory at
           the end because it always holds a reference to its pq_elem. */
        cur = pq_entry(pq_pop_min(&dist_q), struct dist_point, pq_elem);
        if (cur->v == pr.dst || cur->dist == INT_MAX)
        {
            success = cur->dist != INT_MAX;
            break;
        }
        for (int i = 0; i < max_degree && cur->v->edges[i].to; ++i)
        {
            struct prev_vertex pv = {.v = cur->v->edges[i].to};
            const struct set_elem *e
                = set_find(&prev_map, &pv.elem, cmp_set_prev_vertices, NULL);
            assert(e != set_end(&prev_map));
            struct prev_vertex *next = set_entry(e, struct prev_vertex, elem);
            /* We have encountered this element before because we know the
               parent in the path to it. Skip it. */
            if (next->prev)
            {
                continue;
            }
            /* The seen set also holds a pointer to the corresponding
               priority queue element so that this update is easier. */
            struct dist_point *dist
                = pq_entry(next->pq_elem, struct dist_point, pq_elem);
            int alt = cur->dist + cur->v->edges[i].cost;
            if (alt < dist->dist)
            {
                /* This vertex is now "seen" in the set and will be skipped
                   if seen again. */
                next->prev = cur->v;
                /* I would not prefer to do Dijkstra's with the update method
                   but it is the perfect opportunity to test this functionality
                   provided by the pq for helpfullness and correctness. */
                if (!pq_update(&dist_q, &dist->pq_elem, cmp_pq_dist_points,
                               pq_update_dist, &alt))
                {
                    quit("Updating vertex that is not in queue.\n", 1);
                }
            }
        }
    }
    if (success)
    {
        struct prev_vertex key = {.v = cur->v};
        struct prev_vertex *prev = set_entry(
            set_find(&prev_map, &key.elem, cmp_set_prev_vertices, NULL),
            struct prev_vertex, elem);
        while (prev->prev)
        {
            paint_edge(graph, key.v, prev->prev);
            key.v = prev->prev;
            prev = set_entry(
                set_find(&prev_map, &key.elem, cmp_set_prev_vertices, NULL),
                struct prev_vertex, elem);
        }
    }
    /* Choosing when to free gets tricky during the algorithm. So, the
       prev map is the last allocation with access to the priority queue
       elements that have been popped but not freed. It will free its
       own set and its references to priority queue elements. */
    set_clear(&prev_map, set_pq_prev_vertex_dist_point_destructor);
    clear_and_flush_graph(graph);
    return success;
}

static void
paint_edge(struct graph *const g, const struct vertex *const src,
           const struct vertex *const dst)
{
    struct point cur = src->pos;
    const Cell edge_id = sort_vertices(src->name, dst->name) << edge_id_shift;
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
            const Cell next_cell = grid_at(g, next);
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

/*========================  Graph/Grid Helpers  =============================*/

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

static inline Cell *
grid_at_mut(const struct graph *const graph, struct point p)
{
    return &graph->grid[p.r * graph->cols + p.c];
}

static inline Cell
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
get_cell_vertex_title(const Cell cell)
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
print_cell(const Cell cell)
{

    if (cell & vertex_bit)
    {
        printf("\033[38;5;14m%c\033[0m",
               (char)(cell >> vertex_cell_title_shift));
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
is_edge_vertex(const Cell square, Cell edge_id)
{
    const char vertex_name = get_cell_vertex_title(square);
    const char edge_vertex1
        = (char)((edge_id & l_edge_id_mask) >> l_edge_id_shift);
    const char edge_vertex2
        = (char)((edge_id & r_edge_id_mask) >> r_edge_id_shift);
    return vertex_name == edge_vertex1 || vertex_name == edge_vertex2;
}

static bool
is_valid_edge_cell(const Cell square, const Cell edge_id)
{
    return ((square & vertex_bit) && is_edge_vertex(square, edge_id))
           || ((square & path_bit) && (square & edge_id_mask) == edge_id);
}

static void
build_path_cell(struct graph *g, struct point p, const Cell edge_id)
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
insert_prev_vertex(struct set *s, struct prev_vertex pv)
{

    struct prev_vertex *pv_heap = valid_malloc(sizeof(struct prev_vertex));
    *pv_heap = pv;
    return set_insert(s, &pv_heap->elem, cmp_set_prev_vertices, NULL);
}

static bool
insert_parent_cell(struct set *s, struct parent_cell pc)
{
    struct parent_cell *const pc_heap
        = valid_malloc(sizeof(struct parent_cell));
    *pc_heap = pc;
    return set_insert(s, &pc_heap->elem, cmp_parent_cells, NULL);
}

static void
insert_bfs_point(struct pqueue *pq, struct bfs_point bp)
{
    struct bfs_point *const bfs_point_heap
        = valid_malloc(sizeof(struct bfs_point));
    *bfs_point_heap = bp;
    pq_insert(pq, &bfs_point_heap->elem, cmp_queue_points, NULL);
}

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
set_pq_prev_vertex_dist_point_destructor(struct set_elem *const e)
{
    struct prev_vertex *pv = set_entry(e, struct prev_vertex, elem);
    free(pq_entry(pv->pq_elem, struct dist_point, pq_elem));
    free(pv);
}

static void
set_parent_point_destructor(struct set_elem *const e)
{
    free(set_entry(e, struct parent_cell, elem));
}

static void
pq_bfs_point_destructor(struct pq_elem *const e)
{
    free(pq_entry(e, struct bfs_point, elem));
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
    const char end_title
        = (char)(start_vertex_title + set_size(&g->adjacency_list) - 1);
    for (const char *c = sv_begin(r); c != sv_end(r); c = sv_next(c))
    {
        if (*c >= start_vertex_title && *c <= end_title)
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
