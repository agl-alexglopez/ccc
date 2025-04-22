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

This file implements a splay tree that supports duplicates.
The code to support a splay tree that allows duplicates is more complicated than
a traditional splay tree. It requires significant modification. This
implementation is based on the following source.

    1. Daniel Sleator, Carnegie Mellon University. Sleator's implementation of a
       topdown splay tree was instrumental in starting things off, but required
       extensive modification. I had to add the ability to track duplicates,
       update parent and child tracking, and unite the left and right cases for
       fun. See the code for a generalizable strategy to eliminate symmetric
       left and right cases for any binary tree code.
       https://www.link.cs.cmu.edu/splay/

Due to constant time access to duplicates this implementation can be quite fast
in specific workloads such as push and pop of duplicates. However, the overall
cost for update and insert can be quite high. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "impl/impl_ordered_multimap.h"
#include "impl/impl_types.h"
#include "ordered_multimap.h"
#include "types.h"

/** @private Instead of thinking about left and right consider only links in
the abstract sense. Put them in an array and then flip this enum and left and
right code paths can be united into one */
enum tree_link
{
    L = 0,
    R = 1,
};

/** @private Trees are just a different interpretation of the same links used
for doubly linked lists. We take advantage of this for duplicates. */
enum list_link
{
    P = 0,
    N = 1,
};

#define INORDER R
#define R_INORDER L

enum
{
    LR = 2,
};

/* =======================        Prototypes         ====================== */

/* No return value. */

static void init_node(struct ccc_ommap *, struct ccc_ommap_elem *);
static void link_trees(struct ccc_ommap *, struct ccc_ommap_elem *,
                       enum tree_link, struct ccc_ommap_elem *);
static void add_duplicate(struct ccc_ommap *, struct ccc_ommap_elem *,
                          struct ccc_ommap_elem *, struct ccc_ommap_elem *);

/* Boolean returns */

static ccc_tribool empty(struct ccc_ommap const *);
static ccc_tribool contains(struct ccc_ommap *, void const *);
static ccc_tribool has_dups(struct ccc_ommap_elem const *,
                            struct ccc_ommap_elem const *);
static ccc_tribool ccc_ommap_validate(struct ccc_ommap const *t);

/* Returning the user type that is stored in data structure. */

static struct ccc_ent multimap_insert(struct ccc_ommap *,
                                      struct ccc_ommap_elem *);
static void *multimap_erase(struct ccc_ommap *t, void const *key);
static void *find(struct ccc_ommap *, void const *);
static void *struct_base(struct ccc_ommap const *,
                         struct ccc_ommap_elem const *);
static void *multimap_erase_max_or_min(struct ccc_ommap *,
                                       ccc_any_key_cmp_fn *);
static void *multimap_erase_node(struct ccc_ommap *, struct ccc_ommap_elem *);
static void *connect_new_root(struct ccc_ommap *, struct ccc_ommap_elem *,
                              ccc_threeway_cmp);
static void *max(struct ccc_ommap const *);
static void *pop_max(struct ccc_ommap *);
static void *pop_min(struct ccc_ommap *);
static void *min(struct ccc_ommap const *);
static void *key_in_slot(struct ccc_ommap const *t, void const *slot);
static void *key_from_node(struct ccc_ommap const *t,
                           struct ccc_ommap_elem const *n);
static struct ccc_ommap_elem *elem_in_slot(struct ccc_ommap const *t,
                                           void const *slot);
static struct ccc_range_u equal_range(struct ccc_ommap *, void const *,
                                      void const *, enum tree_link);

/* Internal operations that take and return nodes for the tree. */

static struct ccc_ommap_elem *pop_dup_node(struct ccc_ommap *,
                                           struct ccc_ommap_elem *,
                                           struct ccc_ommap_elem *);
static struct ccc_ommap_elem *
pop_front_dup(struct ccc_ommap *, struct ccc_ommap_elem *, void const *old_key);
static struct ccc_ommap_elem *remove_from_tree(struct ccc_ommap *,
                                               struct ccc_ommap_elem *);
static struct ccc_ommap_elem const *
next(struct ccc_ommap const *, struct ccc_ommap_elem const *, enum tree_link);
static struct ccc_ommap_elem const *multimap_next(struct ccc_ommap const *,
                                                  struct ccc_ommap_elem const *,
                                                  enum tree_link);
static struct ccc_ommap_elem *splay(struct ccc_ommap *, struct ccc_ommap_elem *,
                                    void const *key, ccc_any_key_cmp_fn *);
static struct ccc_ommap_elem const *
next_tree_node(struct ccc_ommap const *, struct ccc_ommap_elem const *,
               enum tree_link);
static struct ccc_ommap_elem *parent_r(struct ccc_ommap const *,
                                       struct ccc_ommap_elem const *);

static struct ccc_omultimap_entry container_entry(struct ccc_ommap *,
                                                  void const *key);

/* Comparison function returns */

