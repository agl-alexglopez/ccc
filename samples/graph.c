#include "cli.h"
#include "pqueue.h"
#include "set.h"
#include "str_view.h"
#include <limits.h>
#include <stdint.h>
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

/* A vertex is connected via an edge to another vertex. Each vertex
   shall store a bounded list of edges representing the other vertices
   to which it is connected. The cost of this connection is the distance
   in cells on the screen to reach that vertex. Edges are undirected and
   unique between two vertices. */
struct edge
{
    char to;          /* This vertex shares an edge TO the specified vertex. */
    unsigned cost;    /* Distance in cells to the other vertex. */
    struct point dir; /* Path starts this way from the the vertex. */
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

/* The highest order 16 bits in the grid shall be reserved for the edge
   id if the square is a path. An edge ID is a concatenation of two
   vertex names. Vertex names are 8 bit characters, so two vertices can
   fit into a uint16_t which we have room for in a uint32_t. The concatenation
   shall always be sorted alphabetically so an edge connecting vertex A and
   Z will be uint16_t=AZ. */

const uint32_t wall_mask = 0b1111;
const uint32_t floating_wall = 0b0000;
const uint32_t north_wall = 0b0001;
const uint32_t east_wall = 0b0010;
const uint32_t south_wall = 0b0100;
const uint32_t west_wall = 0b1000;
const uint32_t cached_bit = 0b10000;
const uint32_t path_bit = 0b100000;
const uint32_t vertex_bit = 0b1000000;

/*==========================   Prototypes  ================================= */

static void build_graph(struct graph *);
static void find_shortest_paths(struct graph *);

static struct point random_vertex_placement(const struct graph *);
static bool is_valid_vertex_pos(const struct graph *, struct point);
static int rand_range(int, int);
static uint32_t *grid_at_mut(const struct graph *, struct point);
static uint32_t grid_at(const struct graph *, struct point);
static uint16_t sort_vertices(char a, char b);

static struct int_conversion parse_digits(str_view);
static void *valid_malloc(size_t);
static void help(void);

static threeway_cmp cmp_vertices(const struct set_elem *,
                                 const struct set_elem *, void *);
static void print_vertex(const struct set_elem *);

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
                quit("rows below required minimum or negative.\n");
            }
            graph.rows = row_arg.conversion;
        }
        else if (sv_starts_with(arg, cols))
        {
            const struct int_conversion col_arg = parse_digits(arg);
            if (col_arg.status == CONV_ER || col_arg.conversion < row_col_min)
            {
                quit("cols below required minimum or negative.\n");
            }
            graph.cols = col_arg.conversion;
        }
        else if (sv_starts_with(arg, speed))
        {
            const struct int_conversion speed_arg = parse_digits(arg);
            if (speed_arg.status == CONV_ER || speed_arg.conversion > speed_max
                || speed_arg.conversion < 0)
            {
                quit("speed outside of valid range.\n");
            }
            graph.speed = speed_arg.conversion;
        }
        else if (sv_starts_with(arg, vertices_flag))
        {
            const struct int_conversion vert_arg = parse_digits(arg);
            if (vert_arg.status == CONV_ER || vert_arg.conversion > max_vertices
                || vert_arg.conversion < 1)
            {
                quit("vertices outside of valid range.\n");
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
                 "for now (-r=N, -c=N, -s=N)\n");
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
}

/*========================   Graph Building    ==============================*/

static void
build_graph(struct graph *const graph)
{
    clear_screen();
    for (int vertex = 0, vertex_title = 'A'; vertex < graph->vertices;
         ++vertex, ++vertex_title)
    {
        struct point rand_point = random_vertex_placement(graph);
        struct vertex *new_vertex = valid_malloc(sizeof(struct vertex));
        *new_vertex = (struct vertex){
            .name = (char)vertex_title,
            .pos = rand_point,
            .edges = {{0}, {0}, {0}, {0}},
        };
        if (!set_insert(&graph->adjacency_list, &new_vertex->elem, cmp_vertices,
                        NULL))
        {
            (void)fprintf(
                stderr,
                "Error building the graph. New vertex is already present.\n");
            exit(1);
        }
        set_cursor_position(rand_point.r, rand_point.c);
        printf("%c", vertex_title);
    }
    for (int vertex = 0, vertex_title = 'A'; vertex < graph->vertices;
         ++vertex, ++vertex_title)
    {
        /* Select a random number of out degrees. */
        /* For each out degree, choose a random vertex to try to find. */
        /* If the vertex does not already have 4 in degrees */
        /* Find the vertex with bfs or dfs. */
        /* If another vertex is encountered along the way and
           already shares an edge with the vertex we seek,
           stop. If it does not share an edge with the desired
           vertex already start the search from this vertex
           we have encountered. */
    }
}

static void
find_shortest_paths(struct graph *const graph)
{
    (void)graph;
}

static struct point
random_vertex_placement(const struct graph *const graph)
{
    const int row_end = graph->rows - 2;
    const int col_end = graph->cols - 2;
    /* No vertices should be close to the edge of the map. */
    const int row_start = rand_range(3, graph->rows - 3);
    const int col_start = rand_range(3, graph->cols - 3);
    bool exhausted = false;
    for (int row = row_start; !exhausted && row < row_end;
         row = (row + 1) % row_end)
    {
        for (int col = col_start; !exhausted && col < col_end;
             col = (col + 1) % col_end)
        {
            const struct point cur = {.r = row, .c = col};
            if (is_valid_vertex_pos(graph, cur))
            {
                *grid_at_mut(graph, cur) |= vertex_bit;
                return cur;
            }
            exhausted = (row + 1) % graph->rows == row_start
                        && (col + 1) % graph->cols == col_start;
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

static void
print_vertex(const struct set_elem *const x)
{
    const struct vertex *v = set_entry(x, struct vertex, elem);
    printf("{%c,pos{%d,%d},edges{%c,%c,%c,%c}}", v->name, v->pos.r, v->pos.c,
           v->edges[0].to, v->edges[1].to, v->edges[2].to, v->edges[3].to);
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
