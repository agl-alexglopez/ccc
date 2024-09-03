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
    ccc_alloc_fn *alloc;
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

bool ccc_tree_validate(ccc_tree const *t);
void ccc_tree_print(ccc_tree const *t, ccc_node const *root, ccc_print_fn *fn);
void *ccc_impl_tree_key_in_slot(ccc_tree const *t, void const *slot);
ccc_node *ccc_impl_tree_elem_in_slot(ccc_tree const *t, void const *slot);
void *ccc_impl_tree_key_from_node(ccc_tree const *t, ccc_node const *n);

struct ccc_tree_entry ccc_impl_tree_entry(ccc_tree *t, void const *key);
void *ccc_impl_tree_insert(ccc_tree *t, ccc_node *n);

#define CCC_IMPL_TREE_ENTRY(tree_ptr, key...)                                  \
    ({                                                                         \
        __auto_type _tree_key_ = key;                                          \
        struct ccc_tree_entry _tree_entry_                                     \
            = ccc_impl_tree_entry(&(tree_ptr)->impl, &_tree_key_);             \
        _tree_entry_;                                                          \
    })

#define CCC_IMPL_TREE_GET(tree_ptr, key...)                                    \
    ({                                                                         \
        __auto_type const _tree_key_ = key;                                    \
        struct ccc_impl_fhash_entry _tree_get_ent_                             \
            = ccc_impl_tree_entry(&(tree_ptr)->impl, &_tree_key_);             \
        void const *_tree_get_res_ = NULL;                                     \
        if (_tree_get_ent_.entry.status & CCC_OM_ENTRY_OCCUPIED)               \
        {                                                                      \
            _tree_get_res_ = _tree_get_ent_.entry.entry;                       \
        }                                                                      \
        _tree_get_res_;                                                        \
    })

#define CCC_IMPL_TREE_GET_MUT(tree_ptr, key...)                                \
    ({                                                                         \
        void *_tree_get_mut_res_ = (void *)CCC_IMPL_TREE_GET(fhash_ptr, key);  \
        _tree_get_mut_res_;                                                    \
    })

#define CCC_IMPL_TREE_AND_MODIFY(tree_entry, mod_fn)                           \
    ({                                                                         \
        struct ccc_tree_entry _tree_mod_ent_ = (tree_entry).impl;              \
        if (_tree_mod_ent_.entry.status & CCC_OM_ENTRY_OCCUPIED)               \
        {                                                                      \
            (mod_fn)(                                                          \
                (ccc_update){.container = (void *const)e.entry, .aux = NULL}); \
        }                                                                      \
        _tree_mod_ent_;                                                        \
    })

#define CCC_IMPL_TREE_AND_MODIFY_W(tree_entry, mod_fn, aux_data)               \
    ({                                                                         \
        struct ccc_tree_entry _tree_mod_ent_ = (tree_entry).impl;              \
        if (_tree_mod_ent_.entry.status & CCC_OM_ENTRY_OCCUPIED)               \
        {                                                                      \
            __auto_type _tree_aux_data_ = (aux_data);                          \
            (mod_fn)((ccc_update){.container = (void *const)e.entry,           \
                                  .aux = &_tree_aux_data_});                   \
        }                                                                      \
        _tree_mod_ent_;                                                        \
    })

#define CCC_IMPL_TREE_NEW_INSERT(entry, key_value...)                          \
    ({                                                                         \
        void *_tree_ins_alloc_ret_ = NULL;                                     \
        if ((entry).t->alloc)                                                  \
        {                                                                      \
            _tree_ins_alloc_ret_ = (entry).t->alloc(NULL, (entry).t->elem_sz); \
            if (_tree_ins_alloc_ret_)                                          \
            {                                                                  \
                *((typeof(key_value) *)_tree_ins_alloc_ret_) = key_value;      \
                _tree_ins_alloc_ret_ = ccc_impl_tree_insert(                   \
                    (entry).t, ccc_impl_tree_elem_in_slot(                     \
                                   (entry).t, _tree_ins_alloc_ret_));          \
            }                                                                  \
        }                                                                      \
        _tree_ins_alloc_ret_;                                                  \
    })

#define CCC_IMPL_TREE_INSERT_ENTRY(tree_entry, key_value...)                   \
    ({                                                                         \
        struct ccc_tree_entry _tree_ins_ent_ = (tree_entry).impl;              \
        void *_tree_ins_ent_ret_ = NULL;                                       \
        if (!(_tree_ins_ent_.entry.status & CCC_OM_ENTRY_OCCUPIED))            \
        {                                                                      \
            _tree_ins_ent_ret_                                                 \
                = CCC_IMPL_TREE_NEW_INSERT(_tree_ins_ent_, key_value);         \
        }                                                                      \
        else if (_tree_ins_ent_.entry.status == CCC_OM_ENTRY_OCCUPIED)         \
        {                                                                      \
            struct ccc_node _ins_ent_saved_ = *ccc_impl_tree_elem_in_slot(     \
                _tree_ins_ent_.t, _tree_ins_ent_.entry.entry);                 \
            *((typeof(key_value) *)_tree_ins_ent_.entry.entry) = key_value;    \
            *ccc_impl_tree_elem_in_slot(_tree_ins_ent_.t,                      \
                                        _tree_ins_ent_.entry.entry)            \
                = _ins_ent_saved_;                                             \
            _tree_ins_ent_ret_ = (void *)_tree_ins_ent_.entry.entry;           \
        }                                                                      \
        _tree_ins_ent_ret_;                                                    \
    })

#define CCC_IMPL_TREE_OR_INSERT(tree_entry, key_value...)                      \
    ({                                                                         \
        struct ccc_tree_entry _tree_or_ins_ent_ = (tree_entry).impl;           \
        void *_or_ins_ret_ = NULL;                                             \
        if (_tree_or_ins_ent_.entry.status == CCC_OM_ENTRY_OCCUPIED)           \
        {                                                                      \
            _or_ins_ret_ = (void *)_tree_or_ins_ent_.entry.entry;              \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            _or_ins_ret_                                                       \
                = CCC_IMPL_TREE_NEW_INSERT(_tree_or_ins_ent_, key_value);      \
        }                                                                      \
        _or_ins_ret_;                                                          \
    })

#endif /* CCC_IMPL_TREE_H */
