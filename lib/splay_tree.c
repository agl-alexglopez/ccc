#include "pqueue.h"
#include "tree.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

static void init (struct tree *t);
static bool empty (struct tree *t);

void
pq_init (pqueue *pq)
{
  init (pq);
}

bool
pq_empty (pqueue *pq)
{
  return empty (pq);
}

static void
init (struct tree *t)
{
  t->root = NULL;
  (void)t;
}

static bool
empty (struct tree *t)
{
  assert (t != NULL);
  return NULL == t->root;
}
