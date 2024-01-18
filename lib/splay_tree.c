#include "pqueue.h"
#include "tree.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

static void init (struct tree *);
static bool empty (const struct tree *);
static void multiset_insert (struct tree *, struct node *, tree_cmp_fn *,
                             void *);
static pq_elem *root (const struct tree *);
static size_t size (const struct tree *);
static void give_parent_subtree (struct tree *t, struct node *parent,
                                 enum tree_link dir, struct node *subtree);
static void add_duplicate (struct tree *t, struct node *tree_node,
                           struct dupnode *add, struct node *parent);
static struct node *splay (struct tree *t, struct node *root,
                           const struct node *elem, tree_cmp_fn *cmp);

/* Priority Queue Interface */

void
pq_init (pqueue *pq)
{
  init (pq);
}

bool
pq_empty (const pqueue *pq)
{
  return empty (pq);
}

pq_elem *
pq_root (const pqueue *pq)
{
  return root (pq);
}

void
pq_insert (pqueue *pq, pq_elem *elem, pq_cmp_fn *fn, void *aux)
{
  multiset_insert (pq, elem, fn, aux);
}

size_t
pq_size (const pqueue *pq)
{
  return size (pq);
}

/* All types that use a splay tree are simply wrapper interfaces around
   the core splay tree operations. Splay trees can be used as priority
   queues, sets, and probably much more but we can implement all the
   needed functionality here rather than multiple times for each
   data structure. */

static void
init (struct tree *t)
{
  t->root = t->nil;
  (void)t;
}

static bool
empty (const struct tree *t)
{
  assert (t != NULL);
  return t->root == t->nil;
}

static pq_elem *
root (const struct tree *t)
{
  assert (t != NULL);
  return t->root;
}

static void
multiset_insert (struct tree *t, struct node *elem, tree_cmp_fn *cmp,
                 void *aux)
{
  (void)aux;
  assert (t);
  if (t->root == t->nil)
    {
      elem->links[L] = t->nil;
      elem->links[R] = t->nil;
      elem->dups = as_dupnode (t->nil);
      t->root = elem;
      return;
    }
  t->root = splay (t, t->root, elem, cmp);
  const threeway_cmp root_size = cmp (elem, t->root, NULL);
  if (EQL == root_size)
    {
      if (t->root->dups != as_dupnode (t->nil))
        t->root->dups->parent = t->nil;
      add_duplicate (t, t->root, as_dupnode (elem), t->nil);
      return;
    }
  enum tree_link link = GRT == root_size;
  give_parent_subtree (t, elem, link, t->root->links[link]);
  give_parent_subtree (t, elem, !link, t->root);
  t->root->links[link] = t->nil;
  t->root = elem;
}

static void
add_duplicate (struct tree *t, struct node *tree_node, struct dupnode *add,
               struct node *parent)
{
  /* This is a circular doubly linked list with O(1) append to back
     to maintain round robin fairness for any use of this queue.
     the oldest duplicate should be in the tree so we will add new dup
     to the back. Get the last element and link its new next to new dup
     link the head to this new tail. New elem wraps back to head.
     This operation still works if we previously had size 1 list. */
  if (tree_node->dups == as_dupnode (t->nil))
    {
      add->parent = parent;
      tree_node->dups = add;
      add->links[N] = add;
      add->links[P] = add;
      return;
    }
  add->parent = NULL;
  struct dupnode *list_head = tree_node->dups;
  list_head->links[P]->links[N] = add;
  add->links[P] = list_head->links[P];
  list_head->links[P] = add;
  add->links[N] = list_head;
}

/* This function has proven to be VERY important. The nil node often
   has garbage values associated with real nodes in our tree and if we access
   them by mistake it's bad! */
static inline void
give_parent_subtree (struct tree *t, struct node *parent, enum tree_link dir,
                     struct node *subtree)
{
  parent->links[dir] = subtree;
  if (subtree != t->nil && subtree->dups != as_dupnode (t->nil))
    subtree->dups->parent = parent;
}

static struct node *
splay (struct tree *t, struct node *root, const struct node *elem,
       tree_cmp_fn *cmp)
{
  /* Pointers in an array and we can use the symmetric enum and flip it to
     choose the Left or Right subtree. Another benefit of our nil node: use it
     as our helper tree because we don't need its Left Right fields. */
  t->nil->links[L] = t->nil;
  t->nil->links[R] = t->nil;
  struct node *left_right_subtrees[2] = { t->nil, t->nil };
  struct node *finger = NULL;
  for (;;)
    {
      const threeway_cmp root_size = cmp (elem, root, NULL);
      const enum tree_link link_to_descend = GRT == root_size;
      if (EQL == root_size || root->links[link_to_descend] == t->nil)
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
          if (root->links[link_to_descend] == t->nil)
            break;
        }
      give_parent_subtree (t, left_right_subtrees[!link_to_descend],
                           link_to_descend, root);
      left_right_subtrees[!link_to_descend] = root;
      root = root->links[link_to_descend];
    }
  give_parent_subtree (t, left_right_subtrees[L], R, root->links[L]);
  give_parent_subtree (t, left_right_subtrees[R], L, root->links[R]);
  give_parent_subtree (t, root, L, t->nil->links[R]);
  give_parent_subtree (t, root, R, t->nil->links[L]);
  return root;
}

/* Simple Morris iterative traversal. The only downside is that we
   modify the links of the tree while iterating. Incorrectly,
   guarding from interrupts would be catastrophic. Perhaps we
   could keep a size of the queue if needed for O(1) checks but
   we don't need size that often with a priority queue. Also
   we need to save on stack space so recursion is out.

   Iterate through the tree by sending a node to the inorder predecessor in the
   left subtree. The inorder predecessor creates a link back up to the root we
   dispatched the search from. When the root reaches a leaf add the value and
   use the link to go back up the tree. On the second pass with the inorder
   predecessor we will know it is time to clean up because the inorder
   predecessor is now pointing back to the traversal root. Add the value at
   root because this is our last chance before descending into the right
   subtree. Send the root traversal node into the right subtree. When the root
   can go right no longer, we are done. :)
*/
size_t
size (const pqueue *pq)
{
  assert (pq);
  struct node *iter = pq->root;
  struct node *inorder_pred = iter;
  size_t s = 0;
  while (iter != pq->nil)
    {
      if (pq->nil == pq->root->links[L])
        {
          /* This is where we climb back up a link if it exists */
          iter = iter->links[R];
          ++s;
          continue;
        }
      inorder_pred = iter->links[L];
      while (pq->nil != inorder_pred->links[R]
             && iter != inorder_pred->links[R])
        inorder_pred = inorder_pred->links[R];
      if (pq->nil == inorder_pred->links[R])
        {
          /* The right field is a temporary traversal helper. */
          inorder_pred->links[R] = iter;
          iter = iter->links[L];
          continue;
        }
      /* Here is our last chance to count this value. */
      ++s;
      /* Here is our last chance to repair our wreckage */
      inorder_pred->links[R] = pq->nil;
      /* This is how we get to a right subtree if any exists. */
      iter = iter->links[R];
    }
  return s;
}
