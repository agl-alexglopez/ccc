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

#include "impl/impl_realtime_ordered_map.h"
#include "impl/impl_types.h"
#include "realtime_ordered_map.h"
#include "types.h"

/** @private */
enum romap_link
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
struct romap_query
{
    ccc_threeway_cmp last_cmp;
    union
    {
        struct ccc_romap_elem *found;
        struct ccc_romap_elem *parent;
    };
};

/*==============================  Prototypes   ==============================*/

static void init_node(struct ccc_romap *, struct ccc_romap_elem *);
static ccc_threeway_cmp cmp(struct ccc_romap const *, void const *key,
                            struct ccc_romap_elem const *node,
                            ccc_any_key_cmp_fn *);
static void *struct_base(struct ccc_romap const *,
                         struct ccc_romap_elem const *);
static struct romap_query find(struct ccc_romap const *, void const *key);
static void swap(char tmp[], void *a, void *b, size_t sizeof_type);
static void *maybe_alloc_insert(struct ccc_romap *,
                                struct ccc_romap_elem *parent,
                                ccc_threeway_cmp last_cmp,
                                struct ccc_romap_elem *);
static void *remove_fixup(struct ccc_romap *, struct ccc_romap_elem *);
static void insert_fixup(struct ccc_romap *, struct ccc_romap_elem *z,
                         struct ccc_romap_elem *x);
static void transplant(struct ccc_romap *, struct ccc_romap_elem *remove,
                       struct ccc_romap_elem *replacement);
static void rebalance_3_child(struct ccc_romap *rom, struct ccc_romap_elem *z,
                              struct ccc_romap_elem *x);
static ccc_tribool is_0_child(struct ccc_romap const *,
                              struct ccc_romap_elem const *p,
                              struct ccc_romap_elem const *x);
static ccc_tribool is_1_child(struct ccc_romap const *,
                              struct ccc_romap_elem const *p,
                              struct ccc_romap_elem const *x);
static ccc_tribool is_2_child(struct ccc_romap const *,
                              struct ccc_romap_elem const *p,
                              struct ccc_romap_elem const *x);
static ccc_tribool is_3_child(struct ccc_romap const *,
                              struct ccc_romap_elem const *p,
                              struct ccc_romap_elem const *x);
static ccc_tribool is_01_parent(struct ccc_romap const *,
                                struct ccc_romap_elem const *x,
                                struct ccc_romap_elem const *p,
                                struct ccc_romap_elem const *y);
static ccc_tribool is_11_parent(struct ccc_romap const *,
                                struct ccc_romap_elem const *x,
                                struct ccc_romap_elem const *p,
                                struct ccc_romap_elem const *y);
static ccc_tribool is_02_parent(struct ccc_romap const *,
                                struct ccc_romap_elem const *x,
                                struct ccc_romap_elem const *p,
                                struct ccc_romap_elem const *y);
static ccc_tribool is_22_parent(struct ccc_romap const *,
                                struct ccc_romap_elem const *x,
                                struct ccc_romap_elem const *p,
                                struct ccc_romap_elem const *y);
static ccc_tribool is_leaf(struct ccc_romap const *rom,
                           struct ccc_romap_elem const *x);
static struct ccc_romap_elem *sibling_of(struct ccc_romap const *rom,
                                         struct ccc_romap_elem const *x);
static void promote(struct ccc_romap const *rom, struct ccc_romap_elem *x);
static void demote(struct ccc_romap const *rom, struct ccc_romap_elem *x);
static void double_promote(struct ccc_romap const *rom,
                           struct ccc_romap_elem *x);
static void double_demote(struct ccc_romap const *rom,
                          struct ccc_romap_elem *x);

static void rotate(struct ccc_romap *rom, struct ccc_romap_elem *z,
                   struct ccc_romap_elem *x, struct ccc_romap_elem *y,
                   enum romap_link dir);
static void double_rotate(struct ccc_romap *rom, struct ccc_romap_elem *z,
                          struct ccc_romap_elem *x, struct ccc_romap_elem *y,
                          enum romap_link dir);
static ccc_tribool validate(struct ccc_romap const *rom);
static struct ccc_romap_elem *
next(struct ccc_romap const *, struct ccc_romap_elem const *, enum romap_link);
static struct ccc_romap_elem *min_max_from(struct ccc_romap const *,
                                           struct ccc_romap_elem *start,
                                           enum romap_link);
