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

#include "handle_ordered_map.h"
#include "impl/impl_handle_ordered_map.h"
#include "impl/impl_types.h"
#include "types.h"

/** @private */
enum
{
    LR = 2,
};

/** @private */
enum hom_branch
{
    L = 0,
    R,
};

#define INORDER R
#define R_INORDER L

enum
{
    EMPTY_TREE = 2,
};

/* Buffer allocates before insert. "Empty" has nil 0th slot and one more. */

/*==============================  Prototypes   ==============================*/

/* Returning the internal elem type with stored offsets. */
static size_t splay(struct ccc_homap *t, size_t root, void const *key,
                    ccc_any_key_cmp_fn *cmp_fn);
static struct ccc_homap_elem *node_at(struct ccc_homap const *, size_t);
static void *data_at(struct ccc_homap const *, size_t);
/* Returning the user struct type with stored offsets. */
static struct ccc_htree_handle handle(struct ccc_homap *hom, void const *key);
static size_t erase(struct ccc_homap *t, void const *key);
static size_t maybe_alloc_insert(struct ccc_homap *hom, void const *user_type);
static ccc_result resize(struct ccc_homap *hom, size_t new_capacity,
                         ccc_any_alloc_fn *fn);
static void copy_soa(struct ccc_homap const *src, void *dst_data_base,
                     size_t dst_capacity);
static size_t data_bytes(size_t sizeof_type, size_t capacity);
static size_t node_bytes(size_t capacity);
static struct ccc_homap_elem *node_pos(size_t sizeof_type, void const *data,
                                       size_t capacity);
static size_t find(struct ccc_homap *, void const *key);
static size_t connect_new_root(struct ccc_homap *t, size_t new_root,
                               ccc_threeway_cmp cmp_result);
static void insert(struct ccc_homap *t, size_t n);
static void *key_in_slot(struct ccc_homap const *t, void const *user_struct);
static size_t alloc_slot(struct ccc_homap *t);
static size_t total_bytes(size_t sizeof_type, size_t capacity);
static struct ccc_range_u equal_range(struct ccc_homap *t,
                                      void const *begin_key,
                                      void const *end_key,
                                      enum hom_branch traversal);
/* Returning the user key with stored offsets. */
static void *key_at(struct ccc_homap const *t, size_t i);
/* Returning threeway comparison with user callback. */
static ccc_threeway_cmp cmp_elems(struct ccc_homap const *hom, void const *key,
                                  size_t node, ccc_any_key_cmp_fn *fn);
/* Returning read only indices for tree nodes. */
static size_t remove_from_tree(struct ccc_homap *t, size_t ret);
static size_t min_max_from(struct ccc_homap const *t, size_t start,
                           enum hom_branch dir);
static size_t next(struct ccc_homap const *t, size_t n,
                   enum hom_branch traversal);
static size_t branch_i(struct ccc_homap const *t, size_t parent,
                       enum hom_branch dir);
static size_t parent_i(struct ccc_homap const *t, size_t child);
static size_t index_of(struct ccc_homap const *t, void const *key_val_type);
/* Returning references to index fields for tree nodes. */
static size_t *branch_ref(struct ccc_homap const *t, size_t node,
                          enum hom_branch branch);
static size_t *parent_ref(struct ccc_homap const *t, size_t node);

static ccc_tribool validate(struct ccc_homap const *hom);

/* Returning void as miscellaneous helpers. */
static void init_node(struct ccc_homap const *hom, size_t node);
static void swap(char tmp[const], void *a, void *b, size_t sizeof_type);
static void link(struct ccc_homap *t, size_t parent, enum hom_branch dir,
                 size_t subtree);
static size_t max(size_t, size_t);

/*==============================  Interface    ==============================*/

void *
ccc_hom_at(ccc_handle_ordered_map const *const h, ccc_handle_i const i)
{
    if (!h || !i)
    {
        return NULL;
    }
    return data_at(h, i);
}

