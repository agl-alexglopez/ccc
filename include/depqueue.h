/* Author: Alexander G. Lopez
   --------------------------
   This is the Double Ended Priority Queue interface implemented via Splay
   Tree. In this case we modify a Splay Tree to allow for
   a Double Ended Priority Queue (aka a sorted Multi-Set). See the
   normal set interface as well. While a Red-Black Tree
   would be the more optimal data structure to support
   a DEPQ the underlying implementation of a Splay Tree
   offers some interesting tradeoffs for systems programmers.
   They are working sets that keep frequently (Least
   Recently Used) elements close to the root even if their
   runtime is amortized O(lgN). With the right use cases we
   can frequently benefit from O(1) operations. */
#ifndef DEPQUEUE
#define DEPQUEUE

#include "attrib.h"
#include "tree.h"

#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

/* An element stored in a DEPQ with Round Robin
   fairness if a duplicate. */
typedef struct ccc_depq_elem
{
    ccc_node n;
} ccc_depq_elem;

/* A DEPQ that offers all of the expected operations
   of a DEPQ with the additional benefits of an
   iterator and removal by node id if you remember your
   values that are present in the DEPQ. */
typedef struct ccc_depqueue
{
    ccc_tree t;
} ccc_depqueue;

typedef enum ccc_depq_threeway_cmp
{
    DPQLES = NODE_LES,
    DPQEQL = NODE_EQL,
    DPQGRT = NODE_GRT,
} ccc_depq_threeway_cmp;

/* A comparison function that returns one of the threeway comparison
   values. To use this data structure you must be able to determine
   these three comparison values for two of your type. See example
   above.
      typedef enum
      {
          NODE_LES = -1,
          NODE_EQL = 0,
          NODE_GRT = 1
      } node_threeway_cmp;

   The compare function one must provide to perform queries
   and other operations on the DEPQ. See above. */
typedef ccc_depq_threeway_cmp
ccc_depq_cmp_fn(ccc_depq_elem const *a, ccc_depq_elem const *b, void *aux);

/* Define a function to use printf for your custom struct type.
   For example:
      struct val
      {
         int val;
         ccc_depq_elem elem;
      };

      void print_my_val(ccc_depq_elem *elem)
      {
         struct val const *v = depq_entry(elem, struct val, elem);
         printf("{%d}", v->val);
      }

   Output should be one line with no newline character. Then,
   the printer function will take care of the rest. */
typedef void ccc_depq_print_fn(ccc_depq_elem const *);

/* Provide a new auxilliary value corresponding to the value type used
   for depq comparisons. The old value will be changed to new and
   the element will be reinserted in round robin order to the DEPQ
   even if it is updated to the same value it previously stored O(lgN). */
typedef void ccc_depq_update_fn(ccc_depq_elem *, void *aux);

/* Performs user specified destructor actions on a single depq_elem. This
   depq_elem is assumed to be embedded in user defined structs and therefore
   allows the user to perform any updates to their program before deleting
   this element. The user will know if they have heap allocated their
   own structures and therefore shall call free on the containing structure.
   If the data structure is stack allocated, free is not necessary but other
   actions may be specified by this function. */
typedef void ccc_depq_destructor_fn(ccc_depq_elem *);

/* A container for a simple begin and end pointer to a ccc_depq_elem.
   A user can use the equal_range or equal_rrange function to
   fill the depq_range with the expected begin and end queries.
   The default range in a DEPQ is descending order.
   A depq_range has no sense of iterator directionality and
   provides two typedefs simpy as a reminder to the programmer
   to use the appropriate next function. Use next for a
   depq_range and rnext for a depq_rrange. Otherwise, indefinite
   loops may occur. */
typedef struct ccc_depq_range
{
    ccc_range r ATTRIB_PRIVATE;
} ccc_depq_range;

/* The reverse range container for queries performed with requal_range.
   Be sure to use the rnext function to progress the iterator
   in this type of range. */
typedef struct ccc_depq_rrange
{
    ccc_rrange r ATTRIB_PRIVATE;
} ccc_depq_rrange;

