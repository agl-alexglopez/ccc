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

This file contains my implementation of a realtime ordered map. The added
realtime prefix is to indicate that this map meets specific run time bounds
that can be relied upon consistently. This is may not be the case if a map
is implemented with some self-optimizing data structure like a Splay Tree.

This map, however, promises O(lg N) search, insert, and remove as a true
upper bound, inclusive. This is achieved through a Weak AVL (WAVL) tree
that is derived from the following two sources.

[1] Bernhard Haeupler, Siddhartha Sen, and Robert E. Tarjan, 2014.
Rank-Balanced Trees, J.ACM Transactions on Algorithms 11, 4, Article 0
(June 2015), 24 pages.
https://sidsen.azurewebsites.net//papers/rb-trees-talg.pdf

[2] Phil Vachon (pvachon) https://github.com/pvachon/wavl_tree
This implementation is heavily influential throughout. However there have
been some major adjustments and simplifications. Namely, the allocation has
been adjusted to accommodate this library's ability to be an allocating or
non-allocating container. All left-right symmetric cases have been united
into one and I chose to tackle rotations and deletions slightly differently,
shortening the code significantly. A few other changes and improvements
suggested by the authors of the original paper are implemented. See the required
license at the bottom of the file for BSD-2-Clause compliance.

Overall a WAVL tree is quite impressive for it's simplicity and purported
improvements over AVL and Red-Black trees. The rank framework is intuitive
and flexible in how it can be implemented. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "private/private_realtime_ordered_map.h"
#include "private/private_types.h"
#include "realtime_ordered_map.h"
#include "types.h"

/** @private */
enum Link
{
    L = 0,
    R,
};

#define INORDER R
#define R_INORDER L

/** @private This will utilize safe type punning in C. Both union fields have
the same type and when obtaining an entry we either have the desired element
or its parent. Preserving the known parent is what makes the Entry Interface
No further look ups are required for insertions, modification, or removal. */
struct Query
{
    CCC_Order last_order;
    union
    {
        struct CCC_Realtime_ordered_map_node *found;
        struct CCC_Realtime_ordered_map_node *parent;
    };
};

/*==============================  Prototypes   ==============================*/

static void init_node(struct CCC_Realtime_ordered_map *,
                      struct CCC_Realtime_ordered_map_node *);
static CCC_Order order(struct CCC_Realtime_ordered_map const *, void const *key,
                       struct CCC_Realtime_ordered_map_node const *node,
                       CCC_Key_comparator *);
static void *struct_base(struct CCC_Realtime_ordered_map const *,
                         struct CCC_Realtime_ordered_map_node const *);
static struct Query find(struct CCC_Realtime_ordered_map const *,
                         void const *key);
static void swap(void *tmp, void *a, void *b, size_t sizeof_type);
static void *maybe_alloc_insert(struct CCC_Realtime_ordered_map *,
                                struct CCC_Realtime_ordered_map_node *parent,
                                CCC_Order last_order,
                                struct CCC_Realtime_ordered_map_node *);
static void *remove_fixup(struct CCC_Realtime_ordered_map *,
                          struct CCC_Realtime_ordered_map_node *);
static void insert_fixup(struct CCC_Realtime_ordered_map *,
                         struct CCC_Realtime_ordered_map_node *z,
                         struct CCC_Realtime_ordered_map_node *x);
static void transplant(struct CCC_Realtime_ordered_map *,
                       struct CCC_Realtime_ordered_map_node *remove,
                       struct CCC_Realtime_ordered_map_node *replacement);
static void rebalance_3_child(struct CCC_Realtime_ordered_map *rom,
                              struct CCC_Realtime_ordered_map_node *z,
                              struct CCC_Realtime_ordered_map_node *x);
static CCC_Tribool is_0_child(struct CCC_Realtime_ordered_map const *,
                              struct CCC_Realtime_ordered_map_node const *p,
                              struct CCC_Realtime_ordered_map_node const *x);
static CCC_Tribool is_1_child(struct CCC_Realtime_ordered_map const *,
                              struct CCC_Realtime_ordered_map_node const *p,
                              struct CCC_Realtime_ordered_map_node const *x);
static CCC_Tribool is_2_child(struct CCC_Realtime_ordered_map const *,
                              struct CCC_Realtime_ordered_map_node const *p,
                              struct CCC_Realtime_ordered_map_node const *x);
static CCC_Tribool is_3_child(struct CCC_Realtime_ordered_map const *,
                              struct CCC_Realtime_ordered_map_node const *p,
                              struct CCC_Realtime_ordered_map_node const *x);
static CCC_Tribool is_01_parent(struct CCC_Realtime_ordered_map const *,
                                struct CCC_Realtime_ordered_map_node const *x,
                                struct CCC_Realtime_ordered_map_node const *p,
                                struct CCC_Realtime_ordered_map_node const *y);
static CCC_Tribool is_11_parent(struct CCC_Realtime_ordered_map const *,
                                struct CCC_Realtime_ordered_map_node const *x,
                                struct CCC_Realtime_ordered_map_node const *p,
                                struct CCC_Realtime_ordered_map_node const *y);
static CCC_Tribool is_02_parent(struct CCC_Realtime_ordered_map const *,
                                struct CCC_Realtime_ordered_map_node const *x,
                                struct CCC_Realtime_ordered_map_node const *p,
                                struct CCC_Realtime_ordered_map_node const *y);
static CCC_Tribool is_22_parent(struct CCC_Realtime_ordered_map const *,
                                struct CCC_Realtime_ordered_map_node const *x,
                                struct CCC_Realtime_ordered_map_node const *p,
                                struct CCC_Realtime_ordered_map_node const *y);
