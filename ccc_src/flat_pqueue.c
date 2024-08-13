#include "flat_pqueue.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void *at(ccc_flat_pqueue const *, size_t);
static size_t index_of(ccc_flat_pqueue const *, void const *);
static bool wins(ccc_flat_pqueue const *, void const *winner,
                 void const *loser);
static void swap(ccc_flat_pqueue *, uint8_t tmp[], size_t, size_t);
static void bubble_down(ccc_flat_pqueue *, uint8_t tmp[], size_t);
static void print_node(ccc_flat_pqueue const *, size_t, fpq_print_fn *);
static void print_inner_heap(ccc_flat_pqueue const *, size_t, char const *,
                             enum ccc_print_link, fpq_print_fn *);
static void print_heap(ccc_flat_pqueue const *, size_t, fpq_print_fn *);

ccc_fpq_result
ccc_fpq_realloc(ccc_flat_pqueue *const fpq, size_t const new_capacity,
                ccc_buf_realloc_fn *const fn)
{
    return (ccc_fpq_result)ccc_buf_realloc(fpq->buf, new_capacity, fn);
}

ccc_fpq_result
ccc_fpq_push(ccc_flat_pqueue *const fpq, void const *const val)
{
    void *const new = ccc_buf_alloc(fpq->buf);
    if (!new)
    {
        return CCC_FPQ_FULL;
    }
    if (new != val)
    {
        memcpy(new, val, ccc_buf_elem_size(fpq->buf));
    }
    size_t const buf_sz = ccc_buf_size(fpq->buf);
    size_t const i = buf_sz - 1;
    if (buf_sz > 1)
    {
        uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
        bubble_up(fpq, tmp, i);
    }
    return CCC_FPQ_OK;
}

ccc_buf *
ccc_fpq_buf(ccc_flat_pqueue const *fpq)
{
    return fpq->buf;
}

void *
ccc_fpq_pop(ccc_flat_pqueue *const fpq)
{
    if (ccc_buf_empty(fpq->buf))
    {
        return NULL;
    }
    void *ret = at(fpq, ccc_buf_size(fpq->buf) - 1);
    if (ccc_buf_size(fpq->buf) == 1)
    {
        ccc_buf_pop_back(fpq->buf);
        return ret;
    }
    uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
    swap(fpq, tmp, 0, ccc_buf_size(fpq->buf) - 1);
    ccc_buf_pop_back(fpq->buf);
    bubble_down(fpq, tmp, 0);
    return ret;
}

