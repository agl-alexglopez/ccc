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
#include "types.h"

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

/* The size field is not strictly necessary but seems to be standard
   practice for these types of containers for O(1) access. The end is
   critical for this implementation, especially iterators. */
typedef struct
{
    ccc_node *root;
    ccc_node end;
    ccc_cmp_fn *cmp;
    void *aux;
    size_t size;
    size_t node_elem_offset;
} ccc_tree;

#define CCC_TREE_INIT(struct_name, set_elem_field, tree_name, cmp_fn,          \
                      aux_data)                                                \
    {                                                                          \
        .root = &(tree_name).t.end,                                            \
        .end = {.link = {&(tree_name).t.end, &(tree_name).t.end},              \
                .parent_or_dups = &(tree_name).t.end},                         \
        .cmp = (cmp_fn), .aux = (aux_data), .size = 0,                         \
        .node_elem_offset = offsetof(struct_name, set_elem_field),             \
    }

/* Mostly intended for debugging. Validates the underlying tree
   data structure with invariants that must hold regardless of
   interface. */
bool ccc_tree_validate(ccc_tree const *t);

/* Use this function in gdb or a terminal for some pretty colors.
   Intended for debugging use. */
void ccc_tree_print(ccc_tree const *t, ccc_node const *root, ccc_print_fn *fn);

#endif
