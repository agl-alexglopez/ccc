/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file implements a splay tree that does not support duplicates.
The code to support a splay tree that does not allow duplicates is much simpler
than the code to support a multimap implementation. This implementation is
based on the following source.

    1. Daniel Sleator, Carnegie Mellon University. Sleator's implementation of a
       topdown splay tree was instrumental in starting things off, but required
       extensive modification. I had to update parent and child tracking, and
       unite the left and right cases for fun. See the code for a generalizable
       strategy to eliminate symmetric left and right cases for any binary tree
       code. https://www.link.cs.cmu.edu/splay/

Because this is a self-optimizing data structure it may benefit from many
constant time queries for frequently accessed elements. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "adaptive_map.h"
#include "private/private_adaptive_map.h"
#include "private/private_types.h"
#include "types.h"

/** @internal Instead of thinking about left and right consider only links
    in the abstract sense. Put them in an array and then flip
    this enum and left and right code paths can be united into one */
enum Branch
{
    L = 0,
    R,
};

#define INORDER R
#define R_INORDER L

enum
{
    LR = 2,
};

/* Container entry return value. */

static struct CCC_Adaptive_map_entry container_entry(struct CCC_Adaptive_map *t,
                                                     void const *key);

/* No return value. */

static void init_node(struct CCC_Adaptive_map *,
                      struct CCC_Adaptive_map_node *);
static void swap(char temp[const], void *, void *, size_t);
static void link(struct CCC_Adaptive_map_node *, enum Branch,
                 struct CCC_Adaptive_map_node *);

/* Boolean returns */

static CCC_Tribool empty(struct CCC_Adaptive_map const *);
static CCC_Tribool contains(struct CCC_Adaptive_map *, void const *);
static CCC_Tribool validate(struct CCC_Adaptive_map const *);

/* Returning the user type that is stored in data structure. */

static void *struct_base(struct CCC_Adaptive_map const *,
                         struct CCC_Adaptive_map_node const *);
static void *find(struct CCC_Adaptive_map *, void const *);
static void *erase(struct CCC_Adaptive_map *, void const *key);
static void *allocate_insert(struct CCC_Adaptive_map *,
                             struct CCC_Adaptive_map_node *);
static void *insert(struct CCC_Adaptive_map *, struct CCC_Adaptive_map_node *);
static void *connect_new_root(struct CCC_Adaptive_map *,
                              struct CCC_Adaptive_map_node *, CCC_Order);
static void *max(struct CCC_Adaptive_map const *);
static void *min(struct CCC_Adaptive_map const *);
static void *key_in_slot(struct CCC_Adaptive_map const *t, void const *slot);
static void *key_from_node(struct CCC_Adaptive_map const *,
                           CCC_Adaptive_map_node const *);
static struct CCC_Range equal_range(struct CCC_Adaptive_map *, void const *,
                                    void const *, enum Branch);

/* Internal operations that take and return nodes for the tree. */

static struct CCC_Adaptive_map_node *
remove_from_tree(struct CCC_Adaptive_map *, struct CCC_Adaptive_map_node *);
static struct CCC_Adaptive_map_node const *
next(struct CCC_Adaptive_map const *, struct CCC_Adaptive_map_node const *,
     enum Branch);
static struct CCC_Adaptive_map_node *splay(struct CCC_Adaptive_map *,
                                           struct CCC_Adaptive_map_node *,
                                           void const *key,
                                           CCC_Key_comparator *);
static struct CCC_Adaptive_map_node *
elem_in_slot(struct CCC_Adaptive_map const *t, void const *slot);

/* The key comes first. It is the "left hand side" of the comparison. */
static CCC_Order order(struct CCC_Adaptive_map const *, void const *key,
                       struct CCC_Adaptive_map_node const *,
                       CCC_Key_comparator *);

/* ======================        Map Interface      ====================== */

CCC_Tribool
CCC_adaptive_map_is_empty(CCC_Adaptive_map const *const map)
{
    if (!map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return empty(map);
}

CCC_Count
CCC_adaptive_map_count(CCC_Adaptive_map const *const map)
{
    if (!map)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = map->size};
}

