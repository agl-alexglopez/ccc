/* Author: Alexander G. Lopez
   --------------------------
   This is the Set interface for the Splay Tree
   Set. In this case we modify a Splay Tree to allow for
   a true set (naturally sorted unique elements). See the
   priority queue for another use case of this data
   structure. A set can be an interesting option for a
   LRU cache. Any application such that there is biased
   distribution of access via lookup, insertion, and
   removal brings those elements closer to the root of
   the tree, approaching constant time operations. See
   also the multiset for great benefits of duplicates
   being taken from a data structure. The runtime is
   amortized O(lgN) but with the right use cases we
   may benefit from the O(1) capabilities of the working
   set. The anti-pattern is to seek and splay all elements
   to the tree in sequential order. However, any random
   variants will help maintain tree health and this interface
   provides robust iterators that can be used if read only
   access is required of all elements or only conditional
   modifications. This may combat such an anti-pattern. */
#ifndef SET
#define SET

#include "attrib.h"
#include "tree.h"

#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

/* ========================   CCC_SET_   ============================

   Together the following components make a set as an embedded
   data structure.

      set
      {
          ccc_set_elem *root;
          ccc_set_elem nil;
          size_t size;
      };

   Embed a ccc_set_elem in your struct:

      struct val
      {
          int val;
          ccc_set_elem elem;
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
          ccc_set_elem elem;
      }

      set s;
      set_init(&s);
      struct val my_val;
      my_val.val = 0;

      assert(set_insert(&s, &my_val.elem, set_cmp_fn, NULL)))
      ...I am responsible for the set so I know this is unique...

      ccc_set_elem *e = set_find(&s, &my_val.elem, set_cmp_fn, NULL);
      ...This must be mine, because I am the one who made it earlier...

   However, this approach is not very flexible and loses much of
   the power of a set. The second approach is to use a static
   global or local dummy struct as your query key whenever
   you want to search the set. Then, you have more flexibility
   with how you can use the data structure. For example:

      static struct val set_key;
      static set my_set;

      static threeway_cmp
      val_cmp(const ccc_set_elem *a, const ccc_set_elem *b, void *aux)
      {
          (void)aux;
          struct val *lhs = CCC_SET_OF(struct val, elem, a);
          struct val *rhs = CCC_SET_OF(struct val, elem, b);
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
      val_cmp(const ccc_set_elem *a, const ccc_set_elem *b, void *aux)
      {
          (void)aux;
          struct val *lhs = CCC_SET_OF(struct val, elem, a);
          struct val *rhs = CCC_SET_OF(struct val, elem, b);
          return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }

      ccc_node *
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

          ccc_set_elem *my_query = my_find(5);
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

   ============================================================= */

/* An embedded set data structure for storage and retrieval
   of sorted unique elements for duplicate storage see
   priority queue or multiset */
typedef struct ccc_set
{
    ccc_tree t;
} ccc_set;

/* The element embedded withing a struct that is used to
   store, search, and retrieve data in the tree. */
typedef struct ccc_set_elem
{
    ccc_node n;
} ccc_set_elem;

typedef enum ccc_set_threeway_cmp
{
    CCC_SET_LES = CCC_NODE_LES,
    CCC_SET_EQL = CCC_NODE_EQL,
    CCC_SET_GRT = CCC_NODE_GRT,
} ccc_set_threeway_cmp;

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
          ccc_set_elem elem;
      };

      static threeway_cmp
      val_cmp(const ccc_set_elem *a, const ccc_set_elem *b, void *aux)
      {
          (void)aux;
          struct val *lhs = CCC_SET_OF(struct val, elem, a);
          struct val *rhs = CCC_SET_OF(struct val, elem, b);
          return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }

   ============================================================= */
typedef ccc_set_threeway_cmp ccc_set_cmp_fn(ccc_set_elem const *a,
                                            ccc_set_elem const *b, void *aux);

