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

This map, however, pmapises O(lg N) search, insert, and remove as a true
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
and flexible in how it can be implemented.

Excuse the mathematical variable naming in the WAVL implementation. It is
easiest to check work against the research paper if we use the exact same names
that appear in the paper. We could choose to describe the nodes in terms of
their tree lineage but that changes with rotations so a symbolic representation
is fine. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "bounded_map.h"
#include "private/private_bounded_map.h"
#include "private/private_types.h"
#include "types.h"

/** @internal */
enum Link
{
    L = 0,
    R,
};

#define INORDER R
#define R_INORDER L

/** @internal This will utilize safe type punning in C. Both union fields have
the same type and when obtaining an entry we either have the desired element
or its parent. Preserving the known parent is what makes the Entry Interface
No further look ups are required for insertions, modification, or removal. */
struct Query
{
    CCC_Order last_order;
    union
    {
        struct CCC_Bounded_map_node *found;
        struct CCC_Bounded_map_node *parent;
    };
};

/*==============================  Prototypes   ==============================*/

static void init_node(struct CCC_Bounded_map *, struct CCC_Bounded_map_node *);
static CCC_Order order(struct CCC_Bounded_map const *, void const *key,
                       struct CCC_Bounded_map_node const *node,
                       CCC_Key_comparator *);
static void *struct_base(struct CCC_Bounded_map const *,
                         struct CCC_Bounded_map_node const *);
static struct Query find(struct CCC_Bounded_map const *, void const *key);
static void swap(void *temp, void *a, void *b, size_t sizeof_type);
static void *maybe_allocate_insert(struct CCC_Bounded_map *,
                                   struct CCC_Bounded_map_node *parent,
                                   CCC_Order last_order,
                                   struct CCC_Bounded_map_node *);
static void *remove_fixup(struct CCC_Bounded_map *,
                          struct CCC_Bounded_map_node *);
static void insert_fixup(struct CCC_Bounded_map *,
                         struct CCC_Bounded_map_node *z,
                         struct CCC_Bounded_map_node *x);
static void transplant(struct CCC_Bounded_map *,
                       struct CCC_Bounded_map_node *remove,
                       struct CCC_Bounded_map_node *replacement);
static CCC_Tribool parity(struct CCC_Bounded_map_node const *);
static void rebalance_3_child(struct CCC_Bounded_map *,
                              struct CCC_Bounded_map_node *z,
                              struct CCC_Bounded_map_node *x);
static CCC_Tribool is_0_child(struct CCC_Bounded_map_node const *p,
                              struct CCC_Bounded_map_node const *x);
static CCC_Tribool is_1_child(struct CCC_Bounded_map_node const *p,
                              struct CCC_Bounded_map_node const *x);
static CCC_Tribool is_2_child(struct CCC_Bounded_map_node const *p,
                              struct CCC_Bounded_map_node const *x);
static CCC_Tribool is_3_child(struct CCC_Bounded_map_node const *p,
                              struct CCC_Bounded_map_node const *x);
static CCC_Tribool is_01_parent(struct CCC_Bounded_map_node const *x,
                                struct CCC_Bounded_map_node const *p,
                                struct CCC_Bounded_map_node const *y);
static CCC_Tribool is_11_parent(struct CCC_Bounded_map_node const *x,
                                struct CCC_Bounded_map_node const *p,
                                struct CCC_Bounded_map_node const *y);
static CCC_Tribool is_02_parent(struct CCC_Bounded_map_node const *x,
                                struct CCC_Bounded_map_node const *p,
                                struct CCC_Bounded_map_node const *y);
static CCC_Tribool is_22_parent(struct CCC_Bounded_map_node const *x,
                                struct CCC_Bounded_map_node const *p,
                                struct CCC_Bounded_map_node const *y);
static CCC_Tribool is_leaf(struct CCC_Bounded_map_node const *x);
static struct CCC_Bounded_map_node *
sibling_of(struct CCC_Bounded_map_node const *x);
static void promote(struct CCC_Bounded_map_node *x);
static void demote(struct CCC_Bounded_map_node *x);
static void double_promote(struct CCC_Bounded_map_node *x);
static void double_demote(struct CCC_Bounded_map_node *x);

static void rotate(struct CCC_Bounded_map *map, struct CCC_Bounded_map_node *z,
                   struct CCC_Bounded_map_node *x,
                   struct CCC_Bounded_map_node *y, enum Link dir);
static void double_rotate(struct CCC_Bounded_map *map,
                          struct CCC_Bounded_map_node *z,
                          struct CCC_Bounded_map_node *x,
                          struct CCC_Bounded_map_node *y, enum Link dir);