CCC_Tribool
CCC_adaptive_map_contains(CCC_Adaptive_map *const map, void const *const key)
{
    if (!map || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return contains(map, key);
}

CCC_Adaptive_map_entry
CCC_adaptive_map_entry(CCC_Adaptive_map *const map, void const *const key)
{
    if (!map || !key)
    {
        return (CCC_Adaptive_map_entry){
            {.entry = {.status = CCC_ENTRY_ARGUMENT_ERROR}}};
    }
    return (CCC_Adaptive_map_entry){container_entry(map, key)};
}

void *
CCC_adaptive_map_insert_entry(CCC_Adaptive_map_entry const *const entry,
                              CCC_Adaptive_map_node *const type_intruder)
{
    if (!entry || !type_intruder)
    {
        return NULL;
    }
    if (entry->private.entry.status == CCC_ENTRY_OCCUPIED)
    {
        if (entry->private.entry.type)
        {
            *type_intruder
                = *elem_in_slot(entry->private.map, entry->private.entry.type);
            (void)memcpy(entry->private.entry.type,
                         struct_base(entry->private.map, type_intruder),
                         entry->private.map->sizeof_type);
        }
        return entry->private.entry.type;
    }
    return allocate_insert(entry->private.map, type_intruder);
}

void *
CCC_adaptive_map_or_insert(CCC_Adaptive_map_entry const *const entry,
                           CCC_Adaptive_map_node *const type_intruder)
{
    if (!entry || !type_intruder)
    {
        return NULL;
    }
    if (entry->private.entry.status & CCC_ENTRY_OCCUPIED)
    {
        return entry->private.entry.type;
    }
    return allocate_insert(entry->private.map, type_intruder);
}

CCC_Adaptive_map_entry *
CCC_adaptive_map_and_modify(CCC_Adaptive_map_entry *const entry,
                            CCC_Type_modifier *const modify)
{
    if (!entry)
    {
        return NULL;
    }
    if (modify && (entry->private.entry.status & CCC_ENTRY_OCCUPIED)
        && entry->private.entry.type)
    {
        modify((CCC_Type_context){
            .type = entry->private.entry.type,
            .context = NULL,
        });
    }
    return entry;
}

CCC_Adaptive_map_entry *
CCC_adaptive_map_and_modify_context(CCC_Adaptive_map_entry *const entry,
                                    CCC_Type_modifier *const fn,
                                    void *const context)
{
    if (entry && fn && entry->private.entry.status & CCC_ENTRY_OCCUPIED
        && entry->private.entry.type)
    {
        fn((CCC_Type_context){
            .type = entry->private.entry.type,
            .context = context,
        });
    }
    return entry;
}

CCC_Entry
CCC_adaptive_map_swap_entry(CCC_Adaptive_map *const map,
                            CCC_Adaptive_map_node *const type_intruder,
                            CCC_Adaptive_map_node *const temp_intruder)
{
    if (!map || !type_intruder || !temp_intruder)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    void *const found = find(map, key_from_node(map, type_intruder));
    if (found)
    {
        assert(map->root != &map->end);
        *type_intruder = *map->root;
        void *const any_struct = struct_base(map, type_intruder);
        void *const in_tree = struct_base(map, map->root);
        void *const old_val = struct_base(map, temp_intruder);
        swap(old_val, in_tree, any_struct, map->sizeof_type);
        type_intruder->branch[L] = type_intruder->branch[R]
            = type_intruder->parent = NULL;
        temp_intruder->branch[L] = temp_intruder->branch[R]
            = temp_intruder->parent = NULL;
        return (CCC_Entry){{
            .type = old_val,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = allocate_insert(map, type_intruder);
    if (!inserted)
    {
        return (CCC_Entry){{
            .type = NULL,
            .status = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Entry){{
        .type = NULL,
        .status = CCC_ENTRY_VACANT,
    }};
}

CCC_Entry
CCC_adaptive_map_try_insert(CCC_Adaptive_map *const map,
                            CCC_Adaptive_map_node *const type_intruder)
{
    if (!map || !type_intruder)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    void *const found = find(map, key_from_node(map, type_intruder));
    if (found)
    {
        assert(map->root != &map->end);
        return (CCC_Entry){{
            .type = struct_base(map, map->root),
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = allocate_insert(map, type_intruder);
    if (!inserted)
    {
        return (CCC_Entry){{
            .type = NULL,
            .status = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Entry){{
        .type = inserted,
        .status = CCC_ENTRY_VACANT,
    }};
}

CCC_Entry
CCC_adaptive_map_insert_or_assign(CCC_Adaptive_map *const map,
                                  CCC_Adaptive_map_node *const type_intruder)
{
    if (!map || !type_intruder)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    void *const found = find(map, key_from_node(map, type_intruder));
    if (found)
    {
        *type_intruder = *elem_in_slot(map, found);
        assert(map->root != &map->end);
        memcpy(found, struct_base(map, type_intruder), map->sizeof_type);
        return (CCC_Entry){{
            .type = found,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = allocate_insert(map, type_intruder);
    if (!inserted)
    {
        return (CCC_Entry){{
            .type = NULL,
            .status = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Entry){{
        .type = inserted,
        .status = CCC_ENTRY_VACANT,
    }};
}

CCC_Entry
CCC_adaptive_map_remove(CCC_Adaptive_map *const map,
                        CCC_Adaptive_map_node *const type_output_intruder)
{
    if (!map || !type_output_intruder)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    void *const n = erase(map, key_from_node(map, type_output_intruder));
    if (!n)
    {
        return (CCC_Entry){{
            .type = NULL,
            .status = CCC_ENTRY_VACANT,
        }};
    }
    if (map->allocate)
    {
        void *const any_struct = struct_base(map, type_output_intruder);
        memcpy(any_struct, n, map->sizeof_type);
        map->allocate((CCC_Allocator_context){
            .input = n,
            .bytes = 0,
            .context = map->context,
        });
        return (CCC_Entry){{
            .type = any_struct,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Entry){{
        .type = n,
        .status = CCC_ENTRY_OCCUPIED,
    }};
}

CCC_Entry
CCC_adaptive_map_remove_entry(CCC_Adaptive_map_entry *const e)
{
    if (!e)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    if (e->private.entry.status == CCC_ENTRY_OCCUPIED && e->private.entry.type)
    {
        void *const erased = erase(
            e->private.map, key_in_slot(e->private.map, e->private.entry.type));
        assert(erased);
        if (e->private.map->allocate)
        {
            e->private.map->allocate((CCC_Allocator_context){
                .input = erased,
                .bytes = 0,
                .context = e->private.map->context,
            });
            return (CCC_Entry){{
                .type = NULL,
                .status = CCC_ENTRY_OCCUPIED,
            }};
        }
        return (CCC_Entry){{
            .type = erased,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Entry){{
        .type = NULL,
        .status = CCC_ENTRY_VACANT,
    }};
}

void *
CCC_adaptive_map_get_key_value(CCC_Adaptive_map *const map,
                               void const *const key)
{
    if (!map || !key)
    {
        return NULL;
    }
    return find(map, key);
}

void *
CCC_adaptive_map_unwrap(CCC_Adaptive_map_entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->private.entry.status == CCC_ENTRY_OCCUPIED ? e->private.entry.type
                                                         : NULL;
}

CCC_Tribool
CCC_adaptive_map_insert_error(CCC_Adaptive_map_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->private.entry.status & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_adaptive_map_occupied(CCC_Adaptive_map_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->private.entry.status & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Entry_status
CCC_adaptive_map_entry_status(CCC_Adaptive_map_entry const *const e)
{
    return e ? e->private.entry.status : CCC_ENTRY_ARGUMENT_ERROR;
}

void *
CCC_adaptive_map_begin(CCC_Adaptive_map const *const map)
{
    return map ? min(map) : NULL;
}

void *
CCC_adaptive_map_reverse_begin(CCC_Adaptive_map const *const map)
{
    return map ? max(map) : NULL;
}

void *
CCC_adaptive_map_end(CCC_Adaptive_map const *const)
{
    return NULL;
}

void *
CCC_adaptive_map_reverse_end(CCC_Adaptive_map const *const)
{
    return NULL;
}

void *
CCC_adaptive_map_next(CCC_Adaptive_map const *const map,
                      CCC_Adaptive_map_node const *const iterator_intruder)
{
    if (!map || !iterator_intruder)
    {
        return NULL;
    }
    struct CCC_Adaptive_map_node const *n
        = next(map, iterator_intruder, INORDER);
    return n == &map->end ? NULL : struct_base(map, n);
}

void *
CCC_adaptive_map_reverse_next(
    CCC_Adaptive_map const *const map,
    CCC_Adaptive_map_node const *const iterator_intruder)
{
    if (!map || !iterator_intruder)
    {
        return NULL;
    }
    struct CCC_Adaptive_map_node const *n
        = next(map, iterator_intruder, R_INORDER);
    return n == &map->end ? NULL : struct_base(map, n);
}

CCC_Range
CCC_adaptive_map_equal_range(CCC_Adaptive_map *const map,
                             void const *const begin_key,
                             void const *const end_key)
{
    if (!map || !begin_key || !end_key)
    {
        return (CCC_Range){};
    }
    return (CCC_Range){equal_range(map, begin_key, end_key, INORDER)};
}

CCC_Range_reverse
CCC_adaptive_map_equal_range_reverse(CCC_Adaptive_map *const map,
                                     void const *const reverse_begin_key,
                                     void const *const reverse_end_key)

{
    if (!map || !reverse_begin_key || !reverse_end_key)
    {
        return (CCC_Range_reverse){};
    }
    return (CCC_Range_reverse){
        equal_range(map, reverse_begin_key, reverse_end_key, R_INORDER)};
}

/** This is a linear time constant space deletion of tree nodes via left
rotations so element fields are modified during progression of deletes. */
CCC_Result
CCC_adaptive_map_clear(CCC_Adaptive_map *const map,
                       CCC_Type_destructor *const destructor)
{
    if (!map)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Adaptive_map_node *node = map->root;
    while (node != &map->end)
    {
        if (node->branch[L] != &map->end)
        {
            struct CCC_Adaptive_map_node *const l = node->branch[L];
            node->branch[L] = l->branch[R];
            l->branch[R] = node;
            node = l;
            continue;
        }
        struct CCC_Adaptive_map_node *const next = node->branch[R];
        node->branch[L] = node->branch[R] = NULL;
        node->parent = NULL;
        void *const del = struct_base(map, node);
        if (destructor)
        {
            destructor((CCC_Type_context){
                .type = del,
                .context = map->context,
            });
        }
        if (map->allocate)
        {
            (void)map->allocate((CCC_Allocator_context){
                .input = del,
                .bytes = 0,
                .context = map->context,
            });
        }
        node = next;
    }
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_adaptive_map_validate(CCC_Adaptive_map const *const map)
{
    if (!map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(map);
}

/*==========================  Private Interface  ============================*/

struct CCC_Adaptive_map_entry
CCC_private_adaptive_map_entry(struct CCC_Adaptive_map *const t,
                               void const *const key)
{
    return container_entry(t, key);
}

void *
CCC_private_adaptive_map_insert(struct CCC_Adaptive_map *const t,
                                struct CCC_Adaptive_map_node *n)
{
    return insert(t, n);
}

void *
CCC_private_adaptive_map_key_in_slot(struct CCC_Adaptive_map const *const t,
                                     void const *const slot)
{
    return key_in_slot(t, slot);
}

struct CCC_Adaptive_map_node *
CCC_private_Adaptive_map_node_in_slot(struct CCC_Adaptive_map const *const t,
                                      void const *slot)
{

    return elem_in_slot(t, slot);
}

/*======================  Static Splay Tree Helpers  ========================*/

static struct CCC_Adaptive_map_entry
container_entry(struct CCC_Adaptive_map *const t, void const *const key)
{
    void *const found = find(t, key);
    if (found)
    {
        return (struct CCC_Adaptive_map_entry){
            .map = t,
            .entry = {
                .type = found,
                .status = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct CCC_Adaptive_map_entry){
        .map = t,
        .entry = {
            .type = found,
            .status = CCC_ENTRY_VACANT,
        },
    };
}

static inline void *
key_from_node(struct CCC_Adaptive_map const *const t,
              CCC_Adaptive_map_node const *const n)
{
    return (char *)struct_base(t, n) + t->key_offset;
}

static inline void *
key_in_slot(struct CCC_Adaptive_map const *const t, void const *const slot)
{
    return (char *)slot + t->key_offset;
}

static inline struct CCC_Adaptive_map_node *
elem_in_slot(struct CCC_Adaptive_map const *const t, void const *const slot)
{

    return (struct CCC_Adaptive_map_node *)((char *)slot + t->node_node_offset);
}

static inline void
init_node(struct CCC_Adaptive_map *const t,
          struct CCC_Adaptive_map_node *const n)
{
    n->branch[L] = &t->end;
    n->branch[R] = &t->end;
    n->parent = &t->end;
}

static inline CCC_Tribool
empty(struct CCC_Adaptive_map const *const t)
{
    return !t->size || !t->root || t->root == &t->end;
}

static void *
max(struct CCC_Adaptive_map const *const t)
{
    if (!t->size)
    {
        return NULL;
    }
    struct CCC_Adaptive_map_node *m = t->root;
    for (; m->branch[R] != &t->end; m = m->branch[R])
    {}
    return struct_base(t, m);
}

static void *
min(struct CCC_Adaptive_map const *t)
{
    if (!t->size)
    {
        return NULL;
    }
    struct CCC_Adaptive_map_node *m = t->root;
    for (; m->branch[L] != &t->end; m = m->branch[L])
    {}
    return struct_base(t, m);
}

static struct CCC_Adaptive_map_node const *
next(struct CCC_Adaptive_map const *const t,
     struct CCC_Adaptive_map_node const *n, enum Branch const traversal)
{
    if (!n || n == &t->end)
    {
        return NULL;
    }
    assert(t->root->parent == &t->end);
    /* The node is a parent, backtracked to, or the end. */
    if (n->branch[traversal] != &t->end)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->branch[traversal]; n->branch[!traversal] != &t->end;
             n = n->branch[!traversal])
        {}
        return n;
    }
    /* This is how to return internal nodes on the way back up from a leaf. */
    struct CCC_Adaptive_map_node const *p = n->parent;
    for (; p != &t->end && p->branch[!traversal] != n; n = p, p = n->parent)
    {}
    return p;
}

static struct CCC_Range
equal_range(struct CCC_Adaptive_map *const t, void const *const begin_key,
            void const *const end_key, enum Branch const traversal)
{
    if (!t->size)
    {
        return (struct CCC_Range){};
    }
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    CCC_Order const les_or_grt[2] = {CCC_ORDER_LESSER, CCC_ORDER_GREATER};
    struct CCC_Adaptive_map_node const *b
        = splay(t, t->root, begin_key, t->compare);
    if (order(t, begin_key, b, t->compare) == les_or_grt[traversal])
    {
        b = next(t, b, traversal);
    }
    struct CCC_Adaptive_map_node const *e
        = splay(t, t->root, end_key, t->compare);
    if (order(t, end_key, e, t->compare) != les_or_grt[!traversal])
    {
        e = next(t, e, traversal);
    }
    return (struct CCC_Range){
        .begin = b == &t->end ? NULL : struct_base(t, b),
        .end = e == &t->end ? NULL : struct_base(t, e),
    };
}

static void *
find(struct CCC_Adaptive_map *const t, void const *const key)
{
    if (t->root == &t->end)
    {
        return NULL;
    }
    t->root = splay(t, t->root, key, t->compare);
    return order(t, key, t->root, t->compare) == CCC_ORDER_EQUAL
             ? struct_base(t, t->root)
             : NULL;
}

static CCC_Tribool
contains(struct CCC_Adaptive_map *const t, void const *const key)
{
    t->root = splay(t, t->root, key, t->compare);
    return order(t, key, t->root, t->compare) == CCC_ORDER_EQUAL;
}

static void *
allocate_insert(struct CCC_Adaptive_map *const t,
                struct CCC_Adaptive_map_node *out_handle)
{
    init_node(t, out_handle);
    CCC_Order root_order = CCC_ORDER_ERROR;
    if (!empty(t))
    {
        void const *const key = key_from_node(t, out_handle);
        t->root = splay(t, t->root, key, t->compare);
        root_order = order(t, key, t->root, t->compare);
        if (CCC_ORDER_EQUAL == root_order)
        {
            return NULL;
        }
    }
    if (t->allocate)
    {
        void *const node = t->allocate((CCC_Allocator_context){
            .input = NULL,
            .bytes = t->sizeof_type,
            .context = t->context,
        });
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(t, out_handle), t->sizeof_type);
        out_handle = elem_in_slot(t, node);
        init_node(t, out_handle);
    }
    if (empty(t))
    {
        t->root = out_handle;
        t->size = 1;
        return struct_base(t, out_handle);
    }
    assert(root_order != CCC_ORDER_ERROR);
    t->size++;
    return connect_new_root(t, out_handle, root_order);
}

static void *
insert(struct CCC_Adaptive_map *const t, struct CCC_Adaptive_map_node *const n)
{
    init_node(t, n);
    if (empty(t))
    {
        t->root = n;
        t->size = 1;
        return struct_base(t, n);
    }
    void const *const key = key_from_node(t, n);
    t->root = splay(t, t->root, key, t->compare);
    CCC_Order const root_order = order(t, key, t->root, t->compare);
    if (CCC_ORDER_EQUAL == root_order)
    {
        return NULL;
    }
    t->size++;
    return connect_new_root(t, n, root_order);
}

static void *
connect_new_root(struct CCC_Adaptive_map *const t,
                 struct CCC_Adaptive_map_node *const new_root,
                 CCC_Order const order_result)
{
    enum Branch const dir = CCC_ORDER_GREATER == order_result;
    link(new_root, dir, t->root->branch[dir]);
    link(new_root, !dir, t->root);
    t->root->branch[dir] = &t->end;
    t->root = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link(&t->end, 0, t->root);
    return struct_base(t, new_root);
}

static void *
erase(struct CCC_Adaptive_map *const t, void const *const key)
{
    if (empty(t))
    {
        return NULL;
    }
    struct CCC_Adaptive_map_node *ret = splay(t, t->root, key, t->compare);
    CCC_Order const found = order(t, key, ret, t->compare);
    if (found != CCC_ORDER_EQUAL)
    {
        return NULL;
    }
    ret = remove_from_tree(t, ret);
    ret->branch[L] = ret->branch[R] = ret->parent = NULL;
    t->size--;
    return struct_base(t, ret);
}

static struct CCC_Adaptive_map_node *
remove_from_tree(struct CCC_Adaptive_map *const t,
                 struct CCC_Adaptive_map_node *const ret)
{
    if (ret->branch[L] == &t->end)
    {
        t->root = ret->branch[R];
        link(&t->end, 0, t->root);
    }
    else
    {
        t->root = splay(t, ret->branch[L], key_from_node(t, ret), t->compare);
        link(t->root, R, ret->branch[R]);
    }
    return ret;
}

static struct CCC_Adaptive_map_node *
splay(struct CCC_Adaptive_map *const t, struct CCC_Adaptive_map_node *root,
      void const *const key, CCC_Key_comparator *const order_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    t->end.branch[L] = t->end.branch[R] = t->end.parent = &t->end;
    struct CCC_Adaptive_map_node *l_r_subtrees[LR] = {&t->end, &t->end};
    for (;;)
    {
        CCC_Order const root_order = order(t, key, root, order_fn);
        enum Branch const dir = CCC_ORDER_GREATER == root_order;
        if (CCC_ORDER_EQUAL == root_order || root->branch[dir] == &t->end)
        {
            break;
        }
        CCC_Order const child_order
            = order(t, key, root->branch[dir], order_fn);
        enum Branch const dir_from_child = CCC_ORDER_GREATER == child_order;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_ORDER_EQUAL != child_order && dir == dir_from_child)
        {
            struct CCC_Adaptive_map_node *const pivot = root->branch[dir];
            link(root, dir, pivot->branch[!dir]);
            link(pivot, !dir, root);
            root = pivot;
            if (root->branch[dir] == &t->end)
            {
                break;
            }
        }
        link(l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = root->branch[dir];
    }
    link(l_r_subtrees[L], R, root->branch[L]);
    link(l_r_subtrees[R], L, root->branch[R]);
    link(root, L, t->end.branch[R]);
    link(root, R, t->end.branch[L]);
    t->root = root;
    link(&t->end, 0, t->root);
    return root;
}

static inline void *
struct_base(struct CCC_Adaptive_map const *const t,
            struct CCC_Adaptive_map_node const *const n)
{
    /* Link is the first field of the struct and is an array so no need to get
       pointer address of [0] element of array. That's the same as just the
       array field. */
    return ((char *)n->branch) - t->node_node_offset;
}

static inline CCC_Order
order(struct CCC_Adaptive_map const *const t, void const *const key,
      struct CCC_Adaptive_map_node const *const node,
      CCC_Key_comparator *const fn)
{
    return fn((CCC_Key_comparator_context){
        .key_lhs = key,
        .type_rhs = struct_base(t, node),
        .context = t->context,
    });
}

static inline void
swap(char temp[const], void *const a, void *const b, size_t sizeof_type)
{
    if (a == b || !a || !b)
    {
        return;
    }
    (void)memcpy(temp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, temp, sizeof_type);
}

static inline void
link(struct CCC_Adaptive_map_node *const parent, enum Branch const dir,
     struct CCC_Adaptive_map_node *const subtree)
{
    parent->branch[dir] = subtree;
    subtree->parent = parent;
}

/* NOLINTBEGIN(*misc-no-recursion) */

/* ======================        Debugging           ====================== */

/** @internal Validate binary tree invariants with ranges. Use a recursive
method that does not rely upon the implementation of iterators or any other
possibly buggy implementation. A pure functional range check will provide the
most reliable check regardless of implementation changes throughout code base.
*/
struct Tree_range
{
    struct CCC_Adaptive_map_node const *low;
    struct CCC_Adaptive_map_node const *root;
    struct CCC_Adaptive_map_node const *high;
};

/** @internal */
struct Parent_status
{
    CCC_Tribool correct;
    struct CCC_Adaptive_map_node const *parent;
};

static size_t
recursive_count(struct CCC_Adaptive_map const *const t,
                struct CCC_Adaptive_map_node const *const r)
{
    if (r == &t->end)
    {
        return 0;
    }
    return 1 + recursive_count(t, r->branch[R])
         + recursive_count(t, r->branch[L]);
}

static CCC_Tribool
are_subtrees_valid(struct CCC_Adaptive_map const *const t,
                   struct Tree_range const r,
                   struct CCC_Adaptive_map_node const *const nil)
{
    if (!r.root)
    {
        return CCC_FALSE;
    }
    if (r.root == nil)
    {
        return CCC_TRUE;
    }
    if (r.low != nil
        && order(t, key_from_node(t, r.low), r.root, t->compare)
               != CCC_ORDER_LESSER)
    {
        return CCC_FALSE;
    }
    if (r.high != nil
        && order(t, key_from_node(t, r.high), r.root, t->compare)
               != CCC_ORDER_GREATER)
    {
        return CCC_FALSE;
    }
    return are_subtrees_valid(t,
                              (struct Tree_range){
                                  .low = r.low,
                                  .root = r.root->branch[L],
                                  .high = r.root,
                              },
                              nil)
        && are_subtrees_valid(t,
                              (struct Tree_range){
                                  .low = r.root,
                                  .root = r.root->branch[R],
                                  .high = r.high,
                              },
                              nil);
}

static CCC_Tribool
is_parent_correct(struct CCC_Adaptive_map const *const t,
                  struct CCC_Adaptive_map_node const *const parent,
                  struct CCC_Adaptive_map_node const *const root)
{
    if (root == &t->end)
    {
        return CCC_TRUE;
    }
    if (root->parent != parent)
    {
        return CCC_FALSE;
    }
    return is_parent_correct(t, root, root->branch[L])
        && is_parent_correct(t, root, root->branch[R]);
}

/* Validate tree prefers to use recursion to examine the tree over the
   provided iterators of any implementation so as to avoid using a
   flawed implementation of such iterators. This should help be more
   sure that the implementation is correct because it follows the
   truth of the provided pointers with its own stack as backtracking
   information. */
static CCC_Tribool
validate(struct CCC_Adaptive_map const *const t)
{
    if (!are_subtrees_valid(t,
                            (struct Tree_range){
                                .low = &t->end,
                                .root = t->root,
                                .high = &t->end,
                            },
                            &t->end))
    {
        return CCC_FALSE;
    }
    if (!is_parent_correct(t, &t->end, t->root))
    {
        return CCC_FALSE;
    }
    if (recursive_count(t, t->root) != t->size)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
