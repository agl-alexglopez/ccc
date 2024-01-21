/*
   Author: Alexander G. Lopez
   --------------------------
   This is the internal implementation of a splay tree that
   runs all the data structure interfaces provided by this
   library.

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
#include "pqueue.h"
#include "set.h"
#include "tree.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* Text coloring macros (ANSI character escapes) for printing function
   colorful output. Consider changing to a more portable library like
   ncurses.h. However, I don't want others to install ncurses just to explore
   the project. They already must install gnuplot. Hope this works. */
#define COLOR_BLK "\033[34;1m"
#define COLOR_BLU_BOLD "\033[38;5;12m"
#define COLOR_RED_BOLD "\033[38;5;9m"
#define COLOR_RED "\033[31;1m"
#define COLOR_CYN "\033[36;1m"
#define COLOR_GRN "\033[32;1m"
#define COLOR_NIL "\033[0m"
#define COLOR_ERR COLOR_RED "Error: " COLOR_NIL
#define PRINTER_INDENT (short)13

/* Printing enum for printing tree structures if heap available. */
enum print_link
{
  BRANCH = 0, /* ├── */
  LEAF = 1    /* └── */
};

/* =========================================================================
   =======================        Prototypes          ======================
   ========================================================================= */

static void init_tree (struct tree *);
static void init_node (struct tree *, struct node *);
static bool empty (const struct tree *);
static void multiset_insert (struct tree *, struct node *, tree_cmp_fn *,
                             void *);
static struct node *find (struct tree *, struct node *, tree_cmp_fn *, void *);
static bool contains (struct tree *, struct node *, tree_cmp_fn *, void *);
static struct node *erase (struct tree *, struct node *, tree_cmp_fn *,
                           void *);
static bool insert (struct tree *, struct node *, tree_cmp_fn *, void *);
static struct node *multiset_erase_max_or_min (struct tree *, struct node *,
                                               tree_cmp_fn *, void *);
static struct node *multiset_erase_node (struct tree *, struct node *,
                                         tree_cmp_fn *, void *);
static struct node *pop_dup_node (struct tree *, struct node *, tree_cmp_fn *,
                                  struct node *);
static struct node *pop_front_dup (struct tree *, struct node *,
                                   tree_cmp_fn *);
static struct node *remove_from_tree (struct tree *, struct node *,
                                      tree_cmp_fn *);
static struct node *connect_new_root (struct tree *, struct node *,
                                      threeway_cmp);
static struct node *root (const struct tree *);
static struct node *max (const struct tree *);
static struct node *pop_max (struct tree *);
static struct node *pop_min (struct tree *);
static struct node *min (const struct tree *);
const static struct node *end (const struct tree *);
static threeway_cmp force_find_grt (const struct node *, const struct node *,
                                    void *);
static threeway_cmp force_find_les (const struct node *, const struct node *,
                                    void *);
static size_t size (struct tree *);
static void give_parent_subtree (struct tree *, struct node *, enum tree_link,
                                 struct node *);
static bool stores_dups (const struct tree *, const struct node *);
static void add_duplicate (struct tree *, struct node *, struct node *,
                           struct node *);
static struct node *splay (struct tree *, struct node *, const struct node *,
                           tree_cmp_fn *);

/* =========================================================================
   =======================  Priority Queue Interface  ======================
   ========================================================================= */

void
pq_init (pqueue *pq)
{
  init_tree (pq);
}

bool
pq_empty (const pqueue *const pq)
{
  return empty (pq);
}

pq_elem *
pq_root (const pqueue *const pq)
{
  return root (pq);
}

pq_elem *
pq_max (const pqueue *const pq)
{
  return max (pq);
}

pq_elem *
pq_min (const pqueue *const pq)
{
  return min (pq);
}

void
pq_insert (pqueue *pq, pq_elem *elem, pq_cmp_fn *fn, void *aux)
{
  multiset_insert (pq, elem, fn, aux);
}

pq_elem *
pq_erase (pqueue *pq, pq_elem *elem, pq_cmp_fn *fn, void *aux)
{
  return multiset_erase_node (pq, elem, fn, aux);
}

pq_elem *
pq_pop_max (pqueue *pq)
{
  return pop_max (pq);
}

pq_elem *
pq_pop_min (pqueue *pq)
{
  return pop_min (pq);
}