static ccc_threeway_cmp force_find_grt(ccc_any_key_cmp);
static ccc_threeway_cmp force_find_les(ccc_any_key_cmp);
/* The key comes first. It is the "left hand side" of the comparison. */
static ccc_threeway_cmp cmp(struct ccc_ommap const *, void const *key,
                            struct ccc_ommap_elem const *,
                            ccc_any_key_cmp_fn *);

/* ===========================    Interface     ============================ */

void *
ccc_omm_get_key_val(ccc_ordered_multimap *const mm, void const *const key)
{
    if (!mm || !key)
    {
        return nullptr;
    }
    return find(mm, key);
}

ccc_tribool
ccc_omm_is_empty(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return empty(mm);
}

ccc_ommap_entry
ccc_omm_entry(ccc_ordered_multimap *const mm, void const *const key)
{
    if (!mm || !key)
    {
        return (ccc_ommap_entry){
            {.t = nullptr,
             .entry = {.e = nullptr, .stats = CCC_ENTRY_ARG_ERROR}}};
    }
    return (ccc_ommap_entry){container_entry(mm, key)};
}

void *
ccc_omm_insert_entry(ccc_ommap_entry const *const e,
                     ccc_ommap_elem *const key_val_handle)
{
    if (!e || !key_val_handle)
    {
        return nullptr;
    }
    return multimap_insert(e->impl.t, key_val_handle).e;
}

void *
ccc_omm_or_insert(ccc_ommap_entry const *const e,
                  ccc_ommap_elem *const key_val_handle)
{
    if (!e || !key_val_handle)
    {
        return nullptr;
    }
    if (e->impl.entry.stats & CCC_ENTRY_OCCUPIED)
    {
        return e->impl.entry.e;
    }
    return multimap_insert(e->impl.t, key_val_handle).e;
}

ccc_ommap_entry *
ccc_omm_and_modify(ccc_ommap_entry *const e, ccc_any_type_update_fn *const fn)
{
    if (!e || !fn)
    {
        return nullptr;
    }
    if (e->impl.entry.stats & CCC_ENTRY_OCCUPIED && e->impl.entry.e)
    {
        fn((ccc_any_type){.any_type = e->impl.entry.e, .aux = nullptr});
    }
    return e;
}

ccc_ommap_entry *
ccc_omm_and_modify_aux(ccc_ommap_entry *const e,
                       ccc_any_type_update_fn *const fn, void *const aux)
{
    if (!e || !fn)
    {
        return nullptr;
    }
    if (e->impl.entry.stats & CCC_ENTRY_OCCUPIED && e->impl.entry.e)
    {
        fn((ccc_any_type){.any_type = e->impl.entry.e, .aux = aux});
    }
    return e;
}

