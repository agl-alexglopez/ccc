#include "flat_pqueue.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/* Printing enum for printing tree structures if heap available. */
enum ccc_print_link
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
};

enum fpq_direction
{
    L = 0,
    R,
};

#define COLOR_BLK "\033[34;1m"
#define COLOR_BLU_BOLD "\033[38;5;12m"
#define COLOR_RED_BOLD "\033[38;5;9m"
#define COLOR_RED "\033[31;1m"
#define COLOR_CYN "\033[36;1m"
#define COLOR_GRN "\033[32;1m"
#define COLOR_NIL "\033[0m"
#define COLOR_ERR COLOR_RED "Error: " COLOR_NIL

static size_t const starting_capacity = 8;

static void grow(ccc_flat_pqueue *);
static void swap(ccc_fpq_elem **, ccc_fpq_elem **);
static void bubble_down(ccc_flat_pqueue *, size_t);
static void bubble_up(ccc_flat_pqueue *, size_t);
static void print_node(ccc_flat_pqueue const *, size_t, fpq_print_fn *);
static void print_inner_heap(ccc_flat_pqueue const *, size_t, char const *,
                             enum ccc_print_link, fpq_print_fn *);
static void print_heap(ccc_flat_pqueue const *, size_t, fpq_print_fn *);

void
ccc_fpq_init(ccc_flat_pqueue *const hpq, ccc_fpq_threeway_cmp fpq_ordering,
             ccc_fpq_cmp_fn *cmp, void *aux)
{
    if (fpq_ordering == CCC_FPQ_EQL)
    {
        (void)fprintf(stderr,
                      "heap should be ordered CCC_FPQ_LES or CCC_FPQ_GRT.\n");
    }
    hpq->order = fpq_ordering;
    hpq->sz = 0;
    hpq->capacity = starting_capacity;
    hpq->heap
        = calloc(starting_capacity, starting_capacity * sizeof(ccc_fpq_elem *));
    if (!hpq->heap)
    {
        (void)fprintf(stderr, "heap backing store exhausted.\n");
    }
    hpq->cmp = cmp;
    hpq->aux = aux;
}

void
ccc_fpq_push(ccc_flat_pqueue *const hpq, ccc_fpq_elem *e)
{
    if (hpq->sz == hpq->capacity)
    {
        grow(hpq);
    }
    hpq->heap[hpq->sz] = e;
    e->handle = hpq->sz;
    ++hpq->sz;
    bubble_up(hpq, hpq->sz - 1);
}

ccc_fpq_elem *
ccc_fpq_pop(ccc_flat_pqueue *hpq)
{
    if (!hpq->sz)
    {
        return NULL;
    }
    --hpq->sz;
    swap(&hpq->heap[0], &hpq->heap[hpq->sz]);
    ccc_fpq_elem *ret = hpq->heap[hpq->sz];
    bubble_down(hpq, 0);
    return ret;
}