/* Performs user specified destructor actions on a single ccc_set_elem. This
   ccc_set_elem is assumed to be embedded in user defined structs and therefore
   allows the user to perform any updates to their program before deleting
   this element. The user will know if they have heap allocated their
   own structures and therefore shall call free on the containing structure.
   If the data structure is stack allocated, free is not necessary but other
   updates may be specified by this function. */
typedef void ccc_set_destructor_fn(ccc_set_elem *);

/* A container for a simple begin and end pointer to a ccc_set_elem.

      set_range
      {
         ccc_set_elem *begin;
         ccc_set_elem *end;
      };

   A user can use the equal_range or equal_rrange function to
   fill the set_range with the expected begin and end queries.
   The default range in a priority queue is descending order.
   A set_range has no sense of iterator directionality and
   provides two typedefs simpy as a reminder to the programmer
   to use the appropriate next function. Use next for a
   set_range and rnext for a set_rrange. Otherwise, indefinite
   loops may occur. */
typedef struct ccc_set_range
{
    ccc_range r ATTRIB_PRIVATE;
} ccc_set_range;

/* The reverse range container for queries performed with
   requal_range.

      set_rrange
      {
         ccc_set_elem *rbegin;
         ccc_set_elem *end;
      };

   Be sure to use the rnext function to progress the iterator
   in this type of range.*/
typedef struct ccc_set_rrange
{
    ccc_rrange r ATTRIB_PRIVATE;
} ccc_set_rrange;

/* Define a function to use printf for your custom struct type.
   For example:
      struct val
      {
         int val;
         ccc_pq_elem elem;
      };

      void print_my_val(ccc_set_elem *elem)
      {
         struct val const *v = CCC_SET_OF(struct val, elem, elem);
         printf("{%d}", v->val);
      }

   Output should be one line with no newline character. Then,
   the printer function will take care of the rest. */
typedef void ccc_set_print_fn(ccc_set_elem const *);

/* NOLINTNEXTLINE */
#define CCC_SET_OF(struct, member, set_elem)                                   \
    ((struct *)((uint8_t *)&(set_elem)->n                                      \
                - offsetof(struct, member.n))) /* NOLINT */

#define CCC_SET_INIT(set_name, cmp, aux)                                       \
    {                                                                          \
        .t = CCC_TREE_INIT(set_name, cmp, aux)                                 \
    }

/* Calls the destructor for each element while emptying the set.
   Usually, this destructor function is expected to call free for each
   struct for which a ccc_set_elem is embedded if they are heap allocated.
   However, for stack allocated structures this is not required.
   Other updates before destruction are possible to include in destructor
   if determined necessary by the user. A set has no hidden allocations and
   therefore the only heap memory is controlled by the user. */
void ccc_set_clear(ccc_set *, ccc_set_destructor_fn *destructor);

/* O(1) */
bool ccc_set_empty(ccc_set *);
/* O(1) */
size_t ccc_set_size(ccc_set *);

/* ===================     Set Methods   ======================= */

/* Basic C++ style set operation. Contains does not return
   an element but will tell you if an element with the same
   value you are using as your set keys is present. This is
   an important note. You are not asking if your specific
   element is in the set, simply if we already have one
   with the same key. Do not assume your element is the
   one that is found unless you know it is only one you
   have created. */
bool ccc_set_contains(ccc_set *, ccc_set_elem *);

/* Returns true if the element you have requested to be
   inserted is inserted false if it was present already.
   Becuase this is heap free data structure there is no need
   to return the actual element that is inserted. You already
   have it when you call this function.*/
bool ccc_set_insert(ccc_set *, ccc_set_elem *);

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
ccc_set_elem const *ccc_set_find(ccc_set *, ccc_set_elem *);

/* Erases the element specified by key value and returns a
   pointer to the set element or set end pointer if the
   element cannot be found. It is undefined to use the set
   end element and it does not have any attachment to any
   struct you are using so trying to get the set entry from
   it will return garbage or worse.*/
