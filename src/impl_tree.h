/* Author: Alexander Lopez
   -----------------------
   These are the base types that all interfaces typedef to implement
   specialized data structures. Because everything is based on a tree
   of some sort, we provide those types here and implement the core
   functionality as if we were crafting a tree with ccc_set and multi
   ccc_set abilities. Then we typdedef other interfaces and the user
   can include those instead of remembering how to use the tree the
   correct way for their data structure. */
#ifndef CCC_IMPL_TREE_H
#define CCC_IMPL_TREE_H

#include "types.h"

#include <stdbool.h>
#include <stddef.h>

#define CCC_S_EMPTY ((uint64_t)0)
#define CCC_S_ENTRY_VACANT ((uint8_t)0x0)
#define CCC_S_ENTRY_OCCUPIED ((uint8_t)0x1)
#define CCC_S_ENTRY_INSERT_ERROR ((uint8_t)0x2)
#define CCC_S_ENTRY_SEARCH_ERROR ((uint8_t)0x4)
#define CCC_S_ENTRY_NULL ((uint8_t)0x8)
#define CCC_S_ENTRY_DELETE_ERROR ((uint8_t)0x10)

/* Instead of thinking about left and right consider only links
   in the abstract sense. Put them in an array and then flip
   this enum and left and right code paths can be united into one */
typedef enum
{
    L = 0,
    R = 1
} ccc_tree_link;

/* Trees are just a different interpretation of the same links used
   for doubly linked lists. We take advantage of this for duplicates. */
typedef enum
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
    ccc_realloc_fn *alloc;
    ccc_key_cmp_fn *cmp;
    void *aux;
    size_t size;
    size_t elem_sz;
    size_t node_elem_offset;
    size_t key_offset;
} ccc_tree;

struct ccc_tree_entry
{
    ccc_tree *t;
    ccc_entry entry;
};

#define CCC_TREE_INIT(struct_name, node_elem_field, key_elem_field, tree_name, \
                      realloc_fn, key_cmp_fn, aux_data)                        \
    {                                                                          \
        .impl = {                                                              \
            .root = &(tree_name).impl.end,                                     \
            .end = {.link = {&(tree_name).impl.end, &(tree_name).impl.end},    \
                    .parent_or_dups = &(tree_name).impl.end},                  \
            .alloc = (realloc_fn),                                             \
            .cmp = (key_cmp_fn),                                               \
            .aux = (aux_data),                                                 \
            .size = 0,                                                         \
            .elem_sz = sizeof(struct_name),                                    \
            .node_elem_offset = offsetof(struct_name, node_elem_field),        \
            .key_offset = offsetof(struct_name, key_elem_field),               \
        },                                                                     \
    }

bool ccc_tree_validate(ccc_tree const *t);
void ccc_tree_print(ccc_tree const *t, ccc_node const *root, ccc_print_fn *fn);
void *ccc_impl_tree_key_in_slot(ccc_tree const *t, void const *slot);
ccc_node *ccc_impl_tree_elem_in_slot(ccc_tree const *t, void const *slot);
void *ccc_impl_key_from_node(ccc_tree const *t, ccc_node const *n);

#endif /* CCC_IMPL_TREE_H */
