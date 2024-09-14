#ifndef CCC_IMPL_ORDERED_MAP_H
#define CCC_IMPL_ORDERED_MAP_H

#include "impl_tree.h"

struct ccc_node_;
struct ccc_tree_;

#define CCC_IMPL_OM_INIT(struct_name, node_elem_field, key_elem_field,         \
                         tree_name, alloc_fn, key_cmp_fn, aux_data)            \
    CCC_TREE_INIT(struct_name, node_elem_field, key_elem_field, tree_name,     \
                  alloc_fn, key_cmp_fn, aux_data)

void *ccc_impl_om_key_in_slot(struct ccc_tree_ const *t, void const *slot);
ccc_node_ *ccc_impl_om_elem_in_slot(struct ccc_tree_ const *t,
                                    void const *slot);
void *ccc_impl_om_key_from_node(struct ccc_tree_ const *t, ccc_node_ const *n);

struct ccc_tree_entry_ ccc_impl_om_entry(struct ccc_tree_ *t, void const *key);
void *ccc_impl_om_insert(struct ccc_tree_ *t, ccc_node_ *n);

#define CCC_IMPL_OM_ENTRY(ordered_map_ptr, key...)                             \
    ({                                                                         \
        __auto_type om_key_ = key;                                             \
        struct ccc_tree_entry_ om_entry_                                       \
            = ccc_impl_om_entry(&(ordered_map_ptr)->impl_, &om_key_);          \
        om_entry_;                                                             \
    })

#define CCC_IMPL_OM_GET_KEY_VAL(ordered_map_ptr, key...)                       \
    ({                                                                         \
        __auto_type const om_key_ = key;                                       \
        struct ccc_fhm_entry_ om_get_ent_                                      \
            = ccc_impl_om_entry(&(ordered_map_ptr)->impl_, &om_key_);          \
        void *om_get_res_ = NULL;                                              \
        if (om_get_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                    \
        {                                                                      \
            om_get_res_ = om_get_ent_.entry_.e_;                               \
        }                                                                      \
        om_get_res_;                                                           \
    })

#define CCC_IMPL_OM_AND_MODIFY(ordered_map_entry, mod_fn)                      \
    ({                                                                         \
        struct ccc_tree_entry_ om_mod_ent_ = (ordered_map_entry)->impl_;       \
        if (om_mod_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                    \
        {                                                                      \
            (mod_fn)(                                                          \
                (ccc_update){.container = (void *const)om_mod_ent_.entry_.e_,  \
                             .aux = NULL});                                    \
        }                                                                      \
        om_mod_ent_;                                                           \
    })

#define CCC_IMPL_OM_AND_MODIFY_W(ordered_map_entry, mod_fn, aux_data)          \
    ({                                                                         \
        struct ccc_tree_entry_ om_mod_ent_ = (ordered_map_entry)->impl_;       \
        if (om_mod_ent_.entry_.stats_ & CCC_ENTRY_OCCUPIED)                    \
        {                                                                      \
            __auto_type om_aux_data_ = (aux_data);                             \
            (mod_fn)(                                                          \
                (ccc_update){.container = (void *const)om_mod_ent_.entry_.e_,  \
                             .aux = &om_aux_data_});                           \
        }                                                                      \
        om_mod_ent_;                                                           \
    })

#define CCC_IMPL_OM_NEW_INSERT(ordered_map_entry, key_value...)                \
    ({                                                                         \
        void *om_ins_alloc_ret_ = NULL;                                        \
        if ((ordered_map_entry)->t_->alloc_)                                   \
        {                                                                      \
            om_ins_alloc_ret_                                                  \
                = (ordered_map_entry)                                          \
                      ->t_->alloc_(NULL, (ordered_map_entry)->t_->elem_sz_);   \
            if (om_ins_alloc_ret_)                                             \
            {                                                                  \
                *((typeof(key_value) *)om_ins_alloc_ret_) = key_value;         \
                om_ins_alloc_ret_ = ccc_impl_om_insert(                        \
                    (ordered_map_entry)->t_,                                   \
                    ccc_impl_om_elem_in_slot((ordered_map_entry)->t_,          \
                                             om_ins_alloc_ret_));              \
            }                                                                  \
        }                                                                      \
        om_ins_alloc_ret_;                                                     \
    })

#define CCC_IMPL_OM_INSERT_ENTRY(ordered_map_entry, key_value...)              \
    ({                                                                         \
        struct ccc_tree_entry_ *om_ins_ent_ = &(ordered_map_entry)->impl_;     \
        void *om_ins_ent_ret_ = NULL;                                          \
        if (!(om_ins_ent_->entry_.stats_ & CCC_ENTRY_OCCUPIED))                \
        {                                                                      \
            om_ins_ent_ret_ = CCC_IMPL_OM_NEW_INSERT(om_ins_ent_, key_value);  \
        }                                                                      \
        else if (om_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)             \
        {                                                                      \
            struct ccc_node_ ins_ent_saved_ = *ccc_impl_om_elem_in_slot(       \
                om_ins_ent_->t_, om_ins_ent_->entry_.e_);                      \
            *((typeof(key_value) *)om_ins_ent_->entry_.e_) = key_value;        \
            *ccc_impl_om_elem_in_slot(om_ins_ent_->t_, om_ins_ent_->entry_.e_) \
                = ins_ent_saved_;                                              \
            om_ins_ent_ret_ = (void *)om_ins_ent_->entry_.e_;                  \
        }                                                                      \
        om_ins_ent_ret_;                                                       \
    })

#define CCC_IMPL_OM_OR_INSERT(ordered_map_entry, key_value...)                 \
    ({                                                                         \
        struct ccc_tree_entry_ *om_or_ins_ent_ = &(ordered_map_entry)->impl_;  \
        void *or_ins_ret_ = NULL;                                              \
        if (om_or_ins_ent_->entry_.stats_ == CCC_ENTRY_OCCUPIED)               \
        {                                                                      \
            or_ins_ret_ = (void *)om_or_ins_ent_->entry_.e_;                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            or_ins_ret_ = CCC_IMPL_OM_NEW_INSERT(om_or_ins_ent_, key_value);   \
        }                                                                      \
        or_ins_ret_;                                                           \
    })

#endif /* CCC_IMPL_ORDERED_MAP_H */
