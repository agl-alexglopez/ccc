#include "queue.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void q_grow(struct queue *);

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
    memcpy((uint8_t *)q->mem + (q->back * q->elem_sz), elem, q->elem_sz);
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
    return (uint8_t *)q->mem + (q->front * q->elem_sz);
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
    const size_t back_front_diff = q->capacity - q->front;
    const size_t first_chunk = MIN(q->sz, back_front_diff);
    memcpy(new, (uint8_t *)q->mem + (q->front * q->elem_sz),
           q->elem_sz * first_chunk);
    if (first_chunk < q->sz)
    {
        memcpy((uint8_t *)new + (first_chunk * q->elem_sz), q->mem,
               q->elem_sz * (q->sz - first_chunk));
    }
    q->capacity *= 2;
    q->front = 0;
    q->back = q->sz;
    free(q->mem);
    q->mem = new;
}

#pragma GCC diagnostic pop
