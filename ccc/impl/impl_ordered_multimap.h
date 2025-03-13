#ifndef CCC_IMPL_ORDERED_MULTIMAP_H
#define CCC_IMPL_ORDERED_MULTIMAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private This node type will support more expressive implementations of a
multimap. The array of pointers is to support unifying left and right symmetric
cases. The union is only relevant the multimap. Duplicate values are placed in a
circular doubly linked list. The head of this doubly linked list is the oldest
duplicate (aka round robin). The node still in the tree then sacrifices its
parent field to track this head of all duplicates. The duplicate then uses its
parent field to track the parent of the node in the tree. Duplicates can be
detected by detecting a cycle in the tree (graph) that only uses child/branch
pointers, which is normally impossible in a binary tree. No additional flags or
bits are needed. Splay trees, the current underlying implementation, do not
support duplicate values by default and I have found trimming the tree with
these "fat" tree nodes holding duplicates can net a nice performance boost in
the pop operation for the multimap. This works thanks to type punning in C
combined with the types in the union having the same size so it's safe. This
would be undefined in C++. */
typedef struct ccc_ommap_elem_
{
    union
    {
        struct ccc_ommap_elem_ *branch_[2];
        struct ccc_ommap_elem_ *link_[2];
    };
    union
    {
        struct ccc_ommap_elem_ *parent_;
        struct ccc_ommap_elem_ *dup_head_;
    };
} ccc_ommap_elem_;

/** @private */
struct ccc_ommap_
{
    struct ccc_ommap_elem_ *root_;
    struct ccc_ommap_elem_ end_;
    ccc_alloc_fn *alloc_;
    ccc_key_cmp_fn *cmp_;
    void *aux_;
    ptrdiff_t size_;
    size_t elem_sz_;
    size_t node_elem_offset_;
    size_t key_offset_;
};

/** @private */
struct ccc_omultimap_entry_
{
    struct ccc_ommap_ *t_;
    struct ccc_ent_ entry_;
};

/** @private */
union ccc_ommap_entry_
{
    struct ccc_omultimap_entry_ impl_;
};

/*==========================  Private Interface  ============================*/

/** @private */
void *ccc_impl_omm_key_in_slot(struct ccc_ommap_ const *t, void const *slot);
/** @private */
ccc_ommap_elem_ *ccc_impl_ommap_elem_in_slot(struct ccc_ommap_ const *t,
                                             void const *slot);
/** @private */
struct ccc_omultimap_entry_ ccc_impl_omm_entry(struct ccc_ommap_ *t,
                                               void const *key);
/** @private */
void *ccc_impl_omm_multimap_insert(struct ccc_ommap_ *t, ccc_ommap_elem_ *n);

/*==========================   Initialization  ==============================*/

/** @private */
#define ccc_impl_omm_init(tree_name, struct_name, node_elem_field,             \
                          key_elem_field, key_cmp_fn, alloc_fn, aux_data)      \
    {                                                                          \
        .root_ = &(tree_name).end_,                                            \
        .end_ = {{.branch_ = {&(tree_name).end_, &(tree_name).end_}},          \
                 {.parent_ = &(tree_name).end_}},                              \
        .alloc_ = (alloc_fn),                                                  \
        .cmp_ = (key_cmp_fn),                                                  \
        .aux_ = (aux_data),                                                    \
        .size_ = 0,                                                            \
        .elem_sz_ = sizeof(struct_name),                                       \
        .node_elem_offset_ = offsetof(struct_name, node_elem_field),           \
        .key_offset_ = offsetof(struct_name, key_elem_field),                  \
    }

/*==================   Helper Macros for Repeated Logic     =================*/

/** @private */
#define ccc_impl_omm_new(ordered_map_entry)                                    \
    (__extension__({                                                           \
        void *omm_ins_alloc_ret_ = NULL;                                       \
        if ((ordered_map_entry)->t_->alloc_)                                   \
        {                                                                      \
            omm_ins_alloc_ret_                                                 \
                = (ordered_map_entry)                                          \
                      ->t_->alloc_(NULL, (ordered_map_entry)->t_->elem_sz_,    \
                                   (ordered_map_entry)->t_->aux_);             \
        }                                                                      \
        omm_ins_alloc_ret_;                                                    \
    }))