size_t
pq_size (pqueue *const pq)
{
  return size (pq);
}

/* =========================================================================
   =======================        Set Interface       ======================
   ========================================================================= */

void
set_init (set *s)
{
  init_tree (s);
}

bool
set_empty (set *s)
{
  return empty (s);
}

size_t
set_size (set *s)
{
  return size (s);
}

bool
set_contains (set *s, set_elem *se, set_cmp_fn *cmp, void *aux)
{
  return contains (s, se, cmp, aux);
}

bool
set_insert (set *s, set_elem *se, set_cmp_fn *cmp, void *aux)
{
  return insert (s, se, cmp, aux);
}

const set_elem *
set_end (const set *s)
{
  return end (s);
}

const set_elem *
set_find (set *s, set_elem *se, set_cmp_fn *cmp, void *aux)
{
  return find (s, se, cmp, aux);
}

set_elem *
set_erase (set *s, set_elem *se, set_cmp_fn *cmp, void *aux)
{
  return erase (s, se, cmp, aux);
}

set_elem *
set_root (set *s)
{
  return root (s);
}

/* =========================================================================
   =============    Splay Tree Set and Set Implementations    ==============
   =========================================================================

      (40)0x7fffffffd5c8-0x7fffffffdac8(+1)
       ├──(29)R:0x7fffffffd968
       │   ├──(12)R:0x7fffffffd5a8-0x7fffffffdaa8(+1)
       │   │   ├──(2)R:0x7fffffffd548-0x7fffffffda48(+1)
       │   │   │   └──(1)R:0x7fffffffd4e8-0x7fffffffd9e8(+1)
       │   │   └──(9)L:0x7fffffffd668
       │   │       ├──(1)R:0x7fffffffd608
       │   │       └──(7)L:0x7fffffffd7e8
       │   │           ├──(3)R:0x7fffffffd728
       │   │           │   ├──(1)R:0x7fffffffd6c8
       │   │           │   └──(1)L:0x7fffffffd788
       │   │           └──(3)L:0x7fffffffd8a8
       │   │               ├──(1)R:0x7fffffffd848
       │   │               └──(1)L:0x7fffffffd908
       │   └──(16)L:0x7fffffffd568-0x7fffffffda68(+1)
       │       └──(15)R:0x7fffffffd588-0x7fffffffda88(+1)
       │           ├──(2)R:0x7fffffffd528-0x7fffffffda28(+1)
       │           │   └──(1)R:0x7fffffffd4c8-0x7fffffffd9c8(+1)
       │           └──(12)L:0x7fffffffd508-0x7fffffffda08(+1)
       │               └──(11)R:0x7fffffffd828
       │                   ├──(6)R:0x7fffffffd6a8
       │                   │   ├──(2)R:0x7fffffffd5e8
       │                   │   │   └──(1)L:0x7fffffffd648
       │                   │   └──(3)L:0x7fffffffd768
       │                   │       ├──(1)R:0x7fffffffd708
       │                   │       └──(1)L:0x7fffffffd7c8
       │                   └──(4)L:0x7fffffffd8e8
       │                       ├──(1)R:0x7fffffffd888
       │                       └──(2)L:0x7fffffffd4a8-0x7fffffffd9a8(+1)
       │                           └──(1)R:0x7fffffffd948
       └──(10)L:0x7fffffffd688
           ├──(1)R:0x7fffffffd628
           └──(8)L:0x7fffffffd808
               ├──(3)R:0x7fffffffd748
               │   ├──(1)R:0x7fffffffd6e8
               │   └──(1)L:0x7fffffffd7a8
               └──(4)L:0x7fffffffd8c8
                   ├──(1)R:0x7fffffffd868
                   └──(2)L:0x7fffffffd928
                       └──(1)L:0x7fffffffd988

   Pictured above is the heavy/light decomposition of a splay tree.
   The goal of a splay tree is to take advantage of "good" edges
   that drop half the weight of the tree, weight being the number of
   nodes rooted at X. Blue edges are obviously advantageous so if we
   have a mathematical bound on the cost of those edges, a splay tree
   then amortizes the cost of the red edges, leaving a solid O(lgN) runtime.
   You can't see the color here but check out the printing function.

   All types that use a splay tree are simply wrapper interfaces around
   the core splay tree operations. Splay trees can be used as priority
   queues, sets, and probably much more but we can implement all the
   needed functionality here rather than multiple times for each
   data structure. Through the use of typedefs we only have to write the
   actual code once and then just hand out interfaces as needed.
*/

