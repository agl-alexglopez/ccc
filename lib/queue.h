#ifndef QUEUE
#define QUEUE

#include <stdbool.h>
#include <stddef.h>

struct queue {
  void *mem;
  size_t elem_sz;
  size_t front;
  size_t back;
  size_t sz;
  size_t capacity;
};

void q_init(size_t elem_sz, struct queue *, size_t capacity);
void q_push(struct queue *, void *elem);
void q_pop(struct queue *);
void *q_front(struct queue *);
bool q_empty(struct queue *);
void q_free(struct queue *);

#endif
