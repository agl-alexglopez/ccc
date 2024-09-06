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

#define CCC_OM_EMPTY ((uint64_t)0)
#define CCC_OM_ENTRY_VACANT ((uint8_t)0x0)
#define CCC_OM_ENTRY_OCCUPIED ((uint8_t)0x1)
#define CCC_OM_ENTRY_INSERT_ERROR ((uint8_t)0x2)
#define CCC_OM_ENTRY_SEARCH_ERROR ((uint8_t)0x4)
#define CCC_OM_ENTRY_NULL ((uint8_t)0x8)
#define CCC_OM_ENTRY_DELETE_ERROR ((uint8_t)0x10)

/* The core node of the underlying tree implementation. Using an array
   for the nodes allows symmetric left/right cases to always be united
   into one code path to reduce code bloat and bugs. The parent field
   is required to track duplicates and would not be strictly necessary
   in some interpretations of a multiset. However, the parent field
   gives this implementation flexibility for duplicates, speed, and a
   robust iterator for users. This is important for a priority queue. */
typedef struct ccc_om_node_
{
    struct ccc_om_node_ *link[2];
    struct ccc_om_node_ *parent_or_dups;
} ccc_om_node_;

/* The size field is not strictly necessary but seems to be standard
   practice for these types of containers for O(1) access. The end is
   critical for this implementation, especially iterators. */
struct ccc_om_
{
    ccc_om_node_ *root;
    ccc_om_node_ end;
    ccc_alloc_fn *alloc;
    ccc_key_cmp_fn *cmp;
    void *aux;
    size_t size;
    size_t elem_sz;
    size_t node_elem_offset;
    size_t key_offset;
};

struct ccc_om_entry_
{
    struct ccc_om_ *t;
    ccc_entry entry;
};

#define CCC_TREE_INIT(struct_name, node_elem_field, key_elem_field, tree_name, \
                      alloc_fn, key_cmp_fn, aux_data)                          \
    {                                                                          \
        .impl = {                                                              \
            .root = &(tree_name).impl.end,                                     \
            .end = {.link = {&(tree_name).impl.end, &(tree_name).impl.end},    \
                    .parent_or_dups = &(tree_name).impl.end},                  \
            .alloc = (alloc_fn),                                               \
            .cmp = (key_cmp_fn),                                               \
            .aux = (aux_data),                                                 \
            .size = 0,                                                         \
            .elem_sz = sizeof(struct_name),                                    \
            .node_elem_offset = offsetof(struct_name, node_elem_field),        \
            .key_offset = offsetof(struct_name, key_elem_field),               \
        },                                                                     \
    }

bool ccc_tree_validate(struct ccc_om_ const *t);
void ccc_tree_print(struct ccc_om_ const *t, ccc_om_node_ const *root,
                    ccc_print_fn *fn);
void *ccc_impl_tree_key_in_slot(struct ccc_om_ const *t, void const *slot);
ccc_om_node_ *ccc_impl_tree_elem_in_slot(struct ccc_om_ const *t,
                                         void const *slot);
void *ccc_impl_tree_key_from_node(struct ccc_om_ const *t,
                                  ccc_om_node_ const *n);

struct ccc_om_entry_ ccc_impl_tree_entry(struct ccc_om_ *t, void const *key);
void *ccc_impl_tree_insert(struct ccc_om_ *t, ccc_om_node_ *n);

#define CCC_IMPL_TREE_ENTRY(tree_ptr, key...)                                  \
    ({                                                                         \
        __auto_type tree_key_ = key;                                           \
        struct ccc_om_entry_ tree_entry_                                       \
            = ccc_impl_tree_entry(&(tree_ptr)->impl, &tree_key_);              \
        tree_entry_;                                                           \
    })

#define CCC_IMPL_TREE_GET(tree_ptr, key...)                                    \
    ({                                                                         \
        __auto_type const tree_key_ = key;                                     \
        struct ccc_fhm_entry_ tree_get_ent_                                    \
            = ccc_impl_tree_entry(&(tree_ptr)->impl, &tree_key_);              \
        void const *tree_get_res_ = NULL;                                      \
        if (tree_get_ent_.entry.status & CCC_OM_ENTRY_OCCUPIED)                \
        {                                                                      \
            tree_get_res_ = tree_get_ent_.entry.entry;                         \
        }                                                                      \
        tree_get_res_;                                                         \
    })