static void
init_tree (struct tree *t)
{
  assert (t != NULL);
  t->root = &t->nil;
  t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups = t->root;
  t->size = 0;
}

static void
init_node (struct tree *t, struct node *n)
{
  assert (n != NULL);
  assert (t != NULL);
  n->links[L] = &t->nil;
  n->links[R] = &t->nil;
  n->parent_or_dups = &t->nil;
}

static bool
empty (const struct tree *const t)
{
  assert (t != NULL);
  return 0 == t->size;
}

static struct node *
root (const struct tree *const t)
{
  assert (t != NULL);
  return t->root;
}

static struct node *
max (const struct tree *const t)
{
  assert (t != NULL);
  assert (!empty (t));
  struct node *m = t->root;
  for (; m->links[R] != &t->nil; m = m->links[R])
    ;
  return m;
}

static struct node *
min (const struct tree *t)
{
  assert (t != NULL);
  assert (!empty (t));
  struct node *m = t->root;
  for (; m->links[L] != &t->nil; m = m->links[L])
    ;
  return m;
}

static struct node *
pop_max (struct tree *t)
{
  return multiset_erase_max_or_min (t, &t->nil, force_find_grt, NULL);
}

static struct node *
pop_min (struct tree *t)
{
  return multiset_erase_max_or_min (t, &t->nil, force_find_les, NULL);
}

const static struct node *
end (const struct tree *t)
{
  return &t->nil;
}

static struct node *
find (struct tree *t, struct node *elem, tree_cmp_fn *cmp, void *aux)
{
  (void)aux;
  assert (t != NULL);
  assert (cmp != NULL);
  init_node (t, elem);
  t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups
      = splay (t, t->root, elem, cmp);
  return cmp (elem, t->root, NULL) == EQL ? t->root : &t->nil;
}

static bool
contains (struct tree *t, struct node *dummy_key, tree_cmp_fn *cmp, void *aux)
{
  (void)aux;
  assert (t != NULL);
  assert (cmp != NULL);
  init_node (t, dummy_key);
  t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups
      = splay (t, t->root, dummy_key, cmp);
  return cmp (dummy_key, t->root, NULL) == EQL;
}

static bool
insert (struct tree *t, struct node *elem, tree_cmp_fn *cmp, void *aux)
{
  (void)aux;
  assert (t != NULL);
  assert (cmp != NULL);
  init_node (t, elem);
  t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups
      = splay (t, t->root, elem, cmp);
  const threeway_cmp root_size = cmp (elem, t->root, NULL);
  if (EQL == root_size)
    return false;
  t->size++;
  return connect_new_root (t, elem, root_size);
}

static void
multiset_insert (struct tree *t, struct node *elem, tree_cmp_fn *cmp,
                 void *aux)
{
  (void)aux;
  assert (t);
  init_node (t, elem);
  t->size++;
  assert (t->size != 0);
  if (t->root == &t->nil)
    {
      t->root = elem;
      t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups = t->root;
      return;
    }
  t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups = t->root
      = splay (t, t->root, elem, cmp);
  const threeway_cmp root_size = cmp (elem, t->root, NULL);
  if (EQL == root_size)
    {
      if (stores_dups (t, t->root))
        t->root->parent_or_dups->parent_or_dups = &t->nil;
      else
        t->root->parent_or_dups = &t->nil;
      add_duplicate (t, t->root, elem, &t->nil);
      return;
    }
  t->root->parent_or_dups = &t->nil;
  (void)connect_new_root (t, elem, root_size);
}

static struct node *
connect_new_root (struct tree *t, struct node *new_root,
                  threeway_cmp cmp_result)
{
  enum tree_link link = GRT == cmp_result;
  give_parent_subtree (t, new_root, link, t->root->links[link]);
  give_parent_subtree (t, new_root, !link, t->root);
  t->root->links[link] = &t->nil;
  t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups
      = new_root;
  return new_root;
}

