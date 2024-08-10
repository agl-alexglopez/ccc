#include "flat_pqueue.h"
#include "attrib.h"

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
static ccc_fpq_elem *val(ccc_flat_pqueue const[static 1], size_t i)
    ATTRIB_NONNULL(1);
static size_t index_of(ccc_flat_pqueue const[static 1],
                       ccc_fpq_elem const[static 1]) ATTRIB_NONNULL(1, 2);
static void swap(ccc_flat_pqueue[static 1], size_t, size_t) ATTRIB_NONNULL(1);
static void bubble_down(ccc_flat_pqueue[static 1], size_t, size_t)
    ATTRIB_NONNULL(1);
static void bubble_up(ccc_flat_pqueue[static 1], size_t) ATTRIB_NONNULL(1);
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
    e->buf_key = e->heap_index = buf_sz - 1;
    if (buf_sz > 1)
    {
        bubble_up(fpq, buf_sz - 1);
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
    swap(fpq, 0, ccc_buf_size(fpq->buf) - 1);
    bubble_down(fpq, 0, ccc_buf_size(fpq->buf) - 1);
    size_t const to_delete = ret->buf_key;
    if (to_delete == ccc_buf_size(fpq->buf) - 1)
    {
        ccc_buf_pop_back(fpq->buf);
        return ret;
    }
    ccc_fpq_elem *const prev_entry = at(fpq, to_delete);
    size_t const previously_stored_key = prev_entry->buf_key;
    assert(prev_entry->heap_index == ccc_buf_size(fpq->buf) - 1);
    uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
    ccc_buf_result res
        = ccc_buf_swap(fpq->buf, tmp, to_delete, ccc_buf_size(fpq->buf) - 1);
    (void)res;
    assert(res == CCC_BUF_OK);
    prev_entry->buf_key = previously_stored_key;
    at(fpq, prev_entry->heap_index)->buf_key = to_delete;
    ccc_buf_pop_back(fpq->buf);
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
    size_t const swap_location = e->heap_index;
    swap(fpq, swap_location, ccc_buf_size(fpq->buf) - 1);
    ccc_fpq_elem *erased = at(fpq, ccc_buf_size(fpq->buf) - 1);
    ccc_fpq_threeway_cmp const erased_cmp
        = fpq->cmp(val(fpq, swap_location),
                   val(fpq, ccc_buf_size(fpq->buf) - 1), fpq->aux);
    if (erased_cmp == fpq->order)
    {
        bubble_up(fpq, swap_location);
    }
    else if (erased_cmp != CCC_FPQ_EQL)
    {
        bubble_down(fpq, swap_location, ccc_buf_size(fpq->buf) - 1);
    }
    /* If the comparison is equal do nothing. Element is in right spot. */
    size_t const to_delete = erased->buf_key;
    if (to_delete == ccc_buf_size(fpq->buf) - 1)
    {
        ccc_buf_pop_back(fpq->buf);
        return erased;
    }
    ccc_fpq_elem *const prev_entry = at(fpq, to_delete);
    size_t const previously_stored_key = prev_entry->buf_key;
    assert(prev_entry->heap_index == ccc_buf_size(fpq->buf) - 1);
    uint8_t tmp[ccc_buf_elem_size(fpq->buf)];
    ccc_buf_result res
        = ccc_buf_swap(fpq->buf, tmp, to_delete, ccc_buf_size(fpq->buf) - 1);
    (void)res;
    assert(res == CCC_BUF_OK);
    prev_entry->buf_key = previously_stored_key;
    at(fpq, prev_entry->heap_index)->buf_key = to_delete;
    ccc_buf_pop_back(fpq->buf);
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
    size_t const i = e->heap_index;
    if (!i)
    {
        bubble_down(fpq, 0, ccc_buf_size(fpq->buf));
        return true;
    }
    ccc_fpq_threeway_cmp const parent_cmp
        = fpq->cmp(val(fpq, i), val(fpq, (i - 1) / 2), fpq->aux);
    if (parent_cmp == fpq->order)
    {
        bubble_up(fpq, i);
        return true;
    }
    if (parent_cmp != CCC_FPQ_EQL)
    {
        bubble_down(fpq, i, ccc_buf_size(fpq->buf));
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
    return val(fpq, 0);
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
        fn(val(fpq, i));
    }
    ccc_buf_pop_back_n(fpq->buf, ccc_buf_size(fpq->buf));
    fpq->cmp = NULL;
}

