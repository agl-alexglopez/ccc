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
enum romap_link_
{
    L = 0,
    R,
};

#define INORDER_TRAVERSAL R
#define REVERSE_INORDER_TRAVERSAL L

/** @private This will utilize safe type punning in C. Both union fields have
the same type and when obtaining an entry we either have the desired element
or its parent. Preserving the known parent is what makes the Entry Interface
No further look ups are required for insertions, modification, or removal. */
struct romap_query_
{
    ccc_threeway_cmp last_cmp_;
    union
    {
        struct ccc_romap_elem_ *found_;
        struct ccc_romap_elem_ *parent_;
    };
};

/*==============================  Prototypes   ==============================*/

static void init_node(struct ccc_romap_ *, struct ccc_romap_elem_ *);
static ccc_threeway_cmp cmp(struct ccc_romap_ const *, void const *key,
                            struct ccc_romap_elem_ const *node,
                            ccc_key_cmp_fn *);
static void *struct_base(struct ccc_romap_ const *,
                         struct ccc_romap_elem_ const *);
static struct romap_query_ find(struct ccc_romap_ const *, void const *key);
static void swap(char tmp[], void *a, void *b, size_t elem_sz);
static void *maybe_alloc_insert(struct ccc_romap_ *,
                                struct ccc_romap_elem_ *parent,
                                ccc_threeway_cmp last_cmp,
                                struct ccc_romap_elem_ *);
static void *remove_fixup(struct ccc_romap_ *, struct ccc_romap_elem_ *);
static void insert_fixup(struct ccc_romap_ *, struct ccc_romap_elem_ *z,
                         struct ccc_romap_elem_ *x);
static void transplant(struct ccc_romap_ *, struct ccc_romap_elem_ *remove,
                       struct ccc_romap_elem_ *replacement);
static void rebalance_3_child(struct ccc_romap_ *rom, struct ccc_romap_elem_ *z,
                              struct ccc_romap_elem_ *x);
static ccc_tribool is_0_child(struct ccc_romap_ const *,
                              struct ccc_romap_elem_ const *p,
                              struct ccc_romap_elem_ const *x);
static ccc_tribool is_1_child(struct ccc_romap_ const *,
                              struct ccc_romap_elem_ const *p,
                              struct ccc_romap_elem_ const *x);
static ccc_tribool is_2_child(struct ccc_romap_ const *,
                              struct ccc_romap_elem_ const *p,
                              struct ccc_romap_elem_ const *x);
static ccc_tribool is_3_child(struct ccc_romap_ const *,
                              struct ccc_romap_elem_ const *p,
                              struct ccc_romap_elem_ const *x);
static ccc_tribool is_01_parent(struct ccc_romap_ const *,
                                struct ccc_romap_elem_ const *x,
                                struct ccc_romap_elem_ const *p,
                                struct ccc_romap_elem_ const *y);
static ccc_tribool is_11_parent(struct ccc_romap_ const *,
                                struct ccc_romap_elem_ const *x,
                                struct ccc_romap_elem_ const *p,
                                struct ccc_romap_elem_ const *y);
static ccc_tribool is_02_parent(struct ccc_romap_ const *,
                                struct ccc_romap_elem_ const *x,
                                struct ccc_romap_elem_ const *p,
                                struct ccc_romap_elem_ const *y);
static ccc_tribool is_22_parent(struct ccc_romap_ const *,
                                struct ccc_romap_elem_ const *x,
                                struct ccc_romap_elem_ const *p,
                                struct ccc_romap_elem_ const *y);
static ccc_tribool is_leaf(struct ccc_romap_ const *rom,
                           struct ccc_romap_elem_ const *x);
static struct ccc_romap_elem_ *sibling_of(struct ccc_romap_ const *rom,
                                          struct ccc_romap_elem_ const *x);
static void promote(struct ccc_romap_ const *rom, struct ccc_romap_elem_ *x);
static void demote(struct ccc_romap_ const *rom, struct ccc_romap_elem_ *x);
static void double_promote(struct ccc_romap_ const *rom,
                           struct ccc_romap_elem_ *x);
static void double_demote(struct ccc_romap_ const *rom,
                          struct ccc_romap_elem_ *x);

static void rotate(struct ccc_romap_ *rom, struct ccc_romap_elem_ *z,
                   struct ccc_romap_elem_ *x, struct ccc_romap_elem_ *y,
                   enum romap_link_ dir);