ccc_set_elem *ccc_set_erase(ccc_set *, ccc_set_elem *);

/* Check if the current elem is the min. O(lgN) */
bool ccc_set_is_min(ccc_set *, ccc_set_elem *);
/* Check if the current elem is the max. O(lgN) */
bool ccc_set_is_max(ccc_set *, ccc_set_elem *);

/* Basic C++ style set operation. Contains does not return
   an element but will tell you if an element with the same
   value you are using as your set keys is present. This is
   an important note. You are not asking if your specific
   element is in the set, simply if we already have one
   with the same key. Do not assume your element is the
   one that is found unless you know it is only one you
   have created. The const version does no fixups and should
   be used only rarely. */
bool ccc_set_const_contains(ccc_set *, ccc_set_elem *);

/* Read only seek into the data structure backing the set.
   It is therefore safe for multiple threads to read with
   const find but any other concurrent operations are not
   safe. Also, note that the Splay Tree Implementing this set
   benefits from locality of reference and should be
   allowed to repair itself with lookups with all other
   functions whenever possible. */
ccc_set_elem const *ccc_set_const_find(ccc_set *, ccc_set_elem *);

/* ===================    Iteration   ==========================

   Set iterators are stable and support deletion while iterating.
   For example:

   struct val
   {
       int val;
       ccc_set_elem elem;
   };

   static set s;

   int
   main ()
   {
       set_init(&s);

       ...Fill the container with program logic...

       for (ccc_set_elem *i = set_begin(&s); i != set_end(&s, i); )
       {
          if (meets_criteria(i))
          {
              ccc_set_elem *next = set_next(&s, i);
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

   ============================================================= */

/* This is how you can tell if your set find and set erase
   functions are successful. One should always check that
   the set element does not equal the end element as you
   would in C++. For example:

      struct val {
         int val;
         ccc_set_elem elem;
      }

      set s;
      set_init(&s);
      struct val a;
      a.val = 0;

      if (!set_insert(&s, &a.elem, cmp, NULL))
         ...Do some logic...

      ... Elsewhere ...

      ccc_set_elem *e = set_find(&s, &a.elem, cmp, NULL);
      if (e != set_end(&s))
         ...Proceed with some logic...
      else
         ...Do something else... */
ccc_set_elem *ccc_set_end(ccc_set *);

/* Provides the start for an inorder ascending order traversal
   of the set. Equivalent to end of the set is empty. */
ccc_set_elem *ccc_set_begin(ccc_set *);

/* Provides the start for an inorder descending order traversal
   of the set. Equivalent to end of the set is empty. */
ccc_set_elem *ccc_set_rbegin(ccc_set *);

/* Progresses the pointer to the next greatest element in
   the set or the end if done. */
ccc_set_elem *ccc_set_next(ccc_set *, ccc_set_elem *);

/* Progresses the pointer to the next lesser element in
   the set or the end if done. */
ccc_set_elem *ccc_set_rnext(ccc_set *, ccc_set_elem *);

/* Returns the range with pointers to the first element NOT LESS
   than the requested begin and last element GREATER than the
   provided end element. If either portion of the range cannot
   be found the end node is provided. It is the users responsibility
   to use the correct iterator as the range leaves it to the user to
   iterate.

      struct val b = {.id = 0, .val = 35};
      struct val e = {.id = 0, .val = 64};
      const set_range range = set_equal_range(&s, &b.elem, &e.elem, val_cmp);
      for (ccc_set_elem *i = set_begin_range(&range);
           i != set_end_range(&range);
           i = set_next(&s, i))
      {
          const int cur_val = CCC_SET_OF(struct val, elem, i)->val;
          printf("%d\n", cur_val->val);
      }

   Use the next iterator from begin to end. If there are no values NOT LESS
   than begin last is returned as the begin element. Similarly if there are
   no values GREATER than end, end is returned as end element. */
ccc_set_range ccc_set_equal_range(ccc_set *, ccc_set_elem *begin,
                                  ccc_set_elem *end);

