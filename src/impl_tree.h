/* Author: Alexander Lopez
   -----------------------
   These are the base types that all interfaces typedef to implement
   specialized data structures. Because everything is based on a tree
   of some sort, we provide those types here and implement the core
   functionality as if we were crafting a tree with ccc_ordered_map and multi
   ccc_ordered_map abilities. Then we typdedef other interfaces and the user
   can include those instead of remembering how to use the tree the
   correct way for their data structure. */
#ifndef CCC_IMPL_TREE_H
#define CCC_IMPL_TREE_H

#include "types.h"

#include <stdbool.h>
#include <stddef.h>

#define CCC_TREE_ENTRY_VACANT 0x0
#define CCC_TREE_ENTRY_OCCUPIED 0x1
#define CCC_TREE_ENTRY_INSERT_ERROR 0x2
#define CCC_TREE_ENTRY_SEARCH_ERROR 0x4
#define CCC_TREE_ENTRY_NULL 0x8
#define CCC_TREE_ENTRY_DELETE_ERROR 0x10

/* This node type will support more expressive implementations of a standard
   map and double ended priority queue. The array of pointers is to support
   unifying left and right symmetric cases. The union is only relevant to
   the double ended priority queue code. Duplicate values are placed in
   a circular doubly linked list. The head of this doubly linked list is
   the oldest duplicate (aka round robin). The node still in the tree then
   sacrifices its parent field to track this head of all duplicates. The
   duplicate then uses its parent field to track the parent of the node
   in the tree. Duplicates can be detected by detecting a cycle in the tree
   (graph) that only uses child/branch pointers, which is normally impossible
   in a binary tree. No additional flags or bits are needed. Splay trees, the
   current underlying implementation, do not support duplicate values by
   default and I have found trimming the tree with these "fat" tree nodes
   holding duplicates can net a nice performance boost in the pop operation for
   the priority queue. */
typedef struct ccc_node_
{
    union {
        struct ccc_node_ *parent_;
        struct ccc_node_ *dup_head_;
    };
    union {
        struct ccc_node_ *branch_[2];
        struct ccc_node_ *link_[2];
    };
} ccc_node_;

struct ccc_tree_
{
    struct ccc_node_ *root_;
    struct ccc_node_ end_;
    ccc_alloc_fn *alloc_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
    size_t size_;
    size_t elem_sz_;
    size_t node_elem_offset_;
    size_t key_offset_;
};

struct ccc_tree_entry_
{
    struct ccc_tree_ *t_;
    struct ccc_entry_ entry_;
};

#define CCC_TREE_INIT(struct_name, node_elem_field, key_elem_field, tree_name, \
                      alloc_fn, key_cmp_fn, aux_data)                          \
    {                                                                          \
        .impl_ = {                                                             \
            .root_ = &(tree_name).impl_.end_,                                  \
            .end_ = {{.parent_ = &(tree_name).impl_.end_},                     \
                     {.branch_                                                 \
                      = {&(tree_name).impl_.end_, &(tree_name).impl_.end_}}},  \
            .alloc_ = (alloc_fn),                                              \
            .cmp_ = (key_cmp_fn),                                              \
            .aux_ = (aux_data),                                                \
            .size_ = 0,                                                        \
            .elem_sz_ = sizeof(struct_name),                                   \
            .node_elem_offset_ = offsetof(struct_name, node_elem_field),       \
            .key_offset_ = offsetof(struct_name, key_elem_field),              \
        },                                                                     \
    }

#endif /* CCC_IMPL_TREE_H */
