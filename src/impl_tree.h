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

#define CCC_OM_EMPTY 0
#define CCC_OM_ENTRY_VACANT 0x0
#define CCC_OM_ENTRY_OCCUPIED 0x1
#define CCC_OM_ENTRY_INSERT_ERROR 0x2
#define CCC_OM_ENTRY_SEARCH_ERROR 0x4
#define CCC_OM_ENTRY_NULL 0x8
#define CCC_OM_ENTRY_DELETE_ERROR 0x10

/* The core node of the underlying tree implementation. Using an array
   for the nodes allows symmetric left/right cases to always be united
   into one code path to reduce code bloat and bugs. The parent field
   is required to track duplicates and would not be strictly necessary
   in some interpretations of a multiset. However, the parent field
   gives this implementation flexibility for duplicates, speed, and a
   robust iterator for users. This is important for a priority queue. */
typedef struct ccc_om_elem_
{
    struct ccc_om_elem_ *link_[2];
    struct ccc_om_elem_ *parent_or_dups_;
} ccc_om_elem_;

/* The size field is not strictly necessary but seems to be standard
   practice for these types of containers for O(1) access. The end is
   critical for this implementation, especially iterators. */
struct ccc_om_
{
    struct ccc_om_elem_ *root_;
    struct ccc_om_elem_ end_;
    ccc_alloc_fn *alloc_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
    size_t size_;
    size_t elem_sz_;
    size_t node_elem_offset_;
    size_t key_offset_;
};

struct ccc_om_entry_
{
    struct ccc_om_ *t_;
    struct ccc_entry_ entry_;
};

#define CCC_TREE_INIT(struct_name, node_elem_field, key_elem_field, tree_name, \
                      alloc_fn, key_cmp_fn, aux_data)                          \
    {                                                                          \
        .impl_ = {                                                             \
            .root_ = &(tree_name).impl_.end_,                                  \
            .end_                                                              \
            = {.link_ = {&(tree_name).impl_.end_, &(tree_name).impl_.end_},    \
               .parent_or_dups_ = &(tree_name).impl_.end_},                    \
            .alloc_ = (alloc_fn),                                              \
            .cmp_ = (key_cmp_fn),                                              \
            .aux_ = (aux_data),                                                \
            .size_ = 0,                                                        \
            .elem_sz_ = sizeof(struct_name),                                   \
            .node_elem_offset_ = offsetof(struct_name, node_elem_field),       \
            .key_offset_ = offsetof(struct_name, key_elem_field),              \
        },                                                                     \
    }

bool ccc_tree_validate(struct ccc_om_ const *t);
void ccc_tree_print(struct ccc_om_ const *t, ccc_om_elem_ const *root,
                    ccc_print_fn *fn);
void *ccc_impl_tree_key_in_slot(struct ccc_om_ const *t, void const *slot);
ccc_om_elem_ *ccc_impl_tree_elem_in_slot(struct ccc_om_ const *t,
                                         void const *slot);
void *ccc_impl_tree_key_from_node(struct ccc_om_ const *t,
                                  ccc_om_elem_ const *n);

struct ccc_om_entry_ ccc_impl_tree_entry(struct ccc_om_ *t, void const *key);
void *ccc_impl_tree_insert(struct ccc_om_ *t, ccc_om_elem_ *n);

#define CCC_IMPL_TREE_ENTRY(ordered_map_ptr, key...)                           \
    ({                                                                         \
        __auto_type tree_key_ = key;                                           \
        struct ccc_om_entry_ tree_entry_                                       \
            = ccc_impl_tree_entry(&(ordered_map_ptr)->impl_, &tree_key_);      \
        tree_entry_;                                                           \
    })

#define CCC_IMPL_TREE_GET(ordered_map_ptr, key...)                             \
    ({                                                                         \
        __auto_type const tree_key_ = key;                                     \
        struct ccc_fhm_entry_ tree_get_ent_                                    \
            = ccc_impl_tree_entry(&(ordered_map_ptr)->impl_, &tree_key_);      \
        void const *tree_get_res_ = NULL;                                      \
        if (tree_get_ent_.entry_.stats_ & CCC_OM_ENTRY_OCCUPIED)               \
        {                                                                      \
            tree_get_res_ = tree_get_ent_.entry_.e_;                           \
        }                                                                      \
        tree_get_res_;                                                         \
    })

#define CCC_IMPL_TREE_GET_MUT(ordered_map_ptr, key...)                         \
    (void *)CCC_IMPL_TREE_GET(fhash_ptr, key)