void *
ccc_fpq_erase(ccc_flat_pqueue *const fpq, void *const e)
{
    if (ccc_buf_empty(fpq->buf))
    {
        return NULL;
    }
    if (ccc_buf_size(fpq->buf) == 1)
    {
        void *const ret = at(fpq, 0);
        ccc_buf_pop_back(fpq->buf);
        return ret;
    }
    /* Important to remember this key now to avoid confusion later once the
       elements are swapped and we lose access to original handle index. */
    size_t const swap_location = index_of(fpq, e);
    if (swap_location == ccc_buf_size(fpq->buf) - 1)
    {
        void *const ret = at(fpq, ccc_buf_size(fpq->buf) - 1);
        ccc_buf_pop_back(fpq->buf);
        return ret;
    }
    uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
    swap(fpq, tmp, swap_location, ccc_buf_size(fpq->buf) - 1);
    void *const erased = at(fpq, ccc_buf_size(fpq->buf) - 1);
    ccc_buf_pop_back(fpq->buf);
    ccc_fpq_threeway_cmp const erased_cmp
        = fpq->cmp(at(fpq, swap_location), erased, fpq->aux);
    if (erased_cmp == fpq->order)
    {
        bubble_up(fpq, tmp, swap_location);
    }
    else if (erased_cmp != CCC_FPQ_EQL)
    {
        bubble_down(fpq, tmp, swap_location);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return erased;
}

bool
ccc_fpq_update(ccc_flat_pqueue *const fpq, void *const e, fpq_update_fn *fn,
               void *aux)
{
    if (ccc_buf_empty(fpq->buf))
    {
        return false;
    }
    fn(e, aux);
    uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
    size_t const i = index_of(fpq, e);
    if (!i)
    {
        bubble_down(fpq, tmp, 0);
        return true;
    }
    ccc_fpq_threeway_cmp const parent_cmp
        = fpq->cmp(at(fpq, i), at(fpq, (i - 1) / 2), fpq->aux);
    if (parent_cmp == fpq->order)
    {
        bubble_up(fpq, tmp, i);
        return true;
    }
    if (parent_cmp != CCC_FPQ_EQL)
    {
        bubble_down(fpq, tmp, i);
        return true;
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return true;
}

void const *
ccc_fpq_front(ccc_flat_pqueue const *const fpq)
{
    if (ccc_buf_empty(fpq->buf))
    {
        return NULL;
    }
    return at(fpq, 0);
}

bool
ccc_fpq_empty(ccc_flat_pqueue const *const fpq)
{
    return ccc_buf_empty(fpq->buf);
}

size_t
ccc_fpq_size(ccc_flat_pqueue const *const fpq)
{
    return ccc_buf_size(fpq->buf);
}

ccc_fpq_threeway_cmp
ccc_fpq_order(ccc_flat_pqueue const *const fpq)
{
    return fpq->order;
}

void
ccc_fpq_clear(ccc_flat_pqueue *const fpq, fpq_destructor_fn *fn)
{
    size_t const sz = ccc_buf_size(fpq->buf);
    for (size_t i = 0; i < sz; ++i)
    {
        fn(at(fpq, i));
    }
    ccc_buf_pop_back_n(fpq->buf, ccc_buf_size(fpq->buf));
}

bool
ccc_fpq_validate(ccc_flat_pqueue const *const fpq)
{
    size_t const sz = ccc_buf_size(fpq->buf);
    if (sz <= 1)
    {
        return true;
    }
    for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2;
         i <= (sz - 2) / 2; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
    {
        void *const cur = at(fpq, i);
        /* Putting the child in the comparison function first evaluates
           the childs three way comparison in relation to the parent. If
           the child beats the parent in total ordering (min/max) something
           has gone wrong. */
        if (left < sz && wins(fpq, at(fpq, left), cur))
        {
            return false;
        }
        if (right < sz && wins(fpq, at(fpq, right), cur))
        {
            return false;
        }
    }
    return true;
}

void
ccc_fpq_print(ccc_flat_pqueue const *const fpq, size_t const i,
              fpq_print_fn *const fn)
{
    print_heap(fpq, i, fn);
}

void
bubble_up(ccc_flat_pqueue *const fpq, uint8_t tmp[], size_t i)
{
    for (size_t parent = (i - 1) / 2;
         i && wins(fpq, at(fpq, i), at(fpq, parent));
         i = parent, parent = (parent - 1) / 2)
    {
        swap(fpq, tmp, parent, i);
    }
}

/*===============================  Static Helpers  =========================*/

static void
bubble_down(ccc_flat_pqueue *const fpq, uint8_t tmp[], size_t i)
{
    size_t const sz = ccc_buf_size(fpq->buf);
    for (size_t next = i, left = i * 2 + 1, right = left + 1; left < sz;
         i = next, left = i * 2 + 1, right = left + 1)
    {
        /* Avoid one comparison call if there is no right child. */
        next = right < sz && wins(fpq, at(fpq, right), at(fpq, left)) ? right
                                                                      : left;
        /* If the child beats the parent we must swap. Equal is ok to break. */
        if (!wins(fpq, at(fpq, next), at(fpq, i)))
        {
            break;
        }
        swap(fpq, tmp, next, i);
    }
}

static inline void
swap(ccc_flat_pqueue *const fpq, uint8_t tmp[], size_t const i, size_t const j)
{
    ccc_buf_result const res = ccc_buf_swap(fpq->buf, tmp, i, j);
    (void)res;
    assert(res == CCC_BUF_OK);
}

/* Thin wrapper just for sanity checking in debug mode as index should always
   be valid when this function is used. */
static inline void *
at(ccc_flat_pqueue const *const fpq, size_t const i)
{
    void *const addr = ccc_buf_at(fpq->buf, i);
    assert(addr);
    return addr;
}

/* Flat pqueue code that uses indices of the underlying buffer should always
   be within the buffer range. It should never exceed the current size and
   start at or after the buffer base. Only checked in debug. */
static inline size_t
index_of(ccc_flat_pqueue const *const fpq, void const *const slot)
{
    assert(slot >= ccc_buf_base(fpq->buf));
    size_t const i = (((uint8_t *)slot) - ((uint8_t *)ccc_buf_base(fpq->buf)))
                     / ccc_buf_elem_size(fpq->buf);
    assert(i < ccc_buf_size(fpq->buf));
    return i;
}

static inline bool
wins(ccc_flat_pqueue const *const fpq, void const *const winner,
     void const *const loser)
{
    return fpq->cmp(winner, loser, fpq->aux) == fpq->order;
}

/* NOLINTBEGIN(*misc-no-recursion) */

static void
print_node(ccc_flat_pqueue const *const fpq, size_t i, fpq_print_fn *const fn)
{
    printf(COLOR_CYN);
    if (i)
    {
        at(fpq, index_of(fpq, at(fpq, (i - 1) / 2)) * 2 + 1) == at(fpq, i)
            ? printf("L%zu:", i)
            : printf("R%zu:", i);
    }
    else
    {
        printf("%zu:", i);
    }
    printf(COLOR_NIL);
    fn(at(fpq, i));
    printf("\n");
}

static void
print_inner_heap(ccc_flat_pqueue const *const fpq, size_t i, char const *prefix,
                 enum ccc_print_link const node_type, fpq_print_fn *const fn)
{
    size_t const sz = ccc_buf_size(fpq->buf);
    if (i >= sz)
    {
        return;
    }
    printf("%s", prefix);
    printf("%s", node_type == LEAF ? " └──" : " ├──");
    print_node(fpq, i, fn);

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
        if ((i * 2) + 2 >= sz)
        {
            print_inner_heap(fpq, (i * 2) + 1, str, LEAF, fn);
        }
        else
        {
            print_inner_heap(fpq, (i * 2) + 2, str, BRANCH, fn);
            print_inner_heap(fpq, (i * 2) + 1, str, LEAF, fn);
        }
    }
    else
    {
        printf(COLOR_ERR "memory exceeded. Cannot display tree." COLOR_NIL);
    }
    free(str);
}

static void
print_heap(ccc_flat_pqueue const *const fpq, size_t i, fpq_print_fn *const fn)
{
    size_t const sz = ccc_buf_size(fpq->buf);
    if (i >= sz)
    {
        return;
    }
    printf(" ");
    print_node(fpq, i, fn);

    if ((i * 2) + 2 >= sz)
    {
        print_inner_heap(fpq, (i * 2) + 1, "", LEAF, fn);
    }
    else
    {
        print_inner_heap(fpq, (i * 2) + 2, "", BRANCH, fn);
        print_inner_heap(fpq, (i * 2) + 1, "", LEAF, fn);
    }
}

/* NOLINTEND(*misc-no-recursion) */