ccc_set_elem *ccc_set_begin_range(ccc_set_range const *);

ccc_set_elem *ccc_set_end_range(ccc_set_range const *);

/* Returns the range with pointers to the first element NOT GREATER
   than the requested begin and last element LESS than the
   provided end element. If either portion of the range cannot
   be found the end node is provided. It is the users responsibility
   to use the correct iterator as the range leaves it to the user to
   iterate. Use the next iterator from rbegin to end.

      struct val b = {.id = 0, .val = 64};
      struct val e = {.id = 0, .val = 35};
      const set_range r = set_equal_range(&s, &b.elem, &e.elem, val_cmp);
      for (ccc_set_elem *i = set_begin_rrange(&range);
           i != set_end_rrange(&range);
           i = set_rnext(&s, i))
      {
          const int cur_val = CCC_SET_OF(struct val, elem, i)->val;
          printf("%d\n", cur_val->val);
      }

   Use the next iterator from begin to end. If there are no values NOT GREATER
   than begin last is returned as the begin element. Similarly if there are
   no values LESS than end, end is returned as end element. */
ccc_set_rrange ccc_set_equal_rrange(ccc_set *, ccc_set_elem *rbegin,
                                    ccc_set_elem *end);

ccc_set_elem *ccc_set_begin_rrange(ccc_set_rrange const *);

ccc_set_elem *ccc_set_end_rrange(ccc_set_rrange const *);

/* Internal testing. Mostly useless. User at your own risk
   unless you wish to do some traversal of your own liking.
   However, you should of course not modify keys or nodes.
   You will need to pass this to the print function as a
   starting node for debugging. */
ccc_set_elem *ccc_set_root(ccc_set const *);

/* Prints a tree structure of the underlying ccc_set for readability
   of many values. Helpful for printing debugging or viewing
   storage charactersistics in gdb. See sample output below.
   This function currently uses heap allocation and recursion
   so it may not be a good fit in constrained environments. */
void ccc_set_print(ccc_set const *, ccc_set_elem const *, ccc_set_print_fn *);

/* (40){id:10,val:10}{id:10,val:10}
    ├──(29)R:{id:27,val:27}
    │   ├──(12)R:{id:37,val:37}{id:37,val:37}
    │   │   ├──(2)R:{id:38,val:38}{id:38,val:38}
    │   │   │   └──(1)R:{id:39,val:39}{id:39,val:39}
    │   │   └──(9)L:{id:35,val:35}
    │   │       ├──(1)R:{id:36,val:36}
    │   │       └──(7)L:{id:31,val:31}
    │   │           ├──(3)R:{id:33,val:33}
    │   │           │   ├──(1)R:{id:34,val:34}
    │   │           │   └──(1)L:{id:32,val:32}
    │   │           └──(3)L:{id:29,val:29}
    │   │               ├──(1)R:{id:30,val:30}
    │   │               └──(1)L:{id:28,val:28}
    │   └──(16)L:{id:11,val:11}{id:11,val:11}
    │       └──(15)R:{id:24,val:24}{id:24,val:24}
    │           ├──(2)R:{id:25,val:25}{id:25,val:25}
    │           │   └──(1)R:{id:26,val:26}{id:26,val:26}
    │           └──(12)L:{id:12,val:12}{id:12,val:12}
    │               └──(11)R:{id:17,val:17}
    │                   ├──(6)R:{id:21,val:21}
    │                   │   ├──(2)R:{id:23,val:23}
    │                   │   │   └──(1)L:{id:22,val:22}
    │                   │   └──(3)L:{id:19,val:19}
    │                   │       ├──(1)R:{id:20,val:20}
    │                   │       └──(1)L:{id:18,val:18}
    │                   └──(4)L:{id:15,val:15}
    │                       ├──(1)R:{id:16,val:16}
    │                       └──(2)L:{id:13,val:13}{id:13,val:13}
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

#endif
