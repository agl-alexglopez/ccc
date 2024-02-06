# tree-lib

This repository contains various data structures supported by a no heap splay tree written in C. This library is largely experimental as a byproduct of working with the `pintOS` operating system at Stanford University, but perhaps it can be interesting to others. I chose a Splay Tree as the backing for the set and priority queue for now simply because I find it to be an interesting data structure, but support will be added for real-time ready data structures like a Red-Black Tree Priority Queue in the future.

## Priority Queue Interface

```c
/* ====================  PRIORITY QUEUE ========================

   Together the following form what you would normally expect for
   an embedded data structure. In this case a priority queue.

      pqueue
      {
         pq_elem *root
         pq_elem nil;
         size_t size;
      };

   Embed a pq_elem in your struct:

      struct val
      {
         int val;
         pq_elem elem;
      };

   If interested how these elems are implemented see tree.h
   but each will occupy 24 bytes within your struct. The
   queue itself is a one time 40 byte cost.

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
   However, any other removals, insertions, or queries of
   different values reduce performance back to O(lgN) for the
   first access.

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
   splay tree with a circular doubly linked list attached.

                        *
                      /   \
                     *     *---------
                    / \     \       |
                   *   *     *     -*-*-*-*-*
                                   |_________|

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

/* ===================   Comparisons  ==========================

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
    robust and fancy. In fact it's just a named wrapper around
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

   =============================================================
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

   The compare function one must provide to perform queries
   and other operations on the priority queue. See above. */
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
          if (!pq_update(&pq, &my_val->elem, &my_cmp_fn,
                         &priority_update, &new_priority))
          {
              ...handle failures...
          }
      }

   The above should be well defined because the user determines
   how to cast the auxiliary data themselves based on the types
   they are implementing, but care should be taking when casting
   from void nonetheless.
*/
typedef void pq_update_fn(pq_elem *, void *aux);

/* How to obtain the struct that embeds the pq_elem. For example:

      struct val
      {
          int val;
          pq_elem elem;
      };

      for (pq_elem *e = pq_begin(pq); e != pq_end(pq); e = pq_next(pq, e))
      {
          struct val *my_val = pq_entry(e, struct val, elem);
          printf("%d\n", my_val->val);
      }

   The pq element should be passed by address not by value and the
   struct and member macros represent the type used and the member
   in the struct of the pq element. NOLINTNEXTLINE */
#define pq_entry(PQ_ELEM, STRUCT, MEMBER)                                      \
    ((STRUCT *)((uint8_t *)&(PQ_ELEM)->parent_or_dups                          \
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
   pop we have O(lgN) runtime and then all subsequent
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

/* Reports the maximum priority element in the queue, drawing
   it to the root via splay operations. This, is a good
   function to use if the user wishes to bring frequently
   queried max elements to the root for O(1) popping in
   subsequent calls. This can be especially beneficial if
   multiple elements are tied for the max in round robin
   as all duplicates will be popped in O(1) time. */
pq_elem *pq_max(pqueue *);
/* Same promises as the max except for the minimum pq_elem */
pq_elem *pq_min(pqueue *);

/* If elem is already max this check is O(lgN) as the worst
   case. If not, O(1). However, if multiple  pops have occured
   the max will be close to the root. */
bool pq_is_max(pqueue *, pq_elem *);

/* If the element is already min this check is O(lgN) as the worst
   case. If not, O(1). However, if multiple  pops have occured
   the max will be close to the root. */
bool pq_is_min(pqueue *, pq_elem *);

/* Read only peek at the max and min these operations do
   not modify the tree so multiple threads could call them
   at the same time. However, all other operations are
   most definitely not safe in a splay tree for concurrency.
   Worst case O(lgN). If you have just removed an element
   and it has duplicates those duplicates will remain at
   the root O(1) until another insertion, query, or pop
   occurs. */
const pq_elem *pq_const_max(const pqueue *);
const pq_elem *pq_const_min(const pqueue *);

/* Erases a specified element known to be in the queue.
   Returns the element that follows the previous value
   of the element in round robin sorted order (lower priority).
   This may be another element or it may be the end
   element in which case no values are less than
   the erased. O(lgN). However, in practice you can often
   benefit from O(1) access if that element is a duplicate
   or you are repeatedly erasing duplicates while iterating. */
pq_elem *pq_erase(pqueue *, pq_elem *, pq_cmp_fn *, void *);

/* The same as erase but returns the next element in an
   ascending priority order. */
pq_elem *pq_rerase(pqueue *, pq_elem *, pq_cmp_fn *, void *);

/* Updates the specified elem known to be in the queue with
   a new priority in O(lgN) time. Because an update does not
   remove elements from a queue care should be taken to avoid
   infinite loops. For example, increasing priorities during
   an ascending traversal should be done with care because that
   element will be encountered again later in the queue. This
   function returns true if the update was successful and false
   if it failed. Failure can only occur in the removal phase if
   an element could not be found to be in the queue. Insert
   does not fail in a priority queue. See the iteration section
   for a pattern that might work if updating while iterating. */
bool pq_update(pqueue *, pq_elem *, pq_cmp_fn *, pq_update_fn *, void *);

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

/* A container for a simple begin and end pointer to a pq_elem.

      pq_range
      {
         pq_elem *begin;
         pq_elem *end;
      };

   A user can use the equal_range or equal_rrange function to
   fill the pq_range with the expected begin and end queries.
   The default range in a priority queue is descending order.
   A pq_range has no sense of iterator directionality and
   provides two typedefs simpy as a reminder to the programmer
   to use the appropriate next function. Use next for a
   pq_range and rnext for a pq_rrange. Otherwise, indefinite
   loops may occur. */
typedef struct range pq_range;

/* The reverse range container for queries performed with
   requal_range.

      pq_rrange
      {
         pq_elem *rbegin;
         pq_elem *end;
      };

   Be sure to use the rnext function to progress the iterator
   in this type of range.*/
typedef struct rrange pq_rrange;

/* ===================    Iteration   ==========================

   Priority queue iterators are stable and support updates and
   deletion while iterating. For example:

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

      for (pq_elem *i = pq_begin(&q); i != pq_end(&q, i); )
      {
         if (meets_criteria(i))
         {
            pq_elem *next = pq_next(&q, i);
            int update = generate_updated_priority(i);
            if (!pq_update(&q, i, val_cmp,  val_updater, &update))
            {
               ...handle errors...
            }
            i = next;
         }
         else
         {
            i = pq_next(&q, i);
         }
      }
   }

   By default traversal is by descending priority but ascending
   priority is also possible. Care should be taken while updating
   values while iterating to avoid "indefinite" loops. Also,
   both iteration directions visit duplicates in round robin
   fashion. This means that the value that has been in the
   queue the longest is visited first regardless of ascending
   or descending key order.

   =============================================================
*/

/* Returns the maximum priority element if present and end
   if the queue is empty. By default iteration is in descending
   order by priority. Equal to end if empty. */
pq_elem *pq_begin(pqueue *);
/* Returns the minimum priority element if present and end
   if the queue is empty. This is an ascending traversal
   starting point. Equal to end if empty. */
pq_elem *pq_rbegin(pqueue *);

/* Progresses through the queue in order of highest priority by
   default. Use the reverse order iterator if you prefer ascending
   order. If you plan to modify while iterating, see if the erase
   and update functions are what you are looking for. Both iterators
   visit duplicates in round robin order meaning oldest first so
   that priorities can be organized round robin either ascending
   or descending and visitation is fair. */
pq_elem *pq_next(pqueue *, pq_elem *);
/* Progresses through the queue in ascending order */
pq_elem *pq_rnext(pqueue *, pq_elem *);

/* Returns the range with pointers to the first element NOT GREATER
   than the requested begin and last element LESS than the
   provided end element. If either portion of the range cannot
   be found the end node is provided. It is the users responsibility
   to use the correct iterator as the range leaves it to the user to
   iterate.

      struct val e = {.id = 0, .val = 64};
      struct val b = {.id = 0, .val = 35};
      const pq_range range = pq_equal_range(&pq, &b.elem, &e.elem, val_cmp);
      for (pq_elem *i = range.begin; i != range.end; i = pq_next(&pq, i))
      {
          const int cur_val = pq_entry(i, struct val, elem)->val;
          printf("%d\n", cur_val->val);
      }

   Use the next iterator from begin to end. If there are no values NOT GREATER
   than begin last is returned as the begin element. Similarly if there are
   no values LESS than end, end is returned as end element. */
pq_range pq_equal_range(pqueue *, pq_elem *begin, pq_elem *end, pq_cmp_fn *,
                        void *aux);

/* Returns the range with pointers to the first element NOT LESS
   than the requested begin and last element GREATER than the
   provided end element. If either portion of the range cannot
   be found the end node is provided. It is the users responsibility
   to use the correct iterator as the range leaves it to the user to
   iterate. Use the rnext iterator from rbegin to end.

      struct val e = {.id = 0, .val = 35};
      struct val b = {.id = 0, .val = 64};
      const pq_rrange range = pq_equal_rrange(&pq, &b.elem, &e.elem, val_cmp);
      for (pq_elem *i = range.rbegin; i != range.end; i = pq_rnext(&pq, i))
      {
          const int cur_val = pq_entry(i, struct val, elem)->val;
          printf("%d\n", cur_val->val);
      }

   Use the next iterator from begin to end. If there are no values NOT LESS
   than begin last is returned as the begin element. Similarly if there are
   no values GREATER than end, end is returned as end element. */
pq_rrange pq_equal_rrange(pqueue *, pq_elem *rbegin, pq_elem *end, pq_cmp_fn *,
                          void *aux);

/* The end is not a valid position in the queue so it does not make
   sense to try to use any fields in the iterator once the end
   is reached. The end is same for any iteration order. */
pq_elem *pq_end(pqueue *);

/* To view the underlying tree like structure of the priority queue
   for debugging or other purposes, provide the root of the pqueue
   to the pq_print function as the starting pq_elem. */
pq_elem *pq_root(const pqueue *);

/* Define a function to use printf for your custom struct type.
   For example:
      struct val
      {
         int val;
         pq_elem elem;
      };

      void print_my_val(pq_elem *elem)
      {
         const struct val *v = pq_entry(elem, struct val, elem);
         printf("{%d}", v->val);
      }

   Output should be one line with no newline character. Then,
   the printer function will take care of the rest. */
typedef node_print_fn pq_print_fn;

/* Prints a tree structure of the underlying queu for readability
   of many values. Helpful for printing debugging or viewing
   storage charactersistics in gdb. See sample output below.
   This function currently uses heap allocation and recursion
   so it may not be a good fit in constrained environments. */
void pq_print(pqueue *, pq_elem *, pq_print_fn *);

/*
      (40){id:10,val:10}{id:10,val:10}(+1)
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
```

