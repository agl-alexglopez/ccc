#ifndef CCC_IMPL_ORDERED_MAP_H
#define CCC_IMPL_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl_tree.h"

/** @private */
union ccc_ordered_map_
{
    struct ccc_tree_ impl_;
};

/** @private */
union ccc_omap_elem_
{
    ccc_node_ impl_;
};

/** @private */
union ccc_omap_entry_
{
    struct ccc_tree_entry_ impl_;
};

#define ccc_impl_om_init(tree_name, struct_name, node_elem_field,              \
                         key_elem_field, alloc_fn, key_cmp_fn, aux_data)       \
    ccc_tree_init(tree_name, struct_name, node_elem_field, key_elem_field,     \
                  alloc_fn, key_cmp_fn, aux_data)

void *ccc_impl_om_key_in_slot(struct ccc_tree_ const *t, void const *slot);
ccc_node_ *ccc_impl_omap_elem_in_slot(struct ccc_tree_ const *t,
                                      void const *slot);
void *ccc_impl_om_key_from_node(struct ccc_tree_ const *t, ccc_node_ const *n);

struct ccc_tree_entry_ ccc_impl_om_entry(struct ccc_tree_ *t, void const *key);
void *ccc_impl_om_insert(struct ccc_tree_ *t, ccc_node_ *n);

/* NOLINTBEGIN(readability-identifier-naming) */

/*==================   Helper Macros for Repeated Logic     =================*/

#define ccc_impl_om_new(ordered_map_entry)                                     \
    (__extension__({                                                           \
        void *om_ins_alloc_ret_ = NULL;                                        \
        if ((ordered_map_entry)->t_->alloc_)                                   \
        {                                                                      \
            om_ins_alloc_ret_                                                  \
                = (ordered_map_entry)                                          \
                      ->t_->alloc_(NULL, (ordered_map_entry)->t_->elem_sz_,    \
                                   (ordered_map_entry)->t_->aux_);             \
        }                                                                      \
        om_ins_alloc_ret_;                                                     \
    }))

#define ccc_impl_om_insert_key_val(ordered_map_entry, new_mem,                 \
                                   lazy_key_value...)                          \
    (__extension__({                                                           \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = ccc_impl_om_insert(                                      \
                (ordered_map_entry)->t_,                                       \
                ccc_impl_omap_elem_in_slot((ordered_map_entry)->t_, new_mem)); \
        }                                                                      \
    }))

#define ccc_impl_om_insert_and_copy_key(om_insert_entry, om_insert_entry_ret,  \
                                        key, lazy_value...)                    \
    (__extension__({                                                           \
        typeof(lazy_value) *om_new_ins_base_                                   \
            = ccc_impl_om_new((&om_insert_entry));                             \
        om_insert_entry_ret = (struct ccc_ent_){                               \
            .e_ = om_new_ins_base_,                                            \
            .stats_ = CCC_ENTRY_INSERT_ERROR,                                  \
        };                                                                     \
        if (om_new_ins_base_)                                                  \
        {                                                                      \
            *((typeof(lazy_value) *)om_new_ins_base_) = lazy_value;            \
            *((typeof(key) *)ccc_impl_om_key_in_slot(om_insert_entry.t_,       \
                                                     om_new_ins_base_))        \
                = key;                                                         \
            (void)ccc_impl_om_insert(                                          \
                om_insert_entry.t_,                                            \
                ccc_impl_omap_elem_in_slot(om_insert_entry.t_,                 \
                                           om_new_ins_base_));                 \
        }                                                                      \
    }))

/*=====================     Core Macro Implementations     ==================*/