#define CCC_IMPL_TREE_AND_MODIFY(ordered_map_entry, mod_fn)                    \
    ({                                                                         \
        struct ccc_om_entry_ tree_mod_ent_ = (ordered_map_entry)->impl_;       \
        if (tree_mod_ent_.entry_.stats_ & CCC_OM_ENTRY_OCCUPIED)               \
        {                                                                      \
            (mod_fn)((ccc_update){.container                                   \
                                  = (void *const)tree_mod_ent_.entry_.e_,      \
                                  .aux = NULL});                               \
        }                                                                      \
        tree_mod_ent_;                                                         \
    })

#define CCC_IMPL_TREE_AND_MODIFY_W(ordered_map_entry, mod_fn, aux_data)        \
    ({                                                                         \
        struct ccc_om_entry_ tree_mod_ent_ = (ordered_map_entry)->impl_;       \
        if (tree_mod_ent_.entry_.stats_ & CCC_OM_ENTRY_OCCUPIED)               \
        {                                                                      \
            __auto_type tree_aux_data_ = (aux_data);                           \
            (mod_fn)((ccc_update){.container                                   \
                                  = (void *const)tree_mod_ent_.entry_.e_,      \
                                  .aux = &tree_aux_data_});                    \
        }                                                                      \
        tree_mod_ent_;                                                         \
    })

#define CCC_IMPL_TREE_NEW_INSERT(ordered_map_entry, key_value...)              \
    ({                                                                         \
        void *tree_ins_alloc_ret_ = NULL;                                      \
        if ((ordered_map_entry)->t_->alloc_)                                   \
        {                                                                      \
            tree_ins_alloc_ret_                                                \
                = (ordered_map_entry)                                          \
                      ->t_->alloc_(NULL, (ordered_map_entry)->t_->elem_sz_);   \
            if (tree_ins_alloc_ret_)                                           \
            {                                                                  \
                *((typeof(key_value) *)tree_ins_alloc_ret_) = key_value;       \
                tree_ins_alloc_ret_ = ccc_impl_tree_insert(                    \
                    (ordered_map_entry)->t_,                                   \
                    ccc_impl_tree_elem_in_slot((ordered_map_entry)->t_,        \
                                               tree_ins_alloc_ret_));          \
            }                                                                  \
        }                                                                      \
        tree_ins_alloc_ret_;                                                   \
    })

#define CCC_IMPL_TREE_INSERT_ENTRY(ordered_map_entry, key_value...)            \
    ({                                                                         \
        struct ccc_om_entry_ *tree_ins_ent_ = &(ordered_map_entry)->impl_;     \
        void *tree_ins_ent_ret_ = NULL;                                        \
        if (!(tree_ins_ent_->entry_.stats_ & CCC_OM_ENTRY_OCCUPIED))           \
        {                                                                      \
            tree_ins_ent_ret_                                                  \
                = CCC_IMPL_TREE_NEW_INSERT(tree_ins_ent_, key_value);          \
        }                                                                      \
        else if (tree_ins_ent_->entry_.stats_ == CCC_OM_ENTRY_OCCUPIED)        \
        {                                                                      \
            struct ccc_om_elem_ ins_ent_saved_ = *ccc_impl_tree_elem_in_slot(  \
                tree_ins_ent_->t_, tree_ins_ent_->entry_.e_);                  \
            *((typeof(key_value) *)tree_ins_ent_->entry_.e_) = key_value;      \
            *ccc_impl_tree_elem_in_slot(tree_ins_ent_->t_,                     \
                                        tree_ins_ent_->entry_.e_)              \
                = ins_ent_saved_;                                              \
            tree_ins_ent_ret_ = (void *)tree_ins_ent_->entry_.e_;              \
        }                                                                      \
        tree_ins_ent_ret_;                                                     \
    })

#define CCC_IMPL_TREE_OR_INSERT(ordered_map_entry, key_value...)               \
    ({                                                                         \
        struct ccc_om_entry_ *tree_or_ins_ent_ = &(ordered_map_entry)->impl_;  \
        void *or_ins_ret_ = NULL;                                              \
        if (tree_or_ins_ent_->entry_.stats_ == CCC_OM_ENTRY_OCCUPIED)          \
        {                                                                      \
            or_ins_ret_ = (void *)tree_or_ins_ent_->entry_.e_;                 \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            or_ins_ret_                                                        \
                = CCC_IMPL_TREE_NEW_INSERT(tree_or_ins_ent_, key_value);       \
        }                                                                      \
        or_ins_ret_;                                                           \
    })

#endif /* CCC_IMPL_TREE_H */