static struct ccc_range_u equal_range(struct ccc_romap const *, void const *,
                                      void const *, enum romap_link);
static struct ccc_rtree_entry entry(struct ccc_romap const *rom,
                                    void const *key);
static void *insert(struct ccc_romap *rom, struct ccc_romap_elem *parent,
                    ccc_threeway_cmp last_cmp,
                    struct ccc_romap_elem *out_handle);
static void *key_from_node(struct ccc_romap const *rom,
                           struct ccc_romap_elem const *elem);
static void *key_in_slot(struct ccc_romap const *rom, void const *slot);
static struct ccc_romap_elem *elem_in_slot(struct ccc_romap const *rom,
                                           void const *slot);

/*==============================  Interface    ==============================*/

ccc_tribool
ccc_rom_contains(ccc_realtime_ordered_map const *const rom,
                 void const *const key)
{
    if (!rom || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return CCC_EQL == find(rom, key).last_cmp;
}

void *
ccc_rom_get_key_val(ccc_realtime_ordered_map const *const rom,
                    void const *const key)
{
    if (!rom || !key)
    {
        return NULL;
    }
    struct romap_query const q = find(rom, key);
    return (CCC_EQL == q.last_cmp) ? struct_base(rom, q.found) : NULL;
}

ccc_entry
ccc_rom_swap_entry(ccc_realtime_ordered_map *const rom,
                   ccc_romap_elem *const key_val_handle,
                   ccc_romap_elem *const tmp)
{
    if (!rom || !key_val_handle || !tmp)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct romap_query const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp)
    {
        *key_val_handle = *q.found;
        void *const found = struct_base(rom, q.found);
        void *const any_struct = struct_base(rom, key_val_handle);
        void *const old_val = struct_base(rom, tmp);
        swap(old_val, found, any_struct, rom->sizeof_type);
        key_val_handle->branch[L] = key_val_handle->branch[R]
            = key_val_handle->parent = NULL;
        tmp->branch[L] = tmp->branch[R] = tmp->parent = NULL;
        return (ccc_entry){{
            .e = old_val,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    if (!maybe_alloc_insert(rom, q.parent, q.last_cmp, key_val_handle))
    {
        return (ccc_entry){{
            .e = NULL,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_entry){{
        .e = NULL,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_entry
ccc_rom_try_insert(ccc_realtime_ordered_map *const rom,
                   ccc_romap_elem *const key_val_handle)
{
    if (!rom || !key_val_handle)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct romap_query const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp)
    {
        return (ccc_entry){{
            .e = struct_base(rom, q.found),
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted
        = maybe_alloc_insert(rom, q.parent, q.last_cmp, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{
            .e = NULL,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_entry){{
        .e = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_entry
ccc_rom_insert_or_assign(ccc_realtime_ordered_map *const rom,
                         ccc_romap_elem *const key_val_handle)
{
    if (!rom || !key_val_handle)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct romap_query const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp)
    {
        void *const found = struct_base(rom, q.found);
        *key_val_handle = *elem_in_slot(rom, found);
        memcpy(found, struct_base(rom, key_val_handle), rom->sizeof_type);
        return (ccc_entry){{
            .e = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted
        = maybe_alloc_insert(rom, q.parent, q.last_cmp, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{
            .e = NULL,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_entry){{
        .e = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_romap_entry
ccc_rom_entry(ccc_realtime_ordered_map const *const rom, void const *const key)
{
    if (!rom || !key)
    {
        return (ccc_romap_entry){{.entry = {.stats = CCC_ENTRY_ARG_ERROR}}};
    }
    return (ccc_romap_entry){entry(rom, key)};
}

void *
ccc_rom_or_insert(ccc_romap_entry const *const e, ccc_romap_elem *const elem)
{
    if (!e || !elem || !e->impl.entry.e)
    {
        return NULL;
    }
    if (e->impl.entry.stats == CCC_ENTRY_OCCUPIED)
    {
        return e->impl.entry.e;
    }
    return maybe_alloc_insert(e->impl.rom,
                              elem_in_slot(e->impl.rom, e->impl.entry.e),
                              e->impl.last_cmp, elem);
}

void *
ccc_rom_insert_entry(ccc_romap_entry const *const e, ccc_romap_elem *const elem)
{
    if (!e || !elem || !e->impl.entry.e)
    {
        return NULL;
    }
    if (e->impl.entry.stats == CCC_ENTRY_OCCUPIED)
    {
        *elem = *elem_in_slot(e->impl.rom, e->impl.entry.e);
        memcpy(e->impl.entry.e, struct_base(e->impl.rom, elem),
               e->impl.rom->sizeof_type);
        return e->impl.entry.e;
    }
    return maybe_alloc_insert(e->impl.rom,
                              elem_in_slot(e->impl.rom, e->impl.entry.e),
                              e->impl.last_cmp, elem);
}

ccc_entry
ccc_rom_remove_entry(ccc_romap_entry const *const e)
{
    if (!e || !e->impl.entry.e)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (e->impl.entry.stats == CCC_ENTRY_OCCUPIED)
    {
        void *const erased = remove_fixup(
            e->impl.rom, elem_in_slot(e->impl.rom, e->impl.entry.e));
        assert(erased);
        if (e->impl.rom->alloc)
        {
            e->impl.rom->alloc(erased, 0, e->impl.rom->aux);
            return (ccc_entry){{
                .e = NULL,
                .stats = CCC_ENTRY_OCCUPIED,
            }};
        }
        return (ccc_entry){{
            .e = erased,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (ccc_entry){{
        .e = NULL,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_entry
ccc_rom_remove(ccc_realtime_ordered_map *const rom,
               ccc_romap_elem *const out_handle)
{
    if (!rom || !out_handle)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct romap_query const q = find(rom, key_from_node(rom, out_handle));
    if (q.last_cmp != CCC_EQL)
    {
        return (ccc_entry){{
            .e = NULL,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    void *const removed = remove_fixup(rom, q.found);
    if (rom->alloc)
    {
        void *const any_struct = struct_base(rom, out_handle);
        memcpy(any_struct, removed, rom->sizeof_type);
        rom->alloc(removed, 0, rom->aux);
        return (ccc_entry){{
            .e = any_struct,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (ccc_entry){{
        .e = removed,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

ccc_romap_entry *
ccc_rom_and_modify(ccc_romap_entry *e, ccc_any_type_update_fn *fn)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->impl.entry.stats & CCC_ENTRY_OCCUPIED && e->impl.entry.e)
    {
        fn((ccc_any_type){
            .any_type = e->impl.entry.e,
            NULL,
        });
    }
    return e;
}

ccc_romap_entry *
ccc_rom_and_modify_aux(ccc_romap_entry *e, ccc_any_type_update_fn *fn,
                       void *aux)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->impl.entry.stats & CCC_ENTRY_OCCUPIED && e->impl.entry.e)
    {
        fn((ccc_any_type){
            .any_type = e->impl.entry.e,
            aux,
        });
    }
    return e;
}

void *
ccc_rom_unwrap(ccc_romap_entry const *const e)
{
    if (e && e->impl.entry.stats & CCC_ENTRY_OCCUPIED)
    {
        return e->impl.entry.e;
    }
    return NULL;
}

ccc_tribool
ccc_rom_occupied(ccc_romap_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.entry.stats & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_tribool
ccc_rom_insert_error(ccc_romap_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.entry.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_entry_status
ccc_rom_entry_status(ccc_romap_entry const *const e)
{
    return e ? e->impl.entry.stats : CCC_ENTRY_ARG_ERROR;
}

void *
ccc_rom_begin(ccc_realtime_ordered_map const *rom)
{
    if (!rom)
    {
        return NULL;
    }
    struct ccc_romap_elem *const m = min_max_from(rom, rom->root, L);
    return m == &rom->end ? NULL : struct_base(rom, m);
}

void *
ccc_rom_next(ccc_realtime_ordered_map const *const rom,
             ccc_romap_elem const *const e)
{
    if (!rom || !e)
    {
        return NULL;
    }
    struct ccc_romap_elem const *const n = next(rom, e, INORDER);
    if (n == &rom->end)
    {
        return NULL;
    }
    return struct_base(rom, n);
}

void *
ccc_rom_rbegin(ccc_realtime_ordered_map const *const rom)
{
    if (!rom)
    {
        return NULL;
    }
    struct ccc_romap_elem *const m = min_max_from(rom, rom->root, R);
    return m == &rom->end ? NULL : struct_base(rom, m);
}

void *
ccc_rom_end(ccc_realtime_ordered_map const *const)
{
    return NULL;
}

void *
ccc_rom_rend(ccc_realtime_ordered_map const *const)
{
    return NULL;
}

void *
ccc_rom_rnext(ccc_realtime_ordered_map const *const rom,
              ccc_romap_elem const *const e)
{
    if (!rom || !e)
    {
        return NULL;
    }
    struct ccc_romap_elem const *const n = next(rom, e, R_INORDER);
    return (n == &rom->end) ? NULL : struct_base(rom, n);
}

ccc_range
ccc_rom_equal_range(ccc_realtime_ordered_map const *const rom,
                    void const *const begin_key, void const *const end_key)
{
    if (!rom || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(rom, begin_key, end_key, INORDER)};
}

ccc_rrange
ccc_rom_equal_rrange(ccc_realtime_ordered_map const *const rom,
                     void const *const rbegin_key, void const *const rend_key)
{
    if (!rom || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){equal_range(rom, rbegin_key, rend_key, R_INORDER)};
}

ccc_ucount
ccc_rom_count(ccc_realtime_ordered_map const *const rom)
{
    if (!rom)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = rom->count};
}

ccc_tribool
ccc_rom_is_empty(ccc_realtime_ordered_map const *const rom)
{
    if (!rom)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !rom->count;
}

ccc_tribool
ccc_rom_validate(ccc_realtime_ordered_map const *rom)
{
    if (!rom)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(rom);
}

/** This is a linear time constant space deletion of tree nodes via left
rotations so element fields are modified during progression of deletes. */
ccc_result
ccc_rom_clear(ccc_realtime_ordered_map *const rom,
              ccc_any_type_destructor_fn *const destructor)
{
    if (!rom)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_romap_elem *node = rom->root;
    while (node != &rom->end)
    {
        if (node->branch[L] != &rom->end)
        {
            struct ccc_romap_elem *const left = node->branch[L];
            node->branch[L] = left->branch[R];
            left->branch[R] = node;
            node = left;
            continue;
        }
        struct ccc_romap_elem *const next = node->branch[R];
        node->branch[L] = node->branch[R] = NULL;
        node->parent = NULL;
        void *const destroy = struct_base(rom, node);
        if (destructor)
        {
            destructor((ccc_any_type){
                .any_type = destroy,
                .aux = rom->aux,
            });
        }
        if (rom->alloc)
        {
            (void)rom->alloc(destroy, 0, rom->aux);
        }
        node = next;
    }
    return CCC_RESULT_OK;
}

/*=========================   Private Interface  ============================*/

struct ccc_rtree_entry
ccc_impl_rom_entry(struct ccc_romap const *const rom, void const *const key)
{
    return entry(rom, key);
}

void *
ccc_impl_rom_insert(struct ccc_romap *const rom,
                    struct ccc_romap_elem *const parent,
                    ccc_threeway_cmp const last_cmp,
                    struct ccc_romap_elem *const out_handle)
{
    return insert(rom, parent, last_cmp, out_handle);
}

void *
ccc_impl_rom_key_in_slot(struct ccc_romap const *const rom,
                         void const *const slot)
{
    return key_in_slot(rom, slot);
}

struct ccc_romap_elem *
ccc_impl_romap_elem_in_slot(struct ccc_romap const *const rom,
                            void const *const slot)
{
    return elem_in_slot(rom, slot);
}

/*=========================    Static Helpers    ============================*/

static struct ccc_romap_elem *
min_max_from(struct ccc_romap const *const rom, struct ccc_romap_elem *start,
             enum romap_link const dir)
{
    if (start == &rom->end)
    {
        return start;
    }
    for (; start->branch[dir] != &rom->end; start = start->branch[dir])
    {}
    return start;
}

static struct ccc_rtree_entry
entry(struct ccc_romap const *const rom, void const *const key)
{
    struct romap_query const q = find(rom, key);
    if (CCC_EQL == q.last_cmp)
    {
        return (struct ccc_rtree_entry){
            .rom = (struct ccc_romap *)rom,
            .last_cmp = q.last_cmp,
            .entry = {
                .e = struct_base(rom, q.found),
                .stats = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct ccc_rtree_entry){
        .rom = (struct ccc_romap *)rom,
        .last_cmp = q.last_cmp,
        .entry = {
            .e = struct_base(rom, q.parent),
            .stats = CCC_ENTRY_VACANT | CCC_ENTRY_NO_UNWRAP,
        },
    };
}

static void *
maybe_alloc_insert(struct ccc_romap *const rom,
                   struct ccc_romap_elem *const parent,
                   ccc_threeway_cmp const last_cmp,
                   struct ccc_romap_elem *out_handle)
{
    if (rom->alloc)
    {
        void *const new = rom->alloc(NULL, rom->sizeof_type, rom->aux);
        if (!new)
        {
            return NULL;
        }
        memcpy(new, struct_base(rom, out_handle), rom->sizeof_type);
        out_handle = elem_in_slot(rom, new);
    }
    return insert(rom, parent, last_cmp, out_handle);
}

static void *
insert(struct ccc_romap *const rom, struct ccc_romap_elem *const parent,
       ccc_threeway_cmp const last_cmp, struct ccc_romap_elem *const out_handle)
{
    init_node(rom, out_handle);
    if (!rom->count)
    {
        rom->root = out_handle;
        ++rom->count;
        return struct_base(rom, out_handle);
    }
    assert(last_cmp == CCC_GRT || last_cmp == CCC_LES);
    ccc_tribool const rank_rule_break
        = parent->branch[L] == &rom->end && parent->branch[R] == &rom->end;
    parent->branch[CCC_GRT == last_cmp] = out_handle;
    out_handle->parent = parent;
    if (rank_rule_break)
    {
        insert_fixup(rom, parent, out_handle);
    }
    ++rom->count;
    return struct_base(rom, out_handle);
}

static struct romap_query
find(struct ccc_romap const *const rom, void const *const key)
{
    struct ccc_romap_elem const *parent = &rom->end;
    struct romap_query q = {
        .last_cmp = CCC_CMP_ERROR,
        .found = rom->root,
    };
    while (q.found != &rom->end)
    {
        q.last_cmp = cmp(rom, key, q.found, rom->cmp);
        if (CCC_EQL == q.last_cmp)
        {
            return q;
        }
        parent = q.found;
        q.found = q.found->branch[CCC_GRT == q.last_cmp];
    }
    /* Type punning here OK as both union members have same type and size. */
    q.parent = (struct ccc_romap_elem *)parent;
    return q;
}

static struct ccc_romap_elem *
next(struct ccc_romap const *const rom, struct ccc_romap_elem const *n,
     enum romap_link const traversal)
{
    if (!n || n == &rom->end)
    {
        return (struct ccc_romap_elem *)&rom->end;
    }
    assert(rom->root->parent == &rom->end);
    /* The node is an internal one that has a sub-tree to explore first. */
    if (n->branch[traversal] != &rom->end)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->branch[traversal]; n->branch[!traversal] != &rom->end;
             n = n->branch[!traversal])
        {}
        return (struct ccc_romap_elem *)n;
    }
    for (; n->parent != &rom->end && n->parent->branch[!traversal] != n;
         n = n->parent)
    {}
    return n->parent;
}

static struct ccc_range_u
equal_range(struct ccc_romap const *const rom, void const *const begin_key,
            void const *const end_key, enum romap_link const traversal)
{
    if (!rom->count)
    {
        return (struct ccc_range_u){};
    }
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    struct romap_query b = find(rom, begin_key);
    if (b.last_cmp == les_or_grt[traversal])
    {
        b.found = next(rom, b.found, traversal);
    }
    struct romap_query e = find(rom, end_key);
    if (e.last_cmp != les_or_grt[!traversal])
    {
        e.found = next(rom, e.found, traversal);
    }
    return (struct ccc_range_u){
        .begin = b.found == &rom->end ? NULL : struct_base(rom, b.found),
        .end = e.found == &rom->end ? NULL : struct_base(rom, e.found),
    };
}

static inline void
init_node(struct ccc_romap *const rom, struct ccc_romap_elem *const e)
{
    assert(e != NULL);
    assert(rom != NULL);
    e->branch[L] = e->branch[R] = e->parent = &rom->end;
    e->parity = 0;
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const sizeof_type)
{
    if (a == b || !a || !b)
    {
        return;
    }
    (void)memcpy(tmp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, tmp, sizeof_type);
}

static inline ccc_threeway_cmp
cmp(struct ccc_romap const *const rom, void const *const key,
    struct ccc_romap_elem const *const node, ccc_any_key_cmp_fn *const fn)
{
    return fn((ccc_any_key_cmp){
        .any_key_lhs = key,
        .any_type_rhs = struct_base(rom, node),
        .aux = rom->aux,
    });
}

static inline void *
struct_base(struct ccc_romap const *const rom,
            struct ccc_romap_elem const *const e)
{
    return ((char *)e->branch) - rom->node_elem_offset;
}

static inline void *
key_from_node(struct ccc_romap const *const rom,
              struct ccc_romap_elem const *const elem)
{
    return (char *)struct_base(rom, elem) + rom->key_offset;
}

static inline void *
key_in_slot(struct ccc_romap const *const rom, void const *const slot)
{
    return (char *)slot + rom->key_offset;
}

static inline struct ccc_romap_elem *
elem_in_slot(struct ccc_romap const *const rom, void const *const slot)
{
    return (struct ccc_romap_elem *)((char *)slot + rom->node_elem_offset);
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct ccc_romap *const rom, struct ccc_romap_elem *z,
             struct ccc_romap_elem *x)
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
    enum romap_link const p_to_x_dir = z->branch[R] == x;
    struct ccc_romap_elem *const y = x->branch[!p_to_x_dir];
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
remove_fixup(struct ccc_romap *const rom, struct ccc_romap_elem *const remove)
{
    assert(remove->branch[R] && remove->branch[L]);
    struct ccc_romap_elem *y = NULL;
    struct ccc_romap_elem *x = NULL;
    struct ccc_romap_elem *p_of_xy = NULL;
    ccc_tribool two_child = CCC_FALSE;
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
            ccc_tribool const demote_makes_3_child
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
rebalance_3_child(struct ccc_romap *const rom, struct ccc_romap_elem *z,
                  struct ccc_romap_elem *x)
{
    assert(z != &rom->end);
    ccc_tribool made_3_child = CCC_FALSE;
    do
    {
        struct ccc_romap_elem *const g = z->parent;
        struct ccc_romap_elem *const y = z->branch[z->branch[L] == x];
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
        else /* p(x) is 1,3, y is not a 2,2 parent, and x is 3-child.*/
        {
            assert(is_3_child(rom, z, x));
            enum romap_link const z_to_x_dir = z->branch[R] == x;
            struct ccc_romap_elem *const w = y->branch[!z_to_x_dir];
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
                struct ccc_romap_elem *const v = y->branch[z_to_x_dir];
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
transplant(struct ccc_romap *const rom, struct ccc_romap_elem *const remove,
           struct ccc_romap_elem *const replacement)
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
rotate(struct ccc_romap *const rom, struct ccc_romap_elem *const z,
       struct ccc_romap_elem *const x, struct ccc_romap_elem *const y,
       enum romap_link dir)
{
    assert(z != &rom->end);
    struct ccc_romap_elem *const g = z->parent;
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
double_rotate(struct ccc_romap *const rom, struct ccc_romap_elem *const z,
              struct ccc_romap_elem *const x, struct ccc_romap_elem *const y,
              enum romap_link dir)
{
    assert(z != &rom->end);
    assert(x != &rom->end);
    assert(y != &rom->end);
    struct ccc_romap_elem *const g = z->parent;
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
[[maybe_unused]] static inline ccc_tribool
is_0_child(struct ccc_romap const *const rom,
           struct ccc_romap_elem const *const p,
           struct ccc_romap_elem const *const x)
{
    return p != &rom->end && p->parity == x->parity;
}

/* Returns true for rank difference 1 between the parent and node.
         p
       1/
       x*/
static inline ccc_tribool
is_1_child(struct ccc_romap const *const rom,
           struct ccc_romap_elem const *const p,
           struct ccc_romap_elem const *const x)
{
    return p != &rom->end && p->parity != x->parity;
}

/* Returns true for rank difference 2 between the parent and node.
         p
      2╭─╯
       x */
static inline ccc_tribool
is_2_child(struct ccc_romap const *const rom,
           struct ccc_romap_elem const *const p,
           struct ccc_romap_elem const *const x)
{
    return p != &rom->end && p->parity == x->parity;
}

/* Returns true for rank difference 3 between the parent and node.
         p
      3╭─╯
       x */
[[maybe_unused]] static inline ccc_tribool
is_3_child(struct ccc_romap const *const rom,
           struct ccc_romap_elem const *const p,
           struct ccc_romap_elem const *const x)
{
    return p != &rom->end && p->parity != x->parity;
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮1
       x   y */
static inline ccc_tribool
is_01_parent([[maybe_unused]] struct ccc_romap const *const rom,
             struct ccc_romap_elem const *const x,
             struct ccc_romap_elem const *const p,
             struct ccc_romap_elem const *const y)
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
static inline ccc_tribool
is_11_parent([[maybe_unused]] struct ccc_romap const *const rom,
             struct ccc_romap_elem const *const x,
             struct ccc_romap_elem const *const p,
             struct ccc_romap_elem const *const y)
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
static inline ccc_tribool
is_02_parent([[maybe_unused]] struct ccc_romap const *const rom,
             struct ccc_romap_elem const *const x,
             struct ccc_romap_elem const *const p,
             struct ccc_romap_elem const *const y)
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
static inline ccc_tribool
is_22_parent([[maybe_unused]] struct ccc_romap const *const rom,
             struct ccc_romap_elem const *const x,
             struct ccc_romap_elem const *const p,
             struct ccc_romap_elem const *const y)
{
    assert(p != &rom->end);
    return (x->parity == p->parity) && (p->parity == y->parity);
}

static inline void
promote(struct ccc_romap const *const rom, struct ccc_romap_elem *const x)
{
    if (x != &rom->end)
    {
        x->parity = !x->parity;
    }
}

static inline void
demote(struct ccc_romap const *const rom, struct ccc_romap_elem *const x)
{
    promote(rom, x);
}

/* One could imagine non-parity based rank tracking making this function
   meaningful, but two parity changes are the same as a no-op. Leave for
   clarity of what the code is meant to do through certain sections. */
static inline void
double_promote(struct ccc_romap const *const, struct ccc_romap_elem *const)
{}

/* One could imagine non-parity based rank tracking making this function
   meaningful, but two parity changes are the same as a no-op. Leave for
   clarity of what the code is meant to do through certain sections. */
static inline void
double_demote(struct ccc_romap const *const, struct ccc_romap_elem *const)
{}

static inline ccc_tribool
is_leaf(struct ccc_romap const *const rom, struct ccc_romap_elem const *const x)
{
    return x->branch[L] == &rom->end && x->branch[R] == &rom->end;
}

static inline struct ccc_romap_elem *
sibling_of([[maybe_unused]] struct ccc_romap const *const rom,
           struct ccc_romap_elem const *const x)
{
    assert(x->parent != &rom->end);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return x->parent->branch[x->parent->branch[L] == x];
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

/** @private */
struct tree_range
{
    struct ccc_romap_elem const *low;
    struct ccc_romap_elem const *root;
    struct ccc_romap_elem const *high;
};

static size_t
recursive_count(struct ccc_romap const *const rom,
                struct ccc_romap_elem const *const r)
{
    if (r == &rom->end)
    {
        return 0;
    }
    return 1 + recursive_count(rom, r->branch[R])
         + recursive_count(rom, r->branch[L]);
}

static ccc_tribool
are_subtrees_valid(struct ccc_romap const *t, struct tree_range const r,
                   struct ccc_romap_elem const *const nil)
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
        && cmp(t, key_from_node(t, r.low), r.root, t->cmp) != CCC_LES)
    {
        return CCC_FALSE;
    }
    if (r.high != nil
        && cmp(t, key_from_node(t, r.high), r.root, t->cmp) != CCC_GRT)
    {
        return CCC_FALSE;
    }
    return are_subtrees_valid(t,
                              (struct tree_range){
                                  .low = r.low,
                                  .root = r.root->branch[L],
                                  .high = r.root,
                              },
                              nil)
        && are_subtrees_valid(t,
                              (struct tree_range){
                                  .low = r.root,
                                  .root = r.root->branch[R],
                                  .high = r.high,
                              },
                              nil);
}

static ccc_tribool
is_storing_parent(struct ccc_romap const *const t,
                  struct ccc_romap_elem const *const parent,
                  struct ccc_romap_elem const *const root)
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

static ccc_tribool
validate(struct ccc_romap const *const rom)
{
    if (!rom->end.parity)
    {
        return CCC_FALSE;
    }
    if (!are_subtrees_valid(rom,
                            (struct tree_range){
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
