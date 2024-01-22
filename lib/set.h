/*
   Author: Alexander G. Lopez
   --------------------------
   This is the Set interface for the Splay Tree
   Set. In this case we modify a Splay Tree to allow for
   a true set (naturally sorted unique elements). See the
   priority queue for another use case of this data
   structure.
 */
#ifndef SET
#define SET
#include "tree.h"
#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

/* =============================================================
   ========================   SET   ============================
   =============================================================

   Together the following components make a set as an embedded
   data structure.

      set
      {
         set_elem *root
         set_elem nil;
      };

   Embed a set_elem in your struct:

      struct val {
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

      struct val {
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
      val_cmp (const set_elem *a, const set_elem *b, void *aux)
      {
        (void)aux;
        struct val *lhs = set_entry (a, struct val, elem);
        struct val *rhs = set_entry (b, struct val, elem);
        return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }

      bool my_contains(int key)
      {
         set_key.val = key;
         return set_contains (&s, &set_key, &val_cmp, NULL);
      }


      int main() {
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
      val_cmp (const set_elem *a, const set_elem *b, void *aux)
      {
        (void)aux;
        struct val *lhs = set_entry (a, struct val, elem);
        struct val *rhs = set_entry (b, struct val, elem);
        return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }

      struct node * my_find(int key)
      {
         set_key.val = key;
         return set_find (&s, &set_key, &val_cmp, NULL);
      }


      int main() {
         ... Program logic of generating all values  ...
         ... that will go in the set for later       ...

         struct node *my_query = my_find(5);
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
*/
typedef struct node set_elem;
typedef struct tree set;

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
        set_elem elem;
      };

      static threeway_cmp
      val_cmp (const set_elem *a, const set_elem *b, void *aux)
      {
        (void)aux;
        struct val *lhs = set_entry (a, struct val, elem);
        struct val *rhs = set_entry (b, struct val, elem);
        return (lhs->val > rhs->val) - (lhs->val < rhs->val);
      }
*/
typedef tree_cmp_fn set_cmp_fn;

/* NOLINTNEXTLINE */
#define set_entry(TREE_ELEM, STRUCT, MEMBER)                                  \
  ((STRUCT *)((uint8_t *)&(TREE_ELEM)->dups                                   \
              - offsetof (STRUCT, MEMBER.dups))) /* NOLINT */

/* Basic O(1) initialization and sanity checks for a set. Operations
   should only be used on a set once it has been intialized. */
void set_init (set *);
bool set_empty (set *);
size_t set_size (set *);

/*
   =============================================================
   ===================     Set Methods   =======================
   =============================================================

*/

/* Basic C++ style set operations. Contains does not return
   an element but will tell you if an element with the same
   value you are using as your set keys is present. This is
   an important note. You are not asking if your specific
   element is in the set, simply if we already have one
   wiht the same key. Do not assume your element is the
   one that is found unless you know it is only one you
   have created. */
bool set_contains (set *, set_elem *, set_cmp_fn *, void *);

/* Returns true if the element you have requested to be
   inserted is inserted false if it was present already.
   Becuase this is heap free data structure there is no need
   to return the actual element that is inserted. You already
   have it when you call this function.*/
bool set_insert (set *, set_elem *, set_cmp_fn *, void *);

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
set_elem *set_end (set *);

set_elem *set_begin (set *);
set_elem *set_next (set *, set_elem *);

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
   However, there is little I can do to stop you from ruining
   everything if you choose to do so. */
const set_elem *set_find (set *, set_elem *, set_cmp_fn *, void *);

/* Erases the element specified by key value and returns a
   pointer to the set element or set end pointer if the
   element cannot be found. It is undefined to use the set
   end element and it does not have any attachment to any
   struct you are using so trying to get the set entry from
   it will not work.*/
set_elem *set_erase (set *, set_elem *, set_cmp_fn *, void *);

/* Internal testing. Mostly useless. User at your own risk
   unless you wish to do some traversal of your own liking.
   However, you should of course not modify keys or nodes. */
set_elem *set_root (set *);
#endif
