/*
   Author: Alexander G. Lopez
   --------------------------
   This is the Priority Queue interface for the Splay Tree
   Set. In this case we modify a Splay Tree to allow for
   a Priority Queue (aka a sorted Multi-Set). See the
   normal set interface as well.
 */
#ifndef PQUEUE
#define PQUEUE

#include "tree.h"
#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

/* =============================================================
   ====================  PRIORITY QUEUE ========================
   =============================================================

   Together the following form what you would normally expect for
   an embedded data structure. In this case a priority queue.

      pqueue
      {
         pq_elem *root
         pq_elem nil;
      };

   Embed a pq_elem in your struct:

      struct val
      {
         int val;
         pq_elem elem;
      };

   If interested how these elems are implemented see tree.h

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
   However, any other removals or insertions of different values
   reduce performance back to O(lgN) for the first access.

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
   but I prefer splay trees for all they achieve with far less code.

   Internally, the representation to acheive this is a simple
   tree with a circular doubly linked list attached.

                              *
                            /   \
                           *     *---------
                          / \     \       |
                         *   *     *     -*-*-*-*-*
                                         |_________|

   =============================================================
   =============================================================
   =============================================================
*/

/* An element stored in a priority queue with Round Robin
   fairness if a duplicate. */
typedef struct node pq_elem;

/* A priority queue that offers all of the expected operations
   of a priority queue with the additional benefits of an
   iterator and removal by node id if you remember your
   values that are present in the queue. */
typedef struct tree pqueue;

/*
   =============================================================
   ===================   Comparisons  ==========================
   =============================================================
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
      val_cmp(const pq_elem *a, const pq_elem *b, void *aux)
      {
        (void)aux;
        struct val *lhs = pq_entry(a, struct val, elem);
        struct val *rhs = pq_entry(b, struct val, elem);
        return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }
*/

/* A comparison function that returns one of the threeway comparison
   values. To use this data structure you must be able to determine
   these three comparison values for two of your type. See example
   above.
      typedef enum
      {
         LES = -1,
         EQL = 0,
         GRT = 1
      } threeway_cmp;
*/
typedef tree_cmp_fn pq_cmp_fn;

/* Update priorities with a function that modifies the field
   you are using to store priorities and compare them with
   the pq_cmp_fn. For example:

      struct val
      {
        int val;
        pq_elem elem;
      };

      static pqueue pq;

      static void
      priority_update(const pq_elem *a, void *new_val)
      {
          if (new_val == NULL)
          {
              ...handle errors...
          }
          struct val *old = pq_entry(a, struct val, elem);
          old->val = *((int *)new_val);
      }

      int
      main()
      {
         pq_init(&pq);
         ...Insert various values...
         struct val *my_val = retreived_val();
         const int new_priority = generate_priority(my_val);
         pq_update(&pq, &my_val->elem, &my_cmp_fn,
                   &priority_update, &new_priority);
      }

   The above should be well defined because the user determines
   how to cast the auxiliary data themselves based on the types
   they are implementing, but care should be taking when casting
   from void.
*/
typedef void pq_update_fn(pq_elem *, void *aux);

/* NOLINTNEXTLINE */
#define pq_entry(TREE_ELEM, STRUCT, MEMBER)                                    \
    ((STRUCT *)((uint8_t *)&(TREE_ELEM)->parent_or_dups                        \
                - offsetof(STRUCT, MEMBER.parent_or_dups))) /* NOLINT */

/* Initializes and empty queue with size 0. */
void pq_init(pqueue *);

/* Checks if the priority queue is empty. Undefined if
   pq_init has not been called first. */
bool pq_empty(const pqueue *);
/* O(1) */
size_t pq_size(pqueue *);

/* Inserts the given pq_elem into an initialized pqueue
   any data in the pq_elem member will be overwritten
   The pq_elem must not already be in the queue or the
   behavior is undefined. Priority queue insertion
   shall not fail becuase priority queues support
   round robin duplicates. O(lgN) */
void pq_insert(pqueue *, pq_elem *, pq_cmp_fn *, void *);

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
   of the queue of duplicates. Returns the end element if
   the queue is empty. */