static void double_rotate(struct ccc_romap_ *rom, struct ccc_romap_elem_ *z,
                          struct ccc_romap_elem_ *x, struct ccc_romap_elem_ *y,
                          enum romap_link_ dir);
static ccc_tribool validate(struct ccc_romap_ const *rom);
static struct ccc_romap_elem_ *next(struct ccc_romap_ const *,
                                    struct ccc_romap_elem_ const *,
                                    enum romap_link_);
static struct ccc_romap_elem_ *min_max_from(struct ccc_romap_ const *,
                                            struct ccc_romap_elem_ *start,
                                            enum romap_link_);
static struct ccc_range_u_ equal_range(struct ccc_romap_ const *, void const *,
                                       void const *, enum romap_link_);
static struct ccc_rtree_entry_ entry(struct ccc_romap_ const *rom,
                                     void const *key);
static void *insert(struct ccc_romap_ *rom, struct ccc_romap_elem_ *parent,
                    ccc_threeway_cmp last_cmp,
                    struct ccc_romap_elem_ *out_handle);
static void *key_from_node(struct ccc_romap_ const *rom,
                           struct ccc_romap_elem_ const *elem);
static void *key_in_slot(struct ccc_romap_ const *rom, void const *slot);
static struct ccc_romap_elem_ *elem_in_slot(struct ccc_romap_ const *rom,
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
    return CCC_EQL == find(rom, key).last_cmp_;
}

void *
ccc_rom_get_key_val(ccc_realtime_ordered_map const *const rom,
                    void const *const key)
{
    if (!rom || !key)
    {
        return NULL;
    }
    struct romap_query_ const q = find(rom, key);
    return (CCC_EQL == q.last_cmp_) ? struct_base(rom, q.found_) : NULL;
}