ccc_entry
ccc_omm_swap_entry(ccc_ordered_multimap *const mm,
                   ccc_ommap_elem *const key_val_handle)
{
    if (!mm || !key_val_handle)
    {
        return (ccc_entry){{.e = nullptr, .stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct ccc_ommap_elem *n = key_val_handle;
    if (mm->alloc)
    {
        void *const mem = mm->alloc(nullptr, mm->sizeof_type, mm->aux);
        if (!mem)
        {
            return (ccc_entry){{
                .e = nullptr,
                .stats = CCC_ENTRY_INSERT_ERROR,
            }};
        }
        (void)memcpy(mem, struct_base(mm, key_val_handle), mm->sizeof_type);
        n = elem_in_slot(mm, mem);
    }
    return (ccc_entry){multimap_insert(mm, n)};
}

ccc_entry
ccc_omm_try_insert(ccc_ordered_multimap *const mm,
                   ccc_ommap_elem *const key_val_handle)
{
    if (!mm || !key_val_handle)
    {
        return (ccc_entry){{
            .e = nullptr,
            .stats = CCC_ENTRY_ARG_ERROR,
        }};
    }
    void const *const key = key_from_node(mm, key_val_handle);
    void *const found = find(mm, key);
    if (found)
    {
        return (ccc_entry){{
            .e = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (ccc_entry){multimap_insert(mm, key_val_handle)};
}

ccc_entry
ccc_omm_insert_or_assign(ccc_ordered_multimap *const mm,
                         ccc_ommap_elem *const key_val_handle)
{
    if (!mm || !key_val_handle)
    {
        return (ccc_entry){{
            .e = nullptr,
            .stats = CCC_ENTRY_ARG_ERROR,
        }};
    }
    void *const found = find(mm, key_from_node(mm, key_val_handle));
    if (found)
    {
        *key_val_handle = *elem_in_slot(mm, found);
        assert(mm->root != &mm->end);
        memcpy(found, struct_base(mm, key_val_handle), mm->sizeof_type);
        return (ccc_entry){{
            .e = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (ccc_entry){multimap_insert(mm, key_val_handle)};
}

ccc_entry
ccc_omm_remove(ccc_ordered_multimap *const mm, ccc_ommap_elem *const out_handle)
{

    if (!mm || !out_handle)
    {
        return (ccc_entry){{
            .e = nullptr,
            .stats = CCC_ENTRY_ARG_ERROR,
        }};
    }
    void *const n = multimap_erase(mm, key_from_node(mm, out_handle));
    if (!n)
    {
        return (ccc_entry){{
            .e = nullptr,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    if (mm->alloc)
    {
        void *const any_struct = struct_base(mm, out_handle);
        memcpy(any_struct, n, mm->sizeof_type);
        mm->alloc(n, 0, mm->aux);
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
ccc_omm_remove_entry(ccc_ommap_entry *const e)
{
    if (!e)
    {
        return (ccc_entry){{
            .e = nullptr,
            .stats = CCC_ENTRY_ARG_ERROR,
        }};
    }
    if (e->impl.entry.stats == CCC_ENTRY_OCCUPIED && e->impl.entry.e)
    {
        void *const erased = multimap_erase(
            e->impl.t, key_in_slot(e->impl.t, e->impl.entry.e));
        assert(erased);
        if (e->impl.t->alloc)
        {
            e->impl.t->alloc(erased, 0, e->impl.t->aux);
            return (ccc_entry){{
                .e = nullptr,
                .stats = CCC_ENTRY_OCCUPIED,
            }};
        }
        return (ccc_entry){{
            .e = erased,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (ccc_entry){{
        .e = nullptr,
        .stats = CCC_ENTRY_VACANT,
    }};
}

void *
ccc_omm_max(ccc_ordered_multimap *const mm)
{
    if (!mm)
    {
        return nullptr;
    }
    struct ccc_ommap_elem const *const n
        = splay(mm, mm->root, nullptr, force_find_grt);
    if (!n)
    {
        return nullptr;
    }
    return struct_base(mm, n);
}

void *
ccc_omm_min(ccc_ordered_multimap *const mm)
{
    if (!mm)
    {
        return nullptr;
    }
    struct ccc_ommap_elem const *const n
        = splay(mm, mm->root, nullptr, force_find_les);
    if (!n)
    {
        return nullptr;
    }
    return struct_base(mm, n);
}

void *
ccc_omm_begin(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return nullptr;
    }
    return max(mm);
}

void *
ccc_omm_rbegin(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return nullptr;
    }
    return min(mm);
}

void *
ccc_omm_next(ccc_ordered_multimap const *const mm,
             ccc_ommap_elem const *const iter_handle)
{
    if (!mm || !iter_handle)
    {
        return nullptr;
    }
    struct ccc_ommap_elem const *const n
        = multimap_next(mm, iter_handle, R_INORDER);
    return n == &mm->end ? nullptr : struct_base(mm, n);
}

void *
ccc_omm_rnext(ccc_ordered_multimap const *const mm,
              ccc_ommap_elem const *const iter_handle)
{
    if (!mm || !iter_handle)
    {
        return nullptr;
    }
    struct ccc_ommap_elem const *const n
        = multimap_next(mm, iter_handle, INORDER);
    return n == &mm->end ? nullptr : struct_base(mm, n);
}

void *
ccc_omm_end(ccc_ordered_multimap const *const)
{
    return nullptr;
}

void *
ccc_omm_rend(ccc_ordered_multimap const *const)
{
    return nullptr;
}

ccc_range
ccc_omm_equal_range(ccc_ordered_multimap *const mm, void const *const begin_key,
                    void const *const end_key)
{
    if (!mm || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(mm, begin_key, end_key, R_INORDER)};
}

ccc_rrange
ccc_omm_equal_rrange(ccc_ordered_multimap *const mm,
                     void const *const rbegin_key, void const *const rend_key)
{
    if (!mm || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){equal_range(mm, rbegin_key, rend_key, INORDER)};
}

void *
ccc_omm_extract(ccc_ordered_multimap *const mm,
                ccc_ommap_elem *const key_val_handle)
{
    if (!mm || !key_val_handle)
    {
        return nullptr;
    }
    return multimap_erase_node(mm, key_val_handle);
}

ccc_tribool
ccc_omm_update(ccc_ordered_multimap *const mm,
               ccc_ommap_elem *const key_val_handle,
               ccc_any_type_update_fn *const fn, void *const aux)
{
    if (!mm || !key_val_handle || !fn || !key_val_handle->branch[L]
        || !key_val_handle->branch[R])
    {
        return CCC_TRIBOOL_ERROR;
    }
    void *const e = multimap_erase_node(mm, key_val_handle);
    if (!e)
    {
        return CCC_FALSE;
    }
    fn((ccc_any_type){
        e,
        aux,
    });
    (void)multimap_insert(mm, key_val_handle);
    return CCC_TRUE;
}

ccc_tribool
ccc_omm_increase(ccc_ordered_multimap *const mm,
                 ccc_ommap_elem *const key_val_handle,
                 ccc_any_type_update_fn *const fn, void *const aux)
{
    return ccc_omm_update(mm, key_val_handle, fn, aux);
}

ccc_tribool
ccc_omm_decrease(ccc_ordered_multimap *const mm,
                 ccc_ommap_elem *const key_val_handle,
                 ccc_any_type_update_fn *const fn, void *const aux)
{
    return ccc_omm_update(mm, key_val_handle, fn, aux);
}

ccc_tribool
ccc_omm_contains(ccc_ordered_multimap *const mm, void const *const key)
{
    if (!mm || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return contains(mm, key);
}

ccc_result
ccc_omm_pop_max(ccc_ordered_multimap *const mm)
{
    if (!mm)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    void *const n = pop_max(mm);
    if (!n)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (mm->alloc)
    {
        mm->alloc(n, 0, mm->aux);
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_omm_pop_min(ccc_ordered_multimap *const mm)
{
    if (!mm)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    struct ccc_ommap_elem *const n = pop_min(mm);
    if (!n)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (mm->alloc)
    {
        mm->alloc(struct_base(mm, n), 0, mm->aux);
    }
    return CCC_RESULT_OK;
}

ccc_ucount
ccc_omm_size(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = mm->size};
}

void *
ccc_omm_unwrap(ccc_ommap_entry const *const e)
{
    return ccc_entry_unwrap(&(ccc_entry){e->impl.entry});
}

ccc_tribool
ccc_omm_insert_error(ccc_ommap_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.entry.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_omm_input_error(ccc_ommap_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.entry.stats & CCC_ENTRY_ARG_ERROR) != 0;
}

ccc_tribool
ccc_omm_occupied(ccc_ommap_entry const *const e)
{
    if (!e)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (e->impl.entry.stats & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_entry_status
ccc_omm_entry_status(ccc_ommap_entry const *const e)
{
    return e ? e->impl.entry.stats : CCC_ENTRY_ARG_ERROR;
}

ccc_tribool
ccc_omm_validate(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return ccc_ommap_validate(mm);
}

ccc_result
ccc_omm_clear(ccc_ordered_multimap *const mm,
              ccc_any_type_destructor_fn *const destructor)
{
    if (!mm)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!ccc_omm_is_empty(mm))
    {
        void *const popped = pop_min(mm);
        if (destructor)
        {
            destructor((ccc_any_type){
                .any_type = popped,
                .aux = mm->aux,
            });
        }
        if (mm->alloc)
        {
            (void)mm->alloc(popped, 0, mm->aux);
        }
    }
    return CCC_RESULT_OK;
}

/*==========================  Private Interface  ============================*/

struct ccc_omultimap_entry
ccc_impl_omm_entry(struct ccc_ommap *const t, void const *const key)
{
    return container_entry(t, key);
}

void *
ccc_impl_omm_multimap_insert(struct ccc_ommap *const t,
                             struct ccc_ommap_elem *const n)
{
    return multimap_insert(t, n).e;
}

void *
ccc_impl_omm_key_in_slot(struct ccc_ommap const *const t,
                         void const *const slot)
{
    return key_in_slot(t, slot);
}

struct ccc_ommap_elem *
ccc_impl_ommap_elem_in_slot(struct ccc_ommap const *const t,
                            void const *const slot)
{
    return elem_in_slot(t, slot);
}

/*======================  Static Splay Tree Helpers  ========================*/

static struct ccc_omultimap_entry
container_entry(struct ccc_ommap *const t, void const *const key)
{
    void *const found = find(t, key);
    if (found)
    {
        return (struct ccc_omultimap_entry){
            .t = t,
            .entry = {
                .e = found,
                .stats = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct ccc_omultimap_entry){
        .t = t,
        .entry = {
            .e = nullptr,
            .stats = CCC_ENTRY_VACANT,
        },
    };
}

static void *
find(struct ccc_ommap *const t, void const *const key)
{
    if (t->root == &t->end)
    {
        return nullptr;
    }
    t->root = splay(t, t->root, key, t->cmp);
    return cmp(t, key, t->root, t->cmp) == CCC_EQL ? struct_base(t, t->root)
                                                   : nullptr;
}

static inline void
init_node(struct ccc_ommap *const t, struct ccc_ommap_elem *const n)
{
    n->branch[L] = &t->end;
    n->branch[R] = &t->end;
    n->parent = &t->end;
}

static ccc_tribool
empty(struct ccc_ommap const *const t)
{
    return !t->size;
}

static void *
max(struct ccc_ommap const *const t)
{
    if (!t->size)
    {
        return nullptr;
    }
    struct ccc_ommap_elem *m = t->root;
    for (; m->branch[R] != &t->end; m = m->branch[R])
    {}
    return struct_base(t, m);
}

static void *
min(struct ccc_ommap const *const t)
{
    if (!t->size)
    {
        return nullptr;
    }
    struct ccc_ommap_elem *m = t->root;
    for (; m->branch[L] != &t->end; m = m->branch[L])
    {}
    return struct_base(t, m);
}

static inline void *
pop_max(struct ccc_ommap *const t)
{
    return multimap_erase_max_or_min(t, force_find_grt);
}

static inline void *
pop_min(struct ccc_ommap *const t)
{
    return multimap_erase_max_or_min(t, force_find_les);
}

static inline ccc_tribool
is_dup_head_next(struct ccc_ommap_elem const *const i)
{
    return i->link[R]->parent != nullptr;
}

static inline ccc_tribool
is_dup_head(struct ccc_ommap_elem const *const end,
            struct ccc_ommap_elem const *const i)
{
    return i != end && i->link[P] != end && i->link[P]->link[N] == i;
}

static struct ccc_ommap_elem const *
multimap_next(struct ccc_ommap const *const t,
              struct ccc_ommap_elem const *const i,
              enum tree_link const traversal)
{
    /* An arbitrary node in a doubly linked list of duplicates. */
    if (nullptr == i->parent)
    {
        /* We have finished the lap around the duplicate list. */
        if (is_dup_head_next(i))
        {
            return next_tree_node(t, i->link[N], traversal);
        }
        return i->link[N];
    }
    /* The special head node of a doubly linked list of duplicates. */
    if (is_dup_head(&t->end, i))
    {
        /* The duplicate head can be the only node in the list. */
        if (is_dup_head_next(i))
        {
            return next_tree_node(t, i, traversal);
        }
        return i->link[N];
    }
    if (has_dups(&t->end, i))
    {
        return i->dup_head;
    }
    return next(t, i, traversal);
}

static struct ccc_ommap_elem const *
next_tree_node(struct ccc_ommap const *const t,
               struct ccc_ommap_elem const *const head,
               enum tree_link const traversal)
{
    if (head->parent == &t->end)
    {
        return next(t, t->root, traversal);
    }
    struct ccc_ommap_elem const *parent = head->parent;
    if (parent->branch[L] != &t->end && parent->branch[L]->dup_head == head)
    {
        return next(t, parent->branch[L], traversal);
    }
    if (parent->branch[R] != &t->end && parent->branch[R]->dup_head == head)
    {
        return next(t, parent->branch[R], traversal);
    }
    return &t->end;
}

static struct ccc_ommap_elem const *
next(struct ccc_ommap const *const t, struct ccc_ommap_elem const *n,
     enum tree_link const traversal)
{
    if (!n || n == &t->end)
    {
        return nullptr;
    }
    assert(parent_r(t, t->root) == &t->end);
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
    struct ccc_ommap_elem const *p = parent_r(t, n);
    for (; p != &t->end && p->branch[!traversal] != n;
         n = p, p = parent_r(t, n))
    {}
    return p;
}

static struct ccc_range_u
equal_range(struct ccc_ommap *const t, void const *const begin_key,
            void const *const end_key, enum tree_link const traversal)
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
    struct ccc_ommap_elem const *b = splay(t, t->root, begin_key, t->cmp);
    if (cmp(t, begin_key, b, t->cmp) == les_or_grt[traversal])
    {
        b = next(t, b, traversal);
    }
    struct ccc_ommap_elem const *e = splay(t, t->root, end_key, t->cmp);
    if (cmp(t, end_key, e, t->cmp) != les_or_grt[!traversal])
    {
        e = next(t, e, traversal);
    }
    return (struct ccc_range_u){
        .begin = b == &t->end ? nullptr : struct_base(t, b),
        .end = e == &t->end ? nullptr : struct_base(t, e),
    };
}

static inline ccc_tribool
contains(struct ccc_ommap *const t, void const *const key)
{
    t->root = splay(t, t->root, key, t->cmp);
    return cmp(t, key, t->root, t->cmp) == CCC_EQL;
}

static struct ccc_ent
multimap_insert(struct ccc_ommap *const t,
                struct ccc_ommap_elem *const out_handle)
{
    init_node(t, out_handle);
    if (empty(t))
    {
        t->root = out_handle;
        t->size = 1;
        return (struct ccc_ent){.e = struct_base(t, out_handle),
                                .stats = CCC_ENTRY_VACANT};
    }
    t->size++;
    void const *const key = key_from_node(t, out_handle);
    t->root = splay(t, t->root, key, t->cmp);

    ccc_threeway_cmp const root_cmp = cmp(t, key, t->root, t->cmp);
    if (CCC_EQL == root_cmp)
    {
        add_duplicate(t, t->root, out_handle, &t->end);
        return (struct ccc_ent){.e = struct_base(t, out_handle),
                                .stats = CCC_ENTRY_OCCUPIED};
    }
    return (struct ccc_ent){.e = connect_new_root(t, out_handle, root_cmp),
                            .stats = CCC_ENTRY_VACANT};
}

static void *
connect_new_root(struct ccc_ommap *const t,
                 struct ccc_ommap_elem *const new_root,
                 ccc_threeway_cmp const cmp_result)
{
    enum tree_link const link = CCC_GRT == cmp_result;
    link_trees(t, new_root, link, t->root->branch[link]);
    link_trees(t, new_root, !link, t->root);
    t->root->branch[link] = &t->end;
    t->root = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link_trees(t, &t->end, 0, t->root);
    return struct_base(t, new_root);
}

static void
add_duplicate(struct ccc_ommap *const t, struct ccc_ommap_elem *const tree_node,
              struct ccc_ommap_elem *const add,
              struct ccc_ommap_elem *const parent)
{
    /* This is a circular doubly linked list with O(1) append to back
       to maintain round robin fairness for any use of this queue.
       The oldest duplicate should be in the tree so we will add new dup
       to the back. The head then needs to point to new tail and new
       tail points to already in place head that tree points to.
       This operation still works if we previously had size 1 list. */
    if (!has_dups(&t->end, tree_node))
    {
        add->parent = parent;
        tree_node->dup_head = add;
        add->link[N] = add;
        add->link[P] = add;
        return;
    }
    add->parent = nullptr;
    struct ccc_ommap_elem *const list_head = tree_node->dup_head;
    struct ccc_ommap_elem *const tail = list_head->link[P];
    tail->link[N] = add;
    list_head->link[P] = add;
    add->link[N] = list_head;
    add->link[P] = tail;
}

static void *
multimap_erase(struct ccc_ommap *const t, void const *const key)
{
    if (!t || !key)
    {
        return nullptr;
    }
    if (empty(t))
    {
        return nullptr;
    }
    struct ccc_ommap_elem *ret = splay(t, t->root, key, t->cmp);
    if (cmp(t, key, ret, t->cmp) != CCC_EQL)
    {
        return nullptr;
    }
    --t->size;
    if (has_dups(&t->end, ret))
    {
        ret = pop_front_dup(t, ret, key_from_node(t, ret));
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->branch[L] = ret->branch[R] = ret->parent = nullptr;
    return struct_base(t, ret);
}

/* We need to mindful of what the user is asking for. If they want any
   max or min, we have provided a dummy node and dummy compare function
   that will force us to return the max or min. So this operation
   simply grabs the first node available in the tree for round robin.
   This function expects to be passed the t->impl_il as the node and a
   comparison function that forces either the max or min to be searched. */
static void *
multimap_erase_max_or_min(struct ccc_ommap *const t,
                          ccc_any_key_cmp_fn *const force_max_or_min)
{
    if (!t || !force_max_or_min)
    {
        return nullptr;
    }
    if (empty(t))
    {
        return nullptr;
    }
    t->size--;
    struct ccc_ommap_elem *ret = splay(t, t->root, nullptr, force_max_or_min);
    if (has_dups(&t->end, ret))
    {
        ret = pop_front_dup(t, ret, key_from_node(t, ret));
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->branch[L] = ret->branch[R] = ret->parent = nullptr;
    return struct_base(t, ret);
}

/* We need to mindful of what the user is asking for. This is a request
   to erase the exact node provided in the argument. So extra care is
   taken to only delete that node, especially if a different node with
   the same size is splayed to the root and we are a duplicate in the
   list. */
static void *
multimap_erase_node(struct ccc_ommap *const t,
                    struct ccc_ommap_elem *const node)
{
    /* This is what we set removed nodes to so this is a mistaken query */
    if (nullptr == node->branch[R] || nullptr == node->branch[L])
    {
        return nullptr;
    }
    if (empty(t))
    {
        return nullptr;
    }
    t->size--;
    /* Special case that this must be a duplicate that is in the
       linked list but it is not the special head node. So, it
       is a quick snip to get it out. */
    if (!node->parent)
    {
        assert(node->link[P] && node->link[N] && node->link[N]->link[P] == node
               && node->link[P]->link[N] == node);
        node->link[P]->link[N] = node->link[N];
        node->link[N]->link[P] = node->link[P];
        node->link[N] = node->link[P] = node->parent = nullptr;
        return struct_base(t, node);
    }
    void const *const key = key_from_node(t, node);
    struct ccc_ommap_elem *ret = splay(t, t->root, key, t->cmp);
    if (cmp(t, key, ret, t->cmp) != CCC_EQL)
    {
        return nullptr;
    }
    if (has_dups(&t->end, ret))
    {
        ret = pop_dup_node(t, node, ret);
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->branch[L] = ret->branch[R] = ret->parent = nullptr;
    return struct_base(t, ret);
}

/* This function assumes that splayed is the new root of the tree */
static struct ccc_ommap_elem *
pop_dup_node(struct ccc_ommap *const t, struct ccc_ommap_elem *const dup,
             struct ccc_ommap_elem *const splayed)
{
    if (dup == splayed)
    {
        return pop_front_dup(t, splayed, key_from_node(t, splayed));
    }
    /* This is the head of the list of duplicates and no dups left. */
    if (dup->link[N] == dup)
    {
        splayed->parent = &t->end;
        return dup;
    }
    /* The dup is the head. There is an arbitrary number of dups after the
       head so replace head. Update the tail at back of the list. Easy to
       forget hard to catch because bugs are often delayed. */
    dup->link[P]->link[N] = dup->link[N];
    dup->link[N]->link[P] = dup->link[P];
    dup->link[N]->parent = dup->parent;
    splayed->dup_head = dup->link[N];
    return dup;
}

/* A node is in the tree and has a trailing list of duplicates. Many updates
must occur to remove the first duplicate from the list and make it part of the
splay tree to replace the old node to be deleted. */
static struct ccc_ommap_elem *
pop_front_dup(struct ccc_ommap *const t, struct ccc_ommap_elem *const old,
              void const *const old_key)
{
    struct ccc_ommap_elem *const parent = old->dup_head->parent;
    struct ccc_ommap_elem *const tree_replacement = old->dup_head;
    /* Comparing sizes with the root's parent is undefined. */
    if (old == t->root)
    {
        t->root = tree_replacement;
    }
    else
    {
        parent->branch[CCC_GRT == cmp(t, old_key, parent, t->cmp)]
            = tree_replacement;
    }

    struct ccc_ommap_elem *const new_list_head = old->dup_head->link[N];
    struct ccc_ommap_elem *const list_tail = old->dup_head->link[P];
    ccc_tribool const circular_list_empty
        = new_list_head->link[N] == new_list_head;

    new_list_head->link[P] = list_tail;
    new_list_head->parent = parent;
    list_tail->link[N] = new_list_head;
    tree_replacement->branch[L] = old->branch[L];
    tree_replacement->branch[R] = old->branch[R];
    tree_replacement->dup_head = new_list_head;

    link_trees(t, tree_replacement, L, tree_replacement->branch[L]);
    link_trees(t, tree_replacement, R, tree_replacement->branch[R]);
    if (circular_list_empty)
    {
        tree_replacement->parent = parent;
    }
    return old;
}

static struct ccc_ommap_elem *
remove_from_tree(struct ccc_ommap *const t, struct ccc_ommap_elem *const ret)
{
    if (ret->branch[L] == &t->end)
    {
        t->root = ret->branch[R];
        link_trees(t, &t->end, 0, t->root);
    }
    else
    {
        t->root = splay(t, ret->branch[L], key_from_node(t, ret), t->cmp);
        link_trees(t, t->root, R, ret->branch[R]);
    }
    return ret;
}

static struct ccc_ommap_elem *
splay(struct ccc_ommap *const t, struct ccc_ommap_elem *root,
      void const *const key, ccc_any_key_cmp_fn *const cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    t->end.branch[L] = t->end.branch[R] = t->end.parent = &t->end;
    struct ccc_ommap_elem *l_r_subtrees[LR] = {&t->end, &t->end};
    do
    {
        ccc_threeway_cmp const root_cmp = cmp(t, key, root, cmp_fn);
        enum tree_link const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || root->branch[dir] == &t->end)
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp(t, key, root->branch[dir], cmp_fn);
        enum tree_link const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            struct ccc_ommap_elem *const pivot = root->branch[dir];
            link_trees(t, root, dir, pivot->branch[!dir]);
            link_trees(t, pivot, !dir, root);
            root = pivot;
            if (root->branch[dir] == &t->end)
            {
                break;
            }
        }
        link_trees(t, l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = root->branch[dir];
    } while (1);
    link_trees(t, l_r_subtrees[L], R, root->branch[L]);
    link_trees(t, l_r_subtrees[R], L, root->branch[R]);
    link_trees(t, root, L, t->end.branch[R]);
    link_trees(t, root, R, t->end.branch[L]);
    t->root = root;
    link_trees(t, &t->end, 0, t->root);
    return root;
}

/* Links a parent to a sub-tree updating the parents child pointer in the
direction specified and updating the sub-tree parent field to point back to
parent. The direction of the parent to link is given by dir. This last step is
critical and easy to miss or mess up.

This function has proven to be VERY important. The nil node often has garbage
values associated with real nodes in our tree and if we access them by mistake
it's bad! But the nil is also helpful for some invariant coding patters and
reducing if checks all over the place.  */
static void
link_trees(struct ccc_ommap *const t, struct ccc_ommap_elem *const parent,
           enum tree_link const dir, struct ccc_ommap_elem *const subtree)
{
    parent->branch[dir] = subtree;
    if (has_dups(&t->end, subtree))
    {
        subtree->dup_head->parent = parent;
        return;
    }
    subtree->parent = parent;
}

/* This is tricky but because of how we store our nodes we always have an
   O(1) check available to us to tell whether a node in a tree is storing
   duplicates without any auxiliary data structures or struct fields.

   All nodes are in the tree tracking their parent. If we add duplicates,
   duplicates form a circular doubly linked list and the tree node
   uses its parent pointer to track the duplicate(s). The first duplicate then
   tracks the parent for the tree node. Therefore, we will always know
   how to identify a tree node that stores a duplicate. A tree node with
   a duplicate uses its parent field to point to a node that can
   find itself by checking its doubly linked list. A node in a tree
   could never do this because there is no route back to a node from
   its child pointers by definition of a binary tree. However, we must be
   careful not to access the end helper because it can store any pointers
   in its fields that should not be accessed for directions.

                             A<────┐
                           ┌─┴─┐   ├──┐
                           B   C──>D──E
                          ┌┴┐ ┌┴┐  └──┘
                          F G H I

   Consider the above tree where C is tracking duplicates. It sacrifices its
   parent field to track D. D tracks C's parent, A, and uses its left/right
   fields to track previous/next in a circular list. D's next and previous field
   are E and E's next and previous fields are D. So, D has a path back to itself
   and we can identify duplicates this way. This also means we can also identify
   if we ARE a duplicate but that check is not part of this function. */
static inline ccc_tribool
has_dups(struct ccc_ommap_elem const *const end,
         struct ccc_ommap_elem const *const n)
{
    return n != end && n->dup_head != end && n->dup_head->link[P] != end
           && n->dup_head->link[P]->link[N] == n->dup_head;
}

static inline struct ccc_ommap_elem *
parent_r(struct ccc_ommap const *const t, struct ccc_ommap_elem const *const n)
{
    return has_dups(&t->end, n) ? n->dup_head->parent : n->parent;
}

static inline void *
struct_base(struct ccc_ommap const *const t,
            struct ccc_ommap_elem const *const n)
{
    /* Link is the first field of the struct and is an array so no need to get
       pointer address of [0] element of array. That's the same as just the
       array field. */
    return ((char *)n->branch) - t->node_elem_offset;
}

static inline ccc_threeway_cmp
cmp(struct ccc_ommap const *const t, void const *const key,
    struct ccc_ommap_elem const *const node, ccc_any_key_cmp_fn *const fn)
{
    return fn((ccc_any_key_cmp){
        .any_key_lhs = key,
        .any_type_rhs = struct_base(t, node),
        .aux = t->aux,
    });
}

static inline void *
key_in_slot(struct ccc_ommap const *const t, void const *const slot)
{
    return (char *)slot + t->key_offset;
}

static inline void *
key_from_node(struct ccc_ommap const *const t,
              struct ccc_ommap_elem const *const n)
{
    return (char *)struct_base(t, n) + t->key_offset;
}

static inline struct ccc_ommap_elem *
elem_in_slot(struct ccc_ommap const *const t, void const *const slot)
{
    return (struct ccc_ommap_elem *)((char *)slot + t->node_elem_offset);
}

/* We can trick our splay tree into giving us the max via splaying
   without any input from the user. Our seach evaluates a threeway
   comparison to decide which branch to take in the tree or if we
   found the desired element. Simply force the function to always
   return one or the other and we will end up at the max or min */
static inline ccc_threeway_cmp
force_find_grt(ccc_any_key_cmp const)
{
    return CCC_GRT;
}

static inline ccc_threeway_cmp
force_find_les(ccc_any_key_cmp const)
{
    return CCC_LES;
}

/* NOLINTBEGIN(*misc-no-recursion) */

/* ======================        Debugging           ====================== */

/** @private Validate binary tree invariants with ranges. Use a recursive method
that does not rely upon the implementation of iterators or any other possibly
buggy implementation. A pure functional range check will provide the most
reliable check regardless of implementation changes throughout code base. */
struct tree_range
{
    struct ccc_ommap_elem const *low;
    struct ccc_ommap_elem const *root;
    struct ccc_ommap_elem const *high;
};

/** @private */
struct parent_status
{
    ccc_tribool correct;
    struct ccc_ommap_elem const *parent;
};

static size_t
count_dups(struct ccc_ommap const *const t,
           struct ccc_ommap_elem const *const n)
{
    if (!has_dups(&t->end, n))
    {
        return 0;
    }
    size_t dups = 1;
    for (struct ccc_ommap_elem *cur = n->dup_head->link[N]; cur != n->dup_head;
         cur = cur->link[N])
    {
        ++dups;
    }
    return dups;
}

static size_t
recursive_size(struct ccc_ommap const *const t,
               struct ccc_ommap_elem const *const r)
{
    if (r == &t->end)
    {
        return 0;
    }
    size_t s = count_dups(t, r) + 1;
    return s + recursive_size(t, r->branch[R])
           + recursive_size(t, r->branch[L]);
}

static ccc_tribool
are_subtrees_valid(struct ccc_ommap const *const t, struct tree_range const r,
                   struct ccc_ommap_elem const *const nil)
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
is_duplicate_storing_parent(struct ccc_ommap const *const t,
                            struct ccc_ommap_elem const *const parent,
                            struct ccc_ommap_elem const *const root)
{
    if (root == &t->end)
    {
        return CCC_TRUE;
    }
    if (has_dups(&t->end, root))
    {
        if (root->dup_head->parent != parent)
        {
            return CCC_FALSE;
        }
    }
    else if (root->parent != parent)
    {
        return CCC_FALSE;
    }
    return is_duplicate_storing_parent(t, root, root->branch[L])
           && is_duplicate_storing_parent(t, root, root->branch[R]);
}

/* Validate tree prefers to use recursion to examine the tree over the
   provided iterators of any implementation so as to avoid using a
   flawed implementation of such iterators. This should help be more
   sure that the implementation is correct because it follows the
   truth of the provided pointers with its own stack as backtracking
   information. */
static ccc_tribool
ccc_ommap_validate(struct ccc_ommap const *const t)
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
    if (!is_duplicate_storing_parent(t, &t->end, t->root))
    {
        return CCC_FALSE;
    }
    if (recursive_size(t, t->root) != t->size)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