pq_elem *pq_pop_max(pqueue *);
/* Same promises as pop_max except for the minimum values. */
pq_elem *pq_pop_min(pqueue *);

/* Read only peek at the max and min these operations do
   not modify the tree so multiple threads could call them
   at the same time. However, all other operations are
   most definitely not safe in a splay tree for concurrency.
   Worst case O(lgN). If you have just removed an element
   and it has duplicates those duplicates will remain at
   the root O(1) until another insertion, query, or pop
   occurs. */
pq_elem *pq_max(const pqueue *);
pq_elem *pq_min(const pqueue *);

/* Erases a specified element known to be in the queue.
   Returns the element that follows the previous value
   of the element in round robin sorted order.
   This may be another element or it may be the end
   element in which case no values are greater than
   the last. O(lgN). However, in practice you can often
   benefit from O(1) access if that element is a duplicate
   or you are repeatedly erasing duplicates while iterating. */
pq_elem *pq_erase(pqueue *, pq_elem *, pq_cmp_fn *, void *);

/* Updates the specified elem known to be in the queue with
   a new priority in O(lgN) time. Because an update does not
   remove elements from a queue care should be taken to avoid
   infinite loops. For example, increasing priorities during
   an inorder traversal should be done with care because that
   element will be encountered again later in the queue with one
   exception This function returns the element that followed
   this one at its original value in ascending order. However, the
   newly updated value will be skipped if it is placed directly
   in front of itself with no duplicates of that same updated value. */
pq_elem *pq_update(pqueue *, pq_elem *, pq_cmp_fn *, pq_update_fn *, void *);

/* Returns true if this priority value is in the queue.
   you need not search with any specific struct you have
   previously created. For example using a global static
   or local dummy struct can be sufficent for this check:

      struct priority
      {
         int priority;
         pq_elem elem;
      };

      static pqueue pq;

      bool
      has_priority(int priority)
      {
         struct priority key = { .priority = priority };
         return pq_contains(&pq, my_cmp, NULL);
      }

      int
      main()
      {
         pq_init(&pq);
         ...
      }

   This can be helpful if you need to know if such a priority
   is present regardless of how many round robin duplicates
   are present. Returns the result in O(lgN). */
bool pq_contains(pqueue *, pq_elem *, pq_cmp_fn *, void *);

/*
   =============================================================
   ===================    Iteration   ==========================
   =============================================================

   Priority queue  iterators are stable and support updates and
   deltion while iterating. For example:

   struct val
   {
      int val;
      pq_elem elem;
   };

   static pqueue q;

   int
   main ()
   {
      pq_init(&q);

      ...Fill the container with program logic...

      for (pq_elem *i = pq_begin(&q); i != pq_end(&q, i); i = pq_next(&q, i))
      {
         struct val *cur = pq_entry(pq_from_iter(&i), struct val, elem);
         printf("%d", cur->val);
      }
   }

   This is traversal in order of descending priority oby default with
   duplicates included in round robin order.
*/

/* Returns a struct by copy to aid in iteration through the priority
   queue. It is simply two pointers and a byte so it is not very
   expensive and is only allocated on the stack once. Use the
   following methods to progress through the priority queue. */
pq_elem *pq_begin(pqueue *);
pq_elem *pq_rbegin(pqueue *);

/* Progresses through the queue in order of highest priority by
   default. Use the reverse order iterator if you prefer ascending
   order. If you plan to modify while iterating, see if the erase
   and update functions are what you are looking for. Both iterators
   visit duplicates in round robin order meaning oldest first so
   that priorities can be organized round robin either ascending
   or descending and visitation is fair. */
pq_elem *pq_next(pqueue *, pq_elem *);
pq_elem *pq_rnext(pqueue *, pq_elem *);

/* The end is not a valid position in the queue so it does not make
   sense to try to use any fields in the iterator once the end
   is reached. The end is same for any iteration order. */
pq_elem *pq_end(pqueue *);

/* Not very useful or significant. Helps with tests. Explore at own risk. */
pq_elem *pq_root(const pqueue *);
bool pq_has_dups(pqueue *, pq_elem *);

#endif
