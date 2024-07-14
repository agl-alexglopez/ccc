#ifndef QUEUE
#define QUEUE

#include "attrib.h"

#include <stdbool.h>
#include <stddef.h>

struct queue
{
    void *mem ATTRIB_PRIVATE;
    size_t elem_sz ATTRIB_PRIVATE;
    size_t front ATTRIB_PRIVATE;
    size_t back ATTRIB_PRIVATE;
    size_t sz ATTRIB_PRIVATE;
    size_t capacity ATTRIB_PRIVATE;
};

void q_init(size_t elem_sz, struct queue *, size_t capacity);
void q_push(struct queue *, void *elem);
void q_pop(struct queue *);
void *q_front(struct queue const *);
bool q_empty(struct queue const *);
size_t q_size(struct queue const *);
void q_free(struct queue *);

#endif
