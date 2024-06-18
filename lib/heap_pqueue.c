#include "heap_pqueue.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

enum direction
{
    HPQ_L = 0,
    HPQ_R,
};

static const size_t starting_capacity = 8;

static void grow(struct heap_pqueue *);
static void swap(struct hpq_elem **, struct hpq_elem **);
static void bubble_down(struct heap_pqueue *, struct hpq_elem *, size_t);
static void bubble_up(struct heap_pqueue *, struct hpq_elem *, size_t);

void
hpq_init(struct heap_pqueue *const hpq, enum heap_pq_threeway_cmp hpq_ordering,
         hpq_cmp_fn *cmp, void *aux)
{
    if (hpq_ordering == HPQ_EQL)
    {
        (void)fprintf(stderr, "heap should be ordered hPQ_LES or hPQ_GRT.\n");
    }
    hpq->ordering = hpq_ordering;
    hpq->sz = 0;
    hpq->capacity = starting_capacity;
    hpq->heap = calloc(starting_capacity,
                       starting_capacity * sizeof(struct hpq_elem *));
    hpq->cmp = cmp;
    hpq->aux = aux;
}

void
hpq_push(struct heap_pqueue *const hpq, struct hpq_elem *e)
{
    if (hpq->sz == hpq->capacity)
    {
        grow(hpq);
    }
    hpq->heap[hpq->sz] = e;
    ++hpq->sz;
    bubble_up(hpq, e, hpq->sz);
}

struct hpq_elem *
hpq_pop(struct heap_pqueue *hpq)
{
    if (!hpq->sz)
    {
        return NULL;
    }
    --hpq->sz;
    swap(&hpq->heap[0], &hpq->heap[hpq->sz]);
    struct hpq_elem *ret = hpq->heap[hpq->sz];
    bubble_down(hpq, hpq->heap[0], 0);
    return ret;
}

struct hpq_elem *
hpq_erase(struct heap_pqueue *const hpq, struct hpq_elem *e)
{
    if (!hpq->sz)
    {
        return NULL;
    }
    if (hpq->sz == 1 || e->handle == hpq->sz)
    {
        hpq->sz--;
        return hpq->heap[0];
    }
    --hpq->sz;
    swap(&hpq->heap[e->handle], &hpq->heap[hpq->sz]);
    struct hpq_elem *ret = hpq->heap[hpq->sz];
    const enum heap_pq_threeway_cmp swapped_cmp
        = hpq->cmp(hpq->heap[e->handle], hpq->heap[hpq->sz], hpq->aux);
    if (swapped_cmp == hpq->ordering)
    {
        bubble_up(hpq, hpq->heap[e->handle], e->handle);
    }
    else
    {
        bubble_down(hpq, hpq->heap[e->handle], e->handle);
    }
    return ret;
}

bool
hpq_update(struct heap_pqueue *hpq, struct hpq_elem *e, hpq_update_fn *fn,
           void *aux)
{
    if (!e || !hpq->sz)
    {
        return false;
    }
    hpq_erase(hpq, e);
    fn(e, aux);
    hpq_push(hpq, e);
    return true;
}

const struct hpq_elem *
hpq_front(const struct heap_pqueue *const hpq)
{
    if (!hpq->sz)
    {
        return NULL;
    }
    return hpq->heap[0];
}

bool
hpq_empty(const struct heap_pqueue *const hpq)
{
    if (!hpq)
    {
        return true;
    }
    return !hpq->sz;
}

size_t
hpq_size(const struct heap_pqueue *const hpq)
{
    if (!hpq)
    {
        return 0ULL;
    }
    return hpq->sz;
}

enum heap_pq_threeway_cmp
hpq_order(const struct heap_pqueue *const hpq)
{
    return hpq->ordering;
}

void
hpq_clear(struct heap_pqueue *hpq, hpq_destructor_fn *fn)
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
hpq_validate(const struct heap_pqueue *const hpq)
{
    for (size_t i = 0, left = (i * 2) + 1, right = (i * 2) + 2;
         i <= (hpq->sz - 2) / 2; ++i, left = (i * 2) + 1, right = (i * 2) + 2)
    {
        const struct hpq_elem *const cur = hpq->heap[i];
        if (left < hpq->sz
            && hpq->cmp(cur, hpq->heap[left], hpq->aux) == hpq->ordering)
        {
            return false;
        }
        if (right < hpq->sz
            && hpq->cmp(cur, hpq->heap[right], hpq->aux) == hpq->ordering)
        {
            return false;
        }
    }
    return true;
}

/*===============================  Static Helpers  =========================*/

static void
bubble_up(struct heap_pqueue *const hpq, struct hpq_elem *const e, size_t i)
{
    /* If the this is a min heap we stop when the element is less than OR EQUAL
       to the parent. If it is a max heap we stop when the element is greater
       than OR EQUAL to the parent. Therefore, we only run the loop while the
       opposite of our desired ordering occurs, because equal is an acceptable
       stop condition. */
    for (size_t j = (i - 1) / 2;
         i && hpq->cmp(e, hpq->heap[j], hpq->aux) == hpq->ordering;
         i = j, j = (j - 1) / 2)
    {
        swap(&hpq->heap[j], &hpq->heap[i - 1]);
    }
    e->handle = i ? i - 1 : 0;
}

static void
bubble_down(struct heap_pqueue *hpq, struct hpq_elem *e, size_t i)
{
    const size_t sz = hpq->sz;
    size_t dir[2];
    for (size_t next = i; i * 2 + 1 < sz; i = next)
    {
        dir[HPQ_L] = (i * 2) + 1;
        dir[HPQ_R] = (i * 2) + 2;
        next = dir[hpq->ordering
                   == hpq->cmp(hpq->heap[dir[dir[HPQ_R] < sz]],
                               hpq->heap[dir[HPQ_L]], hpq->aux)];
        if (hpq->cmp(hpq->heap[i], hpq->heap[next], NULL) == hpq->ordering)
        {
            break;
        }
        swap(&hpq->heap[next], &hpq->heap[i]);
    }
    e->handle = i ? i - 1 : 0;
}

static void
grow(struct heap_pqueue *hpq)
{
    struct hpq_elem **new
        = realloc(hpq->heap, sizeof(struct hpq_elem *) * hpq->capacity * 2);
    if (!new)
    {
        (void)fprintf(stderr, "reallocation of flat priority queue failed.\n");
    }
    hpq->heap = new;
    hpq->capacity *= 2;
}

static inline void
swap(struct hpq_elem **a, struct hpq_elem **b)
{
    struct hpq_elem *temp = *a;
    temp = *a;
    *a = *b;
    *b = temp;
}