/* How to obtain the struct that embeds the ccc_depq_elem. For example:

      struct val
      {
          int val;
          ccc_depq_elem elem;
      };

      for (ccc_depq_elem *e = depq_begin(pq);
           e != depq_end(pq); e = depq_next(pq, e))
      {
          struct val *my_val = depq_entry(e, struct val, elem);
          printf("%d\n", my_val->val);
      }

   The pq element should be passed by address not by value and the
   struct and member macros represent the type used and the member
   in the struct of the pq element. NOLINTNEXTLINE */
#define CCC_DEPQ_OF(DEPQ_ELEM, STRUCT, MEMBER)                                 \
    ((STRUCT *)((uint8_t *)&(DEPQ_ELEM)->n                                     \
                - offsetof(STRUCT, MEMBER.n))) /* NOLINT */

/* Initialize the depq on the left hand side with this right hand side
   initializer. Pass the left hand side depq by name to this macro along
   with the comparison function and any necessary auxilliary data. This may
   be used at compile time or runtime. It is undefined to use the depq if
   this has not been called. */
#define CCC_DEPQ_INIT(DEPQ_NAME, CMP, AUX)                                     \
    {                                                                          \
        .t = CCC_TREE_INIT(DEPQ_NAME, CMP, AUX)                                \
    }

/* Calls the destructor for each element while emptying the DEPQ.
   Usually, this destructor function is expected to call free for each
   struct for which a ccc_depq_elem is embedded if they are heap allocated.
   However, for stack allocated structures this is not required.
   Other updates before destruction are possible to include in destructor
   if determined necessary by the user. A DEPQ has no hidden
   allocations and therefore the only heap memory is controlled by the user. */
void ccc_depq_clear(ccc_depqueue *, ccc_depq_destructor_fn *destructor);

/* Checks if the DEPQ is empty. Undefined if
   depq_init has not been called first. */
bool ccc_depq_empty(ccc_depqueue const *);

/* O(1) */
size_t ccc_depq_size(ccc_depqueue *);

/* Inserts the given ccc_depq_elem into an initialized ccc_depqueue
   any data in the ccc_depq_elem member will be overwritten
   The ccc_depq_elem must not already be in the DEPQ or the
   behavior is undefined. DEPQ insertion shall not fail becuase DEPQs
   support round robin duplicates. O(lgN) */
void ccc_depq_push(ccc_depqueue *, ccc_depq_elem *);

/* Pops from the front of the DEPQ. If multiple elements
   with the same priority are to be popped, then upon first
   pop we have amortized O(lgN) runtime and then all subsequent
   pops will be O(1). However, if any other insertions or
   deletions other than the max occur before all duplicates
   have been popped then performance degrades back to O(lgN).
   Given equivalent priorities this DEPQ promises
   round robin scheduling. Importantly, if a priority is reset
   to its same value after having removed the element from
   the tree it is considered new and returns to the back
   of the DEPQ of duplicates. Returns the end element if
   the DEPQ is empty. */
ccc_depq_elem *ccc_depq_pop_max(ccc_depqueue *);
/* Same promises as pop_max except for the minimum values. */
ccc_depq_elem *ccc_depq_pop_min(ccc_depqueue *);

/* Reports the maximum priority element in the DEPQ, drawing
   it to the root via splay operations. This, is a good
   function to use if the user wishes to bring frequently
   queried max elements to the root for O(1) popping in
   subsequent calls. This can be especially beneficial if
   multiple elements are tied for the max in round robin
   as all duplicates will be popped in O(1) time. */
ccc_depq_elem *ccc_depq_max(ccc_depqueue *);
/* Same promises as the max except for the minimum ccc_depq_elem */
ccc_depq_elem *ccc_depq_min(ccc_depqueue *);

/* If elem is already max this check is O(lgN) as the worst
   case. If not, O(1). However, if multiple  pops have occured
   the max will be close to the root. */
bool ccc_depq_is_max(ccc_depqueue *, ccc_depq_elem *);

/* If the element is already min this check is O(lgN) as the worst
   case. If not, O(1). However, if multiple  pops have occured
   the max will be close to the root. */
bool ccc_depq_is_min(ccc_depqueue *, ccc_depq_elem *);

/* Read only peek at the max and min these operations do
   not modify the tree so multiple threads could call them
   at the same time. However, all other operations are
   most definitely not safe in a splay tree for concurrency.
   Worst case O(lgN). If you have just removed an element
   and it has duplicates those duplicates will remain at
   the root O(1) until another insertion, query, or pop
   occurs. */
