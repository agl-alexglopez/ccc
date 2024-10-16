#ifndef CCC_IMPL_ORDERED_MULTIMAP_H
#define CCC_IMPL_ORDERED_MULTIMAP_H

#include "impl_tree.h"

#include <stddef.h>

struct ccc_node_;
struct ccc_tree_;

#define ccc_impl_omm_init(tree_name, struct_name, node_elem_field,             \
                          key_elem_field, alloc_fn, key_cmp_fn, aux_data)      \
    ccc_tree_init(tree_name, struct_name, node_elem_field, key_elem_field,     \
                  alloc_fn, key_cmp_fn, aux_data)

void *ccc_impl_omm_key_in_slot(struct ccc_tree_ const *t, void const *slot);
ccc_node_ *ccc_impl_omm_elem_in_slot(struct ccc_tree_ const *t,
                                     void const *slot);
void *ccc_impl_omm_key_from_node(struct ccc_tree_ const *t, ccc_node_ const *n);
struct ccc_tree_entry_ ccc_impl_omm_entry(struct ccc_tree_ *t, void const *key);
void *ccc_impl_omm_multimap_insert(struct ccc_tree_ *t, ccc_node_ *n);

/* NOLINTBEGIN(readability-identifier-naming) */

/*==================   Helper Macros for Repeated Logic     =================*/

#define ccc_impl_omm_new(ordered_map_entry)                                    \
    ({                                                                         \
        void *omm_ins_alloc_ret_ = NULL;                                       \
        if ((ordered_map_entry)->t_->alloc_)                                   \
        {                                                                      \
            omm_ins_alloc_ret_                                                 \
                = (ordered_map_entry)                                          \
                      ->t_->alloc_(NULL, (ordered_map_entry)->t_->elem_sz_);   \
        }                                                                      \
        omm_ins_alloc_ret_;                                                    \
    })

#define ccc_impl_omm_insert_key_val(ordered_map_entry, new_mem,                \
                                    lazy_key_value...)                         \
    ({                                                                         \
        if (new_mem)                                                           \
        {                                                                      \
            *new_mem = lazy_key_value;                                         \
            new_mem = ccc_impl_omm_multimap_insert(                            \
                (ordered_map_entry)->t_,                                       \
                ccc_impl_omm_elem_in_slot((ordered_map_entry)->t_, new_mem));  \
        }                                                                      \
    })

#define ccc_impl_omm_insert_and_copy_key(                                      \
    omm_insert_entry, omm_insert_entry_ret, key, lazy_value...)                \
    ({                                                                         \
        typeof(lazy_value) *omm_new_ins_base_                                  \
            = ccc_impl_omm_new((&omm_insert_entry));                           \
        omm_insert_entry_ret = (struct ccc_entry_){                            \
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
                ccc_impl_omm_elem_in_slot(omm_insert_entry.t_,                 \
                                          omm_new_ins_base_));                 \
        }                                                                      \
    })

/*=====================     Core Macro Implementations     ==================*/