ccc_tribool
ccc_hom_contains(ccc_handle_ordered_map *const hom, void const *const key)
{
    if (!hom || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    hom->root = splay(hom, hom->root, key, hom->cmp);
    return cmp_elems(hom, key, hom->root, hom->cmp) == CCC_EQL;
}

ccc_handle_i
ccc_hom_get_key_val(ccc_handle_ordered_map *const hom, void const *const key)
{
    if (!hom || !key)
    {
        return 0;
    }
    return find(hom, key);
}

ccc_homap_handle
ccc_hom_handle(ccc_handle_ordered_map *const hom, void const *const key)
{
    if (!hom || !key)
    {
        return (ccc_homap_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    return (ccc_homap_handle){handle(hom, key)};
}

ccc_handle_i
ccc_hom_insert_handle(ccc_homap_handle const *const h,
                      void const *const key_val_type)
{
    if (!h || !key_val_type)
    {
        return 0;
    }
    if (h->impl.stats == CCC_ENTRY_OCCUPIED)
    {
        void *const ret = data_at(h->impl.hom, h->impl.i);
        if (key_val_type != ret)
        {
            (void)memcpy(ret, key_val_type, h->impl.hom->sizeof_type);
        }
        return h->impl.i;
    }
    return maybe_alloc_insert(h->impl.hom, key_val_type);
}

ccc_homap_handle *
ccc_hom_and_modify(ccc_homap_handle *const h, ccc_any_type_update_fn *const fn)
{
    if (!h)
    {
        return NULL;
    }
    if (fn && h->impl.stats & CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_any_type){
            .any_type = data_at(h->impl.hom, h->impl.i),
            .aux = NULL,
        });
    }
    return h;
}

ccc_homap_handle *
ccc_hom_and_modify_aux(ccc_homap_handle *const h,
                       ccc_any_type_update_fn *const fn, void *const aux)
{
    if (!h)
    {
        return NULL;
    }
    if (fn && h->impl.stats & CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_any_type){
            .any_type = data_at(h->impl.hom, h->impl.i),
            .aux = aux,
        });
    }
    return h;
}

ccc_handle_i
ccc_hom_or_insert(ccc_homap_handle const *const h,
                  void const *const key_val_type)
{
    if (!h || !key_val_type)
    {
        return 0;
    }
    if (h->impl.stats & CCC_ENTRY_OCCUPIED)
    {
        return h->impl.i;
    }
    return maybe_alloc_insert(h->impl.hom, key_val_type);
}

