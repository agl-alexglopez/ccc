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

#include "impl/impl_ordered_map.h"
#include "impl/impl_types.h"
#include "ordered_map.h"
#include "types.h"

/** @private Instead of thinking about left and right consider only links
    in the abstract sense. Put them in an array and then flip
    this enum and left and right code paths can be united into one */
typedef enum
{
    L = 0,
    R,
} om_branch;

#define INORDER R
#define R_INORDER L

enum
{
    LR = 2,
};

/* Container entry return value. */

static struct ccc_otree_entry container_entry(struct ccc_omap *t,
                                              void const *key);

/* No return value. */

static void init_node(struct ccc_omap *, struct ccc_omap_elem *);
static void swap(char tmp[const], void *, void *, size_t);
static void link(struct ccc_omap_elem *, om_branch, struct ccc_omap_elem *);

/* Boolean returns */

static ccc_tribool empty(struct ccc_omap const *);
static ccc_tribool contains(struct ccc_omap *, void const *);
static ccc_tribool ccc_omap_validate(struct ccc_omap const *);

/* Returning the user type that is stored in data structure. */

static void *struct_base(struct ccc_omap const *, struct ccc_omap_elem const *);
static void *find(struct ccc_omap *, void const *);
static void *erase(struct ccc_omap *, void const *key);
static void *alloc_insert(struct ccc_omap *, struct ccc_omap_elem *);
static void *insert(struct ccc_omap *, struct ccc_omap_elem *);
static void *connect_new_root(struct ccc_omap *, struct ccc_omap_elem *,
                              ccc_threeway_cmp);
static void *max(struct ccc_omap const *);
static void *min(struct ccc_omap const *);
static void *key_in_slot(struct ccc_omap const *t, void const *slot);
static void *key_from_node(struct ccc_omap const *, ccc_omap_elem const *);
static struct ccc_range_u equal_range(struct ccc_omap *, void const *,
                                      void const *, om_branch);

/* Internal operations that take and return nodes for the tree. */

static struct ccc_omap_elem *remove_from_tree(struct ccc_omap *,
                                              struct ccc_omap_elem *);
static struct ccc_omap_elem const *
next(struct ccc_omap const *, struct ccc_omap_elem const *, om_branch);
static struct ccc_omap_elem *splay(struct ccc_omap *, struct ccc_omap_elem *,
                                   void const *key, ccc_any_key_cmp_fn *);
static struct ccc_omap_elem *elem_in_slot(struct ccc_omap const *t,
                                          void const *slot);

/* The key comes first. It is the "left hand side" of the comparison. */
static ccc_threeway_cmp cmp(struct ccc_omap const *, void const *key,
                            struct ccc_omap_elem const *, ccc_any_key_cmp_fn *);

/* ======================        Map Interface      ====================== */

ccc_tribool
ccc_om_is_empty(ccc_ordered_map const *const om)
{
    if (!om)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return empty(om);
}

ccc_ucount
ccc_om_count(ccc_ordered_map const *const om)
{
    if (!om)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = om->size};
}

ccc_tribool
ccc_om_contains(ccc_ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return contains(om, key);
}

ccc_omap_entry
ccc_om_entry(ccc_ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return (ccc_omap_entry){{.entry = {.stats = CCC_ENTRY_ARG_ERROR}}};
    }
    return (ccc_omap_entry){container_entry(om, key)};
}

void *
ccc_om_insert_entry(ccc_omap_entry const *const e, ccc_omap_elem *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->impl.entry.stats == CCC_ENTRY_OCCUPIED)
    {
        if (e->impl.entry.e)
        {
            *elem = *elem_in_slot(e->impl.t, e->impl.entry.e);
            (void)memcpy(e->impl.entry.e, struct_base(e->impl.t, elem),
                         e->impl.t->sizeof_type);
        }
        return e->impl.entry.e;
    }
    return alloc_insert(e->impl.t, elem);
}