ccc_entry
ccc_rom_swap_entry(ccc_realtime_ordered_map *const rom,
                   ccc_romap_elem *const key_val_handle,
                   ccc_romap_elem *const tmp)
{
    if (!rom || !key_val_handle || !tmp)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    struct romap_query_ const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        *key_val_handle = *q.found_;
        void *const found = struct_base(rom, q.found_);
        void *const any_struct = struct_base(rom, key_val_handle);
        void *const old_val = struct_base(rom, tmp);
        swap(old_val, found, any_struct, rom->elem_sz_);
        key_val_handle->branch_[L] = key_val_handle->branch_[R]
            = key_val_handle->parent_ = NULL;
        tmp->branch_[L] = tmp->branch_[R] = tmp->parent_ = NULL;
        return (ccc_entry){{.e_ = old_val, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    if (!maybe_alloc_insert(rom, q.parent_, q.last_cmp_, key_val_handle))
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_rom_try_insert(ccc_realtime_ordered_map *const rom,
                   ccc_romap_elem *const key_val_handle)
{
    if (!rom || !key_val_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    struct romap_query_ const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        return (ccc_entry){
            {.e_ = struct_base(rom, q.found_), .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *const inserted
        = maybe_alloc_insert(rom, q.parent_, q.last_cmp_, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_rom_insert_or_assign(ccc_realtime_ordered_map *const rom,
                         ccc_romap_elem *const key_val_handle)
{
    if (!rom || !key_val_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    struct romap_query_ const q = find(rom, key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        void *const found = struct_base(rom, q.found_);
        *key_val_handle = *elem_in_slot(rom, found);
        memcpy(found, struct_base(rom, key_val_handle), rom->elem_sz_);
        return (ccc_entry){{.e_ = found, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *const inserted
        = maybe_alloc_insert(rom, q.parent_, q.last_cmp_, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_romap_entry
ccc_rom_entry(ccc_realtime_ordered_map const *const rom, void const *const key)
{
    if (!rom || !key)
    {
        return (ccc_romap_entry){{.entry_ = {.stats_ = CCC_ENTRY_ARG_ERROR}}};
    }
    return (ccc_romap_entry){entry(rom, key)};
}

void *
ccc_rom_or_insert(ccc_romap_entry const *const e, ccc_romap_elem *const elem)
{
    if (!e || !elem || !e->impl_.entry_.e_)
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    return maybe_alloc_insert(e->impl_.rom_,
                              elem_in_slot(e->impl_.rom_, e->impl_.entry_.e_),
                              e->impl_.last_cmp_, elem);
}

void *
ccc_rom_insert_entry(ccc_romap_entry const *const e, ccc_romap_elem *const elem)
{
    if (!e || !elem || !e->impl_.entry_.e_)
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        *elem = *elem_in_slot(e->impl_.rom_, e->impl_.entry_.e_);
        memcpy(e->impl_.entry_.e_, struct_base(e->impl_.rom_, elem),
               e->impl_.rom_->elem_sz_);
        return e->impl_.entry_.e_;
    }
    return maybe_alloc_insert(e->impl_.rom_,
                              elem_in_slot(e->impl_.rom_, e->impl_.entry_.e_),
                              e->impl_.last_cmp_, elem);
}

ccc_entry
ccc_rom_remove_entry(ccc_romap_entry const *const e)
{
    if (!e || !e->impl_.entry_.e_)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        void *const erased = remove_fixup(
            e->impl_.rom_, elem_in_slot(e->impl_.rom_, e->impl_.entry_.e_));
        assert(erased);
        if (e->impl_.rom_->alloc_)
        {
            e->impl_.rom_->alloc_(erased, 0, e->impl_.rom_->aux_);
            return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_OCCUPIED}};
        }
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_rom_remove(ccc_realtime_ordered_map *const rom,
               ccc_romap_elem *const out_handle)
{
    if (!rom || !out_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_ARG_ERROR}};
    }
    struct romap_query_ const q = find(rom, key_from_node(rom, out_handle));
    if (q.last_cmp_ != CCC_EQL)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    void *const removed = remove_fixup(rom, q.found_);
    if (rom->alloc_)
    {
        void *const any_struct = struct_base(rom, out_handle);
        memcpy(any_struct, removed, rom->elem_sz_);
        rom->alloc_(removed, 0, rom->aux_);
        return (ccc_entry){{.e_ = any_struct, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = removed, .stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_romap_entry *
ccc_rom_and_modify(ccc_romap_entry *e, ccc_update_fn *fn)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED && e->impl_.entry_.e_)
    {
        fn((ccc_any_type){.any_type = e->impl_.entry_.e_, NULL});
    }
    return e;
}

ccc_romap_entry *
ccc_rom_and_modify_aux(ccc_romap_entry *e, ccc_update_fn *fn, void *aux)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED && e->impl_.entry_.e_)
    {
        fn((ccc_any_type){.any_type = e->impl_.entry_.e_, aux});
    }
    return e;
}

void *
ccc_rom_unwrap(ccc_romap_entry const *const e)
{
    if (e && e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
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
    return (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_tribool
ccc_rom_insert_error(ccc_romap_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_entry_status
ccc_rom_entry_status(ccc_romap_entry const *const e)
{
    return e ? e->impl_.entry_.stats_ : CCC_ENTRY_ARG_ERROR;
}

void *
ccc_rom_begin(ccc_realtime_ordered_map const *rom)
{
    if (!rom)
    {
        return NULL;
    }
    struct ccc_romap_elem_ *const m = min_max_from(rom, rom->root_, L);
    return m == &rom->end_ ? NULL : struct_base(rom, m);
}

void *
ccc_rom_next(ccc_realtime_ordered_map const *const rom,
             ccc_romap_elem const *const e)
{
    if (!rom || !e)
    {
        return NULL;
    }
    struct ccc_romap_elem_ const *const n = next(rom, e, INORDER_TRAVERSAL);
    if (n == &rom->end_)
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
    struct ccc_romap_elem_ *const m = min_max_from(rom, rom->root_, R);
    return m == &rom->end_ ? NULL : struct_base(rom, m);
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
    struct ccc_romap_elem_ const *const n
        = next(rom, e, REVERSE_INORDER_TRAVERSAL);
    return (n == &rom->end_) ? NULL : struct_base(rom, n);
}

ccc_range
ccc_rom_equal_range(ccc_realtime_ordered_map const *const rom,
                    void const *const begin_key, void const *const end_key)
{
    if (!rom || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(rom, begin_key, end_key, INORDER_TRAVERSAL)};
}

ccc_rrange
ccc_rom_equal_rrange(ccc_realtime_ordered_map const *const rom,
                     void const *const rbegin_key, void const *const rend_key)
{
    if (!rom || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){
        equal_range(rom, rbegin_key, rend_key, REVERSE_INORDER_TRAVERSAL)};
}

ccc_ucount
ccc_rom_size(ccc_realtime_ordered_map const *const rom)
{
    if (!rom)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = rom->sz_};
}

ccc_tribool
ccc_rom_is_empty(ccc_realtime_ordered_map const *const rom)
{
    if (!rom)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !rom->sz_;
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

ccc_result
ccc_rom_clear(ccc_realtime_ordered_map *const rom,
              ccc_destructor_fn *const destructor)
{
    if (!rom)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!ccc_rom_is_empty(rom))
    {
        if (!rom->root_->branch_[L] || !rom->root_->branch_[R])
        {
            return CCC_RESULT_ARG_ERROR;
        }
        void *const deleted = remove_fixup(rom, rom->root_);
        if (destructor)
        {
            destructor((ccc_any_type){.any_type = deleted, .aux = rom->aux_});
        }
        if (rom->alloc_)
        {
            (void)rom->alloc_(deleted, 0, rom->aux_);
        }
    }
    return CCC_RESULT_OK;
}

/*=========================   Private Interface  ============================*/

struct ccc_rtree_entry_
ccc_impl_rom_entry(struct ccc_romap_ const *const rom, void const *const key)
{
    return entry(rom, key);
}

void *
ccc_impl_rom_insert(struct ccc_romap_ *const rom,
                    struct ccc_romap_elem_ *const parent,
                    ccc_threeway_cmp const last_cmp,
                    struct ccc_romap_elem_ *const out_handle)
{
    return insert(rom, parent, last_cmp, out_handle);
}

void *
ccc_impl_rom_key_in_slot(struct ccc_romap_ const *const rom,
                         void const *const slot)
{
    return key_in_slot(rom, slot);
}

struct ccc_romap_elem_ *
ccc_impl_romap_elem_in_slot(struct ccc_romap_ const *const rom,
                            void const *const slot)
{
    return elem_in_slot(rom, slot);
}

/*=========================    Static Helpers    ============================*/

static struct ccc_romap_elem_ *
min_max_from(struct ccc_romap_ const *const rom, struct ccc_romap_elem_ *start,
             enum romap_link_ const dir)
{
    if (start == &rom->end_)
    {
        return start;
    }
    for (; start->branch_[dir] != &rom->end_; start = start->branch_[dir])
    {}
    return start;
}

static struct ccc_rtree_entry_
entry(struct ccc_romap_ const *const rom, void const *const key)
{
    struct romap_query_ const q = find(rom, key);
    if (CCC_EQL == q.last_cmp_)
    {
        return (struct ccc_rtree_entry_){
            .rom_ = (struct ccc_romap_ *)rom,
            .last_cmp_ = q.last_cmp_,
            .entry_ = {
                .e_ = struct_base(rom, q.found_),
                .stats_ = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct ccc_rtree_entry_){
        .rom_ = (struct ccc_romap_ *)rom,
        .last_cmp_ = q.last_cmp_,
        .entry_ = {
            .e_ = struct_base(rom, q.parent_),
            .stats_ = CCC_ENTRY_VACANT | CCC_ENTRY_NO_UNWRAP,
        },
    };
}

static void *
insert(struct ccc_romap_ *const rom, struct ccc_romap_elem_ *const parent,
       ccc_threeway_cmp const last_cmp,
       struct ccc_romap_elem_ *const out_handle)
{
    init_node(rom, out_handle);
    if (!rom->sz_)
    {
        rom->root_ = out_handle;
        ++rom->sz_;
        return struct_base(rom, out_handle);
    }
    assert(last_cmp == CCC_GRT || last_cmp == CCC_LES);
    ccc_tribool const rank_rule_break
        = parent->branch_[L] == &rom->end_ && parent->branch_[R] == &rom->end_;
    parent->branch_[CCC_GRT == last_cmp] = out_handle;
    out_handle->parent_ = parent;
    if (rank_rule_break)
    {
        insert_fixup(rom, parent, out_handle);
    }
    ++rom->sz_;
    return struct_base(rom, out_handle);
}

static void *
maybe_alloc_insert(struct ccc_romap_ *const rom,
                   struct ccc_romap_elem_ *const parent,
                   ccc_threeway_cmp const last_cmp,
                   struct ccc_romap_elem_ *out_handle)
{
    if (rom->alloc_)
    {
        void *const new = rom->alloc_(NULL, rom->elem_sz_, rom->aux_);
        if (!new)
        {
            return NULL;
        }
        memcpy(new, struct_base(rom, out_handle), rom->elem_sz_);
        out_handle = elem_in_slot(rom, new);
    }
    return insert(rom, parent, last_cmp, out_handle);
}

static struct romap_query_
find(struct ccc_romap_ const *const rom, void const *const key)
{
    struct ccc_romap_elem_ const *parent = &rom->end_;
    struct romap_query_ q = {.last_cmp_ = CCC_CMP_ERROR, .found_ = rom->root_};
    for (; q.found_ != &rom->end_;
         parent = q.found_,
         q.found_ = q.found_->branch_[CCC_GRT == q.last_cmp_])
    {
        q.last_cmp_ = cmp(rom, key, q.found_, rom->cmp_);
        if (CCC_EQL == q.last_cmp_)
        {
            return q;
        }
    }
    /* Type punning here OK as both union members have same type and size. */
    q.parent_ = (struct ccc_romap_elem_ *)parent;
    return q;
}

static struct ccc_romap_elem_ *
next(struct ccc_romap_ const *const rom, struct ccc_romap_elem_ const *n,
     enum romap_link_ const traversal)
{
    if (!n || n == &rom->end_)
    {
        return (struct ccc_romap_elem_ *)&rom->end_;
    }
    assert(rom->root_->parent_ == &rom->end_);
    /* The node is an internal one that has a sub-tree to explore first. */
    if (n->branch_[traversal] != &rom->end_)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->branch_[traversal]; n->branch_[!traversal] != &rom->end_;
             n = n->branch_[!traversal])
        {}
        return (struct ccc_romap_elem_ *)n;
    }
    for (; n->parent_ != &rom->end_ && n->parent_->branch_[!traversal] != n;
         n = n->parent_)
    {}
    return n->parent_;
}

static struct ccc_range_u_
equal_range(struct ccc_romap_ const *const rom, void const *const begin_key,
            void const *const end_key, enum romap_link_ const traversal)
{
    if (!rom->sz_)
    {
        return (struct ccc_range_u_){};
    }
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    struct romap_query_ b = find(rom, begin_key);
    if (b.last_cmp_ == les_or_grt[traversal])
    {
        b.found_ = next(rom, b.found_, traversal);
    }
    struct romap_query_ e = find(rom, end_key);
    if (e.last_cmp_ != les_or_grt[!traversal])
    {
        e.found_ = next(rom, e.found_, traversal);
    }
    return (struct ccc_range_u_){
        .begin_ = b.found_ == &rom->end_ ? NULL : struct_base(rom, b.found_),
        .end_ = e.found_ == &rom->end_ ? NULL : struct_base(rom, e.found_),
    };
}

static inline void
init_node(struct ccc_romap_ *const rom, struct ccc_romap_elem_ *const e)
{
    assert(e != NULL);
    assert(rom != NULL);
    e->branch_[L] = e->branch_[R] = e->parent_ = &rom->end_;
    e->parity_ = 0;
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const elem_sz)
{
    if (a == b || !a || !b)
    {
        return;
    }
    (void)memcpy(tmp, a, elem_sz);
    (void)memcpy(a, b, elem_sz);
    (void)memcpy(b, tmp, elem_sz);
}

static inline ccc_threeway_cmp
cmp(struct ccc_romap_ const *const rom, void const *const key,
    struct ccc_romap_elem_ const *const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){
        .key_lhs = key,
        .any_type_rhs = struct_base(rom, node),
        .aux = rom->aux_,
    });
}

static inline void *
struct_base(struct ccc_romap_ const *const rom,
            struct ccc_romap_elem_ const *const e)
{
    return ((char *)e->branch_) - rom->node_elem_offset_;
}

static inline void *
key_from_node(struct ccc_romap_ const *const rom,
              struct ccc_romap_elem_ const *const elem)
{
    return (char *)struct_base(rom, elem) + rom->key_offset_;
}

static inline void *
key_in_slot(struct ccc_romap_ const *const rom, void const *const slot)
{
    return (char *)slot + rom->key_offset_;
}

static inline struct ccc_romap_elem_ *
elem_in_slot(struct ccc_romap_ const *const rom, void const *const slot)
{
    return (struct ccc_romap_elem_ *)((char *)slot + rom->node_elem_offset_);
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct ccc_romap_ *const rom, struct ccc_romap_elem_ *z,
             struct ccc_romap_elem_ *x)
{
    do
    {
        promote(rom, z);
        x = z;
        z = z->parent_;
        if (z == &rom->end_)
        {
            return;
        }
    } while (is_01_parent(rom, x, z, sibling_of(rom, x)));

    if (!is_02_parent(rom, x, z, sibling_of(rom, x)))
    {
        return;
    }
    assert(x != &rom->end_);
    assert(is_0_child(rom, z, x));
    enum romap_link_ const p_to_x_dir = z->branch_[R] == x;
    struct ccc_romap_elem_ *const y = x->branch_[!p_to_x_dir];
    if (y == &rom->end_ || is_2_child(rom, z, y))
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
remove_fixup(struct ccc_romap_ *const rom, struct ccc_romap_elem_ *const remove)
{
    assert(remove->branch_[R] && remove->branch_[L]);
    struct ccc_romap_elem_ *y = NULL;
    struct ccc_romap_elem_ *x = NULL;
    struct ccc_romap_elem_ *p_of_xy = NULL;
    ccc_tribool two_child = CCC_FALSE;
    if (remove->branch_[L] == &rom->end_ || remove->branch_[R] == &rom->end_)
    {
        y = remove;
        p_of_xy = y->parent_;
        x = y->branch_[y->branch_[L] == &rom->end_];
        x->parent_ = y->parent_;
        if (p_of_xy == &rom->end_)
        {
            rom->root_ = x;
        }
        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->branch_[p_of_xy->branch_[R] == y] = x;
    }
    else
    {
        y = min_max_from(rom, remove->branch_[R], L);
        p_of_xy = y->parent_;
        x = y->branch_[y->branch_[L] == &rom->end_];
        x->parent_ = y->parent_;

        /* Save if check and improve readability by assuming this is true. */
        assert(p_of_xy != &rom->end_);

        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->branch_[p_of_xy->branch_[R] == y] = x;
        transplant(rom, remove, y);
        if (remove == p_of_xy)
        {
            p_of_xy = y;
        }
    }

    if (p_of_xy != &rom->end_)
    {
        if (two_child)
        {
            assert(p_of_xy != &rom->end_);
            rebalance_3_child(rom, p_of_xy, x);
        }
        else if (x == &rom->end_ && p_of_xy->branch_[L] == p_of_xy->branch_[R])
        {
            assert(p_of_xy != &rom->end_);
            ccc_tribool const demote_makes_3_child
                = is_2_child(rom, p_of_xy->parent_, p_of_xy);
            demote(rom, p_of_xy);
            if (demote_makes_3_child)
            {
                rebalance_3_child(rom, p_of_xy->parent_, p_of_xy);
            }
        }
        assert(!is_leaf(rom, p_of_xy) || !p_of_xy->parity_);
    }
    remove->branch_[L] = remove->branch_[R] = remove->parent_ = NULL;
    remove->parity_ = 0;
    --rom->sz_;
    return struct_base(rom, remove);
}

static void
rebalance_3_child(struct ccc_romap_ *const rom, struct ccc_romap_elem_ *z,
                  struct ccc_romap_elem_ *x)
{
    assert(z != &rom->end_);
    ccc_tribool made_3_child = CCC_FALSE;
    do
    {
        struct ccc_romap_elem_ *const g = z->parent_;
        struct ccc_romap_elem_ *const y = z->branch_[z->branch_[L] == x];
        made_3_child = is_2_child(rom, g, z);
        if (is_2_child(rom, z, y))
        {
            demote(rom, z);
        }
        else if (is_22_parent(rom, y->branch_[L], y, y->branch_[R]))
        {
            demote(rom, z);
            demote(rom, y);
        }
        else /* p(x) is 1,3, y is not a 2,2 parent, and x is 3-child.*/
        {
            assert(is_3_child(rom, z, x));
            enum romap_link_ const z_to_x_dir = z->branch_[R] == x;
            struct ccc_romap_elem_ *const w = y->branch_[!z_to_x_dir];
            if (is_1_child(rom, y, w))
            {
                rotate(rom, z, y, y->branch_[z_to_x_dir], z_to_x_dir);
                promote(rom, y);
                demote(rom, z);
                if (is_leaf(rom, z))
                {
                    demote(rom, z);
                }
            }
            else /* w is a 2-child and v will be a 1-child. */
            {
                struct ccc_romap_elem_ *const v = y->branch_[z_to_x_dir];
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
                    && is_11_parent(rom, z->branch_[L], z, z->branch_[R]))
                {
                    promote(rom, z);
                }
                else if (!is_leaf(rom, y)
                         && is_11_parent(rom, y->branch_[L], y, y->branch_[R]))
                {
                    promote(rom, y);
                }
            }
            return;
        }
        x = z;
        z = g;
    } while (z != &rom->end_ && made_3_child);
}

static void
transplant(struct ccc_romap_ *const rom, struct ccc_romap_elem_ *const remove,
           struct ccc_romap_elem_ *const replacement)
{
    assert(remove != &rom->end_);
    assert(replacement != &rom->end_);
    replacement->parent_ = remove->parent_;
    if (remove->parent_ == &rom->end_)
    {
        rom->root_ = replacement;
    }
    else
    {
        remove->parent_->branch_[remove->parent_->branch_[R] == remove]
            = replacement;
    }
    remove->branch_[R]->parent_ = replacement;
    remove->branch_[L]->parent_ = replacement;
    replacement->branch_[R] = remove->branch_[R];
    replacement->branch_[L] = remove->branch_[L];
    replacement->parity_ = remove->parity_;
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
rotate(struct ccc_romap_ *const rom, struct ccc_romap_elem_ *const z,
       struct ccc_romap_elem_ *const x, struct ccc_romap_elem_ *const y,
       enum romap_link_ dir)
{
    assert(z != &rom->end_);
    struct ccc_romap_elem_ *const g = z->parent_;
    x->parent_ = g;
    if (g == &rom->end_)
    {
        rom->root_ = x;
    }
    else
    {
        g->branch_[g->branch_[R] == z] = x;
    }
    x->branch_[dir] = z;
    z->parent_ = x;
    z->branch_[!dir] = y;
    y->parent_ = z;
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
double_rotate(struct ccc_romap_ *const rom, struct ccc_romap_elem_ *const z,
              struct ccc_romap_elem_ *const x, struct ccc_romap_elem_ *const y,
              enum romap_link_ dir)
{
    assert(z != &rom->end_);
    assert(x != &rom->end_);
    assert(y != &rom->end_);
    struct ccc_romap_elem_ *const g = z->parent_;
    y->parent_ = g;
    if (g == &rom->end_)
    {
        rom->root_ = y;
    }
    else
    {
        g->branch_[g->branch_[R] == z] = y;
    }
    x->branch_[!dir] = y->branch_[dir];
    y->branch_[dir]->parent_ = x;
    y->branch_[dir] = x;
    x->parent_ = y;

    z->branch_[dir] = y->branch_[!dir];
    y->branch_[!dir]->parent_ = z;
    y->branch_[!dir] = z;
    z->parent_ = y;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
      1╭─╯
       x */
[[maybe_unused]] static inline ccc_tribool
is_0_child(struct ccc_romap_ const *const rom,
           struct ccc_romap_elem_ const *const p,
           struct ccc_romap_elem_ const *const x)
{
    return p != &rom->end_ && p->parity_ == x->parity_;
}

/* Returns true for rank difference 1 between the parent and node.
         p
       1/
       x*/
static inline ccc_tribool
is_1_child(struct ccc_romap_ const *const rom,
           struct ccc_romap_elem_ const *const p,
           struct ccc_romap_elem_ const *const x)
{
    return p != &rom->end_ && p->parity_ != x->parity_;
}

/* Returns true for rank difference 2 between the parent and node.
         p
      2╭─╯
       x */
static inline ccc_tribool
is_2_child(struct ccc_romap_ const *const rom,
           struct ccc_romap_elem_ const *const p,
           struct ccc_romap_elem_ const *const x)
{
    return p != &rom->end_ && p->parity_ == x->parity_;
}

/* Returns true for rank difference 3 between the parent and node.
         p
      3╭─╯
       x */
[[maybe_unused]] static inline ccc_tribool
is_3_child(struct ccc_romap_ const *const rom,
           struct ccc_romap_elem_ const *const p,
           struct ccc_romap_elem_ const *const x)
{
    return p != &rom->end_ && p->parity_ != x->parity_;
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮1
       x   y */
static inline ccc_tribool
is_01_parent([[maybe_unused]] struct ccc_romap_ const *const rom,
             struct ccc_romap_elem_ const *const x,
             struct ccc_romap_elem_ const *const p,
             struct ccc_romap_elem_ const *const y)
{
    assert(p != &rom->end_);
    return (!x->parity_ && !p->parity_ && y->parity_)
           || (x->parity_ && p->parity_ && !y->parity_);
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
      1╭─┴─╮1
       x   y */
static inline ccc_tribool
is_11_parent([[maybe_unused]] struct ccc_romap_ const *const rom,
             struct ccc_romap_elem_ const *const x,
             struct ccc_romap_elem_ const *const p,
             struct ccc_romap_elem_ const *const y)
{
    assert(p != &rom->end_);
    return (!x->parity_ && p->parity_ && !y->parity_)
           || (x->parity_ && !p->parity_ && y->parity_);
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮2
       x   y */
static inline ccc_tribool
is_02_parent([[maybe_unused]] struct ccc_romap_ const *const rom,
             struct ccc_romap_elem_ const *const x,
             struct ccc_romap_elem_ const *const p,
             struct ccc_romap_elem_ const *const y)
{
    assert(p != &rom->end_);
    return (x->parity_ == p->parity_) && (p->parity_ == y->parity_);
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
is_22_parent([[maybe_unused]] struct ccc_romap_ const *const rom,
             struct ccc_romap_elem_ const *const x,
             struct ccc_romap_elem_ const *const p,
             struct ccc_romap_elem_ const *const y)
{
    assert(p != &rom->end_);
    return (x->parity_ == p->parity_) && (p->parity_ == y->parity_);
}

static inline void
promote(struct ccc_romap_ const *const rom, struct ccc_romap_elem_ *const x)
{
    if (x != &rom->end_)
    {
        x->parity_ = !x->parity_;
    }
}

static inline void
demote(struct ccc_romap_ const *const rom, struct ccc_romap_elem_ *const x)
{
    promote(rom, x);
}

/* One could imagine non-parity based rank tracking making this function
   meaningful, but two parity changes are the same as a no-op. Leave for
   clarity of what the code is meant to do through certain sections. */
static inline void
double_promote(struct ccc_romap_ const *const, struct ccc_romap_elem_ *const)
{}

/* One could imagine non-parity based rank tracking making this function
   meaningful, but two parity changes are the same as a no-op. Leave for
   clarity of what the code is meant to do through certain sections. */
static inline void
double_demote(struct ccc_romap_ const *const, struct ccc_romap_elem_ *const)
{}

static inline ccc_tribool
is_leaf(struct ccc_romap_ const *const rom,
        struct ccc_romap_elem_ const *const x)
{
    return x->branch_[L] == &rom->end_ && x->branch_[R] == &rom->end_;
}

static inline struct ccc_romap_elem_ *
sibling_of([[maybe_unused]] struct ccc_romap_ const *const rom,
           struct ccc_romap_elem_ const *const x)
{
    assert(x->parent_ != &rom->end_);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return x->parent_->branch_[x->parent_->branch_[L] == x];
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

/** @private */
struct tree_range_
{
    struct ccc_romap_elem_ const *low;
    struct ccc_romap_elem_ const *root;
    struct ccc_romap_elem_ const *high;
};

static size_t
recursive_size(struct ccc_romap_ const *const rom,
               struct ccc_romap_elem_ const *const r)
{
    if (r == &rom->end_)
    {
        return 0;
    }
    return 1 + recursive_size(rom, r->branch_[R])
           + recursive_size(rom, r->branch_[L]);
}

static ccc_tribool
are_subtrees_valid(struct ccc_romap_ const *t, struct tree_range_ const r,
                   struct ccc_romap_elem_ const *const nil)
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
        && cmp(t, key_from_node(t, r.low), r.root, t->cmp_) != CCC_LES)
    {
        return CCC_FALSE;
    }
    if (r.high != nil
        && cmp(t, key_from_node(t, r.high), r.root, t->cmp_) != CCC_GRT)
    {
        return CCC_FALSE;
    }
    return are_subtrees_valid(t,
                              (struct tree_range_){
                                  .low = r.low,
                                  .root = r.root->branch_[L],
                                  .high = r.root,
                              },
                              nil)
           && are_subtrees_valid(t,
                                 (struct tree_range_){
                                     .low = r.root,
                                     .root = r.root->branch_[R],
                                     .high = r.high,
                                 },
                                 nil);
}

static ccc_tribool
is_storing_parent(struct ccc_romap_ const *const t,
                  struct ccc_romap_elem_ const *const parent,
                  struct ccc_romap_elem_ const *const root)
{
    if (root == &t->end_)
    {
        return CCC_TRUE;
    }
    if (root->parent_ != parent)
    {
        return CCC_FALSE;
    }
    return is_storing_parent(t, root, root->branch_[L])
           && is_storing_parent(t, root, root->branch_[R]);
}

static ccc_tribool
validate(struct ccc_romap_ const *const rom)
{
    if (!rom->end_.parity_)
    {
        return CCC_FALSE;
    }
    if (!are_subtrees_valid(rom,
                            (struct tree_range_){
                                .low = &rom->end_,
                                .root = rom->root_,
                                .high = &rom->end_,
                            },
                            &rom->end_))
    {
        return CCC_FALSE;
    }
    if (recursive_size(rom, rom->root_) != rom->sz_)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(rom, &rom->end_, rom->root_))
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
