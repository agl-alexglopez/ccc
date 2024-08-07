/* Author: Alexander Lopez
   -----------------------
   These are the base types that all interfaces typedef to implement
   specialized data structures. Because everything is based on a tree
   of some sort, we provide those types here and implement the core
   functionality as if we were crafting a tree with ccc_set and multi
   ccc_set abilities. Then we typdedef other interfaces and the user
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
typedef enum ccc_tree_link
{
    L = 0,
    R = 1
} ccc_tree_link;

/* Trees are just a different interpretation of the same links used
   for doubly linked lists. We take advantage of this for duplicates. */
typedef enum ccc_list_link
{
    P = 0,
    N = 1
} ccc_list_link;

/* The core node of the underlying tree implementation. Using an array
   for the nodes allows symmetric left/right cases to always be united
   into one code path to reduce code bloat and bugs. The parent field
   is required to track duplicates and would not be strictly necessary
   in some interpretations of a multiset. However, the parent field
   gives this implementation flexibility for duplicates, speed, and a
   robust iterator for users. This is important for a priority queue. */
typedef struct ccc_node
{
    struct ccc_node *link[2];
    struct ccc_node *parent_or_dups;
} ccc_node;

/* All queries must be able to compare two types utilizing the tree.
   Equality is important for duplicate tracking and speed. */
typedef enum ccc_node_threeway_cmp
{
    CCC_NODE_LES = -1,
    CCC_NODE_EQL = 0,
    CCC_NODE_GRT = 1
} ccc_node_threeway_cmp;

/* To implement three way comparison in C you can try something
   like this:

     return (a > b) - (a < b);

   If such a comparison is not possible for your type you can simply
   return the value of the cmp enum directly with conditionals switch
   statements or whatever other comparison logic you choose. */
typedef ccc_node_threeway_cmp ccc_tree_cmp_fn(ccc_node const *key,
                                              ccc_node const *n, void *aux);

/* The size field is not strictly necessary but seems to be standard
   practice for these types of containers for O(1) access. The end is
   critical for this implementation, especially iterators. */
typedef struct ccc_tree
{
    ccc_node *root;
    ccc_node end;
    ccc_tree_cmp_fn *cmp;
    void *aux;
    size_t size;
} ccc_tree;

/* The underlying tree range can serve as both an inorder and reverse
   inorder traversal. The provided splaytree implementation shall be
   generic enough to allow the calling API data structures to specify
   the traversal orders. */
typedef struct ccc_range
{
    ccc_node *const begin ATTRIB_PRIVATE;
    ccc_node *const end ATTRIB_PRIVATE;
} ccc_range;

typedef struct ccc_rrange
{
    ccc_node *const rbegin ATTRIB_PRIVATE;
    ccc_node *const end ATTRIB_PRIVATE;
} ccc_rrange;

typedef void ccc_node_print_fn(ccc_node const *);

#define CCC_TREE_INIT(tree_name, cmp_fn, aux_data)                             \
    {                                                                          \
        .root = &(tree_name).t.end,                                            \
        .end = {.link = {&(tree_name).t.end, &(tree_name).t.end},              \
                .parent_or_dups = &(tree_name).t.end},                         \
        .cmp = (ccc_tree_cmp_fn *)(cmp_fn), .aux = (aux_data), .size = 0       \
    }

/* Mostly intended for debugging. Validates the underlying tree
   data structure with invariants that must hold regardless of
   interface. */
bool ccc_tree_validate(ccc_tree const *t);

/* Use this function in gdb or a terminal for some pretty colors.
   Intended for debugging use. */
void ccc_tree_print(ccc_tree const *t, ccc_node const *root,
                    ccc_node_print_fn *fn);

#endif