#define ccc_impl_omm_and_modify_w(ordered_map_entry, mod_fn, aux_data...)      \
    ({                                                                         \
        struct ccc_tree_entry_ omm_mod_ent_ = (ordered_map_entry)->impl_;      \
        if (omm_mod_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                   \
        {                                                                      \
            __auto_type omm_aux_data_ = aux_data;                              \
            (mod_fn)((ccc_user_type_mut){                                      \
                .user_type = (void *const)omm_mod_ent_.entry_.e_,              \
                .aux = &omm_aux_data_});                                       \
        }                                                                      \
        omm_mod_ent_;                                                          \
    })

#define ccc_impl_omm_or_insert_w(ordered_map_entry, lazy_key_value...)         \
    ({                                                                         \
        struct ccc_tree_entry_ *omm_or_ins_ent_ = &(ordered_map_entry)->impl_; \
        typeof(lazy_key_value) *or_ins_ret_ = NULL;                            \
        if (omm_or_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)              \
        {                                                                      \
            or_ins_ret_ = omm_or_ins_ent_->entry_.e_;                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            or_ins_ret_ = ccc_impl_omm_new(omm_or_ins_ent_);                   \
            ccc_impl_omm_insert_key_val(omm_or_ins_ent_, or_ins_ret_,          \
                                        lazy_key_value);                       \
        }                                                                      \
        or_ins_ret_;                                                           \
    })

#define ccc_impl_omm_insert_entry_w(ordered_map_entry, lazy_key_value...)      \
    ({                                                                         \
        struct ccc_tree_entry_ *omm_ins_ent_ = &(ordered_map_entry)->impl_;    \
        typeof(lazy_key_value) *omm_ins_ent_ret_                               \
            = ccc_impl_omm_new(omm_ins_ent_);                                  \
        ccc_impl_omm_insert_key_val(omm_ins_ent_, omm_ins_ent_ret_,            \
                                    lazy_key_value);                           \
        omm_ins_ent_ret_;                                                      \
    })

#define ccc_impl_omm_try_insert_w(ordered_map_ptr, key, lazy_value...)         \
    ({                                                                         \
        __auto_type omm_key_ = (key);                                          \
        struct ccc_tree_ *omm_try_ins_map_ptr_ = &(ordered_map_ptr)->impl_;    \
        struct ccc_tree_entry_ omm_try_ins_ent_                                \
            = ccc_impl_omm_entry(omm_try_ins_map_ptr_, &omm_key_);             \
        struct ccc_entry_ omm_try_ins_ent_ret_ = {};                           \
        if (!(omm_try_ins_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED))            \
        {                                                                      \
            ccc_impl_omm_insert_and_copy_key(                                  \
                omm_try_ins_ent_, omm_try_ins_ent_ret_, omm_key_, lazy_value); \
        }                                                                      \
        else if (omm_try_ins_ent_.entry_.stats_ == CCC_ENTRY_OCCUPIED)         \
        {                                                                      \
            omm_try_ins_ent_ret_ = omm_try_ins_ent_.entry_;                    \
        }                                                                      \
        omm_try_ins_ent_ret_;                                                  \
    })

#define ccc_impl_omm_insert_or_assign_w(ordered_map_ptr, key, lazy_value...)   \
    ({                                                                         \
        __auto_type omm_key_ = (key);                                          \
        struct ccc_tree_ *ordered_map_ptr_ = &(ordered_map_ptr)->impl_;        \
        struct ccc_tree_entry_ omm_ins_or_assign_ent_                          \
            = ccc_impl_omm_entry(ordered_map_ptr_, &omm_key_);                 \
        struct ccc_entry_ omm_ins_or_assign_ent_ret_ = {};                     \
        if (!(omm_ins_or_assign_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED))      \
        {                                                                      \
            ccc_impl_omm_insert_and_copy_key(omm_ins_or_assign_ent_,           \
                                             omm_ins_or_assign_ent_ret_,       \
                                             omm_key_, lazy_value);            \
        }                                                                      \
        else if (omm_ins_or_assign_ent_.entry_.stats_ == CCC_ENTRY_OCCUPIED)   \
        {                                                                      \
            struct ccc_node_ ins_ent_saved_ = *ccc_impl_omm_elem_in_slot(      \
                omm_ins_or_assign_ent_.t_, omm_ins_or_assign_ent_.entry_.e_);  \
            *((typeof(lazy_value) *)omm_ins_or_assign_ent_.entry_.e_)          \
                = lazy_value;                                                  \
            *ccc_impl_omm_elem_in_slot(omm_ins_or_assign_ent_.t_,              \
                                       omm_ins_or_assign_ent_.entry_.e_)       \
                = ins_ent_saved_;                                              \
            omm_ins_or_assign_ent_ret_ = omm_ins_or_assign_ent_.entry_;        \
            *((typeof(omm_key_) *)ccc_impl_omm_key_in_slot(                    \
                ordered_map_ptr_, omm_ins_or_assign_ent_ret_.e_))              \
                = omm_key_;                                                    \
        }                                                                      \
        omm_ins_or_assign_ent_ret_;                                            \
    })

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_ORDERED_MULTIMAP_H */
