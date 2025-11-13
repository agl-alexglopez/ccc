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

#include "ordered_map.h"
#include "private/private_ordered_map.h"
#include "private/private_types.h"
#include "types.h"

/** @private Instead of thinking about left and right consider only links
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

static struct CCC_Ordered_map_entry container_entry(struct CCC_Ordered_map *t,
                                                    void const *key);

/* No return value. */

static void init_node(struct CCC_Ordered_map *, struct CCC_Ordered_map_node *);
static void swap(char tmp[const], void *, void *, size_t);
static void link(struct CCC_Ordered_map_node *, enum Branch,
                 struct CCC_Ordered_map_node *);

/* Boolean returns */

static CCC_Tribool empty(struct CCC_Ordered_map const *);
static CCC_Tribool contains(struct CCC_Ordered_map *, void const *);
static CCC_Tribool validate(struct CCC_Ordered_map const *);

/* Returning the user type that is stored in data structure. */

static void *struct_base(struct CCC_Ordered_map const *,
                         struct CCC_Ordered_map_node const *);
static void *find(struct CCC_Ordered_map *, void const *);
static void *erase(struct CCC_Ordered_map *, void const *key);
static void *allocate_insert(struct CCC_Ordered_map *,
                             struct CCC_Ordered_map_node *);
static void *insert(struct CCC_Ordered_map *, struct CCC_Ordered_map_node *);
static void *connect_new_root(struct CCC_Ordered_map *,
                              struct CCC_Ordered_map_node *, CCC_Order);
static void *max(struct CCC_Ordered_map const *);
static void *min(struct CCC_Ordered_map const *);
static void *key_in_slot(struct CCC_Ordered_map const *t, void const *slot);
static void *key_from_node(struct CCC_Ordered_map const *,
                           CCC_Ordered_map_node const *);
static struct CCC_Range equal_range(struct CCC_Ordered_map *, void const *,
                                    void const *, enum Branch);

/* Internal operations that take and return nodes for the tree. */

static struct CCC_Ordered_map_node *
remove_from_tree(struct CCC_Ordered_map *, struct CCC_Ordered_map_node *);
static struct CCC_Ordered_map_node const *
next(struct CCC_Ordered_map const *, struct CCC_Ordered_map_node const *,
     enum Branch);
static struct CCC_Ordered_map_node *splay(struct CCC_Ordered_map *,
                                          struct CCC_Ordered_map_node *,
                                          void const *key,
                                          CCC_Key_comparator *);
static struct CCC_Ordered_map_node *
elem_in_slot(struct CCC_Ordered_map const *t, void const *slot);

/* The key comes first. It is the "left hand side" of the comparison. */
static CCC_Order order(struct CCC_Ordered_map const *, void const *key,
                       struct CCC_Ordered_map_node const *,
                       CCC_Key_comparator *);

/* ======================        Map Interface      ====================== */

CCC_Tribool
CCC_ordered_map_is_empty(CCC_Ordered_map const *const om)
{
    if (!om)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return empty(om);
}

CCC_Count
CCC_ordered_map_count(CCC_Ordered_map const *const om)
{
    if (!om)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = om->size};
}

CCC_Tribool
CCC_ordered_map_contains(CCC_Ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return contains(om, key);
}

CCC_Ordered_map_entry
CCC_ordered_map_entry(CCC_Ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return (CCC_Ordered_map_entry){
            {.entry = {.stats = CCC_ENTRY_ARG_ERROR}}};
    }
    return (CCC_Ordered_map_entry){container_entry(om, key)};
}

void *
CCC_ordered_map_insert_entry(CCC_Ordered_map_entry const *const e,
                             CCC_Ordered_map_node *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->private.entry.stats == CCC_ENTRY_OCCUPIED)
    {
        if (e->private.entry.e)
        {
            *elem = *elem_in_slot(e->private.t, e->private.entry.e);
            (void)memcpy(e->private.entry.e, struct_base(e->private.t, elem),
                         e->private.t->sizeof_type);
        }
        return e->private.entry.e;
    }
    return allocate_insert(e->private.t, elem);
}

void *
CCC_ordered_map_or_insert(CCC_Ordered_map_entry const *const e,
                          CCC_Ordered_map_node *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->private.entry.stats & CCC_ENTRY_OCCUPIED)
    {
        return e->private.entry.e;
    }
    return allocate_insert(e->private.t, elem);
}

