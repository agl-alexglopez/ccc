#include "queue.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void q_grow(struct queue *);

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
q_front(struct queue *const q)
{
    if (!q->sz)
    {
        return NULL;
    }
    return (uint8_t *)q->mem + (q->front * q->elem_sz);
}

bool
q_empty(struct queue *const q)
{
    return !q || !q->sz;
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
    for (size_t i = 0; i < q->sz; i++)
    {
        memcpy((uint8_t *)new + (i * q->elem_sz),
               (uint8_t *)q->mem + (q->front * q->elem_sz), q->elem_sz);
        q->front = (q->front + 1) % q->capacity;
    }
    q->capacity *= 2;
    q->front = 0;
    q->back = q->sz;
    free(q->mem);
    q->mem = new;
}
