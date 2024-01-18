#ifndef PQUEUE
#define PQUEUE

#include "tree.h"
#include <stdbool.h>

/* Together the following form what you would normally expect for
   an embedded data structure. In this case a priority queue.

   pqueue
   {
    pq_elem *root
   };

   Becuase there are a few different helpful data structures
   we can make from our underlying data structure we use typedef
   to expose the expected interface to the user rather than force
   them to remember how to use tree functionality to make a
   priority queue in this case.
*/
typedef struct node pq_elem;
typedef struct tree pqueue;

void pq_init (pqueue *);

bool pq_empty (pqueue *);

#endif
