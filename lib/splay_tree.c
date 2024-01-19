#include "pqueue.h"
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

/* ======================= Function Prototypes  =========================== */

static void init_tree (struct tree *);
static void init_node (struct tree *, struct node *);
static bool empty (const struct tree *);
static void multiset_insert (struct tree *, struct node *, tree_cmp_fn *,
                             void *);
static struct node *multiset_erase (struct tree *, struct node *,
                                    tree_cmp_fn *, void *);
static struct node *delete_oldest_duplicate (struct tree *, struct node *,
                                             tree_cmp_fn *);
static struct node *root (const struct tree *);
static struct node *max (const struct tree *);
static struct node *pop_max (struct tree *);
static struct node *pop_min (struct tree *);
static struct node *min (const struct tree *);
static threeway_cmp force_find_grt (const struct node *, const struct node *,
                                    void *);
static threeway_cmp force_find_les (const struct node *, const struct node *,
                                    void *);
static size_t size (struct tree *);
static void give_parent_subtree (struct tree *, struct node *, enum tree_link,
                                 struct node *);
static void add_duplicate (struct tree *, struct node *, struct dupnode *,
                           struct node *);
static struct node *splay (struct tree *, struct node *, const struct node *,
                           tree_cmp_fn *);

/* =======================  Priority Queue Interface  ====================== */

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
  return multiset_erase (pq, elem, fn, aux);
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

/* =============  Splay Tree Set and Multiset Implementation  =============

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
  t->root = &t->nil;
  (void)t;
}

static void
init_node (struct tree *t, struct node *n)
{
  assert (n);
  n->links[L] = &t->nil;
  n->links[R] = &t->nil;
  n->dups = as_dupnode (&t->nil);
  (void)n;
}

static bool
empty (const struct tree *const t)
{
  assert (t != NULL);
  return t->root == &t->nil;
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
  return multiset_erase (t, &t->nil, force_find_grt, NULL);
}

static struct node *
pop_min (struct tree *t)
{
  return multiset_erase (t, &t->nil, force_find_les, NULL);
}

static void
multiset_insert (struct tree *t, struct node *elem, tree_cmp_fn *cmp,
                 void *aux)
{
  (void)aux;
  assert (t);
  init_node (t, elem);

  if (t->root == &t->nil)
    {
      t->root = elem;
      return;
    }
  t->root = splay (t, t->root, elem, cmp);
  const threeway_cmp root_size = cmp (elem, t->root, NULL);
  if (EQL == root_size)
    {
      if (t->root->dups != as_dupnode (&t->nil))
        t->root->dups->parent = &t->nil;
      add_duplicate (t, t->root, as_dupnode (elem), &t->nil);
      return;
    }
  enum tree_link link = GRT == root_size;
  give_parent_subtree (t, elem, link, t->root->links[link]);
  give_parent_subtree (t, elem, !link, t->root);
  t->root->links[link] = &t->nil;
  t->root = elem;
}

static void
add_duplicate (struct tree *t, struct node *tree_node, struct dupnode *add,
               struct node *parent)
{
  /* This is a circular doubly linked list with O(1) append to back
     to maintain round robin fairness for any use of this queue.
     the oldest duplicate should be in the tree so we will add new dup
     to the back. Get the last element and link its new next to new dup,
     link the head to this new tail. New elem wraps back to head.
     This operation still works if we previously had size 1 list. */
  if (tree_node->dups == as_dupnode (&t->nil))
    {
      add->parent = parent;
      tree_node->dups = add;
      add->links[N] = add;
      add->links[P] = add;
      return;
    }
  add->parent = NULL;
  struct dupnode *list_head = tree_node->dups;
  struct dupnode *tail = list_head->links[P];
  tail->links[N] = add;
  list_head->links[P] = add;
  add->links[N] = list_head;
  add->links[P] = tail;
}