static void
add_duplicate (struct tree *t, struct node *tree_node, struct node *add,
               struct node *parent)
{
  /* This is a circular doubly linked list with O(1) append to back
     to maintain round robin fairness for any use of this queue.
     the oldest duplicate should be in the tree so we will add new dup
     to the back. The head then needs to point to new tail and new
     tail points to already in place head that tree points to.
     This operation still works if we previously had size 1 list. */
  if (!stores_dups (t, tree_node))
    {
      add->parent_or_dups = parent;
      tree_node->parent_or_dups = add;
      add->links[N] = add;
      add->links[P] = add;
      return;
    }
  add->parent_or_dups = NULL;
  struct node *list_head = tree_node->parent_or_dups;
  struct node *tail = list_head->links[P];
  tail->links[N] = add;
  list_head->links[P] = add;
  add->links[N] = list_head;
  add->links[P] = tail;
}

static struct node *
erase (struct tree *t, struct node *elem, tree_cmp_fn *cmp, void *aux)
{
  (void)aux;
  assert (t != NULL);
  assert (elem != NULL);
  assert (cmp != NULL);
  t->size--;
  assert (t->size != ((size_t)-1));
  struct node *ret = splay (t, t->root, elem, cmp);
  assert (ret != NULL);
  const threeway_cmp found = cmp (elem, ret, NULL);
  if (found != EQL)
    return &t->nil;
  t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups = ret;
  return remove_from_tree (t, ret, cmp);
}

/* We need to mindful of what the user is asking for. If they want any
   max or min, we have provided a dummy node and dummy compare function
   that will force us to return the max or min. So this operation
   simply grabs the first node available in the tree for round robin.
   This function expects to be passed the t->nil as the node and a
   comparison function that forces either the max or min to be searched. */
static struct node *
multiset_erase_max_or_min (struct tree *t, struct node *tnil,
                           tree_cmp_fn *force_max_or_min, void *aux)
{
  (void)aux;
  assert (t != NULL);
  assert (tnil != NULL);
  assert (force_max_or_min != NULL);
  t->size--;
  assert (t->size != ((size_t)-1));

  struct node *ret = splay (t, t->root, tnil, force_max_or_min);
  assert (ret != NULL);

  t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups = ret;
  if (stores_dups (t, ret))
    {
      ret->parent_or_dups->parent_or_dups = &t->nil;
      return pop_front_dup (t, ret, force_max_or_min);
    }
  t->root->parent_or_dups = &t->nil;
  return remove_from_tree (t, ret, force_max_or_min);
}

/* We need to mindful of what the user is asking for. This is a request
   to erase the exact node provided in the argument. So extra care is
   taken to only delete that node, especially if a different node with
   the same size is splayed to the root and we are a duplicate in the
   list. */
static struct node *
multiset_erase_node (struct tree *t, struct node *node, tree_cmp_fn *cmp,
                     void *aux)
{
  (void)aux;
  assert (t != NULL);
  assert (node != NULL);
  assert (cmp != NULL);
  t->size--;
  assert (t->size != ((size_t)-1));

  /* Special case that this must be a duplicate that is in the
     linked list but it is not the special head node. So, it
     is a quick snip to get it out. */
  if (NULL == node->parent_or_dups)
    {
      node->links[P]->links[N] = node->links[N];
      node->links[N]->links[P] = node->links[P];
      return node;
    }

  struct node *ret = splay (t, t->root, node, cmp);
  assert (ret != NULL);

  t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups = ret;
  if (stores_dups (t, ret))
    {
      ret->parent_or_dups->parent_or_dups = &t->nil;
      return pop_dup_node (t, node, cmp, ret);
    }
  t->root->parent_or_dups = &t->nil;
  return remove_from_tree (t, ret, cmp);
}

static struct node *
pop_dup_node (struct tree *t, struct node *dup, tree_cmp_fn *cmp,
              struct node *splayed)
{
  if (dup == splayed)
    return pop_front_dup (t, splayed, cmp);
  /* This is the head of the list of duplicates so no dups left. */
  if (dup->links[N] == dup)
    {
      splayed->parent_or_dups = &t->nil;
      return dup;
    }
  /* There is an arbitrary number of dups after the head so replace head */
  const struct node *head = dup;
  /* Update the tail at the back of the list. Easy to forget hard to catch. */
  head->links[P]->links[N] = head->links[N];
  head->links[N]->links[P] = head->links[P];
  head->links[N]->parent_or_dups = head->parent_or_dups;
  splayed->parent_or_dups = head->links[N];
  return dup;
}

