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
   in cells on the screen to reach that vertex. */
struct edge
{
    char to;       /* An edge is a connection TO another vertex */
    unsigned cost; /* Distance in cells to the other vertex. */
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
    uint16_t *grid;
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

const uint16_t wall_mask = 0b1111;
const uint16_t floating_wall = 0b0000;
const uint16_t north_wall = 0b0001;
const uint16_t east_wall = 0b0010;
const uint16_t south_wall = 0b0100;
const uint16_t west_wall = 0b1000;
const uint16_t builder_bit = 0b1000000000000;
const uint16_t path_bit = 0b10000000000000;
const uint16_t vertex_bit = 0b1000000000000000;

/*==========================   Prototypes  ================================= */

void build_graph(struct graph *);
void find_shortest_paths(struct graph *);

void place_rand_vertex(struct graph *, char);
struct point scan_grid(const struct graph *);
bool is_valid_vertex_pos(const struct graph *, struct point);
int rand_range(int, int);

struct int_conversion parse_digits(str_view);
void *valid_malloc(size_t);
void help(void);

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
    graph.grid = calloc((size_t)graph.rows * graph.cols, sizeof(uint16_t));
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

void
build_graph(struct graph *const graph)
{
    clear_screen();
    char vertex_title = 'A';
    for (int vertex = 0; vertex < graph->vertices; ++vertex, ++vertex_title)
    {
        place_rand_vertex(graph, vertex_title);
    }
}

void
find_shortest_paths(struct graph *const graph)
{
    (void)graph;
}

void
place_rand_vertex(struct graph *const graph, const char vertex_title)
{
    /* No vertices should be close to the edge of the map. */
    struct point rand_point = scan_grid(graph);
    struct vertex *new_vertex = valid_malloc(sizeof(struct vertex));
    *new_vertex = (struct vertex){
        .name = vertex_title,
        .pos = rand_point,
        .edges = {{0}, {0}, {0}, {0}},
    };
    set_cursor_position(rand_point.r, rand_point.c);
    printf("%c", vertex_title);
}

struct point
scan_grid(const struct graph *const graph)
{
    const int row_start = rand_range(3, graph->rows - 3);
    const int col_start = rand_range(3, graph->cols - 3);
    bool lapped = false;
    for (int row = row_start; !lapped && row < graph->rows - 3; ++row)
    {
        for (int col = col_start; !lapped && col < graph->cols - 3; ++col)
        {
            const struct point cur = {.r = row, .c = col};
            if (is_valid_vertex_pos(graph, cur))
            {
                return cur;
            }
            lapped = row + 1 == row_start && col + 1 == col_start;
        }
    }
    (void)fprintf(stderr, "cannot find a place for another vertex "
                          "on this grid, quitting now.\n");
    exit(1);
}

bool
is_valid_vertex_pos(const struct graph *graph, struct point p)
{
    return !(graph->grid[p.r * graph->cols + p.c] & vertex_bit)
           && !(graph->grid[(p.r + 1) * graph->cols + p.c] & vertex_bit)
           && !(graph->grid[(p.r - 1) * graph->cols + p.c] & vertex_bit)
           && !(graph->grid[p.r * graph->cols + (p.c - 1)] & vertex_bit)
           && !(graph->grid[p.r * graph->cols + (p.c + 1)] & vertex_bit);
}

int
rand_range(const int min, const int max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

/*===========================    Misc    ====================================*/

struct int_conversion
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
void *
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

void
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
        "0-7.\nExample:\n./build/rel/graph -c=111 -r=33 -s=4\n");
}