bool
ccc_fpq_validate(ccc_flat_pqueue const fpq[static const 1])
{
    size_t const sz = ccc_buf_size(fpq->buf);
    if (sz <= 1)
    {
        return true;
    }
    for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2;
         i <= (sz - 2) / 2; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
    {
        ccc_fpq_elem const *const cur = val(fpq, i);
        /* Putting the child in the comparison function first evaluates
           the childs three way comparison in relation to the parent. If
           the child beats the parent in total ordering (min/max) something
           has gone wrong. */
        if (left < sz && fpq->cmp(val(fpq, left), cur, fpq->aux) == fpq->order)
        {
            return false;
        }
        if (right < sz
            && fpq->cmp(val(fpq, right), cur, fpq->aux) == fpq->order)
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
bubble_up(ccc_flat_pqueue fpq[static const 1], size_t i)
{
    for (size_t parent = (i - 1) / 2;
         i && fpq->cmp(val(fpq, i), val(fpq, parent), fpq->aux) == fpq->order;
         i = parent, parent = (parent - 1) / 2)
    {
        swap(fpq, parent, i);
    }
}

static void
bubble_down(ccc_flat_pqueue fpq[static const 1], size_t i, size_t const sz)
{
    ccc_fpq_threeway_cmp const wrong_order
        = fpq->order == CCC_FPQ_LES ? CCC_FPQ_GRT : CCC_FPQ_LES;
    for (size_t next = i, left = i * 2 + 1, right = left + 1; left < sz;
         i = next, left = i * 2 + 1, right = left + 1)
    {
        /* Without knowing the cost of the user provided comparison function,
           it is important to call the cmp function minimal number of times.
           Avoid one call if there is no right child. */
        next = (right < sz
                && (fpq->order
                    == fpq->cmp(val(fpq, right), val(fpq, left), fpq->aux)))
                   ? right
                   : left;
        if (fpq->cmp(val(fpq, i), val(fpq, next), NULL) != wrong_order)
        {
            break;
        }
        swap(fpq, next, i);
    }
}

static inline void
swap(ccc_flat_pqueue fpq[static const 1], size_t const i, size_t const j)
{
    ccc_fpq_elem *ie = at(fpq, i);
    ccc_fpq_elem *je = at(fpq, j);
    size_t const tmp_buf_key = ie->buf_key;
    ie->buf_key = je->buf_key;
    je->buf_key = tmp_buf_key;
    at(fpq, ie->buf_key)->heap_index = i;
    at(fpq, je->buf_key)->heap_index = j;
}

static inline ccc_fpq_elem *
at(ccc_flat_pqueue const fpq[static const 1], size_t const i)
{
    void *loc = ccc_buf_at(fpq->buf, i);
    assert(loc);
    return (ccc_fpq_elem *)((uint8_t *)loc + fpq->fpq_elem_offset);
}

static ccc_fpq_elem *
val(ccc_flat_pqueue const fpq[static const 1], size_t const i)
{
    return at(fpq, at(fpq, i)->buf_key);
}

static inline size_t
index_of(ccc_flat_pqueue const fpq[static const 1],
         ccc_fpq_elem const e[static const 1])
{
    return (((uint8_t *)e - fpq->fpq_elem_offset)
            - ((uint8_t *)ccc_buf_base(fpq->buf)))
           / ccc_buf_elem_size(fpq->buf);
}

/* NOLINTBEGIN(*misc-no-recursion) */

static void
print_node(ccc_flat_pqueue const fpq[static const 1], size_t i,
           fpq_print_fn *const fn)
{
    printf(COLOR_CYN);
    if (i)
    {
        i == ((i - 1) / 2) * 2 + 1 ? printf("L%zu:", i) : printf("R%zu:", i);
    }
    printf(COLOR_NIL);
    fn(val(fpq, i));
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