#define ccc_impl_om_and_modify_w(ordered_map_entry_ptr, type_name,             \
                                 closure_over_T...)                            \
    (__extension__({                                                           \
        __auto_type om_ent_ptr_ = (ordered_map_entry_ptr);                     \
        struct ccc_tree_entry_ om_mod_ent_                                     \
            = {.entry_ = {.stats_ = CCC_ENTRY_INPUT_ERROR}};                   \
        if (om_ent_ptr_)                                                       \
        {                                                                      \
            om_mod_ent_ = om_ent_ptr_->impl_;                                  \
            if (om_mod_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                \
            {                                                                  \
                type_name *const T = om_mod_ent_.entry_.e_;                    \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        om_mod_ent_;                                                           \
    }))

#define ccc_impl_om_or_insert_w(ordered_map_entry_ptr, lazy_key_value...)      \
    (__extension__({                                                           \
        __auto_type or_ins_entry_ptr_ = (ordered_map_entry_ptr);               \
        typeof(lazy_key_value) *or_ins_ret_ = NULL;                            \
        if (or_ins_entry_ptr_)                                                 \
        {                                                                      \
            struct ccc_tree_entry_ *om_or_ins_ent_                             \
                = &or_ins_entry_ptr_->impl_;                                   \
            if (om_or_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)           \
            {                                                                  \
                or_ins_ret_ = om_or_ins_ent_->entry_.e_;                       \
            }                                                                  \
            else                                                               \
            {                                                                  \
                or_ins_ret_ = ccc_impl_om_new(om_or_ins_ent_);                 \
                ccc_impl_om_insert_key_val(om_or_ins_ent_, or_ins_ret_,        \
                                           lazy_key_value);                    \
            }                                                                  \
        }                                                                      \
        or_ins_ret_;                                                           \
    }))

#define ccc_impl_om_insert_entry_w(ordered_map_entry_ptr, lazy_key_value...)   \
    (__extension__({                                                           \
        __auto_type ins_entry_ptr_ = (ordered_map_entry_ptr);                  \
        typeof(lazy_key_value) *om_ins_ent_ret_ = NULL;                        \
        if (ins_entry_ptr_)                                                    \
        {                                                                      \
            struct ccc_tree_entry_ *om_ins_ent_ = &ins_entry_ptr_->impl_;      \
            if (!(om_ins_ent_->entry_.stats_ & CCC_ENTRY_OCCUPIED))            \
            {                                                                  \
                om_ins_ent_ret_ = ccc_impl_om_new(om_ins_ent_);                \
                ccc_impl_om_insert_key_val(om_ins_ent_, om_ins_ent_ret_,       \
                                           lazy_key_value);                    \
            }                                                                  \
            else if (om_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)         \
            {                                                                  \
                struct ccc_node_ ins_ent_saved_ = *ccc_impl_omap_elem_in_slot( \
                    om_ins_ent_->t_, om_ins_ent_->entry_.e_);                  \
                *((typeof(lazy_key_value) *)om_ins_ent_->entry_.e_)            \
                    = lazy_key_value;                                          \
                *ccc_impl_omap_elem_in_slot(om_ins_ent_->t_,                   \
                                            om_ins_ent_->entry_.e_)            \
                    = ins_ent_saved_;                                          \
                om_ins_ent_ret_ = om_ins_ent_->entry_.e_;                      \
            }                                                                  \
        }                                                                      \
        om_ins_ent_ret_;                                                       \
    }))

#define ccc_impl_om_try_insert_w(ordered_map_ptr, key, lazy_value...)          \
    (__extension__({                                                           \
        __auto_type try_ins_map_ptr_ = (ordered_map_ptr);                      \
        struct ccc_ent_ om_try_ins_ent_ret_                                    \
            = {.stats_ = CCC_ENTRY_INPUT_ERROR};                               \
        if (try_ins_map_ptr_)                                                  \
        {                                                                      \
            __auto_type om_key_ = (key);                                       \
            struct ccc_tree_ *om_try_ins_map_ptr_ = &try_ins_map_ptr_->impl_;  \
            struct ccc_tree_entry_ om_try_ins_ent_                             \
                = ccc_impl_om_entry(om_try_ins_map_ptr_, (void *)&om_key_);    \
            if (!(om_try_ins_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED))         \
            {                                                                  \
                ccc_impl_om_insert_and_copy_key(om_try_ins_ent_,               \
                                                om_try_ins_ent_ret_, om_key_,  \
                                                lazy_value);                   \
            }                                                                  \
            else if (om_try_ins_ent_.entry_.stats_ == CCC_ENTRY_OCCUPIED)      \
            {                                                                  \
                om_try_ins_ent_ret_ = om_try_ins_ent_.entry_;                  \
            }                                                                  \
        }                                                                      \
        om_try_ins_ent_ret_;                                                   \
    }))

#define ccc_impl_om_insert_or_assign_w(ordered_map_ptr, key, lazy_value...)    \
    (__extension__({                                                           \
        __auto_type ins_or_assign_map_ptr_ = (ordered_map_ptr);                \
        struct ccc_ent_ om_ins_or_assign_ent_ret_                              \
            = {.stats_ = CCC_ENTRY_INPUT_ERROR};                               \
        if (ins_or_assign_map_ptr_)                                            \
        {                                                                      \
            __auto_type om_key_ = (key);                                       \
            struct ccc_tree_ *ordered_map_ptr_                                 \
                = &ins_or_assign_map_ptr_->impl_;                              \
            struct ccc_tree_entry_ om_ins_or_assign_ent_                       \
                = ccc_impl_om_entry(ordered_map_ptr_, (void *)&om_key_);       \
            if (!(om_ins_or_assign_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED))   \
            {                                                                  \
                ccc_impl_om_insert_and_copy_key(om_ins_or_assign_ent_,         \
                                                om_ins_or_assign_ent_ret_,     \
                                                om_key_, lazy_value);          \
            }                                                                  \
            else if (om_ins_or_assign_ent_.entry_.stats_                       \
                     == CCC_ENTRY_OCCUPIED)                                    \
            {                                                                  \
                struct ccc_node_ ins_ent_saved_ = *ccc_impl_omap_elem_in_slot( \
                    om_ins_or_assign_ent_.t_,                                  \
                    om_ins_or_assign_ent_.entry_.e_);                          \
                *((typeof(lazy_value) *)om_ins_or_assign_ent_.entry_.e_)       \
                    = lazy_value;                                              \
                *ccc_impl_omap_elem_in_slot(om_ins_or_assign_ent_.t_,          \
                                            om_ins_or_assign_ent_.entry_.e_)   \
                    = ins_ent_saved_;                                          \
                om_ins_or_assign_ent_ret_ = om_ins_or_assign_ent_.entry_;      \
                *((typeof(om_key_) *)ccc_impl_om_key_in_slot(                  \
                    ordered_map_ptr_, om_ins_or_assign_ent_ret_.e_))           \
                    = om_key_;                                                 \
            }                                                                  \
        }                                                                      \
        om_ins_or_assign_ent_ret_;                                             \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_ORDERED_MAP_H */