static struct node *
multiset_erase (struct tree *t, struct node *node, tree_cmp_fn *cmp, void *aux)
{
  (void)aux;
  struct node *ret = splay (t, t->root, node, cmp);
  assert (ret != NULL);

  t->root = ret;
  if (ret->dups != as_dupnode (&t->nil))
    {
      ret->dups->parent = &t->nil;
      return delete_oldest_duplicate (t, ret, cmp);
    }

  if (ret->links[L] == &t->nil)
    t->root = ret->links[R];
  else
    {
      t->root = splay (t, ret->links[L], node, cmp);
      give_parent_subtree (t, t->root, R, ret->links[R]);
    }
  if (t->root != &t->nil && t->root->dups != as_dupnode (&t->nil))
    t->root->dups->parent = &t->nil;
  return ret;
}

static struct node *
delete_oldest_duplicate (struct tree *t, struct node *old, tree_cmp_fn *cmp)
{
  struct node *parent = old->dups->parent;
  struct node *tree_replacement = as_node (old->dups);
  if (old == t->root)
    t->root = tree_replacement;

  struct dupnode *new_list_head = old->dups->links[N];
  struct dupnode *list_tail = old->dups->links[P];
  new_list_head->links[P] = list_tail;
  new_list_head->parent = parent;
  list_tail->links[N] = new_list_head;
  threeway_cmp size_relation = cmp (old, parent, NULL);
  parent->links[GRT == size_relation] = tree_replacement;
  tree_replacement->links[L] = old->links[L];
  tree_replacement->links[R] = old->links[R];
  tree_replacement->dups = new_list_head;
  return old;
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
  t->nil.dups = as_dupnode (&t->nil);
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
  if (subtree != &t->nil && subtree->dups != as_dupnode (&t->nil))
    subtree->dups->parent = parent;
}

size_t
count_dups (struct tree *t, struct node *n)
{
  if (n->dups == as_dupnode (&t->nil))
    return 0;
  size_t dups = 1;
  for (struct dupnode *cur = n->dups->links[N]; cur != n->dups;
       cur = cur->links[N])
    ++dups;
  return dups;
}

/* Simple Morris iterative traversal. The only downside is that we
   modify the links of the tree while iterating. Incorrectly
   guarding from interrupts would be catastrophic. Perhaps we
   could keep a size of the queue if needed for O(1) checks but
   we don't need size that often with a priority queue. Also
   we need to save on stack space so recursion is out for user
   facing library.

   Iterate through the tree by sending a node to the inorder predecessor in the
   left subtree. The inorder predecessor creates a link back up to the root we
   dispatched the search from. When the root reaches a leaf add the value and
   use the link to go back up the tree. On the second pass with the inorder
   predecessor we will know it is time to clean up because the inorder
   predecessor is now pointing back to the traversal root. Add the value at
   root because this is our last chance before descending into the right
   subtree. Send the root traversal node into the right subtree. When the root
   can go right no longer, we are done. :) */