/** @private */
#define ccc_impl_omm_insert_key_val(ordered_map_entry, new_mem,                \
                                    lazy_key_value...)                         \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = ccc_impl_omm_multimap_insert(                            \
                (ordered_map_entry)->t_,                                       \
                ccc_impl_ommap_elem_in_slot((ordered_map_entry)->t_,           \
                                            new_mem));                         \
        }                                                                      \
    }))

/** @private */
#define ccc_impl_omm_insert_and_copy_key(                                      \
    omm_insert_entry, omm_insert_entry_ret, key, lazy_value...)                \
    (__extension__({                                                           \
        typeof(lazy_value) *omm_new_ins_base_                                  \
            = ccc_impl_omm_new((&omm_insert_entry));                           \
        omm_insert_entry_ret = (struct ccc_ent_){                              \
            .e_ = omm_new_ins_base_,                                           \
            .stats_ = CCC_ENTRY_INSERT_ERROR,                                  \
        };                                                                     \
        if (omm_new_ins_base_)                                                 \
        {                                                                      \
            *((typeof(lazy_value) *)omm_new_ins_base_) = lazy_value;           \
            *((typeof(key) *)ccc_impl_omm_key_in_slot(omm_insert_entry.t_,     \
                                                      omm_new_ins_base_))      \
                = key;                                                         \
            (void)ccc_impl_omm_multimap_insert(                                \
                omm_insert_entry.t_,                                           \
                ccc_impl_ommap_elem_in_slot(omm_insert_entry.t_,               \
                                            omm_new_ins_base_));               \
        }                                                                      \
    }))

/*=====================     Core Macro Implementations     ==================*/

/** @private */
#define ccc_impl_omm_and_modify_w(ordered_map_entry_ptr, type_name,            \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type omm_ent_ptr_ = (ordered_map_entry_ptr);                    \
        struct ccc_omultimap_entry_ omm_mod_ent_                               \
            = {.entry_ = {.stats_ = CCC_ENTRY_ARG_ERROR}};                     \
        if (omm_ent_ptr_)                                                      \
        {                                                                      \
            omm_mod_ent_ = omm_ent_ptr_->impl_;                                \
            if (omm_mod_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)               \
            {                                                                  \
                type_name *const T = omm_mod_ent_.entry_.e_;                   \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        omm_mod_ent_;                                                          \
    }))

/** @private */
#define ccc_impl_omm_or_insert_w(ordered_map_entry_ptr, lazy_key_value...)     \
    (__extension__({                                                           \
        __auto_type omm_or_ins_ent_ptr_ = (ordered_map_entry_ptr);             \
        typeof(lazy_key_value) *or_ins_ret_ = NULL;                            \
        if (omm_or_ins_ent_ptr_)                                               \
        {                                                                      \
            struct ccc_omultimap_entry_ *omm_or_ins_ent_                       \
                = &omm_or_ins_ent_ptr_->impl_;                                 \
            if (omm_or_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)          \
            {                                                                  \
                or_ins_ret_ = omm_or_ins_ent_->entry_.e_;                      \
            }                                                                  \
            else                                                               \
            {                                                                  \
                or_ins_ret_ = ccc_impl_omm_new(omm_or_ins_ent_);               \
                ccc_impl_omm_insert_key_val(omm_or_ins_ent_, or_ins_ret_,      \
                                            lazy_key_value);                   \
            }                                                                  \
        }                                                                      \
        or_ins_ret_;                                                           \
    }))