static struct node *
pop_front_dup (struct tree *t, struct node *old, tree_cmp_fn *cmp)
{
  struct node *parent = old->parent_or_dups->parent_or_dups;
  struct node *tree_replacement = old->parent_or_dups;
  if (old == t->root)
    t->root = t->nil.links[R] = t->nil.links[L] = t->nil.parent_or_dups
        = tree_replacement;

  struct node *new_list_head = old->parent_or_dups->links[N];
  struct node *list_tail = old->parent_or_dups->links[P];
  /* Circular linked lists are tricky to detect when empty */
  const bool circular_list_empty = new_list_head->links[N] == new_list_head;

  new_list_head->links[P] = list_tail;
  new_list_head->parent_or_dups = parent;
  list_tail->links[N] = new_list_head;
  threeway_cmp size_relation = cmp (old, parent, NULL);
  parent->links[GRT == size_relation] = tree_replacement;
  tree_replacement->links[L] = old->links[L];
  tree_replacement->links[R] = old->links[R];
  tree_replacement->parent_or_dups = new_list_head;

  give_parent_subtree (t, tree_replacement, L, tree_replacement->links[L]);
  give_parent_subtree (t, tree_replacement, R, tree_replacement->links[R]);
  if (circular_list_empty)
    tree_replacement->parent_or_dups = parent;

  tree_replacement->links[L]->parent_or_dups = tree_replacement;
  tree_replacement->links[R]->parent_or_dups = tree_replacement;
  t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups = t->root;
  return old;
}

static struct node *
remove_from_tree (struct tree *t, struct node *ret, tree_cmp_fn *cmp)
{
  if (ret->links[L] == &t->nil)
    t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups
        = ret->links[R];
  else
    {
      t->root = t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups
          = splay (t, ret->links[L], ret, cmp);
      give_parent_subtree (t, t->root, R, ret->links[R]);
    }

  if (stores_dups (t, t->root))
    t->root->parent_or_dups->parent_or_dups = &t->nil;
  else
    t->root->parent_or_dups = &t->nil;

  t->nil.links[L] = t->nil.links[R] = t->nil.parent_or_dups = t->root;
  return ret;
}

static struct node *
splay (struct tree *t, struct node *root, const struct node *elem,
       tree_cmp_fn *cmp)
{
  /* Pointers in an array and we can use the symmetric enum and flip it to
     choose the Left or Right subtree. Another benefit of our nil node: use it
     as our helper tree because we don't need its Left Right fields. */
  t->nil.links[L] = &t->nil;
  t->nil.links[R] = &t->nil;
  t->nil.parent_or_dups = t->root;
  struct node *left_right_subtrees[2] = { &t->nil, &t->nil };
  struct node *finger = NULL;
  for (;;)
    {
      const threeway_cmp root_size = cmp (elem, root, NULL);
      const enum tree_link link_to_descend = GRT == root_size;
      if (EQL == root_size || root->links[link_to_descend] == &t->nil)
        break;

      const threeway_cmp child_size
          = cmp (elem, root->links[link_to_descend], NULL);
      const enum tree_link link_to_descend_from_child = GRT == child_size;
      if (EQL != child_size && link_to_descend == link_to_descend_from_child)
        {
          finger = root->links[link_to_descend];
          give_parent_subtree (t, root, link_to_descend,
                               finger->links[!link_to_descend]);
          give_parent_subtree (t, finger, !link_to_descend, root);
          root = finger;
          if (root->links[link_to_descend] == &t->nil)
            break;
        }
      give_parent_subtree (t, left_right_subtrees[!link_to_descend],
                           link_to_descend, root);
      left_right_subtrees[!link_to_descend] = root;
      root = root->links[link_to_descend];
    }
  give_parent_subtree (t, left_right_subtrees[L], R, root->links[L]);
  give_parent_subtree (t, left_right_subtrees[R], L, root->links[R]);
  give_parent_subtree (t, root, L, t->nil.links[R]);
  give_parent_subtree (t, root, R, t->nil.links[L]);
  return root;
}

/* This function has proven to be VERY important. The nil node often
   has garbage values associated with real nodes in our tree and if we access
   them by mistake it's bad! But the nil is also helpful for some invariant
   coding patters and reducing if checks all over the place. */