void *
ccc_om_or_insert(ccc_omap_entry const *const e, ccc_omap_elem *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->impl.entry.stats & CCC_ENTRY_OCCUPIED)
    {
        return e->impl.entry.e;
    }
    return alloc_insert(e->impl.t, elem);
}

ccc_omap_entry *
ccc_om_and_modify(ccc_omap_entry *const e, ccc_any_type_update_fn *const fn)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->impl.entry.stats & CCC_ENTRY_OCCUPIED && e->impl.entry.e)
    {
        fn((ccc_any_type){
            .any_type = e->impl.entry.e,
            .aux = NULL,
        });
    }
    return e;
}

ccc_omap_entry *
ccc_om_and_modify_aux(ccc_omap_entry *const e, ccc_any_type_update_fn *const fn,
                      void *const aux)
{
    if (e && fn && e->impl.entry.stats & CCC_ENTRY_OCCUPIED && e->impl.entry.e)
    {
        fn((ccc_any_type){
            .any_type = e->impl.entry.e,
            .aux = aux,
        });
    }
    return e;
}

ccc_entry
ccc_om_swap_entry(ccc_ordered_map *const om,
                  ccc_omap_elem *const key_val_handle, ccc_omap_elem *const tmp)
{
    if (!om || !key_val_handle || !tmp)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
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
        return (ccc_entry){{
            .e = old_val,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = alloc_insert(om, key_val_handle);
    if (!inserted)
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
ccc_om_try_insert(ccc_ordered_map *const om,
                  ccc_omap_elem *const key_val_handle)
{
    if (!om || !key_val_handle)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const found = find(om, key_from_node(om, key_val_handle));
    if (found)
    {
        assert(om->root != &om->end);
        return (ccc_entry){{
            .e = struct_base(om, om->root),
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = alloc_insert(om, key_val_handle);
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
ccc_om_insert_or_assign(ccc_ordered_map *const om,
                        ccc_omap_elem *const key_val_handle)
{
    if (!om || !key_val_handle)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const found = find(om, key_from_node(om, key_val_handle));
    if (found)
    {
        *key_val_handle = *elem_in_slot(om, found);
        assert(om->root != &om->end);
        memcpy(found, struct_base(om, key_val_handle), om->sizeof_type);
        return (ccc_entry){{
            .e = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    void *const inserted = alloc_insert(om, key_val_handle);
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
ccc_om_remove(ccc_ordered_map *const om, ccc_omap_elem *const out_handle)
{
    if (!om || !out_handle)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    void *const n = erase(om, key_from_node(om, out_handle));
    if (!n)
    {
        return (ccc_entry){{
            .e = NULL,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    if (om->alloc)
    {
        void *const any_struct = struct_base(om, out_handle);
        memcpy(any_struct, n, om->sizeof_type);
        om->alloc(n, 0, om->aux);
        return (ccc_entry){{
            .e = any_struct,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (ccc_entry){{
        .e = n,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

ccc_entry
ccc_om_remove_entry(ccc_omap_entry *const e)
{
    if (!e)
    {
        return (ccc_entry){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (e->impl.entry.stats == CCC_ENTRY_OCCUPIED && e->impl.entry.e)
    {
        void *const erased
            = erase(e->impl.t, key_in_slot(e->impl.t, e->impl.entry.e));
        assert(erased);
        if (e->impl.t->alloc)
        {
            e->impl.t->alloc(erased, 0, e->impl.t->aux);
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

void *
ccc_om_get_key_val(ccc_ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return NULL;
    }
    return find(om, key);
}

void *
ccc_om_unwrap(ccc_omap_entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->impl.entry.stats == CCC_ENTRY_OCCUPIED ? e->impl.entry.e : NULL;
}

ccc_tribool
ccc_om_insert_error(ccc_omap_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.entry.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_om_occupied(ccc_omap_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.entry.stats & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_entry_status
ccc_om_entry_status(ccc_omap_entry const *const e)
{
    return e ? e->impl.entry.stats : CCC_ENTRY_ARG_ERROR;
}

void *
ccc_om_begin(ccc_ordered_map const *const om)
{
    return om ? min(om) : NULL;
}

void *
ccc_om_rbegin(ccc_ordered_map const *const om)
{
    return om ? max(om) : NULL;
}

void *
ccc_om_end(ccc_ordered_map const *const)
{
    return NULL;
}

void *
ccc_om_rend(ccc_ordered_map const *const)
{
    return NULL;
}

void *
ccc_om_next(ccc_ordered_map const *const om, ccc_omap_elem const *const e)
{
    if (!om || !e)
    {
        return NULL;
    }
    struct ccc_omap_elem const *n = next(om, e, INORDER);
    return n == &om->end ? NULL : struct_base(om, n);
}

void *
ccc_om_rnext(ccc_ordered_map const *const om, ccc_omap_elem const *const e)
{
    if (!om || !e)
    {
        return NULL;
    }
    struct ccc_omap_elem const *n = next(om, e, R_INORDER);
    return n == &om->end ? NULL : struct_base(om, n);
}

ccc_range
ccc_om_equal_range(ccc_ordered_map *const om, void const *const begin_key,
                   void const *const end_key)
{
    if (!om || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(om, begin_key, end_key, INORDER)};
}

ccc_rrange
ccc_om_equal_rrange(ccc_ordered_map *const om, void const *const rbegin_key,
                    void const *const rend_key)

{
    if (!om || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){equal_range(om, rbegin_key, rend_key, R_INORDER)};
}

/** This is a linear time constant space deletion of tree nodes via left
rotations so element fields are modified during progression of deletes. */
ccc_result
ccc_om_clear(ccc_ordered_map *const om,
             ccc_any_type_destructor_fn *const destructor)
{
    if (!om)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_omap_elem *node = om->root;
    while (node != &om->end)
    {
        if (node->branch[L] != &om->end)
        {
            struct ccc_omap_elem *const l = node->branch[L];
            node->branch[L] = l->branch[R];
            l->branch[R] = node;
            node = l;
            continue;
        }
        struct ccc_omap_elem *const next = node->branch[R];
        node->branch[L] = node->branch[R] = NULL;
        node->parent = NULL;
        void *const del = struct_base(om, node);
        if (destructor)
        {
            destructor((ccc_any_type){
                .any_type = del,
                .aux = om->aux,
            });
        }
        if (om->alloc)
        {
            (void)om->alloc(del, 0, om->aux);
        }
        node = next;
    }
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_om_validate(ccc_ordered_map const *const om)
{
    if (!om)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return ccc_omap_validate(om);
}

/*==========================  Private Interface  ============================*/

struct ccc_otree_entry
ccc_impl_om_entry(struct ccc_omap *const t, void const *const key)
{
    return container_entry(t, key);
}

void *
ccc_impl_om_insert(struct ccc_omap *const t, struct ccc_omap_elem *n)
{
    return insert(t, n);
}

void *
ccc_impl_om_key_in_slot(struct ccc_omap const *const t, void const *const slot)
{
    return key_in_slot(t, slot);
}

struct ccc_omap_elem *
ccc_impl_omap_elem_in_slot(struct ccc_omap const *const t, void const *slot)
{

    return elem_in_slot(t, slot);
}

/*======================  Static Splay Tree Helpers  ========================*/

static struct ccc_otree_entry
container_entry(struct ccc_omap *const t, void const *const key)
{
    void *const found = find(t, key);
    if (found)
    {
        return (struct ccc_otree_entry){
            .t = t,
            .entry = {
                .e = found,
                .stats = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct ccc_otree_entry){
        .t = t,
        .entry = {
            .e = found,
            .stats = CCC_ENTRY_VACANT,
        },
    };
}

static inline void *
key_from_node(struct ccc_omap const *const t, ccc_omap_elem const *const n)
{
    return (char *)struct_base(t, n) + t->key_offset;
}

static inline void *
key_in_slot(struct ccc_omap const *const t, void const *const slot)
{
    return (char *)slot + t->key_offset;
}

static inline struct ccc_omap_elem *
elem_in_slot(struct ccc_omap const *const t, void const *const slot)
{

    return (struct ccc_omap_elem *)((char *)slot + t->node_elem_offset);
}

static inline void
init_node(struct ccc_omap *const t, struct ccc_omap_elem *const n)
{
    n->branch[L] = &t->end;
    n->branch[R] = &t->end;
    n->parent = &t->end;
}

static inline ccc_tribool
empty(struct ccc_omap const *const t)
{
    return !t->size || !t->root || t->root == &t->end;
}

static void *
max(struct ccc_omap const *const t)
{
    if (!t->size)
    {
        return NULL;
    }
    struct ccc_omap_elem *m = t->root;
    for (; m->branch[R] != &t->end; m = m->branch[R])
    {}
    return struct_base(t, m);
}

static void *
min(struct ccc_omap const *t)
{
    if (!t->size)
    {
        return NULL;
    }
    struct ccc_omap_elem *m = t->root;
    for (; m->branch[L] != &t->end; m = m->branch[L])
    {}
    return struct_base(t, m);
}

static struct ccc_omap_elem const *
next(struct ccc_omap const *const t, struct ccc_omap_elem const *n,
     om_branch const traversal)
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
    struct ccc_omap_elem const *p = n->parent;
    for (; p != &t->end && p->branch[!traversal] != n; n = p, p = n->parent)
    {}
    return p;
}

static struct ccc_range_u
equal_range(struct ccc_omap *const t, void const *const begin_key,
            void const *const end_key, om_branch const traversal)
{
    if (!t->size)
    {
        return (struct ccc_range_u){};
    }
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    struct ccc_omap_elem const *b = splay(t, t->root, begin_key, t->cmp);
    if (cmp(t, begin_key, b, t->cmp) == les_or_grt[traversal])
    {
        b = next(t, b, traversal);
    }
    struct ccc_omap_elem const *e = splay(t, t->root, end_key, t->cmp);
    if (cmp(t, end_key, e, t->cmp) != les_or_grt[!traversal])
    {
        e = next(t, e, traversal);
    }
    return (struct ccc_range_u){
        .begin = b == &t->end ? NULL : struct_base(t, b),
        .end = e == &t->end ? NULL : struct_base(t, e),
    };
}

static void *
find(struct ccc_omap *const t, void const *const key)
{
    if (t->root == &t->end)
    {
        return NULL;
    }
    t->root = splay(t, t->root, key, t->cmp);
    return cmp(t, key, t->root, t->cmp) == CCC_EQL ? struct_base(t, t->root)
                                                   : NULL;
}

static ccc_tribool
contains(struct ccc_omap *const t, void const *const key)
{
    t->root = splay(t, t->root, key, t->cmp);
    return cmp(t, key, t->root, t->cmp) == CCC_EQL;
}

static void *
alloc_insert(struct ccc_omap *const t, struct ccc_omap_elem *out_handle)
{
    init_node(t, out_handle);
    ccc_threeway_cmp root_cmp = CCC_CMP_ERROR;
    if (!empty(t))
    {
        void const *const key = key_from_node(t, out_handle);
        t->root = splay(t, t->root, key, t->cmp);
        root_cmp = cmp(t, key, t->root, t->cmp);
        if (CCC_EQL == root_cmp)
        {
            return NULL;
        }
    }
    if (t->alloc)
    {
        void *const node = t->alloc(NULL, t->sizeof_type, t->aux);
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
    assert(root_cmp != CCC_CMP_ERROR);
    t->size++;
    return connect_new_root(t, out_handle, root_cmp);
}

static void *
insert(struct ccc_omap *const t, struct ccc_omap_elem *const n)
{
    init_node(t, n);
    if (empty(t))
    {
        t->root = n;
        t->size = 1;
        return struct_base(t, n);
    }
    void const *const key = key_from_node(t, n);
    t->root = splay(t, t->root, key, t->cmp);
    ccc_threeway_cmp const root_cmp = cmp(t, key, t->root, t->cmp);
    if (CCC_EQL == root_cmp)
    {
        return NULL;
    }
    t->size++;
    return connect_new_root(t, n, root_cmp);
}

static void *
connect_new_root(struct ccc_omap *const t, struct ccc_omap_elem *const new_root,
                 ccc_threeway_cmp const cmp_result)
{
    om_branch const dir = CCC_GRT == cmp_result;
    link(new_root, dir, t->root->branch[dir]);
    link(new_root, !dir, t->root);
    t->root->branch[dir] = &t->end;
    t->root = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link(&t->end, 0, t->root);
    return struct_base(t, new_root);
}

static void *
erase(struct ccc_omap *const t, void const *const key)
{
    if (empty(t))
    {
        return NULL;
    }
    struct ccc_omap_elem *ret = splay(t, t->root, key, t->cmp);
    ccc_threeway_cmp const found = cmp(t, key, ret, t->cmp);
    if (found != CCC_EQL)
    {
        return NULL;
    }
    ret = remove_from_tree(t, ret);
    ret->branch[L] = ret->branch[R] = ret->parent = NULL;
    t->size--;
    return struct_base(t, ret);
}

static struct ccc_omap_elem *
remove_from_tree(struct ccc_omap *const t, struct ccc_omap_elem *const ret)
{
    if (ret->branch[L] == &t->end)
    {
        t->root = ret->branch[R];
        link(&t->end, 0, t->root);
    }
    else
    {
        t->root = splay(t, ret->branch[L], key_from_node(t, ret), t->cmp);
        link(t->root, R, ret->branch[R]);
    }
    return ret;
}

static struct ccc_omap_elem *
splay(struct ccc_omap *const t, struct ccc_omap_elem *root,
      void const *const key, ccc_any_key_cmp_fn *const cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    t->end.branch[L] = t->end.branch[R] = t->end.parent = &t->end;
    struct ccc_omap_elem *l_r_subtrees[LR] = {&t->end, &t->end};
    for (;;)
    {
        ccc_threeway_cmp const root_cmp = cmp(t, key, root, cmp_fn);
        om_branch const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || root->branch[dir] == &t->end)
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp(t, key, root->branch[dir], cmp_fn);
        om_branch const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            struct ccc_omap_elem *const pivot = root->branch[dir];
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
struct_base(struct ccc_omap const *const t, struct ccc_omap_elem const *const n)
{
    /* Link is the first field of the struct and is an array so no need to get
       pointer address of [0] element of array. That's the same as just the
       array field. */
    return ((char *)n->branch) - t->node_elem_offset;
}

static inline ccc_threeway_cmp
cmp(struct ccc_omap const *const t, void const *const key,
    struct ccc_omap_elem const *const node, ccc_any_key_cmp_fn *const fn)
{
    return fn((ccc_any_key_cmp){
        .any_key_lhs = key,
        .any_type_rhs = struct_base(t, node),
        .aux = t->aux,
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
link(struct ccc_omap_elem *const parent, om_branch const dir,
     struct ccc_omap_elem *const subtree)
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
struct tree_range
{
    struct ccc_omap_elem const *low;
    struct ccc_omap_elem const *root;
    struct ccc_omap_elem const *high;
};

/** @private */
struct parent_status
{
    ccc_tribool correct;
    struct ccc_omap_elem const *parent;
};

static size_t
recursive_count(struct ccc_omap const *const t,
                struct ccc_omap_elem const *const r)
{
    if (r == &t->end)
    {
        return 0;
    }
    return 1 + recursive_count(t, r->branch[R])
         + recursive_count(t, r->branch[L]);
}

static ccc_tribool
are_subtrees_valid(struct ccc_omap const *const t, struct tree_range const r,
                   struct ccc_omap_elem const *const nil)
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
is_parent_correct(struct ccc_omap const *const t,
                  struct ccc_omap_elem const *const parent,
                  struct ccc_omap_elem const *const root)
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
static ccc_tribool
ccc_omap_validate(struct ccc_omap const *const t)
{
    if (!are_subtrees_valid(t,
                            (struct tree_range){
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