static CCC_Tribool is_leaf(struct CCC_Realtime_ordered_map const *rom,
                           struct CCC_Realtime_ordered_map_node const *x);
static struct CCC_Realtime_ordered_map_node *
sibling_of(struct CCC_Realtime_ordered_map const *rom,
           struct CCC_Realtime_ordered_map_node const *x);
static void promote(struct CCC_Realtime_ordered_map const *rom,
                    struct CCC_Realtime_ordered_map_node *x);
static void demote(struct CCC_Realtime_ordered_map const *rom,
                   struct CCC_Realtime_ordered_map_node *x);
static void double_promote(struct CCC_Realtime_ordered_map const *rom,
                           struct CCC_Realtime_ordered_map_node *x);
static void double_demote(struct CCC_Realtime_ordered_map const *rom,
                          struct CCC_Realtime_ordered_map_node *x);

static void rotate(struct CCC_Realtime_ordered_map *rom,
                   struct CCC_Realtime_ordered_map_node *z,
                   struct CCC_Realtime_ordered_map_node *x,
                   struct CCC_Realtime_ordered_map_node *y, enum Link dir);
static void double_rotate(struct CCC_Realtime_ordered_map *rom,
                          struct CCC_Realtime_ordered_map_node *z,
                          struct CCC_Realtime_ordered_map_node *x,
                          struct CCC_Realtime_ordered_map_node *y,
                          enum Link dir);
static CCC_Tribool validate(struct CCC_Realtime_ordered_map const *rom);
static struct CCC_Realtime_ordered_map_node *
next(struct CCC_Realtime_ordered_map const *,
     struct CCC_Realtime_ordered_map_node const *, enum Link);
static struct CCC_Realtime_ordered_map_node *
min_max_from(struct CCC_Realtime_ordered_map const *,
             struct CCC_Realtime_ordered_map_node *start, enum Link);
static struct CCC_Range equal_range(struct CCC_Realtime_ordered_map const *,
                                    void const *, void const *, enum Link);
static struct CCC_rtree_entry entry(struct CCC_Realtime_ordered_map const *rom,
                                    void const *key);
static void *insert(struct CCC_Realtime_ordered_map *rom,
                    struct CCC_Realtime_ordered_map_node *parent,
                    CCC_Order last_order,
                    struct CCC_Realtime_ordered_map_node *out_handle);
static void *key_from_node(struct CCC_Realtime_ordered_map const *rom,
                           struct CCC_Realtime_ordered_map_node const *elem);
static void *key_in_slot(struct CCC_Realtime_ordered_map const *rom,
                         void const *slot);
static struct CCC_Realtime_ordered_map_node *
elem_in_slot(struct CCC_Realtime_ordered_map const *rom, void const *slot);

/*==============================  Interface    ==============================*/

