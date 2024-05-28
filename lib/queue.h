#ifndef QUEUE
#define QUEUE

#include <stdbool.h>
#include <stddef.h>

struct queue
{
    void *mem [[gnu::deprecated("private")]];
    size_t elem_sz [[gnu::deprecated("private")]];
    size_t front [[gnu::deprecated("private")]];
    size_t back [[gnu::deprecated("private")]];
    size_t sz [[gnu::deprecated("private")]];
    size_t capacity [[gnu::deprecated("private")]];
};

void q_init(size_t elem_sz, struct queue *, size_t capacity);
void q_push(struct queue *, void *elem);
void q_pop(struct queue *);
void *q_front(const struct queue *);
bool q_empty(const struct queue *);
size_t q_size(const struct queue *);
void q_free(struct queue *);

#endif