/** @private */
#define ccc_impl_omm_insert_entry_w(ordered_map_entry_ptr, lazy_key_value...)  \
    (__extension__({                                                           \
        __auto_type omm_ins_ent_ptr_ = (ordered_map_entry_ptr);                \
        typeof(lazy_key_value) *omm_ins_ent_ret_ = NULL;                       \
        if (omm_ins_ent_ptr_)                                                  \
        {                                                                      \
            struct ccc_omultimap_entry_ *omm_ins_ent_                          \
                = &omm_ins_ent_ptr_->impl_;                                    \
            omm_ins_ent_ret_ = ccc_impl_omm_new(omm_ins_ent_);                 \
            ccc_impl_omm_insert_key_val(omm_ins_ent_, omm_ins_ent_ret_,        \
                                        lazy_key_value);                       \
        }                                                                      \
        omm_ins_ent_ret_;                                                      \
    }))

/** @private */
#define ccc_impl_omm_try_insert_w(ordered_map_ptr, key, lazy_value...)         \
    (__extension__({                                                           \
        __auto_type omm_try_ins_ptr_ = (ordered_map_ptr);                      \
        struct ccc_ent_ omm_try_ins_ent_ret_                                   \
            = {.stats_ = CCC_ENTRY_ARG_ERROR};                                 \
        if (omm_try_ins_ptr_)                                                  \
        {                                                                      \
            __auto_type omm_key_ = (key);                                      \
            struct ccc_omultimap_entry_ omm_try_ins_ent_                       \
                = ccc_impl_omm_entry(omm_try_ins_ptr_, (void *)&omm_key_);     \
            if (!(omm_try_ins_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED))        \
            {                                                                  \
                ccc_impl_omm_insert_and_copy_key(omm_try_ins_ent_,             \
                                                 omm_try_ins_ent_ret_,         \
                                                 omm_key_, lazy_value);        \
            }                                                                  \
            else if (omm_try_ins_ent_.entry_.stats_ == CCC_ENTRY_OCCUPIED)     \
            {                                                                  \
                omm_try_ins_ent_ret_ = omm_try_ins_ent_.entry_;                \
            }                                                                  \
        }                                                                      \
        omm_try_ins_ent_ret_;                                                  \
    }))

/** @private */
#define ccc_impl_omm_insert_or_assign_w(ordered_map_ptr, key, lazy_value...)   \
    (__extension__({                                                           \
        __auto_type omm_ins_or_assign_ptr_ = (ordered_map_ptr);                \
        struct ccc_ent_ omm_ins_or_assign_ent_ret_                             \
            = {.stats_ = CCC_ENTRY_ARG_ERROR};                                 \
        if (omm_ins_or_assign_ptr_)                                            \
        {                                                                      \
            __auto_type omm_key_ = (key);                                      \
            struct ccc_omultimap_entry_ omm_ins_or_assign_ent_                 \
                = ccc_impl_omm_entry(omm_ins_or_assign_ptr_,                   \
                                     (void *)&omm_key_);                       \
            if (!(omm_ins_or_assign_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED))  \
            {                                                                  \
                ccc_impl_omm_insert_and_copy_key(omm_ins_or_assign_ent_,       \
                                                 omm_ins_or_assign_ent_ret_,   \
                                                 omm_key_, lazy_value);        \
            }                                                                  \
            else if (omm_ins_or_assign_ent_.entry_.stats_                      \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_ommap_elem_ ins_ent_saved_                          \
                    = *ccc_impl_ommap_elem_in_slot(                            \
                        omm_ins_or_assign_ent_.t_,                             \
                        omm_ins_or_assign_ent_.entry_.e_);                     \
                *((typeof(lazy_value) *)omm_ins_or_assign_ent_.entry_.e_)      \
                    = lazy_value;                                              \
                *ccc_impl_ommap_elem_in_slot(omm_ins_or_assign_ent_.t_,        \
                                             omm_ins_or_assign_ent_.entry_.e_) \
                    = ins_ent_saved_;                                          \
                omm_ins_or_assign_ent_ret_ = omm_ins_or_assign_ent_.entry_;    \
                *((typeof(omm_key_) *)ccc_impl_omm_key_in_slot(                \
                    omm_ins_or_assign_ptr_, omm_ins_or_assign_ent_ret_.e_))    \
                    = omm_key_;                                                \
            }                                                                  \
        }                                                                      \
        omm_ins_or_assign_ent_ret_;                                            \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_ORDERED_MULTIMAP_H */