size_t
size (struct tree *const t)
{
  assert (t);
  struct node *iter = t->root;
  struct node *inorder_pred = NULL;
  size_t s = 0;
  while (iter != &t->nil)
    {
      if (&t->nil == iter->links[L])
        {
          /* This is where we climb back up a link if it exists */
          s += count_dups (t, iter) + 1;
          iter = iter->links[R];
          continue;
        }
      inorder_pred = iter->links[L];
      while (&t->nil != inorder_pred->links[R]
             && iter != inorder_pred->links[R])
        inorder_pred = inorder_pred->links[R];
      if (&t->nil == inorder_pred->links[R])
        {
          /* The right field is a temporary traversal helper. */
          inorder_pred->links[R] = iter;
          iter = iter->links[L];
          continue;
        }
      /* Here is our last chance to count this value. */
      s += count_dups (t, iter) + 1;
      /* Here is our last chance to repair our wreckage */
      inorder_pred->links[R] = &t->nil;
      /* This is how we get to a right subtree if any exists. */
      iter = iter->links[R];
    }
  return s;
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

/* NOLINTEND(*swappable-parameters) NOLINTBEGIN(*misc-no-recursion) */

/* This section has recursion so it should probably not be used in
   a custom operating system environment with constrained stack space.
   Needless to mention the stdlib.h heap implementation that would need
   to be replaced with the custom drop in. */

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
is_duplicate_storing_parent (const struct node *parent,
                             const struct node *root, const void *nil_and_tail)
{
  if (root == nil_and_tail)
    return true;
  if (root->dups != nil_and_tail && root->dups->parent != parent)
    return false;
  return is_duplicate_storing_parent (root, root->links[L], nil_and_tail)
         && is_duplicate_storing_parent (root, root->links[R], nil_and_tail);
}

bool
validate_tree (struct tree *t, tree_cmp_fn *cmp)
{
  if (!are_subtrees_valid (t->root, cmp, &t->nil))
    return false;
  if (!is_duplicate_storing_parent (&t->nil, t->root, &t->nil))
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
print_node (const struct node *root, const void *nil_and_tail)
{
  printf ("%p", root);
  printf (COLOR_CYN);
  /* If a node is a duplicate, we will give it a special mark among nodes. */
  if (root->dups != nil_and_tail)
    {
      int duplicates = 1;
      const struct dupnode *head = root->dups;
      if (head != nil_and_tail)
        {
          printf ("%p", head);
          for (struct dupnode *i = head->links[N]; i != head;
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
                  const struct node *nil)
{
  if (root == nil)
    return;
  size_t subtree_size = get_subtree_size (root, nil);
  printf ("%s", prefix);
  printf ("%s%s%s",
          subtree_size <= parent_size / 2 ? COLOR_BLU_BOLD : COLOR_RED_BOLD,
          node_type == LEAF ? " └──" : " ├──", COLOR_NIL);
  printf (COLOR_CYN);
  printf ("(%zu)", subtree_size);
  dir == L ? printf ("L:" COLOR_NIL) : printf ("R:" COLOR_NIL);
  print_node (root, nil);

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
      = get_edge_color (root->links[L], subtree_size, nil);
  if (root->links[R] == nil)
    print_inner_tree (root->links[L], subtree_size, str, left_edge_color, LEAF,
                      L, nil);
  else if (root->links[L] == nil)
    print_inner_tree (root->links[R], subtree_size, str, left_edge_color, LEAF,
                      R, nil);
  else
    {
      print_inner_tree (root->links[R], subtree_size, str, left_edge_color,
                        BRANCH, R, nil);
      print_inner_tree (root->links[L], subtree_size, str, left_edge_color,
                        LEAF, L, nil);
    }
  free (str);
}

void
print_tree (const struct node *root, const void *nil_and_tail)
{
  if (root == nil_and_tail)
    return;
  size_t subtree_size = get_subtree_size (root, nil_and_tail);
  printf ("%s(%zu)%s", COLOR_CYN, subtree_size, COLOR_NIL);
  print_node (root, nil_and_tail);

  const char *left_edge_color
      = get_edge_color (root->links[L], subtree_size, nil_and_tail);
  if (root->links[R] == nil_and_tail)
    print_inner_tree (root->links[L], subtree_size, "", left_edge_color, LEAF,
                      L, nil_and_tail);
  else if (root->links[L] == nil_and_tail)
    print_inner_tree (root->links[R], subtree_size, "", left_edge_color, LEAF,
                      R, nil_and_tail);
  else
    {
      print_inner_tree (root->links[R], subtree_size, "", left_edge_color,
                        BRANCH, R, nil_and_tail);
      print_inner_tree (root->links[L], subtree_size, "", left_edge_color,
                        LEAF, L, nil_and_tail);
    }
}

/* NOLINTEND(*misc-no-recursion) */
