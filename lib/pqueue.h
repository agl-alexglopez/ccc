/*
   Author: Alexander G. Lopez
   --------------------------
   This is the Priority Queue interface for the Splay Tree
   Set. In this case we modify a Splay Tree to allow for
   a Priority Queue (aka a sorted Multi-Set). I intend to
   add a normal Set interface as well.

   Citations:
   ---------------------------
   1. This is taken from my own work and research on heap
      allocator performance through various implementations.

      https://github.com/agl-alexglopez/heap-allocator-workshop
      /blob/main/lib/splaytree_topdown.c

   2. However, I based it off of work by Daniel Sleator, Carnegie Mellon
      University. Sleator's implementation of a topdown splay tree was
      instrumental in starting things off, but required extensive modification.
      I had to add the ability to track duplicates, update parent and child
      tracking, and unite the left and right cases for fun. See the .c file
      for my generalizable strategy to eliminate symmetric left and right cases
      for any binary tree code which I have been working on for a while and
      don't see very often!

      https://www.link.cs.cmu.edu/link/ftp-site/splaying/top-down-splay.c
 */
#ifndef PQUEUE
#define PQUEUE

#include "tree.h"
#include <stdbool.h>
#include <stddef.h>

/* ====================  PRIORITY QUEUE ========================

   Together the following form what you would normally expect for
   an embedded data structure. In this case a priority queue.

   pqueue
   {
      pq_elem *root
      pq_elem nil;
   };

   I have the additional space taken by the nil to afford some
   nice conveniences in implementation. You could technically
   get rid of it but that would make things harder and add
   code bloat. It is a choice worth considering, however.

   Becuase there are a few different helpful data structures
   we can make from our underlying data structure we use typedef
   to expose the expected interface to the user rather than force
   them to remember how to use tree functionality to make a
   priority queue in this case.

   A Priority Queue can be used to maintain a max or min. If
   you access the min or max for removal any future access
   to duplicates of that priority are guaranteed to be O(1).
   This may be an important consideration for Priority Queues.
   However, any other removals or insertions reduce performance
   back to O(lgN) for the first access.

   This Priority Queue also guarantees Round-Robin Fairness
   among duplicate priorities. However, if you remove a node
   to change it's priority to the same value it already was
   it will go to the back of the round-robin queue.

   Otherwise, the performance defaults to O(lgN). Technically,
   I can only promise amortized O(lgN) but the implementation
   tends to perform well in most cases. If you need an absolute
   guarantee that performance shall be bounded at O(lgN) for
   real-time use cases, prefer a Red-Black Tree based structure.
   This is built using a Splay Tree. I can add a Red-Black tree
   if we see the need but this is way more boring.

   Internally, the representation to acheive this is a simple
   tree with a circular doubly linked list attached.

                              *
                            /   \
                           *     *---------
                          / \     \       |
                         *   *     *     -*-*-*-*-*
                                         |_________|

   =============================================================
*/
typedef struct node pq_elem;
typedef struct tree pqueue;

/*
   To implement three way comparison in C you can try something
   like this:

     return (a > b) - (a < b);

   If such a comparison is not possible for your type you can simply
   return the value of the cmp enum directly with conditionals switch
   statements or whatever other comparison logic you choose.
   The user is responsible for returning one of the three possible
   comparison results for the threeway_cmp enum.

      typedef enum
      {
         LES = -1,
         EQL = 0,
         GRT = 1
      } threeway_cmp;

    This is modeled after the <=> operator in C++ but it is FAR less
    robust and fancy. In fact it's just a fancy named wrapper around
    what you are used to providing for a function like qsort in C.

    Example:

      struct val
      {
        int val;
        pq_elem elem;
      };

      static threeway_cmp
      val_cmp (const pq_elem *a, const pq_elem *b, void *aux)
      {
        (void)aux;
        struct val *lhs = tree_entry (a, struct val, elem);
        struct val *rhs = tree_entry (b, struct val, elem);
        return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }
*/
typedef tree_cmp_fn pq_cmp_fn;

void pq_init (pqueue *);

/* Checks if the priority queue is empty. Undefined if
   pq_init has not been called first. */
bool pq_empty (const pqueue *);

/* Inserts the given qp_elem into an initialized pqueue
   any data in the pq_elem member will be overwritten
   The pq_elem must not already be in the queue or the
   behavior is undefined. O(lgN) */
void pq_insert (pqueue *, pq_elem *, pq_cmp_fn *, void *);

/* Erases a specified element known to be in the queue.
   The behavior is undefined if the element is not in
   the queue. O(lgN). */
pq_elem *pq_erase (pqueue *, pq_elem *, pq_cmp_fn *, void *);

/* Pops from the front of the queue. If multiple elements
   with the same priority are to be popped, then upon first
   pop we guarantee O(lgN) runtime and then all subsequent
   pops will be O(1). However, if any other insertions or
   deletions other than the max occur before all duplicates
   have been popped then performance degrades back to O(lgN).
   Given equivalent priorities this priority queue promises
   round robin scheduling. Importantly, if a priority is reset
   to its same value after having removed the element from
   the tree it is considered new and returns to the back
   of the queue of duplicates. */
pq_elem *pq_pop_max (pqueue *);
/* Same promises as pop_max except for the minimum values. */
pq_elem *pq_pop_min (pqueue *);

/* Read only peek at the max and min these operations do
   not modify the tree so multiple threads could call them
   at the same time. However, all other operations are
   most definitely not safe in a splay tree for concurrency.
   Worst case O(lgN). If you have just */
pq_elem *pq_max (const pqueue *);
pq_elem *pq_min (const pqueue *);

/* O(n) time O(1) space iterative traversal count of nodes. */
size_t pq_size (pqueue *);
/* Not very useful or significant. Helps with tests. */
pq_elem *pq_root (const pqueue *);

#endif