static CCC_Tribool validate(struct CCC_Bounded_map const *map);
static struct CCC_Bounded_map_node *next(struct CCC_Bounded_map const *,
                                         struct CCC_Bounded_map_node const *,
                                         enum Link);
static struct CCC_Bounded_map_node *
min_max_from(struct CCC_Bounded_map_node *start, enum Link);
static struct CCC_Range equal_range(struct CCC_Bounded_map const *,
                                    void const *, void const *, enum Link);
static struct CCC_Bounded_map_entry entry(struct CCC_Bounded_map const *map,
                                          void const *key);
static void *insert(struct CCC_Bounded_map *map,
                    struct CCC_Bounded_map_node *parent, CCC_Order last_order,
                    struct CCC_Bounded_map_node *type_output_intruder);
static void *key_from_node(struct CCC_Bounded_map const *map,
                           struct CCC_Bounded_map_node const *node);
static void *key_in_slot(struct CCC_Bounded_map const *map, void const *slot);
static struct CCC_Bounded_map_node *
elem_in_slot(struct CCC_Bounded_map const *map, void const *slot);

/*==============================  Interface    ==============================*/

CCC_Tribool
CCC_bounded_map_contains(CCC_Bounded_map const *const map,
                         void const *const key)
{
    if (!map || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return CCC_ORDER_EQUAL == find(map, key).last_order;
}

void *
CCC_bounded_map_get_key_value(CCC_Bounded_map const *const map,
                              void const *const key)
{
    if (!map || !key)
    {
        return NULL;
    }
    struct Query const q = find(map, key);
    return (CCC_ORDER_EQUAL == q.last_order) ? struct_base(map, q.found) : NULL;
}

CCC_Entry
CCC_bounded_map_swap_entry(CCC_Bounded_map *const map,
                           CCC_Bounded_map_node *const type_intruder,
                           CCC_Bounded_map_node *const temp_intruder)
{
    if (!map || !type_intruder || !temp_intruder)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    struct Query const q = find(map, key_from_node(map, type_intruder));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        *type_intruder = *q.found;
        void *const found = struct_base(map, q.found);
        void *const any_struct = struct_base(map, type_intruder);
        void *const old_val = struct_base(map, temp_intruder);
        swap(old_val, found, any_struct, map->sizeof_type);
        type_intruder->branch[L] = type_intruder->branch[R]
            = type_intruder->parent = NULL;
        temp_intruder->branch[L] = temp_intruder->branch[R]
            = temp_intruder->parent = NULL;
        return (CCC_Entry){{
            .type = old_val,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    if (!maybe_allocate_insert(map, q.parent, q.last_order, type_intruder))
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
CCC_bounded_map_try_insert(CCC_Bounded_map *const map,
                           CCC_Bounded_map_node *const type_intruder)
{
    if (!map || !type_intruder)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    struct Query const q = find(map, key_from_node(map, type_intruder));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        return (CCC_Entry){{
            .type = struct_base(map, q.found),
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted
        = maybe_allocate_insert(map, q.parent, q.last_order, type_intruder);
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
CCC_bounded_map_insert_or_assign(CCC_Bounded_map *const map,
                                 CCC_Bounded_map_node *const type_intruder)
{
    if (!map || !type_intruder)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    struct Query const q = find(map, key_from_node(map, type_intruder));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        void *const found = struct_base(map, q.found);
        *type_intruder = *elem_in_slot(map, found);
        memcpy(found, struct_base(map, type_intruder), map->sizeof_type);
        return (CCC_Entry){{
            .type = found,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted
        = maybe_allocate_insert(map, q.parent, q.last_order, type_intruder);
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

CCC_Bounded_map_entry
CCC_bounded_map_entry(CCC_Bounded_map const *const map, void const *const key)
{
    if (!map || !key)
    {
        return (CCC_Bounded_map_entry){
            {.entry = {.status = CCC_ENTRY_ARGUMENT_ERROR}}};
    }
    return (CCC_Bounded_map_entry){entry(map, key)};
}

void *
CCC_bounded_map_or_insert(CCC_Bounded_map_entry const *const entry,
                          CCC_Bounded_map_node *const type_intruder)
{
    if (!entry || !type_intruder)
    {
        return NULL;
    }
    if (entry->private.entry.status == CCC_ENTRY_OCCUPIED)
    {
        return entry->private.entry.type;
    }
    return maybe_allocate_insert(
        entry->private.map,
        elem_in_slot(entry->private.map, entry->private.entry.type),
        entry->private.last_order, type_intruder);
}

void *
CCC_bounded_map_insert_entry(CCC_Bounded_map_entry const *const entry,
                             CCC_Bounded_map_node *const type_intruder)
{
    if (!entry || !type_intruder)
    {
        return NULL;
    }
    if (entry->private.entry.status == CCC_ENTRY_OCCUPIED)
    {
        *type_intruder
            = *elem_in_slot(entry->private.map, entry->private.entry.type);
        memcpy(entry->private.entry.type,
               struct_base(entry->private.map, type_intruder),
               entry->private.map->sizeof_type);
        return entry->private.entry.type;
    }
    return maybe_allocate_insert(
        entry->private.map,
        elem_in_slot(entry->private.map, entry->private.entry.type),
        entry->private.last_order, type_intruder);
}

CCC_Entry
CCC_bounded_map_remove_entry(CCC_Bounded_map_entry const *const entry)
{
    if (!entry)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    if (entry->private.entry.status == CCC_ENTRY_OCCUPIED)
    {
        void *const erased = remove_fixup(
            entry->private.map,
            elem_in_slot(entry->private.map, entry->private.entry.type));
        assert(erased);
        if (entry->private.map->allocate)
        {
            entry->private.map->allocate((CCC_Allocator_context){
                .input = erased,
                .bytes = 0,
                .context = entry->private.map->context,
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

CCC_Entry
CCC_bounded_map_remove(CCC_Bounded_map *const map,
                       CCC_Bounded_map_node *const type_output_intruder)
{
    if (!map || !type_output_intruder)
    {
        return (CCC_Entry){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    struct Query const q = find(map, key_from_node(map, type_output_intruder));
    if (q.last_order != CCC_ORDER_EQUAL)
    {
        return (CCC_Entry){{
            .type = NULL,
            .status = CCC_ENTRY_VACANT,
        }};
    }
    void *const removed = remove_fixup(map, q.found);
    if (map->allocate)
    {
        void *const any_struct = struct_base(map, type_output_intruder);
        memcpy(any_struct, removed, map->sizeof_type);
        map->allocate((CCC_Allocator_context){
            .input = removed,
            .bytes = 0,
            .context = map->context,
        });
        return (CCC_Entry){{
            .type = any_struct,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Entry){{
        .type = removed,
        .status = CCC_ENTRY_OCCUPIED,
    }};
}

CCC_Bounded_map_entry *
CCC_bounded_map_and_modify(CCC_Bounded_map_entry *e, CCC_Type_modifier *fn)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->private.entry.status & CCC_ENTRY_OCCUPIED
        && e->private.entry.type)
    {
        fn((CCC_Type_context){
            .type = e->private.entry.type,
            NULL,
        });
    }
    return e;
}

CCC_Bounded_map_entry *
CCC_bounded_map_and_modify_context(CCC_Bounded_map_entry *e,
                                   CCC_Type_modifier *fn, void *context)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->private.entry.status & CCC_ENTRY_OCCUPIED
        && e->private.entry.type)
    {
        fn((CCC_Type_context){
            .type = e->private.entry.type,
            context,
        });
    }
    return e;
}

void *
CCC_bounded_map_unwrap(CCC_Bounded_map_entry const *const e)
{
    if (e && e->private.entry.status & CCC_ENTRY_OCCUPIED)
    {
        return e->private.entry.type;
    }
    return NULL;
}

CCC_Tribool
CCC_bounded_map_occupied(CCC_Bounded_map_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->private.entry.status & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Tribool
CCC_bounded_map_insert_error(CCC_Bounded_map_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->private.entry.status & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Entry_status
CCC_bounded_map_entry_status(CCC_Bounded_map_entry const *const e)
{
    return e ? e->private.entry.status : CCC_ENTRY_ARGUMENT_ERROR;
}

void *
CCC_bounded_map_begin(CCC_Bounded_map const *map)
{
    if (!map)
    {
        return NULL;
    }
    struct CCC_Bounded_map_node *const m = min_max_from(map->root, L);
    return m == NULL ? NULL : struct_base(map, m);
}

void *
CCC_bounded_map_next(CCC_Bounded_map const *const map,
                     CCC_Bounded_map_node const *const iterator_intruder)
{
    if (!map || !iterator_intruder)
    {
        return NULL;
    }
    struct CCC_Bounded_map_node const *const n
        = next(map, iterator_intruder, INORDER);
    if (n == NULL)
    {
        return NULL;
    }
    return struct_base(map, n);
}

void *
CCC_bounded_map_reverse_begin(CCC_Bounded_map const *const map)
{
    if (!map)
    {
        return NULL;
    }
    struct CCC_Bounded_map_node *const m = min_max_from(map->root, R);
    return m == NULL ? NULL : struct_base(map, m);
}

void *
CCC_bounded_map_end(CCC_Bounded_map const *const)
{
    return NULL;
}

void *
CCC_bounded_map_reverse_end(CCC_Bounded_map const *const)
{
    return NULL;
}

void *
CCC_bounded_map_reverse_next(
    CCC_Bounded_map const *const map,
    CCC_Bounded_map_node const *const iterator_intruder)
{
    if (!map || !iterator_intruder)
    {
        return NULL;
    }
    struct CCC_Bounded_map_node const *const n
        = next(map, iterator_intruder, R_INORDER);
    return (n == NULL) ? NULL : struct_base(map, n);
}

CCC_Range
CCC_bounded_map_equal_range(CCC_Bounded_map const *const map,
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
CCC_bounded_map_equal_range_reverse(CCC_Bounded_map const *const map,
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

CCC_Count
CCC_bounded_map_count(CCC_Bounded_map const *const map)
{
    if (!map)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = map->count};
}

CCC_Tribool
CCC_bounded_map_is_empty(CCC_Bounded_map const *const map)
{
    if (!map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !map->count;
}

CCC_Tribool
CCC_bounded_map_validate(CCC_Bounded_map const *map)
{
    if (!map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(map);
}

/** This is a linear time constant space deletion of tree nodes via left
rotations so element fields are modified during progression of deletes. */
CCC_Result
CCC_bounded_map_clear(CCC_Bounded_map *const map,
                      CCC_Type_destructor *const destructor)
{
    if (!map)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Bounded_map_node *node = map->root;
    while (node != NULL)
    {
        if (node->branch[L] != NULL)
        {
            struct CCC_Bounded_map_node *const left = node->branch[L];
            node->branch[L] = left->branch[R];
            left->branch[R] = node;
            node = left;
            continue;
        }
        struct CCC_Bounded_map_node *const next = node->branch[R];
        node->branch[L] = node->branch[R] = NULL;
        node->parent = NULL;
        void *const destroy = struct_base(map, node);
        if (destructor)
        {
            destructor((CCC_Type_context){
                .type = destroy,
                .context = map->context,
            });
        }
        if (map->allocate)
        {
            (void)map->allocate((CCC_Allocator_context){
                .input = destroy,
                .bytes = 0,
                .context = map->context,
            });
        }
        node = next;
    }
    return CCC_RESULT_OK;
}

/*=========================   Private Interface  ============================*/

struct CCC_Bounded_map_entry
CCC_private_bounded_map_entry(struct CCC_Bounded_map const *const map,
                              void const *const key)
{
    return entry(map, key);
}

void *
CCC_private_bounded_map_insert(
    struct CCC_Bounded_map *const map,
    struct CCC_Bounded_map_node *const parent, CCC_Order const last_order,
    struct CCC_Bounded_map_node *const type_output_intruder)
{
    return insert(map, parent, last_order, type_output_intruder);
}

void *
CCC_private_bounded_map_key_in_slot(struct CCC_Bounded_map const *const map,
                                    void const *const slot)
{
    return key_in_slot(map, slot);
}

struct CCC_Bounded_map_node *
CCC_private_Bounded_map_node_in_slot(struct CCC_Bounded_map const *const map,
                                     void const *const slot)
{
    return elem_in_slot(map, slot);
}

/*=========================    Static Helpers    ============================*/

static struct CCC_Bounded_map_node *
min_max_from(struct CCC_Bounded_map_node *start, enum Link const dir)
{
    if (start == NULL)
    {
        return start;
    }
    for (; start->branch[dir] != NULL; start = start->branch[dir])
    {}
    return start;
}

static struct CCC_Bounded_map_entry
entry(struct CCC_Bounded_map const *const map, void const *const key)
{
    struct Query const q = find(map, key);
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        return (struct CCC_Bounded_map_entry){
            .map = (struct CCC_Bounded_map *)map,
            .last_order = q.last_order,
            .entry = {
                .type = struct_base(map, q.found),
                .status = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct CCC_Bounded_map_entry){
        .map = (struct CCC_Bounded_map *)map,
        .last_order = q.last_order,
        .entry = {
            .type = struct_base(map, q.parent),
            .status = CCC_ENTRY_VACANT | CCC_ENTRY_NO_UNWRAP,
        },
    };
}

static void *
maybe_allocate_insert(struct CCC_Bounded_map *const map,
                      struct CCC_Bounded_map_node *const parent,
                      CCC_Order const last_order,
                      struct CCC_Bounded_map_node *type_output_intruder)
{
    if (map->allocate)
    {
        void *const new = map->allocate((CCC_Allocator_context){
            .input = NULL,
            .bytes = map->sizeof_type,
            .context = map->context,
        });
        if (!new)
        {
            return NULL;
        }
        memcpy(new, struct_base(map, type_output_intruder), map->sizeof_type);
        type_output_intruder = elem_in_slot(map, new);
    }
    return insert(map, parent, last_order, type_output_intruder);
}

static void *
insert(struct CCC_Bounded_map *const map,
       struct CCC_Bounded_map_node *const parent, CCC_Order const last_order,
       struct CCC_Bounded_map_node *const type_output_intruder)
{
    init_node(map, type_output_intruder);
    if (!map->count)
    {
        map->root = type_output_intruder;
        ++map->count;
        return struct_base(map, type_output_intruder);
    }
    assert(last_order == CCC_ORDER_GREATER || last_order == CCC_ORDER_LESSER);
    CCC_Tribool rank_rule_break = false;
    if (parent)
    {
        rank_rule_break
            = parent->branch[L] == NULL && parent->branch[R] == NULL;
        parent->branch[CCC_ORDER_GREATER == last_order] = type_output_intruder;
    }
    type_output_intruder->parent = parent;
    if (rank_rule_break)
    {
        insert_fixup(map, parent, type_output_intruder);
    }
    ++map->count;
    return struct_base(map, type_output_intruder);
}

static struct Query
find(struct CCC_Bounded_map const *const map, void const *const key)
{
    struct CCC_Bounded_map_node const *parent = NULL;
    struct Query q = {
        .last_order = CCC_ORDER_ERROR,
        .found = map->root,
    };
    while (q.found != NULL)
    {
        q.last_order = order(map, key, q.found, map->compare);
        if (CCC_ORDER_EQUAL == q.last_order)
        {
            return q;
        }
        parent = q.found;
        q.found = q.found->branch[CCC_ORDER_GREATER == q.last_order];
    }
    /* Type punning here OK as both union members have same type and size. */
    q.parent = (struct CCC_Bounded_map_node *)parent;
    return q;
}

static struct CCC_Bounded_map_node *
next(struct CCC_Bounded_map const *const map,
     struct CCC_Bounded_map_node const *n, enum Link const traversal)
{
    if (!n)
    {
        return NULL;
    }
    assert(map->root->parent == NULL);
    if (n->branch[traversal])
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->branch[traversal]; n->branch[!traversal];
             n = n->branch[!traversal])
        {}
        return (struct CCC_Bounded_map_node *)n;
    }
    for (; n->parent && n->parent->branch[!traversal] != n; n = n->parent)
    {}
    return n->parent;
}

static struct CCC_Range
equal_range(struct CCC_Bounded_map const *const map,
            void const *const begin_key, void const *const end_key,
            enum Link const traversal)
{
    if (!map->count)
    {
        return (struct CCC_Range){};
    }
    CCC_Order const les_or_grt[2] = {CCC_ORDER_LESSER, CCC_ORDER_GREATER};
    struct Query b = find(map, begin_key);
    if (b.last_order == les_or_grt[traversal])
    {
        b.found = next(map, b.found, traversal);
    }
    struct Query e = find(map, end_key);
    if (e.last_order != les_or_grt[!traversal])
    {
        e.found = next(map, e.found, traversal);
    }
    return (struct CCC_Range){
        .begin = b.found == NULL ? NULL : struct_base(map, b.found),
        .end = e.found == NULL ? NULL : struct_base(map, e.found),
    };
}

static inline void
init_node(struct CCC_Bounded_map *const map,
          struct CCC_Bounded_map_node *const e)
{
    assert(e != NULL);
    assert(map != NULL);
    e->branch[L] = e->branch[R] = e->parent = NULL;
    e->parity = 0;
}

static inline void
swap(void *const temp, void *const a, void *const b, size_t const sizeof_type)
{
    if (a == b || !a || !b)
    {
        return;
    }
    (void)memcpy(temp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, temp, sizeof_type);
}

static inline CCC_Order
order(struct CCC_Bounded_map const *const map, void const *const key,
      struct CCC_Bounded_map_node const *const node,
      CCC_Key_comparator *const fn)
{
    return fn((CCC_Key_comparator_context){
        .key_left = key,
        .type_right = struct_base(map, node),
        .context = map->context,
    });
}

static inline void *
struct_base(struct CCC_Bounded_map const *const map,
            struct CCC_Bounded_map_node const *const e)
{
    return e ? ((char *)e->branch) - map->type_intruder_offset : NULL;
}

static inline void *
key_from_node(struct CCC_Bounded_map const *const map,
              struct CCC_Bounded_map_node const *const node)
{
    return node ? (char *)struct_base(map, node) + map->key_offset : NULL;
}

static inline void *
key_in_slot(struct CCC_Bounded_map const *const map, void const *const slot)
{
    return slot ? (char *)slot + map->key_offset : NULL;
}

static inline struct CCC_Bounded_map_node *
elem_in_slot(struct CCC_Bounded_map const *const map, void const *const slot)
{
    return slot ? (struct CCC_Bounded_map_node *)((char *)slot
                                                  + map->type_intruder_offset)
                : NULL;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct CCC_Bounded_map *const map, struct CCC_Bounded_map_node *z,
             struct CCC_Bounded_map_node *x)
{
    assert(z);
    do
    {
        promote(z);
        x = z;
        z = z->parent;
        if (z == NULL)
        {
            return;
        }
    }
    while (is_01_parent(x, z, sibling_of(x)));

    if (!is_02_parent(x, z, sibling_of(x)))
    {
        return;
    }
    assert(x != NULL);
    assert(is_0_child(z, x));
    enum Link const p_to_x_dir = z->branch[R] == x;
    struct CCC_Bounded_map_node *const y = x->branch[!p_to_x_dir];
    if (y == NULL || is_2_child(z, y))
    {
        rotate(map, z, x, y, !p_to_x_dir);
        demote(z);
    }
    else
    {
        assert(is_1_child(z, y));
        double_rotate(map, z, x, y, p_to_x_dir);
        promote(y);
        demote(x);
        demote(z);
    }
}

static void *
remove_fixup(struct CCC_Bounded_map *const map,
             struct CCC_Bounded_map_node *const remove)
{
    struct CCC_Bounded_map_node *y = NULL;
    struct CCC_Bounded_map_node *x = NULL;
    struct CCC_Bounded_map_node *p_of_xy = NULL;
    CCC_Tribool two_child = CCC_FALSE;
    if (remove->branch[L] == NULL || remove->branch[R] == NULL)
    {
        y = remove;
        p_of_xy = y->parent;
        x = y->branch[y->branch[L] == NULL];
        if (x)
        {
            x->parent = y->parent;
        }
        if (p_of_xy == NULL)
        {
            map->root = x;
        }
        else
        {
            p_of_xy->branch[p_of_xy->branch[R] == y] = x;
        }
        two_child = is_2_child(p_of_xy, y);
    }
    else
    {
        y = min_max_from(remove->branch[R], L);
        p_of_xy = y->parent;
        x = y->branch[y->branch[L] == NULL];
        if (x)
        {
            x->parent = y->parent;
        }

        /* Save if check and improve readability by assuming this is true. */
        assert(p_of_xy != NULL);

        two_child = is_2_child(p_of_xy, y);
        p_of_xy->branch[p_of_xy->branch[R] == y] = x;
        transplant(map, remove, y);
        if (remove == p_of_xy)
        {
            p_of_xy = y;
        }
    }

    if (p_of_xy != NULL)
    {
        if (two_child)
        {
            assert(p_of_xy != NULL);
            rebalance_3_child(map, p_of_xy, x);
        }
        else if (x == NULL && p_of_xy->branch[L] == p_of_xy->branch[R])
        {
            assert(p_of_xy != NULL);
            CCC_Tribool const demote_makes_3_child
                = is_2_child(p_of_xy->parent, p_of_xy);
            demote(p_of_xy);
            if (demote_makes_3_child)
            {
                rebalance_3_child(map, p_of_xy->parent, p_of_xy);
            }
        }
        assert(!is_leaf(p_of_xy) || !parity(p_of_xy));
    }
    remove->branch[L] = remove->branch[R] = remove->parent = NULL;
    remove->parity = 0;
    --map->count;
    return struct_base(map, remove);
}

/** Follows the specification in the "Rank-Balanced Trees" paper by Haeupler,
Sen, and Tarjan (Fig. 3. pg 8). */
static void
rebalance_3_child(struct CCC_Bounded_map *const map,
                  struct CCC_Bounded_map_node *z,
                  struct CCC_Bounded_map_node *x)
{
    CCC_Tribool made_3_child = CCC_TRUE;
    while (z && made_3_child)
    {
        assert(z->branch[L] == x || z->branch[R] == x);
        struct CCC_Bounded_map_node *const g = z->parent;
        struct CCC_Bounded_map_node *const y = z->branch[z->branch[L] == x];
        made_3_child = g != NULL && is_2_child(g, z);
        if (is_2_child(z, y))
        {
            demote(z);
        }
        else if (y && is_22_parent(y->branch[L], y, y->branch[R]))
        {
            demote(z);
            demote(y);
        }
        else if (y)
        {
            assert(is_1_child(z, y));
            assert(is_3_child(z, x));
            assert(!is_2_child(z, y));
            assert(!is_22_parent(y->branch[L], y, y->branch[R]));
            enum Link const z_to_x_dir = z->branch[R] == x;
            struct CCC_Bounded_map_node *const w = y->branch[!z_to_x_dir];
            if (is_1_child(y, w))
            {
                rotate(map, z, y, y->branch[z_to_x_dir], z_to_x_dir);
                promote(y);
                demote(z);
                if (is_leaf(z))
                {
                    demote(z);
                }
            }
            else /* w is a 2-child and v will be a 1-child. */
            {
                struct CCC_Bounded_map_node *const v = y->branch[z_to_x_dir];
                assert(is_2_child(y, w));
                assert(is_1_child(y, v));
                double_rotate(map, z, y, v, !z_to_x_dir);
                double_promote(v);
                demote(y);
                double_demote(z);
                /* Optional "Rebalancing with Promotion," defined as follows:
                       if node z is a non-leaf 1,1 node, we promote it;
                       otherwise, if y is a non-leaf 1,1 node, we promote it.
                       (See Figure 4.) (Haeupler et. al. 2014, 17).
                   This reduces constants in some of theorems mentioned in the
                   paper but may not be worth doing. Rotations stay at 2 worst
                   case. Should revisit after more performance testing. */
                if (!is_leaf(z) && is_11_parent(z->branch[L], z, z->branch[R]))
                {
                    promote(z);
                }
                else if (!is_leaf(y)
                         && is_11_parent(y->branch[L], y, y->branch[R]))
                {
                    promote(y);
                }
            }
            /* Returning here confirms O(1) rotations for re-balance. */
            return;
        }
        x = z;
        z = g;
    }
}

static void
transplant(struct CCC_Bounded_map *const map,
           struct CCC_Bounded_map_node *const remove,
           struct CCC_Bounded_map_node *const replacement)
{
    assert(remove != NULL);
    assert(replacement != NULL);
    replacement->parent = remove->parent;
    if (remove->parent == NULL)
    {
        map->root = replacement;
    }
    else
    {
        remove->parent->branch[remove->parent->branch[R] == remove]
            = replacement;
    }
    if (remove->branch[R])
    {
        remove->branch[R]->parent = replacement;
    }
    if (remove->branch[L])
    {
        remove->branch[L]->parent = replacement;
    }
    replacement->branch[R] = remove->branch[R];
    replacement->branch[L] = remove->branch[L];
    replacement->parity = parity(remove);
}

/** A single rotation is symmetric. Here is the right case. Lowercase are nodes
and uppercase are arbitrary subtrees.
        z            x
     ╭──┴──╮      ╭──┴──╮
     x     C      A     z
   ╭─┴─╮      ->      ╭─┴─╮
   A   y              y   C
       │              │
       B              B

Taking a link as input allows us to code both symmetrical cases at once. */
static void
rotate(struct CCC_Bounded_map *const map, struct CCC_Bounded_map_node *const z,
       struct CCC_Bounded_map_node *const x,
       struct CCC_Bounded_map_node *const y, enum Link dir)
{
    assert(z != NULL);
    struct CCC_Bounded_map_node *const g = z->parent;
    x->parent = g;
    if (g == NULL)
    {
        map->root = x;
    }
    else
    {
        g->branch[g->branch[R] == z] = x;
    }
    x->branch[dir] = z;
    z->parent = x;
    z->branch[!dir] = y;
    if (y)
    {
        y->parent = z;
    }
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
     B   C

Taking a link as input allows us to code both symmetrical cases at once. */
static void
double_rotate(struct CCC_Bounded_map *const map,
              struct CCC_Bounded_map_node *const z,
              struct CCC_Bounded_map_node *const x,
              struct CCC_Bounded_map_node *const y, enum Link dir)
{
    assert(z != NULL);
    assert(x != NULL);
    assert(y != NULL);
    struct CCC_Bounded_map_node *const g = z->parent;
    y->parent = g;
    if (g == NULL)
    {
        map->root = y;
    }
    else
    {
        g->branch[g->branch[R] == z] = y;
    }
    x->branch[!dir] = y->branch[dir];
    if (y->branch[dir])
    {
        y->branch[dir]->parent = x;
    }
    y->branch[dir] = x;
    x->parent = y;

    z->branch[dir] = y->branch[!dir];
    if (y->branch[!dir])
    {
        y->branch[!dir]->parent = z;
    }
    y->branch[!dir] = z;
    z->parent = y;
}

/* Returns the parity of a node. A NULL node has a parity of 1 aka CCC_TRUE. */
static inline CCC_Tribool
parity(struct CCC_Bounded_map_node const *const x)
{
    return x ? x->parity : CCC_TRUE;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
      1╭─╯
       x */
[[maybe_unused]] static inline CCC_Tribool
is_0_child(struct CCC_Bounded_map_node const *const p,
           struct CCC_Bounded_map_node const *const x)
{
    return parity(p) == parity(x);
}

/* Returns true for rank difference 1 between the parent and node.
         p
       1/
       x*/
static inline CCC_Tribool
is_1_child(struct CCC_Bounded_map_node const *const p,
           struct CCC_Bounded_map_node const *const x)
{
    return parity(p) != parity(x);
}

/* Returns true for rank difference 2 between the parent and node.
         p
      2╭─╯
       x */
static inline CCC_Tribool
is_2_child(struct CCC_Bounded_map_node const *const p,
           struct CCC_Bounded_map_node const *const x)
{
    return parity(p) == parity(x);
}

/* Returns true for rank difference 3 between the parent and node.
         p
      3╭─╯
       x */
[[maybe_unused]] static inline CCC_Tribool
is_3_child(struct CCC_Bounded_map_node const *const p,
           struct CCC_Bounded_map_node const *const x)
{
    return parity(p) != parity(x);
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮1
       x   y */
static inline CCC_Tribool
is_01_parent(struct CCC_Bounded_map_node const *const x,
             struct CCC_Bounded_map_node const *const p,
             struct CCC_Bounded_map_node const *const y)
{
    return (!parity(x) && !parity(p) && parity(y))
        || (parity(x) && parity(p) && !parity(y));
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
      1╭─┴─╮1
       x   y */
static inline CCC_Tribool
is_11_parent(struct CCC_Bounded_map_node const *const x,
             struct CCC_Bounded_map_node const *const p,
             struct CCC_Bounded_map_node const *const y)
{
    return (!parity(x) && parity(p) && !parity(y))
        || (parity(x) && !parity(p) && parity(y));
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮2
       x   y */
static inline CCC_Tribool
is_02_parent(struct CCC_Bounded_map_node const *const x,
             struct CCC_Bounded_map_node const *const p,
             struct CCC_Bounded_map_node const *const y)
{
    return (parity(x) == parity(p)) && (parity(p) == parity(y));
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
is_22_parent(struct CCC_Bounded_map_node const *const x,
             struct CCC_Bounded_map_node const *const p,
             struct CCC_Bounded_map_node const *const y)
{
    return (parity(x) == parity(p)) && (parity(p) == parity(y));
}

static inline void
promote(struct CCC_Bounded_map_node *const x)
{
    if (x)
    {
        x->parity = !x->parity;
    }
}

static inline void
demote(struct CCC_Bounded_map_node *const x)
{
    promote(x);
}

/* One could imagine non-parity based rank tracking making this function
   meaningful, but two parity changes are the same as a no-op. Leave for
   clarity of what the code is meant to do through certain sections. */
static inline void
double_promote(struct CCC_Bounded_map_node *const)
{}

/* One could imagine non-parity based rank tracking making this function
   meaningful, but two parity changes are the same as a no-op. Leave for
   clarity of what the code is meant to do through certain sections. */
static inline void
double_demote(struct CCC_Bounded_map_node *const)
{}

static inline CCC_Tribool
is_leaf(struct CCC_Bounded_map_node const *const x)
{
    return x->branch[L] == NULL && x->branch[R] == NULL;
}

static inline struct CCC_Bounded_map_node *
sibling_of(struct CCC_Bounded_map_node const *const x)
{
    if (x->parent == NULL)
    {
        return NULL;
    }
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return x->parent->branch[x->parent->branch[L] == x];
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

/** @internal */
struct Tree_range
{
    struct CCC_Bounded_map_node const *low;
    struct CCC_Bounded_map_node const *root;
    struct CCC_Bounded_map_node const *high;
};

static size_t
recursive_count(struct CCC_Bounded_map const *const map,
                struct CCC_Bounded_map_node const *const r)
{
    if (r == NULL)
    {
        return 0;
    }
    return 1 + recursive_count(map, r->branch[R])
         + recursive_count(map, r->branch[L]);
}

static CCC_Tribool
are_subtrees_valid(struct CCC_Bounded_map const *t, struct Tree_range const r)
{
    if (!r.root)
    {
        return CCC_TRUE;
    }
    if (r.low
        && order(t, key_from_node(t, r.low), r.root, t->compare)
               != CCC_ORDER_LESSER)
    {
        return CCC_FALSE;
    }
    if (r.high
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
                              })
        && are_subtrees_valid(t, (struct Tree_range){
                                     .low = r.root,
                                     .root = r.root->branch[R],
                                     .high = r.high,
                                 });
}

static CCC_Tribool
is_storing_parent(struct CCC_Bounded_map const *const t,
                  struct CCC_Bounded_map_node const *const parent,
                  struct CCC_Bounded_map_node const *const root)
{
    if (root == NULL)
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
validate(struct CCC_Bounded_map const *const map)
{
    if (!are_subtrees_valid(map, (struct Tree_range){
                                     .low = NULL,
                                     .root = map->root,
                                     .high = NULL,
                                 }))
    {
        return CCC_FALSE;
    }
    if (recursive_count(map, map->root) != map->count)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(map, NULL, map->root))
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