ccc_depq_elem const *ccc_depq_const_max(ccc_depqueue const *);
/* Read only peek at the min. Does not alter tree and thus
   is thread safe. */
ccc_depq_elem const *ccc_depq_const_min(ccc_depqueue const *);

/* Erases a specified element known to be in the DEPQ.
   Returns the element that follows the previous value
   of the element in round robin sorted order (lower priority).
   This may be another element or it may be the end
   element in which case no values are less than
   the erased. O(lgN). However, in practice you can often
   benefit from O(1) access if that element is a duplicate
   or you are repeatedly erasing duplicates while iterating. */
ccc_depq_elem *ccc_depq_erase(ccc_depqueue *, ccc_depq_elem *);

/* The same as erase but returns the next element in an
   ascending priority order. */
ccc_depq_elem *ccc_depq_rerase(ccc_depqueue *, ccc_depq_elem *);

/* Updates the specified elem known to be in the DEPQ with
   a new priority in O(lgN) time. Because an update does not
   remove elements from a DEPQ care should be taken to avoid
   infinite loops. For example, increasing priorities during
   an ascending traversal should be done with care because that
   element will be encountered again later in the DEPQ. This
   function returns true if the update was successful and false
   if it failed. Failure can only occur in the removal phase if
   an element could not be found to be in the DEPQ. Insert
   does not fail in a DEPQ. See the iteration section
   for a pattern that might work if updating while iterating. */
bool ccc_depq_update(ccc_depqueue *, ccc_depq_elem *, ccc_depq_update_fn *,
                     void *);

/* Returns true if this priority value is in the DEPQ.
   you need not search with any specific struct you have
   previously created. For example using a global static
   or local dummy struct can be sufficent for this check:
   This can be helpful if you need to know if such a priority
   is present regardless of how many round robin duplicates
   are present. Returns the result in O(lgN). */
bool ccc_depq_contains(ccc_depqueue *, ccc_depq_elem *);

/* Returns the maximum priority element if present and end
   if the DEPQ is empty. By default iteration is in descending
   order by priority. Equal to end if empty. */
ccc_depq_elem *ccc_depq_begin(ccc_depqueue *);

/* Returns the minimum priority element if present and end
   if the DEPQ is empty. This is an ascending traversal
   starting point. Equal to end if empty. */
ccc_depq_elem *ccc_depq_rbegin(ccc_depqueue *);

/* Progresses through the DEPQ in order of highest priority by
   default. Use the reverse order iterator if you prefer ascending
   order. If you plan to modify while iterating, see if the erase
   and update functions are what you are looking for. Both iterators
   visit duplicates in round robin order meaning oldest first so
   that priorities can be organized round robin either ascending
   or descending and visitation is fair. */
ccc_depq_elem *ccc_depq_next(ccc_depqueue *, ccc_depq_elem *);

/* Progresses through the DEPQ in ascending order */
ccc_depq_elem *ccc_depq_rnext(ccc_depqueue *, ccc_depq_elem *);

/* The end is not a valid position in the DEPQ so it does not make
   sense to try to use any fields in the iterator once the end
   is reached. The end is same for any iteration order. */
ccc_depq_elem *ccc_depq_end(ccc_depqueue *);

/* Returns the range with pointers to the first element NOT GREATER
   than the requested begin and last element LESS than the
   provided end element. If either portion of the range cannot
   be found the end node is provided. It is the users responsibility
   to use the correct iterator as the range leaves it to the user to
   iterate. Use the next iterator from begin to end. If there are no values
   NOT GREATER than begin last is returned as the begin element. Similarly if
   there are no values LESS than end, end is returned as end element. */
ccc_depq_range ccc_depq_equal_range(ccc_depqueue *, ccc_depq_elem *begin,
                                    ccc_depq_elem *end);

/* Access the beginning of a range. Prefer this function to attempting to
   access fields of ranges directly. */
ccc_depq_elem *ccc_depq_begin_range(ccc_depq_range const *);

/* Access the ending of a range. Prefer this function to attempting to
   access fields of ranges directly. */
ccc_depq_elem *ccc_depq_end_range(ccc_depq_range const *);