CCC_Tribool
CCC_realtime_ordered_map_contains(CCC_Realtime_ordered_map const *const rom,
                                  void const *const key)
{
    if (!rom || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return CCC_ORDER_EQUAL == find(rom, key).last_order;
}

void *
CCC_realtime_ordered_map_get_key_val(CCC_Realtime_ordered_map const *const rom,
                                     void const *const key)
{
    if (!rom || !key)
    {
        return NULL;
    }
    struct Query const q = find(rom, key);
    return (CCC_ORDER_EQUAL == q.last_order) ? struct_base(rom, q.found) : NULL;
}

CCC_Entry
CCC_realtime_ordered_map_swap_entry(
    CCC_Realtime_ordered_map *const rom,
    CCC_Realtime_ordered_map_node *const key_val_handle,
    CCC_Realtime_ordered_map_node *const tmp)
{
    if (!rom || !key_val_handle || !tmp)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct Query const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        *key_val_handle = *q.found;
        void *const found = struct_base(rom, q.found);
        void *const any_struct = struct_base(rom, key_val_handle);
        void *const old_val = struct_base(rom, tmp);
        swap(old_val, found, any_struct, rom->sizeof_type);
        key_val_handle->branch[L] = key_val_handle->branch[R]
            = key_val_handle->parent = NULL;
        tmp->branch[L] = tmp->branch[R] = tmp->parent = NULL;
        return (CCC_Entry){{
            .e = old_val,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    if (!maybe_alloc_insert(rom, q.parent, q.last_order, key_val_handle))
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
CCC_realtime_ordered_map_try_insert(
    CCC_Realtime_ordered_map *const rom,
    CCC_Realtime_ordered_map_node *const key_val_handle)
{
    if (!rom || !key_val_handle)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct Query const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        return (CCC_Entry){{
            .e = struct_base(rom, q.found),
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted
        = maybe_alloc_insert(rom, q.parent, q.last_order, key_val_handle);
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
CCC_realtime_ordered_map_insert_or_assign(
    CCC_Realtime_ordered_map *const rom,
    CCC_Realtime_ordered_map_node *const key_val_handle)
{
    if (!rom || !key_val_handle)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct Query const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        void *const found = struct_base(rom, q.found);
        *key_val_handle = *elem_in_slot(rom, found);
        memcpy(found, struct_base(rom, key_val_handle), rom->sizeof_type);
        return (CCC_Entry){{
            .e = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted
        = maybe_alloc_insert(rom, q.parent, q.last_order, key_val_handle);
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

CCC_Realtime_ordered_map_entry
CCC_realtime_ordered_map_entry(CCC_Realtime_ordered_map const *const rom,
                               void const *const key)
{
    if (!rom || !key)
    {
        return (CCC_Realtime_ordered_map_entry){
            {.entry = {.stats = CCC_ENTRY_ARG_ERROR}}};
    }
    return (CCC_Realtime_ordered_map_entry){entry(rom, key)};
}

void *
CCC_realtime_ordered_map_or_insert(
    CCC_Realtime_ordered_map_entry const *const e,
    CCC_Realtime_ordered_map_node *const elem)
{
    if (!e || !elem || !e->private.entry.e)
    {
        return NULL;
    }
    if (e->private.entry.stats == CCC_ENTRY_OCCUPIED)
    {
        return e->private.entry.e;
    }
    return maybe_alloc_insert(e->private.rom,
                              elem_in_slot(e->private.rom, e->private.entry.e),
                              e->private.last_order, elem);
}

void *
CCC_realtime_ordered_map_insert_entry(
    CCC_Realtime_ordered_map_entry const *const e,
    CCC_Realtime_ordered_map_node *const elem)
{
    if (!e || !elem || !e->private.entry.e)
    {
        return NULL;
    }
    if (e->private.entry.stats == CCC_ENTRY_OCCUPIED)
    {
        *elem = *elem_in_slot(e->private.rom, e->private.entry.e);
        memcpy(e->private.entry.e, struct_base(e->private.rom, elem),
               e->private.rom->sizeof_type);
        return e->private.entry.e;
    }
    return maybe_alloc_insert(e->private.rom,
                              elem_in_slot(e->private.rom, e->private.entry.e),
                              e->private.last_order, elem);
}

CCC_Entry
CCC_realtime_ordered_map_remove_entry(
    CCC_Realtime_ordered_map_entry const *const e)
{
    if (!e || !e->private.entry.e)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (e->private.entry.stats == CCC_ENTRY_OCCUPIED)
    {
        void *const erased = remove_fixup(
            e->private.rom, elem_in_slot(e->private.rom, e->private.entry.e));
        assert(erased);
        if (e->private.rom->alloc)
        {
            e->private.rom->alloc((CCC_Allocator_context){
                .input = erased,
                .bytes = 0,
                .context = e->private.rom->context,
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

CCC_Entry
CCC_realtime_ordered_map_remove(CCC_Realtime_ordered_map *const rom,
                                CCC_Realtime_ordered_map_node *const out_handle)
{
    if (!rom || !out_handle)
    {
        return (CCC_Entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct Query const q = find(rom, key_from_node(rom, out_handle));
    if (q.last_order != CCC_ORDER_EQUAL)
    {
        return (CCC_Entry){{
            .e = NULL,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    void *const removed = remove_fixup(rom, q.found);
    if (rom->alloc)
    {
        void *const any_struct = struct_base(rom, out_handle);
        memcpy(any_struct, removed, rom->sizeof_type);
        rom->alloc((CCC_Allocator_context){
            .input = removed,
            .bytes = 0,
            .context = rom->context,
        });
        return (CCC_Entry){{
            .e = any_struct,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Entry){{
        .e = removed,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

CCC_Realtime_ordered_map_entry *
CCC_realtime_ordered_map_and_modify(CCC_Realtime_ordered_map_entry *e,
                                    CCC_Type_updater *fn)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->private.entry.stats & CCC_ENTRY_OCCUPIED && e->private.entry.e)
    {
        fn((CCC_Type_context){
            .type = e->private.entry.e,
            NULL,
        });
    }
    return e;
}

CCC_Realtime_ordered_map_entry *
CCC_realtime_ordered_map_and_modify_context(CCC_Realtime_ordered_map_entry *e,
                                            CCC_Type_updater *fn, void *context)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->private.entry.stats & CCC_ENTRY_OCCUPIED && e->private.entry.e)
    {
        fn((CCC_Type_context){
            .type = e->private.entry.e,
            context,
        });
    }
    return e;
}

void *
CCC_realtime_ordered_map_unwrap(CCC_Realtime_ordered_map_entry const *const e)
{
    if (e && e->private.entry.stats & CCC_ENTRY_OCCUPIED)
    {
        return e->private.entry.e;
    }
    return NULL;
}

CCC_Tribool
CCC_realtime_ordered_map_occupied(CCC_Realtime_ordered_map_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->private.entry.stats & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Tribool
CCC_realtime_ordered_map_insert_error(
    CCC_Realtime_ordered_map_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->private.entry.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Entry_status
CCC_realtime_ordered_map_entry_status(
    CCC_Realtime_ordered_map_entry const *const e)
{
    return e ? e->private.entry.stats : CCC_ENTRY_ARG_ERROR;
}

void *
CCC_realtime_ordered_map_begin(CCC_Realtime_ordered_map const *rom)
{
    if (!rom)
    {
        return NULL;
    }
    struct CCC_Realtime_ordered_map_node *const m
        = min_max_from(rom, rom->root, L);
    return m == &rom->end ? NULL : struct_base(rom, m);
}

void *
CCC_realtime_ordered_map_next(CCC_Realtime_ordered_map const *const rom,
                              CCC_Realtime_ordered_map_node const *const e)
{
    if (!rom || !e)
    {
        return NULL;
    }
    struct CCC_Realtime_ordered_map_node const *const n = next(rom, e, INORDER);
    if (n == &rom->end)
    {
        return NULL;
    }
    return struct_base(rom, n);
}

void *
CCC_realtime_ordered_map_rbegin(CCC_Realtime_ordered_map const *const rom)
{
    if (!rom)
    {
        return NULL;
    }
    struct CCC_Realtime_ordered_map_node *const m
        = min_max_from(rom, rom->root, R);
    return m == &rom->end ? NULL : struct_base(rom, m);
}

void *
CCC_realtime_ordered_map_end(CCC_Realtime_ordered_map const *const)
{
    return NULL;
}

void *
CCC_realtime_ordered_map_rend(CCC_Realtime_ordered_map const *const)
{
    return NULL;
}

void *
CCC_realtime_ordered_map_rnext(CCC_Realtime_ordered_map const *const rom,
                               CCC_Realtime_ordered_map_node const *const e)
{
    if (!rom || !e)
    {
        return NULL;
    }
    struct CCC_Realtime_ordered_map_node const *const n
        = next(rom, e, R_INORDER);
    return (n == &rom->end) ? NULL : struct_base(rom, n);
}

CCC_Range
CCC_realtime_ordered_map_equal_range(CCC_Realtime_ordered_map const *const rom,
                                     void const *const begin_key,
                                     void const *const end_key)
{
    if (!rom || !begin_key || !end_key)
    {
        return (CCC_Range){};
    }
    return (CCC_Range){equal_range(rom, begin_key, end_key, INORDER)};
}

CCC_Reverse_range
CCC_realtime_ordered_map_equal_rrange(CCC_Realtime_ordered_map const *const rom,
                                      void const *const rbegin_key,
                                      void const *const rend_key)
{
    if (!rom || !rbegin_key || !rend_key)
    {
        return (CCC_Reverse_range){};
    }
    return (CCC_Reverse_range){
        equal_range(rom, rbegin_key, rend_key, R_INORDER)};
}

CCC_Count
CCC_realtime_ordered_map_count(CCC_Realtime_ordered_map const *const rom)
{
    if (!rom)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = rom->count};
}

CCC_Tribool
CCC_realtime_ordered_map_is_empty(CCC_Realtime_ordered_map const *const rom)
{
    if (!rom)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !rom->count;
}

CCC_Tribool
CCC_realtime_ordered_map_validate(CCC_Realtime_ordered_map const *rom)
{
    if (!rom)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(rom);
}

/** This is a linear time constant space deletion of tree nodes via left
rotations so element fields are modified during progression of deletes. */
CCC_Result
CCC_realtime_ordered_map_clear(CCC_Realtime_ordered_map *const rom,
                               CCC_Type_destructor *const destructor)
{
    if (!rom)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Realtime_ordered_map_node *node = rom->root;
    while (node != &rom->end)
    {
        if (node->branch[L] != &rom->end)
        {
            struct CCC_Realtime_ordered_map_node *const left = node->branch[L];
            node->branch[L] = left->branch[R];
            left->branch[R] = node;
            node = left;
            continue;
        }
        struct CCC_Realtime_ordered_map_node *const next = node->branch[R];
        node->branch[L] = node->branch[R] = NULL;
        node->parent = NULL;
        void *const destroy = struct_base(rom, node);
        if (destructor)
        {
            destructor((CCC_Type_context){
                .type = destroy,
                .context = rom->context,
            });
        }
        if (rom->alloc)
        {
            (void)rom->alloc((CCC_Allocator_context){
                .input = destroy,
                .bytes = 0,
                .context = rom->context,
            });
        }
        node = next;
    }
    return CCC_RESULT_OK;
}

/*=========================   Private Interface  ============================*/

struct CCC_rtree_entry
CCC_private_realtime_ordered_map_entry(
    struct CCC_Realtime_ordered_map const *const rom, void const *const key)
{
    return entry(rom, key);
}

void *
CCC_private_realtime_ordered_map_insert(
    struct CCC_Realtime_ordered_map *const rom,
    struct CCC_Realtime_ordered_map_node *const parent,
    CCC_Order const last_order,
    struct CCC_Realtime_ordered_map_node *const out_handle)
{
    return insert(rom, parent, last_order, out_handle);
}

void *
CCC_private_realtime_ordered_map_key_in_slot(
    struct CCC_Realtime_ordered_map const *const rom, void const *const slot)
{
    return key_in_slot(rom, slot);
}

struct CCC_Realtime_ordered_map_node *
CCC_private_Realtime_ordered_map_node_in_slot(
    struct CCC_Realtime_ordered_map const *const rom, void const *const slot)
{
    return elem_in_slot(rom, slot);
}

/*=========================    Static Helpers    ============================*/

static struct CCC_Realtime_ordered_map_node *
min_max_from(struct CCC_Realtime_ordered_map const *const rom,
             struct CCC_Realtime_ordered_map_node *start, enum Link const dir)
{
    if (start == &rom->end)
    {
        return start;
    }
    for (; start->branch[dir] != &rom->end; start = start->branch[dir])
    {}
    return start;
}

static struct CCC_rtree_entry
entry(struct CCC_Realtime_ordered_map const *const rom, void const *const key)
{
    struct Query const q = find(rom, key);
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        return (struct CCC_rtree_entry){
            .rom = (struct CCC_Realtime_ordered_map *)rom,
            .last_order = q.last_order,
            .entry = {
                .e = struct_base(rom, q.found),
                .stats = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct CCC_rtree_entry){
        .rom = (struct CCC_Realtime_ordered_map *)rom,
        .last_order = q.last_order,
        .entry = {
            .e = struct_base(rom, q.parent),
            .stats = CCC_ENTRY_VACANT | CCC_ENTRY_NO_UNWRAP,
        },
    };
}

static void *
maybe_alloc_insert(struct CCC_Realtime_ordered_map *const rom,
                   struct CCC_Realtime_ordered_map_node *const parent,
                   CCC_Order const last_order,
                   struct CCC_Realtime_ordered_map_node *out_handle)
{
    if (rom->alloc)
    {
        void *const new = rom->alloc((CCC_Allocator_context){
            .input = NULL,
            .bytes = rom->sizeof_type,
            .context = rom->context,
        });
        if (!new)
        {
            return NULL;
        }
        memcpy(new, struct_base(rom, out_handle), rom->sizeof_type);
        out_handle = elem_in_slot(rom, new);
    }
    return insert(rom, parent, last_order, out_handle);
}

static void *
insert(struct CCC_Realtime_ordered_map *const rom,
       struct CCC_Realtime_ordered_map_node *const parent,
       CCC_Order const last_order,
       struct CCC_Realtime_ordered_map_node *const out_handle)
{
    init_node(rom, out_handle);
    if (!rom->count)
    {
        rom->root = out_handle;
        ++rom->count;
        return struct_base(rom, out_handle);
    }
    assert(last_order == CCC_ORDER_GREATER || last_order == CCC_ORDER_LESSER);
    CCC_Tribool const rank_rule_break
        = parent->branch[L] == &rom->end && parent->branch[R] == &rom->end;
    parent->branch[CCC_ORDER_GREATER == last_order] = out_handle;
    out_handle->parent = parent;
    if (rank_rule_break)
    {
        insert_fixup(rom, parent, out_handle);
    }
    ++rom->count;
    return struct_base(rom, out_handle);
}

static struct Query
find(struct CCC_Realtime_ordered_map const *const rom, void const *const key)
{
    struct CCC_Realtime_ordered_map_node const *parent = &rom->end;
    struct Query q = {
        .last_order = CCC_ORDER_ERROR,
        .found = rom->root,
    };
    while (q.found != &rom->end)
    {
        q.last_order = order(rom, key, q.found, rom->order);
        if (CCC_ORDER_EQUAL == q.last_order)
        {
            return q;
        }
        parent = q.found;
        q.found = q.found->branch[CCC_ORDER_GREATER == q.last_order];
    }
    /* Type punning here OK as both union members have same type and size. */
    q.parent = (struct CCC_Realtime_ordered_map_node *)parent;
    return q;
}

static struct CCC_Realtime_ordered_map_node *
next(struct CCC_Realtime_ordered_map const *const rom,
     struct CCC_Realtime_ordered_map_node const *n, enum Link const traversal)
{
    if (!n || n == &rom->end)
    {
        return (struct CCC_Realtime_ordered_map_node *)&rom->end;
    }
    assert(rom->root->parent == &rom->end);
    /* The node is an internal one that has a sub-tree to explore first. */
    if (n->branch[traversal] != &rom->end)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->branch[traversal]; n->branch[!traversal] != &rom->end;
             n = n->branch[!traversal])
        {}
        return (struct CCC_Realtime_ordered_map_node *)n;
    }
    for (; n->parent != &rom->end && n->parent->branch[!traversal] != n;
         n = n->parent)
    {}
    return n->parent;
}

static struct CCC_Range
equal_range(struct CCC_Realtime_ordered_map const *const rom,
            void const *const begin_key, void const *const end_key,
            enum Link const traversal)
{
    if (!rom->count)
    {
        return (struct CCC_Range){};
    }
    CCC_Order const les_or_grt[2] = {CCC_ORDER_LESSER, CCC_ORDER_GREATER};
    struct Query b = find(rom, begin_key);
    if (b.last_order == les_or_grt[traversal])
    {
        b.found = next(rom, b.found, traversal);
    }
    struct Query e = find(rom, end_key);
    if (e.last_order != les_or_grt[!traversal])
    {
        e.found = next(rom, e.found, traversal);
    }
    return (struct CCC_Range){
        .begin = b.found == &rom->end ? NULL : struct_base(rom, b.found),
        .end = e.found == &rom->end ? NULL : struct_base(rom, e.found),
    };
}

static inline void
init_node(struct CCC_Realtime_ordered_map *const rom,
          struct CCC_Realtime_ordered_map_node *const e)
{
    assert(e != NULL);
    assert(rom != NULL);
    e->branch[L] = e->branch[R] = e->parent = &rom->end;
    e->parity = 0;
}

static inline void
swap(void *const tmp, void *const a, void *const b, size_t const sizeof_type)
{
    if (a == b || !a || !b)
    {
        return;
    }
    (void)memcpy(tmp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, tmp, sizeof_type);
}

static inline CCC_Order
order(struct CCC_Realtime_ordered_map const *const rom, void const *const key,
      struct CCC_Realtime_ordered_map_node const *const node,
      CCC_Key_comparator *const fn)
{
    return fn((CCC_Key_comparator_context){
        .key_lhs = key,
        .type_rhs = struct_base(rom, node),
        .context = rom->context,
    });
}

static inline void *
struct_base(struct CCC_Realtime_ordered_map const *const rom,
            struct CCC_Realtime_ordered_map_node const *const e)
{
    return ((char *)e->branch) - rom->node_node_offset;
}

static inline void *
key_from_node(struct CCC_Realtime_ordered_map const *const rom,
              struct CCC_Realtime_ordered_map_node const *const elem)
{
    return (char *)struct_base(rom, elem) + rom->key_offset;
}

static inline void *
key_in_slot(struct CCC_Realtime_ordered_map const *const rom,
            void const *const slot)
{
    return (char *)slot + rom->key_offset;
}

static inline struct CCC_Realtime_ordered_map_node *
elem_in_slot(struct CCC_Realtime_ordered_map const *const rom,
             void const *const slot)
{
    return (struct CCC_Realtime_ordered_map_node *)((char *)slot
                                                    + rom->node_node_offset);
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct CCC_Realtime_ordered_map *const rom,
             struct CCC_Realtime_ordered_map_node *z,
             struct CCC_Realtime_ordered_map_node *x)
{
    do
    {
        promote(rom, z);
        x = z;
        z = z->parent;
        if (z == &rom->end)
        {
            return;
        }
    }
    while (is_01_parent(rom, x, z, sibling_of(rom, x)));

    if (!is_02_parent(rom, x, z, sibling_of(rom, x)))
    {
        return;
    }
    assert(x != &rom->end);
    assert(is_0_child(rom, z, x));
    enum Link const p_to_x_dir = z->branch[R] == x;
    struct CCC_Realtime_ordered_map_node *const y = x->branch[!p_to_x_dir];
    if (y == &rom->end || is_2_child(rom, z, y))
    {
        rotate(rom, z, x, y, !p_to_x_dir);
        demote(rom, z);
    }
    else
    {
        assert(is_1_child(rom, z, y));
        double_rotate(rom, z, x, y, p_to_x_dir);
        promote(rom, y);
        demote(rom, x);
        demote(rom, z);
    }
}

static void *
remove_fixup(struct CCC_Realtime_ordered_map *const rom,
             struct CCC_Realtime_ordered_map_node *const remove)
{
    assert(remove->branch[R] && remove->branch[L]);
    struct CCC_Realtime_ordered_map_node *y = NULL;
    struct CCC_Realtime_ordered_map_node *x = NULL;
    struct CCC_Realtime_ordered_map_node *p_of_xy = NULL;
    CCC_Tribool two_child = CCC_FALSE;
    if (remove->branch[L] == &rom->end || remove->branch[R] == &rom->end)
    {
        y = remove;
        p_of_xy = y->parent;
        x = y->branch[y->branch[L] == &rom->end];
        x->parent = y->parent;
        if (p_of_xy == &rom->end)
        {
            rom->root = x;
        }
        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->branch[p_of_xy->branch[R] == y] = x;
    }
    else
    {
        y = min_max_from(rom, remove->branch[R], L);
        p_of_xy = y->parent;
        x = y->branch[y->branch[L] == &rom->end];
        x->parent = y->parent;

        /* Save if check and improve readability by assuming this is true. */
        assert(p_of_xy != &rom->end);

        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->branch[p_of_xy->branch[R] == y] = x;
        transplant(rom, remove, y);
        if (remove == p_of_xy)
        {
            p_of_xy = y;
        }
    }

    if (p_of_xy != &rom->end)
    {
        if (two_child)
        {
            assert(p_of_xy != &rom->end);
            rebalance_3_child(rom, p_of_xy, x);
        }
        else if (x == &rom->end && p_of_xy->branch[L] == p_of_xy->branch[R])
        {
            assert(p_of_xy != &rom->end);
            CCC_Tribool const demote_makes_3_child
                = is_2_child(rom, p_of_xy->parent, p_of_xy);
            demote(rom, p_of_xy);
            if (demote_makes_3_child)
            {
                rebalance_3_child(rom, p_of_xy->parent, p_of_xy);
            }
        }
        assert(!is_leaf(rom, p_of_xy) || !p_of_xy->parity);
    }
    remove->branch[L] = remove->branch[R] = remove->parent = NULL;
    remove->parity = 0;
    --rom->count;
    return struct_base(rom, remove);
}

static void
rebalance_3_child(struct CCC_Realtime_ordered_map *const rom,
                  struct CCC_Realtime_ordered_map_node *z,
                  struct CCC_Realtime_ordered_map_node *x)
{
    assert(z != &rom->end);
    CCC_Tribool made_3_child = CCC_FALSE;
    do
    {
        struct CCC_Realtime_ordered_map_node *const g = z->parent;
        struct CCC_Realtime_ordered_map_node *const y
            = z->branch[z->branch[L] == x];
        made_3_child = is_2_child(rom, g, z);
        if (is_2_child(rom, z, y))
        {
            demote(rom, z);
        }
        else if (is_22_parent(rom, y->branch[L], y, y->branch[R]))
        {
            demote(rom, z);
            demote(rom, y);
        }
        else /* parent of x is 1,3, y is not a 2,2 parent, and x is 3-child. */
        {
            assert(is_3_child(rom, z, x));
            enum Link const z_to_x_dir = z->branch[R] == x;
            struct CCC_Realtime_ordered_map_node *const w
                = y->branch[!z_to_x_dir];
            if (is_1_child(rom, y, w))
            {
                rotate(rom, z, y, y->branch[z_to_x_dir], z_to_x_dir);
                promote(rom, y);
                demote(rom, z);
                if (is_leaf(rom, z))
                {
                    demote(rom, z);
                }
            }
            else /* w is a 2-child and v will be a 1-child. */
            {
                struct CCC_Realtime_ordered_map_node *const v
                    = y->branch[z_to_x_dir];
                assert(is_2_child(rom, y, w));
                assert(is_1_child(rom, y, v));
                double_rotate(rom, z, y, v, !z_to_x_dir);
                double_promote(rom, v);
                demote(rom, y);
                double_demote(rom, z);
                /* Optional "Rebalancing with Promotion," defined as follows:
                       if node z is a non-leaf 1,1 node, we promote it;
                       otherwise, if y is a non-leaf 1,1 node, we promote it.
                       (See Figure 4.) (Haeupler et. al. 2014, 17).
                   This reduces constants in some of theorems mentioned in the
                   paper but may not be worth doing. Rotations stay at 2 worst
                   case. Should revisit after more performance testing. */
                if (!is_leaf(rom, z)
                    && is_11_parent(rom, z->branch[L], z, z->branch[R]))
                {
                    promote(rom, z);
                }
                else if (!is_leaf(rom, y)
                         && is_11_parent(rom, y->branch[L], y, y->branch[R]))
                {
                    promote(rom, y);
                }
            }
            return;
        }
        x = z;
        z = g;
    }
    while (z != &rom->end && made_3_child);
}

static void
transplant(struct CCC_Realtime_ordered_map *const rom,
           struct CCC_Realtime_ordered_map_node *const remove,
           struct CCC_Realtime_ordered_map_node *const replacement)
{
    assert(remove != &rom->end);
    assert(replacement != &rom->end);
    replacement->parent = remove->parent;
    if (remove->parent == &rom->end)
    {
        rom->root = replacement;
    }
    else
    {
        remove->parent->branch[remove->parent->branch[R] == remove]
            = replacement;
    }
    remove->branch[R]->parent = replacement;
    remove->branch[L]->parent = replacement;
    replacement->branch[R] = remove->branch[R];
    replacement->branch[L] = remove->branch[L];
    replacement->parity = remove->parity;
}

/** A single rotation is symmetric. Here is the right case. Lowercase are nodes
and uppercase are arbitrary subtrees.
        z            x
     ╭──┴──╮      ╭──┴──╮
     x     C      A     z
   ╭─┴─╮      ->      ╭─┴─╮
   A   y              y   C
       │              │
       B              B*/
static void
rotate(struct CCC_Realtime_ordered_map *const rom,
       struct CCC_Realtime_ordered_map_node *const z,
       struct CCC_Realtime_ordered_map_node *const x,
       struct CCC_Realtime_ordered_map_node *const y, enum Link dir)
{
    assert(z != &rom->end);
    struct CCC_Realtime_ordered_map_node *const g = z->parent;
    x->parent = g;
    if (g == &rom->end)
    {
        rom->root = x;
    }
    else
    {
        g->branch[g->branch[R] == z] = x;
    }
    x->branch[dir] = z;
    z->parent = x;
    z->branch[!dir] = y;
    y->parent = z;
}

/** A double rotation shouldn't actually be two calls to rotate because that
would invoke pointless memory writes. Here is an example of double right.
Lowercase are nodes and uppercase are arbitrary subtrees.

        z            y
     ╭──┴──╮      ╭──┴──╮
     x     D      x     z
   ╭─┴─╮     -> ╭─┴─╮ ╭─┴─╮
   A   y        A   B C   D
     ╭─┴─╮
     B   C */
static void
double_rotate(struct CCC_Realtime_ordered_map *const rom,
              struct CCC_Realtime_ordered_map_node *const z,
              struct CCC_Realtime_ordered_map_node *const x,
              struct CCC_Realtime_ordered_map_node *const y, enum Link dir)
{
    assert(z != &rom->end);
    assert(x != &rom->end);
    assert(y != &rom->end);
    struct CCC_Realtime_ordered_map_node *const g = z->parent;
    y->parent = g;
    if (g == &rom->end)
    {
        rom->root = y;
    }
    else
    {
        g->branch[g->branch[R] == z] = y;
    }
    x->branch[!dir] = y->branch[dir];
    y->branch[dir]->parent = x;
    y->branch[dir] = x;
    x->parent = y;

    z->branch[dir] = y->branch[!dir];
    y->branch[!dir]->parent = z;
    y->branch[!dir] = z;
    z->parent = y;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
      1╭─╯
       x */
[[maybe_unused]] static inline CCC_Tribool
is_0_child(struct CCC_Realtime_ordered_map const *const rom,
           struct CCC_Realtime_ordered_map_node const *const p,
           struct CCC_Realtime_ordered_map_node const *const x)
{
    return p != &rom->end && p->parity == x->parity;
}

/* Returns true for rank difference 1 between the parent and node.
         p
       1/
       x*/
static inline CCC_Tribool
is_1_child(struct CCC_Realtime_ordered_map const *const rom,
           struct CCC_Realtime_ordered_map_node const *const p,
           struct CCC_Realtime_ordered_map_node const *const x)
{
    return p != &rom->end && p->parity != x->parity;
}

/* Returns true for rank difference 2 between the parent and node.
         p
      2╭─╯
       x */
static inline CCC_Tribool
is_2_child(struct CCC_Realtime_ordered_map const *const rom,
           struct CCC_Realtime_ordered_map_node const *const p,
           struct CCC_Realtime_ordered_map_node const *const x)
{
    return p != &rom->end && p->parity == x->parity;
}

/* Returns true for rank difference 3 between the parent and node.
         p
      3╭─╯
       x */
[[maybe_unused]] static inline CCC_Tribool
is_3_child(struct CCC_Realtime_ordered_map const *const rom,
           struct CCC_Realtime_ordered_map_node const *const p,
           struct CCC_Realtime_ordered_map_node const *const x)
{
    return p != &rom->end && p->parity != x->parity;
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮1
       x   y */
static inline CCC_Tribool
is_01_parent([[maybe_unused]] struct CCC_Realtime_ordered_map const *const rom,
             struct CCC_Realtime_ordered_map_node const *const x,
             struct CCC_Realtime_ordered_map_node const *const p,
             struct CCC_Realtime_ordered_map_node const *const y)
{
    assert(p != &rom->end);
    return (!x->parity && !p->parity && y->parity)
        || (x->parity && p->parity && !y->parity);
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
      1╭─┴─╮1
       x   y */
static inline CCC_Tribool
is_11_parent([[maybe_unused]] struct CCC_Realtime_ordered_map const *const rom,
             struct CCC_Realtime_ordered_map_node const *const x,
             struct CCC_Realtime_ordered_map_node const *const p,
             struct CCC_Realtime_ordered_map_node const *const y)
{
    assert(p != &rom->end);
    return (!x->parity && p->parity && !y->parity)
        || (x->parity && !p->parity && y->parity);
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮2
       x   y */
static inline CCC_Tribool
is_02_parent([[maybe_unused]] struct CCC_Realtime_ordered_map const *const rom,
             struct CCC_Realtime_ordered_map_node const *const x,
             struct CCC_Realtime_ordered_map_node const *const p,
             struct CCC_Realtime_ordered_map_node const *const y)
{
    assert(p != &rom->end);
    return (x->parity == p->parity) && (p->parity == y->parity);
}

/* Returns true if a parent is a 2,2 or 2,2 node, which is allowed. 2,2 nodes
   are allowed in a WAVL tree but the absence of any 2,2 nodes is the exact
   equivalent of a normal AVL tree which can occur if only insertions occur
   for a WAVL tree. Either child may be the sentinel node which has a parity of
   1 and rank -1.
         p
      2╭─┴─╮2
       x   y */
static inline CCC_Tribool
is_22_parent([[maybe_unused]] struct CCC_Realtime_ordered_map const *const rom,
             struct CCC_Realtime_ordered_map_node const *const x,
             struct CCC_Realtime_ordered_map_node const *const p,
             struct CCC_Realtime_ordered_map_node const *const y)
{
    assert(p != &rom->end);
    return (x->parity == p->parity) && (p->parity == y->parity);
}

static inline void
promote(struct CCC_Realtime_ordered_map const *const rom,
        struct CCC_Realtime_ordered_map_node *const x)
{
    if (x != &rom->end)
    {
        x->parity = !x->parity;
    }
}

static inline void
demote(struct CCC_Realtime_ordered_map const *const rom,
       struct CCC_Realtime_ordered_map_node *const x)
{
    promote(rom, x);
}

/* One could imagine non-parity based rank tracking making this function
   meaningful, but two parity changes are the same as a no-op. Leave for
   clarity of what the code is meant to do through certain sections. */
static inline void
double_promote(struct CCC_Realtime_ordered_map const *const,
               struct CCC_Realtime_ordered_map_node *const)
{}

/* One could imagine non-parity based rank tracking making this function
   meaningful, but two parity changes are the same as a no-op. Leave for
   clarity of what the code is meant to do through certain sections. */
static inline void
double_demote(struct CCC_Realtime_ordered_map const *const,
              struct CCC_Realtime_ordered_map_node *const)
{}

static inline CCC_Tribool
is_leaf(struct CCC_Realtime_ordered_map const *const rom,
        struct CCC_Realtime_ordered_map_node const *const x)
{
    return x->branch[L] == &rom->end && x->branch[R] == &rom->end;
}

static inline struct CCC_Realtime_ordered_map_node *
sibling_of([[maybe_unused]] struct CCC_Realtime_ordered_map const *const rom,
           struct CCC_Realtime_ordered_map_node const *const x)
{
    assert(x->parent != &rom->end);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return x->parent->branch[x->parent->branch[L] == x];
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

/** @private */
struct Tree_range
{
    struct CCC_Realtime_ordered_map_node const *low;
    struct CCC_Realtime_ordered_map_node const *root;
    struct CCC_Realtime_ordered_map_node const *high;
};

static size_t
recursive_count(struct CCC_Realtime_ordered_map const *const rom,
                struct CCC_Realtime_ordered_map_node const *const r)
{
    if (r == &rom->end)
    {
        return 0;
    }
    return 1 + recursive_count(rom, r->branch[R])
         + recursive_count(rom, r->branch[L]);
}

static CCC_Tribool
are_subtrees_valid(struct CCC_Realtime_ordered_map const *t,
                   struct Tree_range const r,
                   struct CCC_Realtime_ordered_map_node const *const nil)
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
is_storing_parent(struct CCC_Realtime_ordered_map const *const t,
                  struct CCC_Realtime_ordered_map_node const *const parent,
                  struct CCC_Realtime_ordered_map_node const *const root)
{
    if (root == &t->end)
    {
        return CCC_TRUE;
    }
    if (root->parent != parent)
    {
        return CCC_FALSE;
    }
    return is_storing_parent(t, root, root->branch[L])
        && is_storing_parent(t, root, root->branch[R]);
}

static CCC_Tribool
validate(struct CCC_Realtime_ordered_map const *const rom)
{
    if (!rom->end.parity)
    {
        return CCC_FALSE;
    }
    if (!are_subtrees_valid(rom,
                            (struct Tree_range){
                                .low = &rom->end,
                                .root = rom->root,
                                .high = &rom->end,
                            },
                            &rom->end))
    {
        return CCC_FALSE;
    }
    if (recursive_count(rom, rom->root) != rom->count)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(rom, &rom->end, rom->root))
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */

/* Below you will find the required license for code that inspired the
implementation of a WAVL tree in this repository for some map containers.

The original repository can be found here:

https://github.com/pvachon/wavl_tree

The original implementation has be changed to eliminate left and right cases
and work within the C Container Collection memory framework.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