static inline void
give_parent_subtree (struct tree *t, struct node *parent, enum tree_link dir,
                     struct node *subtree)
{
  parent->links[dir] = subtree;
  if (stores_dups (t, subtree))
    subtree->parent_or_dups->parent_or_dups = parent;
  else if (subtree != &t->nil)
    subtree->parent_or_dups = parent;
}

static bool
stores_dups (const struct tree *t, const struct node *n)
{
  return (n != &t->nil)
         && (n->parent_or_dups->parent_or_dups->links[L] == n
             || n->parent_or_dups->parent_or_dups->links[R] == n);
}

size_t
count_dups (struct tree *t, struct node *n)
{
  if (!stores_dups (t, n))
    return 0;
  size_t dups = 1;
  /* TODO this function is broken because we loop infinitely 
     when we try to count duplicates insert or delete duplicates
     is broken somewhere! */
  for (struct node *cur = n->parent_or_dups->links[N];
       cur != n->parent_or_dups; cur = cur->links[N])
    ++dups;
  return dups;
}

/* NOLINTBEGIN(*misc-no-recursion) */

static size_t
recursive_size (struct tree *const t, struct node *r)
{
  if (r == &t->nil)
    return 0;
  size_t s = 1;
  if (stores_dups(t, r))
    s += count_dups(t, r); 
  return s + recursive_size(t, r->links[R]) + recursive_size(t, r->links[L]);
}

size_t
size (struct tree *const t)
{
  return recursive_size(t, t->root);
}

/* We can trick our splay tree into giving us the max via splaying
   without any input from the user. Our seach evaluates a threeway
   comparison to decide which branch to take in the tree or if we
   found the desired element. Simply force the function to always
   return one or the other and we will end up at the max or min
   NOLINTBEGIN(*swappable-parameters)*/
static threeway_cmp
force_find_grt (const struct node *a, const struct node *b,
                void *aux) // NOLINT
{
  (void)a;
  (void)b;
  (void)aux;
  return GRT;
}

static threeway_cmp
force_find_les (const struct node *a, const struct node *b,
                void *aux) // NOLINT
{
  (void)a;
  (void)b;
  (void)aux;
  return LES;
}

/* NOLINTEND(*swappable-parameters) */

/* =========================================================================
   =======================        Debugging           ======================
   ========================================================================= */

/* This section has recursion so it should probably not be used in
   a custom operating system environment with constrained stack space.
   Needless to mention the stdlib.h heap implementation that would need
   to be replaced with the custom OS drop in. */

static bool
strict_bound_met (const struct node *prev, enum tree_link dir,
                  const struct node *root, const struct node *nil,
                  tree_cmp_fn *cmp)
{
  if (root == nil)
    return true;
  threeway_cmp size_cmp = cmp (root, prev, NULL);
  if (dir == L && size_cmp != LES)
    return false;
  if (dir == R && size_cmp != GRT)
    return false;
  return strict_bound_met (root, L, root->links[L], nil, cmp)
         && strict_bound_met (root, R, root->links[R], nil, cmp);
}

static bool
are_subtrees_valid (const struct node *root, tree_cmp_fn *cmp,
                    const struct node *nil)
{
  if (root == nil)
    return true;
  if (root->links[R] == root || root->links[L] == root)
    return false;
  if (!strict_bound_met (root, L, root->links[L], nil, cmp)
      || !strict_bound_met (root, R, root->links[R], nil, cmp))
    return false;
  return are_subtrees_valid (root->links[L], cmp, nil)
         && are_subtrees_valid (root->links[R], cmp, nil);
}

static bool
is_duplicate_storing_parent (const struct tree *t, const struct node *parent,
                             const struct node *root)
{
  if (root == &t->nil)
    return true;
  if (stores_dups (t, root) && root->parent_or_dups->parent_or_dups != parent)
    return false;
  return is_duplicate_storing_parent (t, root, root->links[L])
         && is_duplicate_storing_parent (t, root, root->links[R]);
}

bool
validate_tree (struct tree *t, tree_cmp_fn *cmp)
{
  if (!are_subtrees_valid (t->root, cmp, &t->nil))
    return false;
  if (!is_duplicate_storing_parent (t, &t->nil, t->root))
    return false;
  return true;
}