/* Returns the range with pointers to the first element NOT LESS
   than the requested begin and last element GREATER than the
   provided end element. If either portion of the range cannot
   be found the end node is provided. It is the users responsibility
   to use the correct iterator as the range leaves it to the user to
   iterate. Use the rnext iterator from rbegin to end. If there are no values
   NOT LESS than rbegin last is returned as the begin element. Similarly if
   there are no values GREATER than end, end is returned as end element. */
ccc_depq_rrange ccc_depq_equal_rrange(ccc_depqueue *, ccc_depq_elem *rbegin,
                                      ccc_depq_elem *end);

/* Access the beginning of a rrange. Prefer this function to attempting to
   access fields of rranges directly. */
ccc_depq_elem *ccc_depq_begin_rrange(ccc_depq_rrange const *);

/* Access the ending of a rrange. Prefer this function to attempting to
   access fields of rranges directly. */
ccc_depq_elem *ccc_depq_end_rrange(ccc_depq_rrange const *);

/* To view the underlying tree like structure of the DEPQ
   for debugging or other purposes, provide the root of the ccc_depqueue
   to the depq_print function as the starting ccc_depq_elem. */
ccc_depq_elem *ccc_depq_root(ccc_depqueue const *);

/* Prints a tree structure of the underlying DEPQ for readability
   of many values. Helpful for printing debugging or viewing
   storage charactersistics in gdb. See sample output below.
   This function currently uses heap allocation and recursion
   so it may not be a good fit in constrained environments. Duplicates
   are indicated with plus signs followed by the number of additional
   duplicates. */
void ccc_depq_print(ccc_depqueue const *, ccc_depq_elem const *,
                    ccc_depq_print_fn *);

/* (40){id:10,val:10}{id:10,val:10}(+1)
    ├──(29)R:{id:27,val:27}
    │   ├──(12)R:{id:37,val:37}{id:37,val:37}(+1)
    │   │   ├──(2)R:{id:38,val:38}{id:38,val:38}(+1)
    │   │   │   └──(1)R:{id:39,val:39}{id:39,val:39}(+1)
    │   │   └──(9)L:{id:35,val:35}
    │   │       ├──(1)R:{id:36,val:36}
    │   │       └──(7)L:{id:31,val:31}
    │   │           ├──(3)R:{id:33,val:33}
    │   │           │   ├──(1)R:{id:34,val:34}
    │   │           │   └──(1)L:{id:32,val:32}
    │   │           └──(3)L:{id:29,val:29}
    │   │               ├──(1)R:{id:30,val:30}
    │   │               └──(1)L:{id:28,val:28}
    │   └──(16)L:{id:11,val:11}{id:11,val:11}(+1)
    │       └──(15)R:{id:24,val:24}{id:24,val:24}(+1)
    │           ├──(2)R:{id:25,val:25}{id:25,val:25}(+1)
    │           │   └──(1)R:{id:26,val:26}{id:26,val:26}(+1)
    │           └──(12)L:{id:12,val:12}{id:12,val:12}(+1)
    │               └──(11)R:{id:17,val:17}
    │                   ├──(6)R:{id:21,val:21}
    │                   │   ├──(2)R:{id:23,val:23}
    │                   │   │   └──(1)L:{id:22,val:22}
    │                   │   └──(3)L:{id:19,val:19}
    │                   │       ├──(1)R:{id:20,val:20}
    │                   │       └──(1)L:{id:18,val:18}
    │                   └──(4)L:{id:15,val:15}
    │                       ├──(1)R:{id:16,val:16}
    │                       └──(2)L:{id:13,val:13}{id:13,val:13}(+1)
    │                           └──(1)R:{id:14,val:14}
    └──(10)L:{id:8,val:8}
        ├──(1)R:{id:9,val:9}
        └──(8)L:{id:4,val:4}
            ├──(3)R:{id:6,val:6}
            │   ├──(1)R:{id:7,val:7}
            │   └──(1)L:{id:5,val:5}
            └──(4)L:{id:2,val:2}
                ├──(1)R:{id:3,val:3}
                └──(2)L:{id:1,val:1}
                    └──(1)L:{id:0,val:0} */

#endif /* DEPQUEUE */
