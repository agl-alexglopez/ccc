#ifndef PQUEUE
#define PQUEUE

#include "tree.h"
#include <stdbool.h>
#include <stddef.h>

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

/* To implement three way comparison in C you can try something
   like this:

     return (a > b) - (a < b);

   If such a comparison is not possible for your type you can simply
   return the value of the cmp enum directly with conditionals switch
   statements or whatever other comparison logic you choose. */
typedef tree_cmp_fn pq_cmp_fn;

void pq_init (pqueue *);

bool pq_empty (const pqueue *);
void pq_insert (pqueue *, pq_elem *, pq_cmp_fn *fn, void *aux);
pq_elem *pq_root (const pqueue *);

/* O(n) time O(1) space iterative traversal count of nodes. */
size_t pq_size (pqueue *);

#endif