static size_t
get_subtree_size (const struct node *root, const void *nil)
{
  if (root == nil)
    return 0;
  return 1 + get_subtree_size (root->links[L], nil)
         + get_subtree_size (root->links[R], nil);
}

static const char *
get_edge_color (const struct node *root, size_t parent_size,
                const struct node *nil)
{
  if (root == nil)
    return "";
  return get_subtree_size (root, nil) <= parent_size / 2 ? COLOR_BLU_BOLD
                                                         : COLOR_RED_BOLD;
}

static void
print_node (const struct tree *t, const struct node *root)
{
  printf ("%p", root);
  printf (COLOR_CYN);
  /* If a node is a duplicate, we will give it a special mark among nodes. */
  if (stores_dups (t, root))
    {
      int duplicates = 1;
      const struct node *head = root->parent_or_dups;
      if (head != &t->nil)
        {
          printf ("%p", head);
          for (struct node *i = head->links[N]; i != head;
               i = i->links[N], ++duplicates)
            printf ("-%p", i);
        }
      printf ("(+%d)", duplicates);
    }
  printf (COLOR_NIL);
  printf ("\n");
}

/* I know this function is rough but it's tricky to focus on edge color rather
   than node color. */
static void
print_inner_tree (const struct node *root, size_t parent_size,
                  const char *prefix, const char *prefix_branch_color,
                  const enum print_link node_type, const enum tree_link dir,
                  const struct tree *t)
{
  if (root == &t->nil)
    return;
  size_t subtree_size = get_subtree_size (root, &t->nil);
  printf ("%s", prefix);
  printf ("%s%s%s",
          subtree_size <= parent_size / 2 ? COLOR_BLU_BOLD : COLOR_RED_BOLD,
          node_type == LEAF ? " └──" : " ├──", COLOR_NIL);
  printf (COLOR_CYN);
  printf ("(%zu)", subtree_size);
  dir == L ? printf ("L:" COLOR_NIL) : printf ("R:" COLOR_NIL);
  print_node (t, root);

  char *str = NULL;
  int string_length
      = snprintf (NULL, 0, "%s%s%s", prefix, prefix_branch_color, // NOLINT
                  node_type == LEAF ? "     " : " │   ");
  if (string_length > 0)
    {
      str = malloc (string_length + 1);                     // NOLINT
      (void)snprintf (str, string_length, "%s%s%s", prefix, // NOLINT
                      prefix_branch_color,
                      node_type == LEAF ? "     " : " │   ");
    }
  if (str == NULL)
    {
      printf (COLOR_ERR "memory exceeded. Cannot display tree." COLOR_NIL);
      return;
    }

  const char *left_edge_color
      = get_edge_color (root->links[L], subtree_size, &t->nil);
  if (root->links[R] == &t->nil)
    print_inner_tree (root->links[L], subtree_size, str, left_edge_color, LEAF,
                      L, t);
  else if (root->links[L] == &t->nil)
    print_inner_tree (root->links[R], subtree_size, str, left_edge_color, LEAF,
                      R, t);
  else
    {
      print_inner_tree (root->links[R], subtree_size, str, left_edge_color,
                        BRANCH, R, t);
      print_inner_tree (root->links[L], subtree_size, str, left_edge_color,
                        LEAF, L, t);
    }
  free (str);
}

void
print_tree (const struct tree *t, const struct node *root)
{
  if (root == &t->nil)
    return;
  size_t subtree_size = get_subtree_size (root, &t->nil);
  printf ("%s(%zu)%s", COLOR_CYN, subtree_size, COLOR_NIL);
  print_node (t, root);

  const char *left_edge_color
      = get_edge_color (root->links[L], subtree_size, &t->nil);
  if (root->links[R] == &t->nil)
    print_inner_tree (root->links[L], subtree_size, "", left_edge_color, LEAF,
                      L, t);
  else if (root->links[L] == &t->nil)
    print_inner_tree (root->links[R], subtree_size, "", left_edge_color, LEAF,
                      R, t);
  else
    {
      print_inner_tree (root->links[R], subtree_size, "", left_edge_color,
                        BRANCH, R, t);
      print_inner_tree (root->links[L], subtree_size, "", left_edge_color,
                        LEAF, L, t);
    }
}

/* NOLINTEND(*misc-no-recursion) */
