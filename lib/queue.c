#include "queue.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void q_grow(struct queue *);
static void *q_at(const struct queue *, size_t);
static size_t q_bytes(const struct queue *, size_t);

/* We don't need the deprecated warnings as implementer of the queue. The
   warning is for users to stop them and force them to use provided
   functions. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void
q_init(const size_t elem_sz, struct queue *const q, const size_t capacity)
{
    if (!q)
    {
        return;
    }
    q->capacity = capacity;
    q->elem_sz = elem_sz;
    q->sz = q->front = q->back = 0;
    q->mem = malloc(elem_sz * capacity);
    if (!q->mem)
    {
        (void)fprintf(stderr, "Heap exhausted.\n");
    }
}

void
q_free(struct queue *const q)
{
    if (!q)
    {
        return;
    }
    free(q->mem);
    *q = (struct queue){0};
}

void
q_push(struct queue *const q, void *const elem)
{
    if (!q)
    {
        return;
    }
    if (q->sz == q->capacity)
    {
        q_grow(q);
    }
    memcpy(q_at(q, q->back), elem, q->elem_sz);
    q->back = (q->back + 1) % q->capacity;
    q->sz++;
}

void
q_pop(struct queue *const q)
{
    if (!q->sz)
    {
        (void)fprintf(stderr, "Cannot pop from empty queue.\n");
        return;
    }
    q->front = (q->front + 1) % q->capacity;
    q->sz--;
}

void *
q_front(const struct queue *const q)
{
    if (!q->sz)
    {
        return NULL;
    }
    return q_at(q, q->front);
}

bool
q_empty(const struct queue *const q)
{
    return !q || !q->sz;
}

size_t
q_size(const struct queue *const q)
{
    return !q ? 0ULL : q->sz;
}

static void
q_grow(struct queue *const q)
{
    void *const new = malloc(q->elem_sz * (q->capacity * 2));
    if (!new)
    {
        (void)fprintf(stderr, "reallocation failed.\n");
        return;
    }
    const size_t first_chunk = MIN(q->sz, q->capacity - q->front);
    memcpy(new, q_at(q, q->front), q_bytes(q, first_chunk));
    if (first_chunk < q->sz)
    {
        memcpy((uint8_t *)new + q_bytes(q, first_chunk), q->mem,
               q_bytes(q, q->sz - first_chunk));
    }
    q->capacity *= 2;
    q->front = 0;
    q->back = q->sz;
    free(q->mem);
    q->mem = new;
}

static inline void *
q_at(const struct queue *const q, const size_t i)
{
    return (uint8_t *)q->mem + (q->elem_sz * i);
}

static inline size_t
q_bytes(const struct queue *q, size_t n)
{
    return q->elem_sz * n;
}

#pragma GCC diagnostic pop