CCC_Ordered_map_entry *
CCC_ordered_map_and_modify(CCC_Ordered_map_entry *const e,
                           CCC_Type_updater *const fn)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->private.entry.stats & CCC_ENTRY_OCCUPIED && e->private.entry.e)
    {
        fn((CCC_Type_context){
            .type = e->private.entry.e,
            .context = NULL,
        });
    }
    return e;
}

CCC_Ordered_map_entry *
CCC_ordered_map_and_modify_context(CCC_Ordered_map_entry *const e,
                                   CCC_Type_updater *const fn,
                                   void *const context)
{
    if (e && fn && e->private.entry.stats & CCC_ENTRY_OCCUPIED
        && e->private.entry.e)
    {
        fn((CCC_Type_context){
            .type = e->private.entry.e,
            .context = context,
        });
    }
    return e;
}

CCC_Entry
CCC_ordered_map_swap_entry(CCC_Ordered_map *const om,
                           CCC_Ordered_map_node *const key_val_handle,
                           CCC_Ordered_map_node *const tmp)
{
    if (!om || !key_val_handle || !tmp)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const found = find(om, key_from_node(om, key_val_handle));
    if (found)
    {
        assert(om->root != &om->end);
        *key_val_handle = *om->root;
        void *const any_struct = struct_base(om, key_val_handle);
        void *const in_tree = struct_base(om, om->root);
        void *const old_val = struct_base(om, tmp);
        swap(old_val, in_tree, any_struct, om->sizeof_type);
        key_val_handle->branch[L] = key_val_handle->branch[R]
            = key_val_handle->parent = NULL;
        tmp->branch[L] = tmp->branch[R] = tmp->parent = NULL;
        return (CCC_Entry){{
            .e = old_val,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = allocate_insert(om, key_val_handle);
    if (!inserted)
    {
        return (CCC_Entry){{
            .e = NULL,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Entry){{
        .e = NULL,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Entry
CCC_ordered_map_try_insert(CCC_Ordered_map *const om,
                           CCC_Ordered_map_node *const key_val_handle)
{
    if (!om || !key_val_handle)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const found = find(om, key_from_node(om, key_val_handle));
    if (found)
    {
        assert(om->root != &om->end);
        return (CCC_Entry){{
            .e = struct_base(om, om->root),
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = allocate_insert(om, key_val_handle);
    if (!inserted)
    {
        return (CCC_Entry){{
            .e = NULL,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Entry){{
        .e = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Entry
CCC_ordered_map_insert_or_assign(CCC_Ordered_map *const om,
                                 CCC_Ordered_map_node *const key_val_handle)
{
    if (!om || !key_val_handle)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const found = find(om, key_from_node(om, key_val_handle));
    if (found)
    {
        *key_val_handle = *elem_in_slot(om, found);
        assert(om->root != &om->end);
        memcpy(found, struct_base(om, key_val_handle), om->sizeof_type);
        return (CCC_Entry){{
            .e = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = allocate_insert(om, key_val_handle);
    if (!inserted)
    {
        return (CCC_Entry){{
            .e = NULL,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Entry){{
        .e = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Entry
CCC_ordered_map_remove(CCC_Ordered_map *const om,
                       CCC_Ordered_map_node *const out_handle)
{
    if (!om || !out_handle)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const n = erase(om, key_from_node(om, out_handle));
    if (!n)
    {
        return (CCC_Entry){{
            .e = NULL,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    if (om->allocate)
    {
        void *const any_struct = struct_base(om, out_handle);
        memcpy(any_struct, n, om->sizeof_type);
        om->allocate((CCC_Allocator_context){
            .input = n,
            .bytes = 0,
            .context = om->context,
        });
        return (CCC_Entry){{
            .e = any_struct,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Entry){{
        .e = n,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

CCC_Entry
CCC_ordered_map_remove_entry(CCC_Ordered_map_entry *const e)
{
    if (!e)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (e->private.entry.stats == CCC_ENTRY_OCCUPIED && e->private.entry.e)
    {
        void *const erased = erase(
            e->private.t, key_in_slot(e->private.t, e->private.entry.e));
        assert(erased);
        if (e->private.t->allocate)
        {
            e->private.t->allocate((CCC_Allocator_context){
                .input = erased,
                .bytes = 0,
                .context = e->private.t->context,
            });
            return (CCC_Entry){{
                .e = NULL,
                .stats = CCC_ENTRY_OCCUPIED,
            }};
        }
        return (CCC_Entry){{
            .e = erased,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Entry){{
        .e = NULL,
        .stats = CCC_ENTRY_VACANT,
    }};
}

void *
CCC_ordered_map_get_key_val(CCC_Ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return NULL;
    }
    return find(om, key);
}

void *
CCC_ordered_map_unwrap(CCC_Ordered_map_entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->private.entry.stats == CCC_ENTRY_OCCUPIED ? e->private.entry.e
                                                        : NULL;
}

CCC_Tribool
CCC_ordered_map_insert_error(CCC_Ordered_map_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->private.entry.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_ordered_map_occupied(CCC_Ordered_map_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->private.entry.stats & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Entry_status
CCC_ordered_map_entry_status(CCC_Ordered_map_entry const *const e)
{
    return e ? e->private.entry.stats : CCC_ENTRY_ARG_ERROR;
}

void *
CCC_ordered_map_begin(CCC_Ordered_map const *const om)
{
    return om ? min(om) : NULL;
}

void *
CCC_ordered_map_rbegin(CCC_Ordered_map const *const om)
{
    return om ? max(om) : NULL;
}

void *
CCC_ordered_map_end(CCC_Ordered_map const *const)
{
    return NULL;
}

void *
CCC_ordered_map_rend(CCC_Ordered_map const *const)
{
    return NULL;
}

void *
CCC_ordered_map_next(CCC_Ordered_map const *const om,
                     CCC_Ordered_map_node const *const e)
{
    if (!om || !e)
    {
        return NULL;
    }
    struct CCC_Ordered_map_node const *n = next(om, e, INORDER);
    return n == &om->end ? NULL : struct_base(om, n);
}

void *
CCC_ordered_map_rnext(CCC_Ordered_map const *const om,
                      CCC_Ordered_map_node const *const e)
{
    if (!om || !e)
    {
        return NULL;
    }
    struct CCC_Ordered_map_node const *n = next(om, e, R_INORDER);
    return n == &om->end ? NULL : struct_base(om, n);
}

CCC_Range
CCC_ordered_map_equal_range(CCC_Ordered_map *const om,
                            void const *const begin_key,
                            void const *const end_key)
{
    if (!om || !begin_key || !end_key)
    {
        return (CCC_Range){};
    }
    return (CCC_Range){equal_range(om, begin_key, end_key, INORDER)};
}

CCC_Reverse_range
CCC_ordered_map_equal_rrange(CCC_Ordered_map *const om,
                             void const *const rbegin_key,
                             void const *const rend_key)

{
    if (!om || !rbegin_key || !rend_key)
    {
        return (CCC_Reverse_range){};
    }
    return (CCC_Reverse_range){
        equal_range(om, rbegin_key, rend_key, R_INORDER)};
}

/** This is a linear time constant space deletion of tree nodes via left
rotations so element fields are modified during progression of deletes. */
CCC_Result
CCC_ordered_map_clear(CCC_Ordered_map *const om,
                      CCC_Type_destructor *const destructor)
{
    if (!om)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Ordered_map_node *node = om->root;
    while (node != &om->end)
    {
        if (node->branch[L] != &om->end)
        {
            struct CCC_Ordered_map_node *const l = node->branch[L];
            node->branch[L] = l->branch[R];
            l->branch[R] = node;
            node = l;
            continue;
        }
        struct CCC_Ordered_map_node *const next = node->branch[R];
        node->branch[L] = node->branch[R] = NULL;
        node->parent = NULL;
        void *const del = struct_base(om, node);
        if (destructor)
        {
            destructor((CCC_Type_context){
                .type = del,
                .context = om->context,
            });
        }
        if (om->allocate)
        {
            (void)om->allocate((CCC_Allocator_context){
                .input = del,
                .bytes = 0,
                .context = om->context,
            });
        }
        node = next;
    }
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_ordered_map_validate(CCC_Ordered_map const *const om)
{
    if (!om)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(om);
}

/*==========================  Private Interface  ============================*/

struct CCC_Ordered_map_entry
CCC_private_ordered_map_entry(struct CCC_Ordered_map *const t,
                              void const *const key)
{
    return container_entry(t, key);
}

void *
CCC_private_ordered_map_insert(struct CCC_Ordered_map *const t,
                               struct CCC_Ordered_map_node *n)
{
    return insert(t, n);
}

void *
CCC_private_ordered_map_key_in_slot(struct CCC_Ordered_map const *const t,
                                    void const *const slot)
{
    return key_in_slot(t, slot);
}

struct CCC_Ordered_map_node *
CCC_private_Ordered_map_node_in_slot(struct CCC_Ordered_map const *const t,
                                     void const *slot)
{

    return elem_in_slot(t, slot);
}

/*======================  Static Splay Tree Helpers  ========================*/

static struct CCC_Ordered_map_entry
container_entry(struct CCC_Ordered_map *const t, void const *const key)
{
    void *const found = find(t, key);
    if (found)
    {
        return (struct CCC_Ordered_map_entry){
            .t = t,
            .entry = {
                .e = found,
                .stats = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct CCC_Ordered_map_entry){
        .t = t,
        .entry = {
            .e = found,
            .stats = CCC_ENTRY_VACANT,
        },
    };
}

static inline void *
key_from_node(struct CCC_Ordered_map const *const t,
              CCC_Ordered_map_node const *const n)
{
    return (char *)struct_base(t, n) + t->key_offset;
}

static inline void *
key_in_slot(struct CCC_Ordered_map const *const t, void const *const slot)
{
    return (char *)slot + t->key_offset;
}

static inline struct CCC_Ordered_map_node *
elem_in_slot(struct CCC_Ordered_map const *const t, void const *const slot)
{

    return (struct CCC_Ordered_map_node *)((char *)slot + t->node_node_offset);
}

static inline void
init_node(struct CCC_Ordered_map *const t, struct CCC_Ordered_map_node *const n)
{
    n->branch[L] = &t->end;
    n->branch[R] = &t->end;
    n->parent = &t->end;
}

static inline CCC_Tribool
empty(struct CCC_Ordered_map const *const t)
{
    return !t->size || !t->root || t->root == &t->end;
}

static void *
max(struct CCC_Ordered_map const *const t)
{
    if (!t->size)
    {
        return NULL;
    }
    struct CCC_Ordered_map_node *m = t->root;
    for (; m->branch[R] != &t->end; m = m->branch[R])
    {}
    return struct_base(t, m);
}

static void *
min(struct CCC_Ordered_map const *t)
{
    if (!t->size)
    {
        return NULL;
    }
    struct CCC_Ordered_map_node *m = t->root;
    for (; m->branch[L] != &t->end; m = m->branch[L])
    {}
    return struct_base(t, m);
}

static struct CCC_Ordered_map_node const *
next(struct CCC_Ordered_map const *const t,
     struct CCC_Ordered_map_node const *n, enum Branch const traversal)
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
    struct CCC_Ordered_map_node const *p = n->parent;
    for (; p != &t->end && p->branch[!traversal] != n; n = p, p = n->parent)
    {}
    return p;
}

static struct CCC_Range
equal_range(struct CCC_Ordered_map *const t, void const *const begin_key,
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
    struct CCC_Ordered_map_node const *b
        = splay(t, t->root, begin_key, t->order);
    if (order(t, begin_key, b, t->order) == les_or_grt[traversal])
    {
        b = next(t, b, traversal);
    }
    struct CCC_Ordered_map_node const *e = splay(t, t->root, end_key, t->order);
    if (order(t, end_key, e, t->order) != les_or_grt[!traversal])
    {
        e = next(t, e, traversal);
    }
    return (struct CCC_Range){
        .begin = b == &t->end ? NULL : struct_base(t, b),
        .end = e == &t->end ? NULL : struct_base(t, e),
    };
}

static void *
find(struct CCC_Ordered_map *const t, void const *const key)
{
    if (t->root == &t->end)
    {
        return NULL;
    }
    t->root = splay(t, t->root, key, t->order);
    return order(t, key, t->root, t->order) == CCC_ORDER_EQUAL
             ? struct_base(t, t->root)
             : NULL;
}

static CCC_Tribool
contains(struct CCC_Ordered_map *const t, void const *const key)
{
    t->root = splay(t, t->root, key, t->order);
    return order(t, key, t->root, t->order) == CCC_ORDER_EQUAL;
}

static void *
allocate_insert(struct CCC_Ordered_map *const t,
                struct CCC_Ordered_map_node *out_handle)
{
    init_node(t, out_handle);
    CCC_Order root_order = CCC_ORDER_ERROR;
    if (!empty(t))
    {
        void const *const key = key_from_node(t, out_handle);
        t->root = splay(t, t->root, key, t->order);
        root_order = order(t, key, t->root, t->order);
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
insert(struct CCC_Ordered_map *const t, struct CCC_Ordered_map_node *const n)
{
    init_node(t, n);
    if (empty(t))
    {
        t->root = n;
        t->size = 1;
        return struct_base(t, n);
    }
    void const *const key = key_from_node(t, n);
    t->root = splay(t, t->root, key, t->order);
    CCC_Order const root_order = order(t, key, t->root, t->order);
    if (CCC_ORDER_EQUAL == root_order)
    {
        return NULL;
    }
    t->size++;
    return connect_new_root(t, n, root_order);
}

static void *
connect_new_root(struct CCC_Ordered_map *const t,
                 struct CCC_Ordered_map_node *const new_root,
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
erase(struct CCC_Ordered_map *const t, void const *const key)
{
    if (empty(t))
    {
        return NULL;
    }
    struct CCC_Ordered_map_node *ret = splay(t, t->root, key, t->order);
    CCC_Order const found = order(t, key, ret, t->order);
    if (found != CCC_ORDER_EQUAL)
    {
        return NULL;
    }
    ret = remove_from_tree(t, ret);
    ret->branch[L] = ret->branch[R] = ret->parent = NULL;
    t->size--;
    return struct_base(t, ret);
}

static struct CCC_Ordered_map_node *
remove_from_tree(struct CCC_Ordered_map *const t,
                 struct CCC_Ordered_map_node *const ret)
{
    if (ret->branch[L] == &t->end)
    {
        t->root = ret->branch[R];
        link(&t->end, 0, t->root);
    }
    else
    {
        t->root = splay(t, ret->branch[L], key_from_node(t, ret), t->order);
        link(t->root, R, ret->branch[R]);
    }
    return ret;
}

static struct CCC_Ordered_map_node *
splay(struct CCC_Ordered_map *const t, struct CCC_Ordered_map_node *root,
      void const *const key, CCC_Key_comparator *const order_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    t->end.branch[L] = t->end.branch[R] = t->end.parent = &t->end;
    struct CCC_Ordered_map_node *l_r_subtrees[LR] = {&t->end, &t->end};
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
            struct CCC_Ordered_map_node *const pivot = root->branch[dir];
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
struct_base(struct CCC_Ordered_map const *const t,
            struct CCC_Ordered_map_node const *const n)
{
    /* Link is the first field of the struct and is an array so no need to get
       pointer address of [0] element of array. That's the same as just the
       array field. */
    return ((char *)n->branch) - t->node_node_offset;
}

static inline CCC_Order
order(struct CCC_Ordered_map const *const t, void const *const key,
      struct CCC_Ordered_map_node const *const node,
      CCC_Key_comparator *const fn)
{
    return fn((CCC_Key_comparator_context){
        .key_lhs = key,
        .type_rhs = struct_base(t, node),
        .context = t->context,
    });
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t sizeof_type)
{
    if (a == b || !a || !b)
    {
        return;
    }
    (void)memcpy(tmp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, tmp, sizeof_type);
}

static inline void
link(struct CCC_Ordered_map_node *const parent, enum Branch const dir,
     struct CCC_Ordered_map_node *const subtree)
{
    parent->branch[dir] = subtree;
    subtree->parent = parent;
}

/* NOLINTBEGIN(*misc-no-recursion) */

/* ======================        Debugging           ====================== */

/** @private Validate binary tree invariants with ranges. Use a recursive method
that does not rely upon the implementation of iterators or any other possibly
buggy implementation. A pure functional range check will provide the most
reliable check regardless of implementation changes throughout code base. */
struct Tree_range
{
    struct CCC_Ordered_map_node const *low;
    struct CCC_Ordered_map_node const *root;
    struct CCC_Ordered_map_node const *high;
};

/** @private */
struct Parent_status
{
    CCC_Tribool correct;
    struct CCC_Ordered_map_node const *parent;
};

static size_t
recursive_count(struct CCC_Ordered_map const *const t,
                struct CCC_Ordered_map_node const *const r)
{
    if (r == &t->end)
    {
        return 0;
    }
    return 1 + recursive_count(t, r->branch[R])
         + recursive_count(t, r->branch[L]);
}

static CCC_Tribool
are_subtrees_valid(struct CCC_Ordered_map const *const t,
                   struct Tree_range const r,
                   struct CCC_Ordered_map_node const *const nil)
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
        && order(t, key_from_node(t, r.low), r.root, t->order)
               != CCC_ORDER_LESSER)
    {
        return CCC_FALSE;
    }
    if (r.high != nil
        && order(t, key_from_node(t, r.high), r.root, t->order)
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
is_parent_correct(struct CCC_Ordered_map const *const t,
                  struct CCC_Ordered_map_node const *const parent,
                  struct CCC_Ordered_map_node const *const root)
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
validate(struct CCC_Ordered_map const *const t)
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
