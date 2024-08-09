#include "flat_pqueue.h"
#include "attrib.h"
#include "test.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

static ccc_fpq_elem *at(ccc_flat_pqueue const[static 1], size_t i)
    ATTRIB_NONNULL(1);
static void swap(ccc_flat_pqueue[static 1], uint8_t[static 1], size_t, size_t)
    ATTRIB_NONNULL(1, 2);
static void bubble_down(ccc_flat_pqueue[static 1], uint8_t tmp[static 1],
                        size_t) ATTRIB_NONNULL(1, 2);
static void bubble_up(ccc_flat_pqueue[static 1], uint8_t tmp[static 1], size_t)
    ATTRIB_NONNULL(1, 2);
static void print_node(ccc_flat_pqueue const[static 1], size_t, fpq_print_fn *)
    ATTRIB_NONNULL(1);
static void print_inner_heap(ccc_flat_pqueue const[static 1], size_t,
                             char const *, enum ccc_print_link, fpq_print_fn *)
    ATTRIB_NONNULL(1);
static void print_heap(ccc_flat_pqueue const[static 1], size_t, fpq_print_fn *)
    ATTRIB_NONNULL(1);

ccc_buf_result
ccc_fpq_push(ccc_flat_pqueue fpq[static const 1],
             ccc_fpq_elem e[static const 1])
{
    if (ccc_buf_empty(fpq->buf) || at(fpq, ccc_buf_size(fpq->buf) - 1) != e)
    {
        return CCC_BUF_ERR;
    }
    size_t const buf_sz = ccc_buf_size(fpq->buf);
    e->handle = buf_sz - 1;
    if (buf_sz > 1)
    {
        uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
        bubble_up(fpq, tmp, e->handle);
    }
    return CCC_BUF_OK;
}

ccc_buf *
ccc_fpq_buf(ccc_flat_pqueue const fpq[static 1])
{
    return fpq->buf;
}