#define CCC_IMPL_TREE_GET_MUT(tree_ptr, key...)                                \
    ({                                                                         \
        void *tree_get_mut_res_ = (void *)CCC_IMPL_TREE_GET(fhash_ptr, key);   \
        tree_get_mut_res_;                                                     \
    })

#define CCC_IMPL_TREE_AND_MODIFY(tree_entry, mod_fn)                           \
    ({                                                                         \
        struct ccc_om_entry_ tree_mod_ent_ = (tree_entry).impl;                \
        if (tree_mod_ent_.entry.status & CCC_OM_ENTRY_OCCUPIED)                \
        {                                                                      \
            (mod_fn)(                                                          \
                (ccc_update){.container = (void *const)e.entry, .aux = NULL}); \
        }                                                                      \
        tree_mod_ent_;                                                         \
    })

#define CCC_IMPL_TREE_AND_MODIFY_W(tree_entry, mod_fn, aux_data)               \
    ({                                                                         \
        struct ccc_om_entry_ tree_mod_ent_ = (tree_entry).impl;                \
        if (tree_mod_ent_.entry.status & CCC_OM_ENTRY_OCCUPIED)                \
        {                                                                      \
            __auto_type tree_aux_data_ = (aux_data);                           \
            (mod_fn)((ccc_update){.container = (void *const)e.entry,           \
                                  .aux = &tree_aux_data_});                    \
        }                                                                      \
        tree_mod_ent_;                                                         \
    })

#define CCC_IMPL_TREE_NEW_INSERT(entry, key_value...)                          \
    ({                                                                         \
        void *tree_ins_alloc_ret_ = NULL;                                      \
        if ((entry).t->alloc)                                                  \
        {                                                                      \
            tree_ins_alloc_ret_ = (entry).t->alloc(NULL, (entry).t->elem_sz);  \
            if (tree_ins_alloc_ret_)                                           \
            {                                                                  \
                *((typeof(key_value) *)tree_ins_alloc_ret_) = key_value;       \
                tree_ins_alloc_ret_ = ccc_impl_tree_insert(                    \
                    (entry).t, ccc_impl_tree_elem_in_slot(                     \
                                   (entry).t, tree_ins_alloc_ret_));           \
            }                                                                  \
        }                                                                      \
        tree_ins_alloc_ret_;                                                   \
    })

#define CCC_IMPL_TREE_INSERT_ENTRY(tree_entry, key_value...)                   \
    ({                                                                         \
        struct ccc_om_entry_ tree_ins_ent_ = (tree_entry).impl;                \
        void *tree_ins_ent_ret_ = NULL;                                        \
        if (!(tree_ins_ent_.entry.status & CCC_OM_ENTRY_OCCUPIED))             \
        {                                                                      \
            tree_ins_ent_ret_                                                  \
                = CCC_IMPL_TREE_NEW_INSERT(tree_ins_ent_, key_value);          \
        }                                                                      \
        else if (tree_ins_ent_.entry.status == CCC_OM_ENTRY_OCCUPIED)          \
        {                                                                      \
            struct ccc_om_node_ ins_ent_saved_ = *ccc_impl_tree_elem_in_slot(  \
                tree_ins_ent_.t, tree_ins_ent_.entry.entry);                   \
            *((typeof(key_value) *)tree_ins_ent_.entry.entry) = key_value;     \
            *ccc_impl_tree_elem_in_slot(tree_ins_ent_.t,                       \
                                        tree_ins_ent_.entry.entry)             \
                = ins_ent_saved_;                                              \
            tree_ins_ent_ret_ = (void *)tree_ins_ent_.entry.entry;             \
        }                                                                      \
        tree_ins_ent_ret_;                                                     \
    })

#define CCC_IMPL_TREE_OR_INSERT(tree_entry, key_value...)                      \
    ({                                                                         \
        struct ccc_om_entry_ tree_or_ins_ent_ = (tree_entry).impl;             \
        void *or_ins_ret_ = NULL;                                              \
        if (tree_or_ins_ent_.entry.status == CCC_OM_ENTRY_OCCUPIED)            \
        {                                                                      \
            or_ins_ret_ = (void *)tree_or_ins_ent_.entry.entry;                \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            or_ins_ret_                                                        \
                = CCC_IMPL_TREE_NEW_INSERT(tree_or_ins_ent_, key_value);       \
        }                                                                      \
        or_ins_ret_;                                                           \
    })

#endif /* CCC_IMPL_TREE_H */