## Set Interface

```c
/* ========================   SET   ============================

   Together the following components make a set as an embedded
   data structure.

      set
      {
          set_elem *root;
          set_elem nil;
          size_t size;
      };

   Embed a set_elem in your struct:

      struct val
      {
          int val;
          set_elem elem;
      };

   The nil is helpful in this case to make the Set data structure
   align with what you would expect from a C++ like set. It is
   the default returned node for some failed membership insertion
   and deletion tests.

   You have a few options when using the embedded set data structure
   for lookups removal and insertion. The first option is to
   proceed with your program logic as you would use a list or
   queue data structure. Assume that you are responsible for
   every value in the set.

      struct val
      {
          int val;
          set_elem elem;
      }

      set s;
      set_init(&s);
      struct val my_val;
      my_val.val = 0;

      assert(set_insert(&s, &my_val.elem, set_cmp_fn, NULL)))
      ...I am responsible for the set so I know this is unique...

      struct set_elem *e = set_find(&s, &my_val.elem, set_cmp_fn, NULL);
      ...This must be mine, because I am the one who made it earlier...

   However, this approach is not very flexible and loses much of
   the power of a set. The second approach is to use a static
   global or local dummy struct as your query key whenever
   you want to search the set. Then, you have more flexibility
   with how you can use the data structure. For example:

      static struct val set_key;
      static set my_set;

      static threeway_cmp
      val_cmp(const set_elem *a, const set_elem *b, void *aux)
      {
          (void)aux;
          struct val *lhs = set_entry(a, struct val, elem);
          struct val *rhs = set_entry(b, struct val, elem);
          return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }

      bool
      my_contains(int key)
      {
          set_key.val = key;
          return set_contains(&s, &set_key, &val_cmp, NULL);
      }


      int
      main()
      {
          ... Program logic of generating all values  ...
          ... that will go in the set for later       ...
      }

   This is asking a slightly different question. We are
   querying not to see if our specific struct is contained
   in the set but rather if any struct with that value
   is contained in the struct. This also works for
   getting a specific struct back from a query. You don't
   need to have the exact node you are searching for to
   retrieve it. For example:

      static struct val set_key;
      static set my_set;

      static threeway_cmp
      val_cmp(const set_elem *a, const set_elem *b, void *aux)
      {
          (void)aux;
          struct val *lhs = set_entry(a, struct val, elem);
          struct val *rhs = set_entry(b, struct val, elem);
          return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }

      struct node *
      my_find(int key)
      {
          set_key.val = key;
          return set_find(&s, &set_key, &val_cmp, NULL);
      }


      int
      main()
      {
          ... Program logic of generating all values  ...
          ... that will go in the set for later       ...

          set_elem *my_query = my_find(5);
          if (my_query != set_end(&s))
             ... Proceed with some logic .....
      }

   Here you were able to retrieve the node that was in the
   data structure only if it was present. This could be
   useful as well. However, consider scope because this
   is not a heap based data structure but rather one
   that only makes sense if the structs stay available
   for the lifetime of the program which is your
   responsibility.

   =============================================================
*/

/* An embedded set data structure for storage and retrieval
   of sorted unique elements for duplicate storage see
   priority queue or multiset */
typedef struct tree set;

/* The element embedded withing a struct that is used to
   store, search, and retrieve data in the tree. */
typedef struct node set_elem;

/* ===================   Comparisons  ==========================

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
          set_elem elem;
      };

      static threeway_cmp
      val_cmp(const set_elem *a, const set_elem *b, void *aux)
      {
          (void)aux;
          struct val *lhs = set_entry (a, struct val, elem);
          struct val *rhs = set_entry (b, struct val, elem);
          return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }

   =============================================================
*/
typedef tree_cmp_fn set_cmp_fn;

/* A container for a simple begin and end pointer to a set_elem.

      set_range
      {
         set_elem *begin;
         set_elem *end;
      };

   A user can use the equal_range or equal_rrange function to
   fill the set_range with the expected begin and end queries.
   The default range in a priority queue is descending order.
   A set_range has no sense of iterator directionality and
   provides two typedefs simpy as a reminder to the programmer
   to use the appropriate next function. Use next for a
   set_range and rnext for a set_rrange. Otherwise, indefinite
   loops may occur. */
typedef struct range set_range;

/* The reverse range container for queries performed with
   requal_range.

      set_rrange
      {
         set_elem *rbegin;
         set_elem *end;
      };

   Be sure to use the rnext function to progress the iterator
   in this type of range.*/
typedef struct rrange set_rrange;

/* NOLINTNEXTLINE */
#define set_entry(SET_ELEM, STRUCT, MEMBER)                                    \
    ((STRUCT *)((uint8_t *)&(SET_ELEM)->parent_or_dups                         \
                - offsetof(STRUCT, MEMBER.parent_or_dups))) /* NOLINT */

/* Basic O(1) initialization and sanity checks for a set. Operations
   should only be used on a set once it has been intialized. */
void set_init(set *);
/* O(1) */
bool set_empty(set *);
/* O(1) */
size_t set_size(set *);

/* ===================     Set Methods   ======================= */

/* Basic C++ style set operation. Contains does not return
   an element but will tell you if an element with the same
   value you are using as your set keys is present. This is
   an important note. You are not asking if your specific
   element is in the set, simply if we already have one
   with the same key. Do not assume your element is the
   one that is found unless you know it is only one you
   have created. */
bool set_contains(set *, set_elem *, set_cmp_fn *, void *);

/* Returns true if the element you have requested to be
   inserted is inserted false if it was present already.
   Becuase this is heap free data structure there is no need
   to return the actual element that is inserted. You already
   have it when you call this function.*/
bool set_insert(set *, set_elem *, set_cmp_fn *, void *);

/* IT IS UNDEFINED BEHAVIOR TO MODIFY THE KEY OF A FOUND ELEM.
   THIS FUNCTION DOES NOT REMOVE THE ELEMENT YOU SEEK.
   Returns the element sought after if found otherwise returns
   the set end element that can be confirmed with set_end.
   it is undefined to use the set end element and it does not
   have any attachment to any struct you are using so trying
   to get the set entry from it is BAD. This function tries
   to enforce that you should not modify the element for
   a read only lookup. You can modify other fields you may
   be using for your program, but please read the warning.
   There is little I can do to stop you from ruining
   everything if you choose to do so. */
const set_elem *set_find(set *, set_elem *, set_cmp_fn *, void *);

/* Erases the element specified by key value and returns a
   pointer to the set element or set end pointer if the
   element cannot be found. It is undefined to use the set
   end element and it does not have any attachment to any
   struct you are using so trying to get the set entry from
   it will return garbage or worse.*/
set_elem *set_erase(set *, set_elem *, set_cmp_fn *, void *);

/* Check if the current elem is the min. O(lgN) */
bool set_is_min(set *, set_elem *);
/* Check if the current elem is the max. O(lgN) */
bool set_is_max(set *, set_elem *);

/* Basic C++ style set operation. Contains does not return
   an element but will tell you if an element with the same
   value you are using as your set keys is present. This is
   an important note. You are not asking if your specific
   element is in the set, simply if we already have one
   with the same key. Do not assume your element is the
   one that is found unless you know it is only one you
   have created. The const version does no fixups and should
   be used only rarely. */
bool set_const_contains(set *, set_elem *, set_cmp_fn *, void *);

/* Read only seek into the data structure backing the set.
   It is therefore safe for multiple threads to read with
   const find but any other concurrent operations are not
   safe. Also, note that the Splay Implementing this set
   benefits from locality of reference and should be
   allowed to repare itself with lookups with all other
   functions whenever possible. */
const set_elem *set_const_find(set *, set_elem *, set_cmp_fn *, void *);

/* ===================    Iteration   ==========================

   Set iterators are stable and support deletion while iterating.
   For example:

   struct val
   {
       int val;
       set_elem elem;
   };

   static set s;

   int
   main ()
   {
       set_init(&s);

       ...Fill the container with program logic...

       for (set_elem *i = set_begin(&s); i != set_end(&s, i); )
       {
          if (meets_criteria(i))
          {
              set_elem *next = set_next(&s, i);
              assert(set_erase(&s, i) != set_end(&s);
              i = next;
          }
          else
          {
              i = set_next(&s, i);
          }
       }
   }

   By default traversal is by ascending sorted value but descending
   order is also possible.

   =============================================================
*/

/* This is how you can tell if your set find and set erase
   functions are successful. One should always check that
   the set element does not equal the end element as you
   would in C++. For example:

      struct val {
         int val;
         set_elem elem;
      }

      set s;
      set_init(&s);
      struct val a;
      a.val = 0;

      if (!set_insert(&s, &a.elem, cmp, NULL))
         ...Do some logic...

      ... Elsewhere ...

      struct set_elem *e = set_find(&s, &a.elem, cmp, NULL);
      if (e != set_end(&s))
         ...Proceed with some logic...
      else
         ...Do something else...
*/
set_elem *set_end(set *);

/* Provides the start for an inorder ascending order traversal
   of the set. Equivalent to end of the set is empty. */
set_elem *set_begin(set *);

/* Provides the start for an inorder descending order traversal
   of the set. Equivalent to end of the set is empty. */
set_elem *set_rbegin(set *);

/* Progresses the pointer to the next greatest element in
   the set or the end if done. */
set_elem *set_next(set *, set_elem *);

/* Progresses the pointer to the next lesser element in
   the set or the end if done. */
set_elem *set_rnext(set *, set_elem *);

/* Returns the range with pointers to the first element NOT LESS
   than the requested begin and last element GREATER than the
   provided end element. If either portion of the range cannot
   be found the end node is provided. It is the users responsibility
   to use the correct iterator as the range leaves it to the user to
   iterate.

      struct val b = {.id = 0, .val = 35};
      struct val e = {.id = 0, .val = 64};
      const set_range range = set_equal_range(&s, &b.elem, &e.elem, val_cmp);
      for (set_elem *i = range.begin; i != range.end; i = set_next(&s, i))
      {
          const int cur_val = set_entry(i, struct val, elem)->val;
          printf("%d\n", cur_val->val);
      }

   Use the next iterator from begin to end. If there are no values NOT LESS
   than begin last is returned as the begin element. Similarly if there are
   no values GREATER than end, end is returned as end element. */
set_range set_equal_range(set *, set_elem *begin, set_elem *end, set_cmp_fn *,
                          void *aux);

/* Returns the range with pointers to the first element NOT GREATER
   than the requested begin and last element LESS than the
   provided end element. If either portion of the range cannot
   be found the end node is provided. It is the users responsibility
   to use the correct iterator as the range leaves it to the user to
   iterate. Use the next iterator from rbegin to end.

      struct val b = {.id = 0, .val = 64};
      struct val e = {.id = 0, .val = 35};
      const set_range r = set_equal_range(&s, &b.elem, &e.elem, val_cmp);
      for (set_elem *i = r.rbegin; i != r.end; i = set_rnext(&s, i))
      {
          const int cur_val = set_entry(i, struct val, elem)->val;
          printf("%d\n", cur_val->val);
      }

   Use the next iterator from begin to end. If there are no values NOT GREATER
   than begin last is returned as the begin element. Similarly if there are
   no values LESS than end, end is returned as end element. */
set_rrange set_equal_rrange(set *, set_elem *rbegin, set_elem *end,
                            set_cmp_fn *, void *aux);

/* Internal testing. Mostly useless. User at your own risk
   unless you wish to do some traversal of your own liking.
   However, you should of course not modify keys or nodes.
   You will need to pass this to the print function as a
   starting node for debugging. */
set_elem *set_root(const set *);

/* Define a function to use printf for your custom struct type.
   For example:
      struct val
      {
         int val;
         pq_elem elem;
      };

      void print_my_val(set_elem *elem)
      {
         const struct val *v = set_entry(elem, struct val, elem);
         printf("{%d}", v->val);
      }

   Output should be one line with no newline character. Then,
   the printer function will take care of the rest. */
typedef node_print_fn set_print_fn;

/* Prints a tree structure of the underlying set for readability
   of many values. Helpful for printing debugging or viewing
   storage charactersistics in gdb. See sample output below.
   This function currently uses heap allocation and recursion
   so it may not be a good fit in constrained environments. */
void set_print(set *, set_elem *, set_print_fn *);

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
```