ccc_fpq_elem *
ccc_fpq_pop(ccc_flat_pqueue fpq[static const 1])
{
    if (ccc_buf_empty(fpq->buf))
    {
        return NULL;
    }
    ccc_fpq_elem *ret = at(fpq, ccc_buf_size(fpq->buf) - 1);
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

ccc_fpq_elem *
ccc_fpq_erase(ccc_flat_pqueue fpq[static const 1],
              ccc_fpq_elem e[static const 1])
{
    if (ccc_buf_empty(fpq->buf))
    {
        return NULL;
    }
    if (ccc_buf_size(fpq->buf) == 1)
    {
        ccc_fpq_elem *const ret = at(fpq, 0);
        ccc_buf_pop_back(fpq->buf);
        return ret;
    }
    if (e->handle == ccc_buf_size(fpq->buf) - 1)
    {
        ccc_fpq_elem *const ret = at(fpq, ccc_buf_size(fpq->buf) - 1);
        ccc_buf_pop_back(fpq->buf);
        return ret;
    }
    /* Important to remember this key now to avoid confusion later once the
       elements are swapped and we lose access to original handle index. */
    size_t const swap_location = e->handle;
    uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
    swap(fpq, tmp, swap_location, ccc_buf_size(fpq->buf) - 1);
    ccc_fpq_elem *erased = at(fpq, ccc_buf_size(fpq->buf) - 1);
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
ccc_fpq_update(ccc_flat_pqueue fpq[static const 1],
               ccc_fpq_elem e[static const 1], fpq_update_fn *fn, void *aux)
{
    if (ccc_buf_empty(fpq->buf))
    {
        return false;
    }
    fn(e, aux);
    uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
    if (!e->handle)
    {
        bubble_down(fpq, tmp, 0);
        return true;
    }
    ccc_fpq_threeway_cmp const parent_cmp
        = fpq->cmp(at(fpq, e->handle), at(fpq, (e->handle - 1) / 2), fpq->aux);
    if (parent_cmp == fpq->order)
    {
        bubble_up(fpq, tmp, e->handle);
        return true;
    }
    if (parent_cmp != CCC_FPQ_EQL)
    {
        bubble_down(fpq, tmp, e->handle);
        return true;
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    return true;
}

ccc_fpq_elem const *
ccc_fpq_front(ccc_flat_pqueue const fpq[static const 1])
{
    if (ccc_buf_empty(fpq->buf))
    {
        return NULL;
    }
    return at(fpq, 0);
}

bool
ccc_fpq_empty(ccc_flat_pqueue const fpq[static const 1])
{
    return ccc_buf_empty(fpq->buf);
}

size_t
ccc_fpq_size(ccc_flat_pqueue const fpq[static const 1])
{
    return ccc_buf_size(fpq->buf);
}

ccc_fpq_threeway_cmp
ccc_fpq_order(ccc_flat_pqueue const fpq[static const 1])
{
    return fpq->order;
}

void
ccc_fpq_clear(ccc_flat_pqueue fpq[static const 1], fpq_destructor_fn *fn)
{
    size_t const sz = ccc_buf_size(fpq->buf);
    for (size_t i = 0; i < sz; ++i)
    {
        fn(at(fpq, i));
    }
    ccc_buf_pop_back_n(fpq->buf, ccc_buf_size(fpq->buf));
    fpq->cmp = NULL;
}

bool
ccc_fpq_validate(ccc_flat_pqueue const fpq[static const 1])
{
    size_t const sz = ccc_buf_size(fpq->buf);
    if (sz > 1)
    {
        for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2;
             i <= (sz - 2) / 2; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
        {
            ccc_fpq_elem const *const cur = at(fpq, i);
            /* Putting the child in the comparison function first evaluates
               the childs three way comparison in relation to the parent. If
               the child beats the parent in total ordering (min/max) something
               has gone wrong. */
            if (left < sz
                && fpq->cmp(at(fpq, left), cur, fpq->aux) == fpq->order)
            {
                return false;
            }
            if (right < sz
                && fpq->cmp(at(fpq, right), cur, fpq->aux) == fpq->order)
            {
                return false;
            }
        }
    }
    for (size_t i = 0; i < sz; ++i)
    {
        if (at(fpq, i)->handle != i)
        {
            return false;
        }
    }
    return true;
}

void
ccc_fpq_print(ccc_flat_pqueue const fpq[static const 1], size_t const i,
              fpq_print_fn *const fn)
{
    print_heap(fpq, i, fn);
}

/*===============================  Static Helpers  =========================*/

static void
bubble_up(ccc_flat_pqueue fpq[static const 1], uint8_t tmp[static 1], size_t i)
{
    for (size_t parent = (i - 1) / 2;
         i && fpq->cmp(at(fpq, i), at(fpq, parent), fpq->aux) == fpq->order;
         i = parent, parent = (parent - 1) / 2)
    {
        swap(fpq, tmp, parent, i);
    }
}

static void
bubble_down(ccc_flat_pqueue fpq[static const 1], uint8_t tmp[static 1],
            size_t i)
{
    ccc_fpq_threeway_cmp const wrong_order
        = fpq->order == CCC_FPQ_LES ? CCC_FPQ_GRT : CCC_FPQ_LES;
    size_t const sz = ccc_buf_size(fpq->buf);
    for (size_t next = i, left = i * 2 + 1, right = left + 1; left < sz;
         i = next, left = i * 2 + 1, right = left + 1)
    {
        /* Without knowing the cost of the user provided comparison function,
           it is important to call the cmp function minimal number of times.
           Avoid one call if there is no right child. */
        next = (right < sz
                && (fpq->order
                    == fpq->cmp(at(fpq, right), at(fpq, left), fpq->aux)))
                   ? right
                   : left;
        if (fpq->cmp(at(fpq, i), at(fpq, next), NULL) != wrong_order)
        {
            break;
        }
        swap(fpq, tmp, next, i);
    }
}

static inline void
swap(ccc_flat_pqueue fpq[static const 1], uint8_t tmp[static const 1],
     size_t const i, size_t const j)
{
    ccc_buf_result const res = ccc_buf_swap(fpq->buf, tmp, i, j);
    if (res != CCC_BUF_OK)
    {
        BREAKPOINT();
    }
    ccc_fpq_elem *const i_elem = at(fpq, i);
    size_t const tmp_handle = i_elem->handle;
    ccc_fpq_elem *const j_elem = at(fpq, j);
    i_elem->handle = j_elem->handle;
    j_elem->handle = tmp_handle;
}

static inline ccc_fpq_elem *
at(ccc_flat_pqueue const fpq[static 1], size_t const i)
{
    void *loc = ccc_buf_at(fpq->buf, i);
    assert(loc);
    return (ccc_fpq_elem *)((uint8_t *)loc + fpq->fpq_elem_offset);
}

/* NOLINTBEGIN(*misc-no-recursion) */

static void
print_node(ccc_flat_pqueue const fpq[static const 1], size_t i,
           fpq_print_fn *const fn)
{
    printf(COLOR_CYN);
    if (at(fpq, i)->handle)
    {
        at(fpq, at(fpq, (i - 1) / 2)->handle * 2 + 1) == at(fpq, i)
            ? printf("L%zu:", i)
            : printf("R%zu:", i);
    }
    printf(COLOR_NIL);
    fn(at(fpq, i));
    printf("\n");
}

static void
print_inner_heap(ccc_flat_pqueue const fpq[static const 1], size_t i,
                 char const *prefix, enum ccc_print_link const node_type,
                 fpq_print_fn *const fn)
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
print_heap(ccc_flat_pqueue const fpq[static const 1], size_t i,
           fpq_print_fn *const fn)
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
