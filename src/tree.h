/* Author: Alexander Lopez
   -----------------------
   These are the base types that all interfaces typedef to implement
   specialized data structures. Because everything is based on a tree
   of some sort, we provide those types here and implement the core
   functionality as if we were crafting a tree with set and multi
   set abilities. Then we typdedef other interfaces and the user
   can include those instead of remembering how to use the tree the
   correct way for their data structure. */
#ifndef TREE
#define TREE

#include "attrib.h"

#include <stdbool.h>
#include <stddef.h>

/* Instead of thinking about left and right consider only links
   in the abstract sense. Put them in an array and then flip
   this enum and left and right code paths can be united into one */
enum tree_link
{
    L = 0,
    R = 1
};

/* Trees are just a different interpretation of the same links used
   for doubly linked lists. We take advantage of this for duplicates. */
enum list_link
{
    P = 0,
    N = 1
};

/* The core node of the underlying tree implementation. Using an array
   for the nodes allows symmetric left/right cases to always be united
   into one code path to reduce code bloat and bugs. The parent field
   is required to track duplicates and would not be strictly necessary
   in some interpretations of a multiset. However, the parent field
   gives this implementation flexibility for duplicates, speed, and a
   robust iterator for users. This is important for a priority queue. */
struct node
{
    struct node *link[2];
    struct node *parent_or_dups;
};

/* All queries must be able to compare two types utilizing the tree.
   Equality is important for duplicate tracking and speed. */
typedef enum
{
    NODE_LES = -1,
    NODE_EQL = 0,
    NODE_GRT = 1
} node_threeway_cmp;

/* To implement three way comparison in C you can try something
   like this:

     return (a > b) - (a < b);

   If such a comparison is not possible for your type you can simply
   return the value of the cmp enum directly with conditionals switch
   statements or whatever other comparison logic you choose. */
typedef node_threeway_cmp tree_cmp_fn(struct node const *key,
                                      struct node const *n, void *aux);

/* The size field is not strictly necessary but seems to be standard
   practice for these types of containers for O(1) access. The end is
   critical for this implementation, especially iterators. */
struct tree
{
    struct node *root;
    struct node end;
    tree_cmp_fn *cmp;
    void *aux;
    size_t size;
};

/* The underlying tree range can serve as both an inorder and reverse
   inorder traversal. The provided splaytree implementation shall be
   generic enough to allow the calling API data structures to specify
   the traversal orders. */
struct range
{
    struct node *const begin ATTRIB_PRIVATE;
    struct node *const end ATTRIB_PRIVATE;
};

struct rrange
{
    struct node *const rbegin ATTRIB_PRIVATE;
    struct node *const end ATTRIB_PRIVATE;
};

typedef void node_print_fn(struct node const *);

#define TREE_INIT(TREE_NAME, CMP, AUX)                                         \
    {                                                                          \
        .root = &(TREE_NAME).t.end,                                            \
        .end = {.link = {&(TREE_NAME).t.end, &(TREE_NAME).t.end},              \
                .parent_or_dups = &(TREE_NAME).t.end},                         \
        .cmp = (tree_cmp_fn *)(CMP), .aux = (AUX), .size = 0                   \
    }

/* Mostly intended for debugging. Validates the underlying tree
   data structure with invariants that must hold regardless of
   interface. */
bool validate_tree(struct tree const *t);

/* Use this function in gdb or a terminal for some pretty colors.
   Intended for debugging use. */
void print_tree(struct tree const *t, struct node const *root,
                node_print_fn *fn);

#endif