ccc_handle
ccc_hom_swap_handle(ccc_handle_ordered_map *const hom,
                    void *const key_val_output)
{
    if (!hom || !key_val_output)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    size_t const found = find(hom, key_in_slot(hom, key_val_output));
    if (found)
    {
        assert(hom->root);
        void *const ret = data_at(hom, hom->root);
        void *const tmp = data_at(hom, 0);
        swap(tmp, key_val_output, ret, hom->sizeof_type);
        return (ccc_handle){{
            .i = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted = maybe_alloc_insert(hom, key_val_output);
    if (!inserted)
    {
        return (ccc_handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_handle){{
        .i = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_handle
ccc_hom_try_insert(ccc_handle_ordered_map *const hom,
                   void const *const key_val_type)
{
    if (!hom || !key_val_type)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    size_t const found = find(hom, key_in_slot(hom, key_val_type));
    if (found)
    {
        assert(hom->root);
        return (ccc_handle){{
            .i = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted = maybe_alloc_insert(hom, key_val_type);
    if (!inserted)
    {
        return (ccc_handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_handle){{
        .i = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_handle
ccc_hom_insert_or_assign(ccc_handle_ordered_map *const hom,
                         void const *const key_val_type)
{
    if (!hom || !key_val_type)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    size_t const found = find(hom, key_in_slot(hom, key_val_type));
    if (found)
    {
        assert(hom->root);
        void *const f_base = data_at(hom, found);
        if (key_val_type != f_base)
        {
            memcpy(f_base, key_val_type, hom->sizeof_type);
        }
        return (ccc_handle){{
            .i = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted = maybe_alloc_insert(hom, key_val_type);
    if (!inserted)
    {
        return (ccc_handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_handle){{
        .i = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_handle
ccc_hom_remove(ccc_handle_ordered_map *const hom, void *const key_val_output)
{
    if (!hom || !key_val_output)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    size_t const removed = erase(hom, key_in_slot(hom, key_val_output));
    if (!removed)
    {
        return (ccc_handle){{
            .i = 0,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    assert(removed);
    void const *const r = data_at(hom, removed);
    if (key_val_output != r)
    {
        (void)memcpy(key_val_output, r, hom->sizeof_type);
    }
    return (ccc_handle){{
        .i = 0,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

ccc_handle
ccc_hom_remove_handle(ccc_homap_handle *const h)
{
    if (!h)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (h->impl.stats == CCC_ENTRY_OCCUPIED)
    {
        size_t const erased
            = erase(h->impl.hom, key_at(h->impl.hom, h->impl.i));
        assert(erased);
        return (ccc_handle){{
            .i = erased,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (ccc_handle){{
        .i = 0,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_handle_i
ccc_hom_unwrap(ccc_homap_handle const *const h)
{
    if (!h)
    {
        return 0;
    }
    return h->impl.stats == CCC_ENTRY_OCCUPIED ? h->impl.i : 0;
}

ccc_tribool
ccc_hom_insert_error(ccc_homap_handle const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (h->impl.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_hom_occupied(ccc_homap_handle const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (h->impl.stats & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_handle_status
ccc_hom_handle_status(ccc_homap_handle const *const h)
{
    return h ? h->impl.stats : CCC_ENTRY_ARG_ERROR;
}

ccc_tribool
ccc_hom_is_empty(ccc_handle_ordered_map const *const hom)
{
    if (!hom)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !ccc_hom_size(hom).count;
}

ccc_ucount
ccc_hom_size(ccc_handle_ordered_map const *const hom)
{
    if (!hom)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = hom->count ? hom->count - 1 : 0};
}

ccc_ucount
ccc_hom_capacity(ccc_handle_ordered_map const *const hom)
{
    if (!hom)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = hom->capacity};
}

void *
ccc_hom_begin(ccc_handle_ordered_map const *const hom)
{
    if (!hom || !hom->capacity)
    {
        return NULL;
    }
    size_t const n = min_max_from(hom, hom->root, L);
    return data_at(hom, n);
}

void *
ccc_hom_rbegin(ccc_handle_ordered_map const *const hom)
{
    if (!hom || !hom->capacity)
    {
        return NULL;
    }
    size_t const n = min_max_from(hom, hom->root, R);
    return data_at(hom, n);
}

void *
ccc_hom_next(ccc_handle_ordered_map const *const hom, void const *const e)
{
    if (!hom || !hom->capacity)
    {
        return NULL;
    }
    size_t const n = next(hom, index_of(hom, e), INORDER);
    return data_at(hom, n);
}

void *
ccc_hom_rnext(ccc_handle_ordered_map const *const hom, void const *const e)
{
    if (!hom || !e || !hom->capacity)
    {
        return NULL;
    }
    size_t const n = next(hom, index_of(hom, e), R_INORDER);
    return data_at(hom, n);
}

void *
ccc_hom_end(ccc_handle_ordered_map const *const hom)
{
    if (!hom || !hom->capacity)
    {
        return NULL;
    }
    return data_at(hom, 0);
}

void *
ccc_hom_rend(ccc_handle_ordered_map const *const hom)
{
    if (!hom || !hom->capacity)
    {
        return NULL;
    }
    return data_at(hom, 0);
}

void *
ccc_hom_data(ccc_handle_ordered_map const *const hom)
{
    return hom ? hom->data : NULL;
}

ccc_range
ccc_hom_equal_range(ccc_handle_ordered_map *const hom,
                    void const *const begin_key, void const *const end_key)
{
    if (!hom || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(hom, begin_key, end_key, INORDER)};
}

ccc_rrange
ccc_hom_equal_rrange(ccc_handle_ordered_map *const hom,
                     void const *const rbegin_key, void const *const rend_key)

{
    if (!hom || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){equal_range(hom, rbegin_key, rend_key, R_INORDER)};
}

ccc_result
ccc_hom_reserve(ccc_handle_ordered_map *const hom, size_t const to_add,
                ccc_any_alloc_fn *const fn)
{
    if (!hom || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Once initialized the buffer always has a size of one for root node. */
    size_t const needed = hom->count + to_add + (hom->count == 0);
    if (needed <= hom->capacity)
    {
        return CCC_RESULT_OK;
    }
    size_t const old_count = hom->count;
    size_t old_cap = hom->capacity;
    ccc_result const r = resize(hom, needed, fn);
    if (r != CCC_RESULT_OK)
    {
        return r;
    }
    if (!old_cap)
    {
        hom->count = 1;
    }
    old_cap = old_count ? old_cap : 0;
    size_t const new_cap = hom->capacity;
    size_t prev = 0;
    for (ptrdiff_t i = (ptrdiff_t)new_cap - 1; i > 0 && i >= (ptrdiff_t)old_cap;
         prev = i, --i)
    {
        node_at(hom, i)->next_free = prev;
    }
    if (!hom->free_list)
    {
        hom->free_list = prev;
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_hom_copy(ccc_handle_ordered_map *const dst,
             ccc_handle_ordered_map const *const src,
             ccc_any_alloc_fn *const fn)
{
    if (!dst || !src || src == dst || (dst->capacity < src->capacity && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    void *const dst_mem = dst->data;
    struct ccc_homap_elem *const dst_nodes = dst->nodes;
    size_t const dst_cap = dst->capacity;
    ccc_any_alloc_fn *const dst_alloc = dst->alloc;
    *dst = *src;
    dst->data = dst_mem;
    dst->nodes = dst_nodes;
    dst->capacity = dst_cap;
    dst->alloc = dst_alloc;
    if (!src->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (dst->capacity < src->capacity)
    {
        ccc_result const r = resize(dst, src->capacity, fn);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
    }
    else
    {
        /* Might not be necessary but not worth finding out. Do every time. */
        dst->nodes = node_pos(dst->sizeof_type, dst->data, dst->capacity);
    }
    if (!dst->data || !src->data)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    copy_soa(src, dst->data, dst->capacity);
    return CCC_RESULT_OK;
}

ccc_result
ccc_hom_clear(ccc_handle_ordered_map *const hom,
              ccc_any_type_destructor_fn *const fn)
{
    if (!hom)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!fn)
    {
        hom->root = 0;
        hom->count = 1;
        return CCC_RESULT_OK;
    }
    while (!ccc_hom_is_empty(hom))
    {
        size_t const i = remove_from_tree(hom, hom->root);
        assert(i);
        fn((ccc_any_type){
            .any_type = data_at(hom, i),
            .aux = hom->aux,
        });
    }
    hom->count = 1;
    hom->root = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_hom_clear_and_free(ccc_handle_ordered_map *const hom,
                       ccc_any_type_destructor_fn *const fn)
{
    if (!hom || !hom->alloc)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!fn)
    {
        hom->root = 0;
        hom->count = 0;
        hom->capacity = 0;
        (void)hom->alloc(hom->data, 0, hom->aux);
        return CCC_RESULT_OK;
    }
    while (!ccc_hom_is_empty(hom))
    {
        size_t const i = remove_from_tree(hom, hom->root);
        assert(i);
        fn((ccc_any_type){
            .any_type = data_at(hom, i),
            .aux = hom->aux,
        });
    }
    hom->root = 0;
    hom->capacity = 0;
    (void)hom->alloc(hom->data, 0, hom->aux);
    return CCC_RESULT_OK;
}

ccc_result
ccc_hom_clear_and_free_reserve(ccc_handle_ordered_map *const hom,
                               ccc_any_type_destructor_fn *const destructor,
                               ccc_any_alloc_fn *const alloc)
{
    if (!hom || !alloc)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        hom->root = 0;
        hom->count = 0;
        hom->capacity = 0;
        (void)alloc(hom->data, 0, hom->aux);
        return CCC_RESULT_OK;
    }
    while (!ccc_hom_is_empty(hom))
    {
        size_t const i = remove_from_tree(hom, hom->root);
        assert(i);
        destructor((ccc_any_type){
            .any_type = data_at(hom, i),
            .aux = hom->aux,
        });
    }
    hom->root = 0;
    hom->capacity = 0;
    (void)alloc(hom->data, 0, hom->aux);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_hom_validate(ccc_handle_ordered_map const *const hom)
{
    if (!hom)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(hom);
}

/*===========================   Private Interface ===========================*/

void
ccc_impl_hom_insert(struct ccc_homap *const hom, size_t const elem_i)
{
    insert(hom, elem_i);
}

struct ccc_htree_handle
ccc_impl_hom_handle(struct ccc_homap *const hom, void const *const key)
{
    return handle(hom, key);
}

void *
ccc_impl_hom_key_at(struct ccc_homap const *const hom, size_t const slot)
{
    return key_at(hom, slot);
}

void *
ccc_impl_hom_data_at(struct ccc_homap const *const hom, size_t const slot)
{
    return data_at(hom, slot);
}

size_t
ccc_impl_hom_alloc_slot(struct ccc_homap *const hom)
{
    return alloc_slot(hom);
}

/*===========================   Static Helpers    ===========================*/

static struct ccc_range_u
equal_range(struct ccc_homap *const t, void const *const begin_key,
            void const *const end_key, enum hom_branch const traversal)
{
    if (ccc_hom_is_empty(t))
    {
        return (struct ccc_range_u){};
    }
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    size_t b = splay(t, t->root, begin_key, t->cmp);
    if (cmp_elems(t, begin_key, b, t->cmp) == les_or_grt[traversal])
    {
        b = next(t, b, traversal);
    }
    size_t e = splay(t, t->root, end_key, t->cmp);
    if (cmp_elems(t, end_key, e, t->cmp) != les_or_grt[!traversal])
    {
        e = next(t, e, traversal);
    }
    return (struct ccc_range_u){
        .begin = data_at(t, b),
        .end = data_at(t, e),
    };
}

static struct ccc_htree_handle
handle(struct ccc_homap *const hom, void const *const key)
{
    size_t const found = find(hom, key);
    if (found)
    {
        return (struct ccc_htree_handle){
            .hom = hom,
            .i = found,
            .stats = CCC_ENTRY_OCCUPIED,
        };
    }
    return (struct ccc_htree_handle){
        .hom = hom,
        .i = 0,
        .stats = CCC_ENTRY_VACANT,
    };
}

static size_t
maybe_alloc_insert(struct ccc_homap *const hom, void const *const user_type)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const node = alloc_slot(hom);
    if (!node)
    {
        return 0;
    }
    (void)memcpy(data_at(hom, node), user_type, hom->sizeof_type);
    insert(hom, node);
    return node;
}

static size_t
alloc_slot(struct ccc_homap *const t)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const old_count = t->count;
    size_t old_cap = t->capacity;
    if (!old_count || old_count == old_cap)
    {
        assert(!t->free_list);
        if (old_count == old_cap)
        {
            if (resize(t, max(old_cap * 2, 8), t->alloc) != CCC_RESULT_OK)
            {
                return 0;
            }
        }
        else
        {
            t->nodes = node_pos(t->sizeof_type, t->data, t->capacity);
        }
        old_cap = old_count ? old_cap : 0;
        size_t const new_cap = t->capacity;
        size_t prev = 0;
        for (size_t i = new_cap - 1; i > 0 && i >= old_cap; prev = i, --i)
        {
            node_at(t, i)->next_free = prev;
        }
        t->free_list = prev;
        t->count = max(old_count, 1);
    }
    if (!t->free_list)
    {
        return 0;
    }
    ++t->count;
    size_t const slot = t->free_list;
    t->free_list = node_at(t, slot)->next_free;
    return slot;
}

static ccc_result
resize(struct ccc_homap *const hom, size_t const new_capacity,
       ccc_any_alloc_fn *const fn)
{
    if (hom->capacity && new_capacity <= hom->capacity - 1)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    void *const new_data
        = fn(NULL, total_bytes(hom->sizeof_type, new_capacity), hom->aux);
    if (!new_data)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    copy_soa(hom, new_data, new_capacity);
    hom->nodes = node_pos(hom->sizeof_type, new_data, new_capacity);
    fn(hom->data, 0, hom->aux);
    hom->data = new_data;
    hom->capacity = new_capacity;
    return CCC_RESULT_OK;
}

static void
insert(struct ccc_homap *const t, size_t const n)
{
    init_node(t, n);
    if (t->count == EMPTY_TREE)
    {
        t->root = n;
        return;
    }
    void const *const key = key_at(t, n);
    t->root = splay(t, t->root, key, t->cmp);
    ccc_threeway_cmp const root_cmp = cmp_elems(t, key, t->root, t->cmp);
    if (CCC_EQL == root_cmp)
    {
        return;
    }
    (void)connect_new_root(t, n, root_cmp);
}

static size_t
erase(struct ccc_homap *const t, void const *const key)
{
    if (ccc_hom_is_empty(t))
    {
        return 0;
    }
    size_t ret = splay(t, t->root, key, t->cmp);
    ccc_threeway_cmp const found = cmp_elems(t, key, ret, t->cmp);
    if (found != CCC_EQL)
    {
        return 0;
    }
    ret = remove_from_tree(t, ret);
    return ret;
}

static size_t
remove_from_tree(struct ccc_homap *const t, size_t const ret)
{
    if (!branch_i(t, ret, L))
    {
        t->root = branch_i(t, ret, R);
        link(t, 0, 0, t->root);
    }
    else
    {
        t->root = splay(t, branch_i(t, ret, L), key_at(t, ret), t->cmp);
        link(t, t->root, R, branch_i(t, ret, R));
    }
    node_at(t, ret)->next_free = t->free_list;
    t->free_list = ret;
    --t->count;
    return ret;
}

static size_t
connect_new_root(struct ccc_homap *const t, size_t const new_root,
                 ccc_threeway_cmp const cmp_result)
{
    enum hom_branch const dir = CCC_GRT == cmp_result;
    link(t, new_root, dir, branch_i(t, t->root, dir));
    link(t, new_root, !dir, t->root);
    *branch_ref(t, t->root, dir) = 0;
    t->root = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link(t, 0, 0, t->root);
    return new_root;
}

static size_t
find(struct ccc_homap *const t, void const *const key)
{
    if (!t->root)
    {
        return 0;
    }
    t->root = splay(t, t->root, key, t->cmp);
    return cmp_elems(t, key, t->root, t->cmp) == CCC_EQL ? t->root : 0;
}

static size_t
splay(struct ccc_homap *const t, size_t root, void const *const key,
      ccc_any_key_cmp_fn *const cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    struct ccc_homap_elem *const nil = node_at(t, 0);
    nil->branch[L] = nil->branch[R] = nil->parent = 0;
    size_t l_r_subtrees[LR] = {0, 0};
    do
    {
        ccc_threeway_cmp const root_cmp = cmp_elems(t, key, root, cmp_fn);
        enum hom_branch const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || !branch_i(t, root, dir))
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp_elems(t, key, branch_i(t, root, dir), cmp_fn);
        enum hom_branch const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            size_t const pivot = branch_i(t, root, dir);
            link(t, root, dir, branch_i(t, pivot, !dir));
            link(t, pivot, !dir, root);
            root = pivot;
            if (!branch_i(t, root, dir))
            {
                break;
            }
        }
        link(t, l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = branch_i(t, root, dir);
    }
    while (1);
    link(t, l_r_subtrees[L], R, branch_i(t, root, L));
    link(t, l_r_subtrees[R], L, branch_i(t, root, R));
    link(t, root, L, nil->branch[R]);
    link(t, root, R, nil->branch[L]);
    t->root = root;
    link(t, 0, 0, t->root);
    return root;
}

/** Links the parent node to node starting at subtree root via direction dir.
updates the parent of the child being picked up by the new parent as well. */
static inline void
link(struct ccc_homap *const t, size_t const parent, enum hom_branch const dir,
     size_t const subtree)
{
    *branch_ref(t, parent, dir) = subtree;
    *parent_ref(t, subtree) = parent;
}

static size_t
min_max_from(struct ccc_homap const *const t, size_t start,
             enum hom_branch const dir)
{
    if (!start)
    {
        return 0;
    }
    for (; branch_i(t, start, dir); start = branch_i(t, start, dir))
    {}
    return start;
}

static size_t
next(struct ccc_homap const *const t, size_t n, enum hom_branch const traversal)
{
    if (!n)
    {
        return 0;
    }
    assert(!parent_i(t, t->root));
    /* The node is an internal one that has a sub-tree to explore first. */
    if (branch_i(t, n, traversal))
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = branch_i(t, n, traversal); branch_i(t, n, !traversal);
             n = branch_i(t, n, !traversal))
        {}
        return n;
    }
    /* This is how to return internal nodes on the way back up from a leaf. */
    size_t p = parent_i(t, n);
    for (; p && branch_i(t, p, !traversal) != n; n = p, p = parent_i(t, p))
    {}
    return p;
}

static inline ccc_threeway_cmp
cmp_elems(struct ccc_homap const *const hom, void const *const key,
          size_t const node, ccc_any_key_cmp_fn *const fn)
{
    return fn((ccc_any_key_cmp){
        .any_key_lhs = key,
        .any_type_rhs = data_at(hom, node),
        .aux = hom->aux,
    });
}

static inline void
init_node(struct ccc_homap const *const hom, size_t const node)
{
    struct ccc_homap_elem *const e = node_at(hom, node);
    e->branch[L] = e->branch[R] = e->parent = 0;
}

/** Calculates the number of bytes needed for user data INCLUDING any bytes we
need to add to the end of the array such that the following nodes array starts
on an aligned byte boundary given the alignment requirements of a node. This
means the value returned from this function may or may not be slightly larger
then the raw size of just user elements if rounding up must occur. */
static inline size_t
data_bytes(size_t const sizeof_type, size_t const capacity)
{
    return ((sizeof_type * capacity) + alignof(*(struct ccc_homap){}.nodes) - 1)
         & ~(alignof(*(struct ccc_homap){}.nodes) - 1);
}

/** Calculates the number of bytes needed for the nodes array without any
consideration for end padding as no arrays follow. */
static inline size_t
node_bytes(size_t const capacity)
{
    return sizeof(*(struct ccc_homap){}.nodes) * capacity;
}

/** Calculates the number of bytes needed for all arrays in the Struct of Arrays
map design INCLUDING any extra padding bytes that need to be added between the
data and node arrays and the node and parity arrays. Padding might be needed if
the alignment of the type in next array that follows a preceding array is
different from the preceding array. In that case it is the preceding array's
responsibility to add padding bytes to its end such that the next array begins
on an aligned byte boundary for its own type. This means that the bytes returned
by this function may be greater than summing the (sizeof(type) * capacity) for
each array in the conceptual struct. */
static inline size_t
total_bytes(size_t sizeof_type, size_t const capacity)
{
    return data_bytes(sizeof_type, capacity) + node_bytes(capacity);
}

/** Returns the base of the node array relative to the data base pointer. This
positions is guaranteed to be the first aligned byte given the alignment of the
node type after the data array. The data array has added any necessary padding
after it to ensure that the base of the node array is aligned for its type. */
static inline struct ccc_homap_elem *
node_pos(size_t const sizeof_type, void const *const data,
         size_t const capacity)
{
    return (struct ccc_homap_elem *)((char *)data
                                     + data_bytes(sizeof_type, capacity));
}

/** Copies over the Struct of Arrays contained within the one contiguous
allocation of the map to the new memory provided. Assumes the new_data pointer
points to the base of an allocation that has been allocated with sufficient
bytes to support the user data, nodes, and parity arrays for the provided new
capacity. */
static inline void
copy_soa(struct ccc_homap const *const src, void *const dst_data_base,
         size_t const dst_capacity)
{
    if (!src->data)
    {
        return;
    }
    assert(dst_capacity >= src->capacity);
    size_t const sizeof_type = src->sizeof_type;
    /* Each section of the allocation "grows" when we re-size so one copy would
       not work. Instead each component is copied over allowing each to grow. */
    (void)memcpy(dst_data_base, src->data,
                 data_bytes(sizeof_type, src->capacity));
    (void)memcpy(node_pos(sizeof_type, dst_data_base, dst_capacity),
                 node_pos(sizeof_type, src->data, src->capacity),
                 node_bytes(src->capacity));
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const sizeof_type)
{
    if (a == b)
    {
        return;
    }
    (void)memcpy(tmp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, tmp, sizeof_type);
}

static inline struct ccc_homap_elem *
node_at(struct ccc_homap const *const t, size_t const i)
{
    return &t->nodes[i];
}

static inline void *
data_at(struct ccc_homap const *const t, size_t const i)
{
    return (char *)t->data + (i * t->sizeof_type);
}

static inline size_t
branch_i(struct ccc_homap const *const t, size_t const parent,
         enum hom_branch const dir)
{
    return node_at(t, parent)->branch[dir];
}

static inline size_t
parent_i(struct ccc_homap const *const t, size_t const child)
{
    return node_at(t, child)->parent;
}

static inline size_t
index_of(struct ccc_homap const *const t, void const *const key_val_type)
{
    assert(key_val_type >= t->data
           && (char *)key_val_type
                  < ((char *)t->data + (t->capacity * t->sizeof_type)));
    return ((char *)key_val_type - (char *)t->data) / t->sizeof_type;
}

static inline size_t *
branch_ref(struct ccc_homap const *const t, size_t const node,
           enum hom_branch const branch)
{
    return &node_at(t, node)->branch[branch];
}

static inline size_t *
parent_ref(struct ccc_homap const *const t, size_t const node)
{
    return &node_at(t, node)->parent;
}

static inline void *
key_at(struct ccc_homap const *const t, size_t const i)
{
    return (char *)data_at(t, i) + t->key_offset;
}

static void *
key_in_slot(struct ccc_homap const *t, void const *const user_struct)
{
    return (char *)user_struct + t->key_offset;
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

/** @private */
struct tree_range
{
    size_t low;
    size_t root;
    size_t high;
};

static size_t
recursive_size(struct ccc_homap const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_size(t, branch_i(t, r, R))
         + recursive_size(t, branch_i(t, r, L));
}

static ccc_tribool
are_subtrees_valid(struct ccc_homap const *t, struct tree_range const r)
{
    if (!r.root)
    {
        return CCC_TRUE;
    }
    if (r.low && cmp_elems(t, key_at(t, r.low), r.root, t->cmp) != CCC_LES)
    {
        return CCC_FALSE;
    }
    if (r.high && cmp_elems(t, key_at(t, r.high), r.root, t->cmp) != CCC_GRT)
    {
        return CCC_FALSE;
    }
    return are_subtrees_valid(
               t, (struct tree_range){.low = r.low,
                                      .root = branch_i(t, r.root, L),
                                      .high = r.root})
        && are_subtrees_valid(
               t, (struct tree_range){.low = r.root,
                                      .root = branch_i(t, r.root, R),
                                      .high = r.high});
}

static ccc_tribool
is_storing_parent(struct ccc_homap const *const t, size_t const p,
                  size_t const root)
{
    if (!root)
    {
        return CCC_TRUE;
    }
    if (parent_i(t, root) != p)
    {
        return CCC_FALSE;
    }
    return is_storing_parent(t, root, branch_i(t, root, L))
        && is_storing_parent(t, root, branch_i(t, root, R));
}

static ccc_tribool
is_free_list_valid(struct ccc_homap const *const t)
{
    if (!t->count)
    {
        return CCC_TRUE;
    }
    size_t list_check = 0;
    for (size_t cur = t->free_list; cur && list_check < t->capacity;
         cur = node_at(t, cur)->next_free, ++list_check)
    {}
    return (list_check + t->count == t->capacity);
}

static ccc_tribool
validate(struct ccc_homap const *const hom)
{
    if (!hom->count)
    {
        return CCC_TRUE;
    }
    if (!are_subtrees_valid(hom, (struct tree_range){.root = hom->root}))
    {
        return CCC_FALSE;
    }
    size_t const size = recursive_size(hom, hom->root);
    if (size && size != hom->count - 1)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(hom, 0, hom->root))
    {
        return CCC_FALSE;
    }
    if (!is_free_list_valid(hom))
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