ccc_fpq_elem *
ccc_fpq_erase(ccc_flat_pqueue *const hpq, ccc_fpq_elem *e)
{
    if (!hpq->sz)
    {
        return NULL;
    }
    --hpq->sz;
    if (hpq->sz == 1)
    {
        return hpq->heap[0];
    }
    if (e->handle == hpq->sz)
    {
        return hpq->heap[hpq->sz];
    }
    /* Important to remember this key now to avoid confusion later once the
       elements are swapped and we lose access to original handle index. */
    size_t const swap_location = e->handle;
    swap(&hpq->heap[swap_location], &hpq->heap[hpq->sz]);
    ccc_fpq_elem *erased = hpq->heap[hpq->sz];
    ccc_fpq_threeway_cmp const erased_cmp
        = hpq->cmp(hpq->heap[swap_location], erased, hpq->aux);
    if (erased_cmp == hpq->order)
    {
        bubble_up(hpq, swap_location);
    }
    else if (erased_cmp != CCC_FPQ_EQL)
    {
        bubble_down(hpq, swap_location);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return erased;
}

bool
ccc_fpq_update(ccc_flat_pqueue *hpq, ccc_fpq_elem *e, fpq_update_fn *fn,
               void *aux)
{
    if (!e || !hpq->sz)
    {
        return false;
    }
    fn(e, aux);
    if (!e->handle)
    {
        bubble_down(hpq, 0);
        return true;
    }
    ccc_fpq_threeway_cmp const parent_cmp = hpq->cmp(
        hpq->heap[e->handle], hpq->heap[(e->handle - 1) / 2], hpq->aux);
    if (parent_cmp == hpq->order)
    {
        bubble_up(hpq, e->handle);
        return true;
    }
    if (parent_cmp != CCC_FPQ_EQL)
    {
        bubble_down(hpq, e->handle);
        return true;
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return true;
}

ccc_fpq_elem const *
ccc_fpq_front(ccc_flat_pqueue const *const hpq)
{
    if (!hpq->sz)
    {
        return NULL;
    }
    return hpq->heap[0];
}

bool
ccc_fpq_empty(ccc_flat_pqueue const *const hpq)
{
    if (!hpq)
    {
        return true;
    }
    return !hpq->sz;
}

size_t
ccc_fpq_size(ccc_flat_pqueue const *const hpq)
{
    if (!hpq)
    {
        return 0ULL;
    }
    return hpq->sz;
}

ccc_fpq_threeway_cmp
ccc_fpq_order(ccc_flat_pqueue const *const hpq)
{
    return hpq->order;
}

void
ccc_fpq_clear(ccc_flat_pqueue *hpq, fpq_destructor_fn *fn)
{
    for (size_t i = 0; i < hpq->sz; ++i)
    {
        fn(hpq->heap[i]);
    }
    free(hpq->heap);
    hpq->sz = hpq->capacity = 0;
    hpq->cmp = NULL;
    hpq->heap = NULL;
}

bool
ccc_fpq_validate(ccc_flat_pqueue const *const hpq)
{
    if (hpq->sz > 1)
    {
        for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2;
             i <= (hpq->sz - 2) / 2;
             ++i, left = (i * 2) + 1, right = (i * 2) + 2)
        {
            ccc_fpq_elem const *const cur = hpq->heap[i];
            /* Putting the child in the comparison function first evaluates
               the childs three way comparison in relation to the parent. If
               the child beats the parent in total ordering (min/max) something
               has gone wrong. */
            if (left < hpq->sz
                && hpq->cmp(hpq->heap[left], cur, hpq->aux) == hpq->order)
            {
                return false;
            }
            if (right < hpq->sz
                && hpq->cmp(hpq->heap[right], cur, hpq->aux) == hpq->order)
            {
                return false;
            }
        }
    }
    for (size_t i = 0; i < hpq->sz; ++i)
    {
        if (hpq->heap[i]->handle != i)
        {
            return false;
        }
    }
    return true;
}

void
ccc_fpq_print(ccc_flat_pqueue const *hpq, size_t const i,
              fpq_print_fn *const fn)
{
    print_heap(hpq, i, fn);
}

/*===============================  Static Helpers  =========================*/

static void
bubble_up(ccc_flat_pqueue *const hpq, size_t i)
{
    for (size_t parent = (i - 1) / 2;
         i && hpq->cmp(hpq->heap[i], hpq->heap[parent], hpq->aux) == hpq->order;
         i = parent, parent = (parent - 1) / 2)
    {
        swap(&hpq->heap[parent], &hpq->heap[i]);
    }
    hpq->heap[i]->handle = i;
}

static void
bubble_down(ccc_flat_pqueue *hpq, size_t i)
{
    ccc_fpq_threeway_cmp const wrong_order
        = hpq->order == CCC_FPQ_LES ? CCC_FPQ_GRT : CCC_FPQ_LES;
    for (size_t next = i, left = i * 2 + 1, right = left + 1; left < hpq->sz;
         i = next, left = i * 2 + 1, right = left + 1)
    {
        /* Without knowing the cost of the user provided comparison function,
           it is important to call the cmp function minimal number of times.
           Avoid one call if there is no right child. */
        next = (right < hpq->sz
                && (hpq->order
                    == hpq->cmp(hpq->heap[right], hpq->heap[left], hpq->aux)))
                   ? right
                   : left;
        if (hpq->cmp(hpq->heap[i], hpq->heap[next], NULL) != wrong_order)
        {
            break;
        }
        swap(&hpq->heap[next], &hpq->heap[i]);
    }
    hpq->heap[i]->handle = i;
}

static void
grow(ccc_flat_pqueue *hpq)
{
    ccc_fpq_elem **new
        = realloc(hpq->heap, sizeof(ccc_fpq_elem *) * hpq->capacity * 2);
    if (!new)
    {
        (void)fprintf(stderr, "reallocation of flat priority queue failed.\n");
    }
    hpq->heap = new;
    hpq->capacity *= 2;
}

static inline void
swap(ccc_fpq_elem **a, ccc_fpq_elem **b)
{
    size_t const temp_handle = (*a)->handle;
    (*a)->handle = (*b)->handle;
    (*b)->handle = temp_handle;
    ccc_fpq_elem *temp = *a;
    *a = *b;
    *b = temp;
}

/* NOLINTBEGIN(*misc-no-recursion) */

static void
print_node(ccc_flat_pqueue const *const hpq, size_t i, fpq_print_fn *const fn)
{
    printf(COLOR_CYN);
    if (hpq->heap[i]->handle)
    {
        hpq->heap[hpq->heap[(i - 1) / 2]->handle * 2 + 1] == hpq->heap[i]
            ? printf("L%zu:", i)
            : printf("R%zu:", i);
    }
    printf(COLOR_NIL);
    fn(hpq->heap[i]);
    printf("\n");
}

static void
print_inner_heap(ccc_flat_pqueue const *const hpq, size_t i, char const *prefix,
                 enum ccc_print_link const node_type, fpq_print_fn *const fn)
{
    if (i >= hpq->sz)
    {
        return;
    }
    printf("%s", prefix);
    printf("%s", node_type == LEAF ? " └──" : " ├──");
    print_node(hpq, i, fn);

    char *str = NULL;
    int string_length
        = snprintf(NULL, 0, "%s%s", prefix,
                   node_type == LEAF ? "     " : " │   "); // NOLINT
    if (string_length > 0)
    {
        str = malloc(string_length + 1);
        (void)snprintf(str, string_length, "%s%s", prefix,
                       node_type == LEAF ? "     " : " │   "); // NOLINT
    }
    if (str != NULL)
    {
        if ((i * 2) + 2 >= hpq->sz)
        {
            print_inner_heap(hpq, (i * 2) + 1, str, LEAF, fn);
        }
        else
        {
            print_inner_heap(hpq, (i * 2) + 2, str, BRANCH, fn);
            print_inner_heap(hpq, (i * 2) + 1, str, LEAF, fn);
        }
    }
    else
    {
        printf(COLOR_ERR "memory exceeded. Cannot display tree." COLOR_NIL);
    }
    free(str);
}

static void
print_heap(ccc_flat_pqueue const *const hpq, size_t i, fpq_print_fn *const fn)
{
    if (i >= hpq->sz)
    {
        return;
    }
    printf(" ");
    print_node(hpq, i, fn);

    if ((i * 2) + 2 >= hpq->sz)
    {
        print_inner_heap(hpq, (i * 2) + 1, "", LEAF, fn);
    }
    else
    {
        print_inner_heap(hpq, (i * 2) + 2, "", BRANCH, fn);
        print_inner_heap(hpq, (i * 2) + 1, "", LEAF, fn);
    }
}

/* NOLINTEND(*misc-no-recursion) */
