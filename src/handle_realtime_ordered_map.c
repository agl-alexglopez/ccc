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

This file contains my implementation of a handle realtime ordered map. The added
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
suggested by the authors of the original paper are implemented. Finally, the
data structure has been placed into a buffer with relative indices rather
than pointers. See the required license at the bottom of the file for
BSD-2-Clause compliance.

Overall a WAVL tree is quite impressive for it's simplicity and purported
improvements over AVL and Red-Black trees. The rank framework is intuitive
and flexible in how it can be implemented. */
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "handle_realtime_ordered_map.h"
#include "impl/impl_handle_realtime_ordered_map.h"
#include "impl/impl_types.h"
#include "types.h"

/*========================   Data Alignment Test   ==========================*/

enum : size_t
{
    /* @private Test capacity. */
    TCAP = 3,
};
/** @private Type used for a test exclusive to this translation unit. */
struct type_with_padding_data
{
    size_t s;
    uint8_t u;
};
ccc_hrm_declare_fixed_map(fixed_map_test_type, struct type_with_padding_data,
                          TCAP);
/** @private This is a static fixed size map exclusive to this translation unit
used to ensure assumptions about data layout are correct. The following static
asserts must be true in order to support the Struct of Array style layout we
use for the data, nodes, and parity arrays. */
static fixed_map_test_type data_nodes_parity_layout_test;
static_assert(
    (char *)&data_nodes_parity_layout_test.parity[ccc_impl_hrm_blocks(TCAP)]
            - (char *)&data_nodes_parity_layout_test.data[0]
        == ((sizeof(*data_nodes_parity_layout_test.data) * TCAP)
            + (sizeof(*data_nodes_parity_layout_test.nodes) * TCAP)
            + (sizeof(typeof(*data_nodes_parity_layout_test.parity))
               * (ccc_impl_hrm_blocks(TCAP)))),
    "The pointer difference in bytes between end of parity bit array and start "
    "of user data array must be the same as the total bytes we assume to be "
    "stored in that range.");
static_assert((char *)&data_nodes_parity_layout_test.data[TCAP]
                  == (char *)&data_nodes_parity_layout_test.nodes,
              "The start of the nodes array must begin at the next byte past "
              "the final user data element.");
static_assert((char *)&data_nodes_parity_layout_test.nodes
                  == ((char *)&data_nodes_parity_layout_test.data
                      + (sizeof(*data_nodes_parity_layout_test.data) * TCAP)),
              "Manual pointer arithmetic from the base of data array to find "
              "nodes array should result in correct location.");
static_assert(
    (char *)&data_nodes_parity_layout_test.nodes[TCAP]
        == (char *)&data_nodes_parity_layout_test.parity,
    "The start of the parity bit array must begin at the next byte past "
    "the final internal node element.");
static_assert((char *)&data_nodes_parity_layout_test.parity
                  == ((char *)&data_nodes_parity_layout_test.data
                      + (sizeof(*data_nodes_parity_layout_test.data) * TCAP)
                      + (sizeof(*data_nodes_parity_layout_test.nodes) * TCAP)),
              "Manual pointer arithmetic from the base of data array to find "
              "parity array should result in correct location.");
static_assert((char *)&data_nodes_parity_layout_test.parity
                  == ((char *)&data_nodes_parity_layout_test.nodes
                      + (sizeof(*data_nodes_parity_layout_test.nodes) * TCAP)),
              "Manual pointer arithmetic from the base of nodes array to find "
              "parity array should result in correct location.");

/*==========================  Type Declarations   ===========================*/

/** @private */
enum hrm_branch
{
    L = 0,
    R,
};

/** @private */
struct hrm_query
{
    ccc_threeway_cmp last_cmp;
    union
    {
        size_t found;
        size_t parent;
    };
};

#define INORDER R
#define R_INORDER L
#define MINDIR L
#define MAXDIR R

enum
{
    SINGLE_TREE_NODE = 2,
};

typedef typeof(*(struct ccc_hromap){}.parity) hrm_block;

enum : size_t
{
    HRM_BLOCK_BITS = sizeof(hrm_block) * CHAR_BIT,
};

/*==============================  Prototypes   ==============================*/

/* Returning the user struct type with stored offsets. */
static void insert(struct ccc_hromap *hrm, size_t parent_i,
                   ccc_threeway_cmp last_cmp, size_t elem_i);
static ccc_result resize(struct ccc_hromap *hrm, size_t new_capacity,
                         ccc_any_alloc_fn *fn);
static void copy_soa(struct ccc_hromap const *src, void *dst_data_base,
                     size_t dst_capacity);
static size_t data_bytes(size_t sizeof_type, size_t capacity);
static size_t node_bytes(size_t capacity);
static size_t parity_bytes(size_t capacity);
static struct ccc_hromap_elem *node_pos(size_t sizeof_type, void const *data,
                                        size_t capacity);
static hrm_block *parity_pos(size_t sizeof_type, void const *data,
                             size_t capacity);
static size_t maybe_alloc_insert(struct ccc_hromap *hrm, size_t parent,
                                 ccc_threeway_cmp last_cmp,
                                 void const *user_type);
static size_t remove_fixup(struct ccc_hromap *t, size_t remove);
static size_t alloc_slot(struct ccc_hromap *t);
/* Returning the user key with stored offsets. */
static void *key_at(struct ccc_hromap const *t, size_t i);
static void *key_in_slot(struct ccc_hromap const *t, void const *user_struct);
/* Returning the internal elem type with stored offsets. */
static struct ccc_hromap_elem *node_at(struct ccc_hromap const *, size_t);
static void *data_at(struct ccc_hromap const *, size_t);
/* Returning the internal query helper to aid in handle handling. */
static struct hrm_query find(struct ccc_hromap const *hrm, void const *key);
/* Returning the handle core to the Handle Interface. */
static inline struct ccc_hrtree_handle handle(struct ccc_hromap const *hrm,
                                              void const *key);
/* Returning a generic range that can be use for range or rrange. */
static struct ccc_range_u equal_range(struct ccc_hromap const *, void const *,
                                      void const *, enum hrm_branch);
/* Returning threeway comparison with user callback. */
static ccc_threeway_cmp cmp_elems(struct ccc_hromap const *hrm, void const *key,
                                  size_t node, ccc_any_key_cmp_fn *fn);
/* Returning read only indices for tree nodes. */
static size_t sibling_of(struct ccc_hromap const *t, size_t x);
static size_t next(struct ccc_hromap const *t, size_t n,
                   enum hrm_branch traversal);
static size_t min_max_from(struct ccc_hromap const *t, size_t start,
                           enum hrm_branch dir);
static size_t branch_i(struct ccc_hromap const *t, size_t parent,
                       enum hrm_branch dir);
static size_t parent_i(struct ccc_hromap const *t, size_t child);
static size_t index_of(struct ccc_hromap const *t, void const *key_val_type);
/* Returning references to index fields for tree nodes. */
static size_t *branch_r(struct ccc_hromap const *t, size_t node,
                        enum hrm_branch branch);
static size_t *parent_r(struct ccc_hromap const *t, size_t node);
/* Returning WAVL tree status. */
static ccc_tribool is_0_child(struct ccc_hromap const *, size_t p, size_t x);
static ccc_tribool is_1_child(struct ccc_hromap const *, size_t p, size_t x);
static ccc_tribool is_2_child(struct ccc_hromap const *, size_t p, size_t x);
static ccc_tribool is_3_child(struct ccc_hromap const *, size_t p, size_t x);
static ccc_tribool is_01_parent(struct ccc_hromap const *, size_t x, size_t p,
                                size_t y);
static ccc_tribool is_11_parent(struct ccc_hromap const *, size_t x, size_t p,
                                size_t y);
static ccc_tribool is_02_parent(struct ccc_hromap const *, size_t x, size_t p,
                                size_t y);
static ccc_tribool is_22_parent(struct ccc_hromap const *, size_t x, size_t p,
                                size_t y);
static ccc_tribool is_leaf(struct ccc_hromap const *t, size_t x);
static ccc_tribool parity(struct ccc_hromap const *t, size_t node);
static void set_parity(struct ccc_hromap const *t, size_t node,
                       ccc_tribool status);
static size_t total_bytes(size_t sizeof_type, size_t capacity);
static size_t block_count(size_t node_count);
static ccc_tribool validate(struct ccc_hromap const *hrm);
/* Returning void and maintaining the WAVL tree. */
static void init_node(struct ccc_hromap const *t, size_t node);
static void insert_fixup(struct ccc_hromap *t, size_t z, size_t x);
static void rebalance_3_child(struct ccc_hromap *t, size_t z, size_t x);
static void transplant(struct ccc_hromap *t, size_t remove, size_t replacement);
static void promote(struct ccc_hromap const *t, size_t x);
static void demote(struct ccc_hromap const *t, size_t x);
static void double_promote(struct ccc_hromap const *t, size_t x);
static void double_demote(struct ccc_hromap const *t, size_t x);

static void rotate(struct ccc_hromap *t, size_t z, size_t x, size_t y,
                   enum hrm_branch dir);
static void double_rotate(struct ccc_hromap *t, size_t z, size_t x, size_t y,
                          enum hrm_branch dir);
/* Returning void as miscellaneous helpers. */
static void swap(char tmp[const], void *a, void *b, size_t sizeof_type);
static size_t max(size_t, size_t);

/*==============================  Interface    ==============================*/

void *
ccc_hrm_at(ccc_handle_realtime_ordered_map const *const h, ccc_handle_i const i)
{
    if (!h || !i)
    {
        return NULL;
    }
    return data_at(h, i);
}

ccc_tribool
ccc_hrm_contains(ccc_handle_realtime_ordered_map const *const hrm,
                 void const *const key)
{
    if (!hrm || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return CCC_EQL == find(hrm, key).last_cmp;
}

ccc_handle_i
ccc_hrm_get_key_val(ccc_handle_realtime_ordered_map const *const hrm,
                    void const *const key)
{
    if (!hrm || !key)
    {
        return 0;
    }
    struct hrm_query const q = find(hrm, key);
    return (CCC_EQL == q.last_cmp) ? q.found : 0;
}

ccc_handle
ccc_hrm_swap_handle(ccc_handle_realtime_ordered_map *const hrm,
                    void *const key_val_type_output)
{
    if (!hrm || !key_val_type_output)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct hrm_query const q = find(hrm, key_in_slot(hrm, key_val_type_output));
    if (CCC_EQL == q.last_cmp)
    {
        void *const slot = data_at(hrm, q.found);
        void *const tmp = data_at(hrm, 0);
        swap(tmp, key_val_type_output, slot, hrm->sizeof_type);
        return (ccc_handle){{
            .i = q.found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const i
        = maybe_alloc_insert(hrm, q.parent, q.last_cmp, key_val_type_output);
    if (!i)
    {
        return (ccc_handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_handle){{
        .i = i,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_handle
ccc_hrm_try_insert(ccc_handle_realtime_ordered_map *const hrm,
                   void const *const key_val_type)
{
    if (!hrm || !key_val_type)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct hrm_query const q = find(hrm, key_in_slot(hrm, key_val_type));
    if (CCC_EQL == q.last_cmp)
    {
        return (ccc_handle){{
            .i = q.found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const i
        = maybe_alloc_insert(hrm, q.parent, q.last_cmp, key_val_type);
    if (!i)
    {
        return (ccc_handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_handle){{
        .i = i,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_handle
ccc_hrm_insert_or_assign(ccc_handle_realtime_ordered_map *const hrm,
                         void const *const key_val_type)
{
    if (!hrm || !key_val_type)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct hrm_query const q = find(hrm, key_in_slot(hrm, key_val_type));
    if (CCC_EQL == q.last_cmp)
    {
        void *const found = data_at(hrm, q.found);
        (void)memcpy(found, key_val_type, hrm->sizeof_type);
        return (ccc_handle){{
            .i = q.found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const i
        = maybe_alloc_insert(hrm, q.parent, q.last_cmp, key_val_type);
    if (!i)
    {
        return (ccc_handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (ccc_handle){{
        .i = i,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_hromap_handle *
ccc_hrm_and_modify(ccc_hromap_handle *const h, ccc_any_type_update_fn *const fn)
{
    if (h && fn && h->impl.handle.stats & CCC_ENTRY_OCCUPIED
        && h->impl.handle.i > 0)
    {
        fn((ccc_any_type){
            .any_type = data_at(h->impl.hrm, h->impl.handle.i),
            NULL,
        });
    }
    return h;
}

ccc_hromap_handle *
ccc_hrm_and_modify_aux(ccc_hromap_handle *const h,
                       ccc_any_type_update_fn *const fn, void *const aux)
{
    if (h && fn && h->impl.handle.stats & CCC_ENTRY_OCCUPIED
        && h->impl.handle.stats > 0)
    {
        fn((ccc_any_type){
            .any_type = data_at(h->impl.hrm, h->impl.handle.i),
            aux,
        });
    }
    return h;
}

ccc_handle_i
ccc_hrm_or_insert(ccc_hromap_handle const *const h,
                  void const *const key_val_type)
{
    if (!h || !key_val_type)
    {
        return 0;
    }
    if (h->impl.handle.stats == CCC_ENTRY_OCCUPIED)
    {
        return h->impl.handle.i;
    }
    return maybe_alloc_insert(h->impl.hrm, h->impl.handle.i, h->impl.last_cmp,
                              key_val_type);
}

ccc_handle_i
ccc_hrm_insert_handle(ccc_hromap_handle const *const h,
                      void const *const key_val_type)
{
    if (!h || !key_val_type)
    {
        return 0;
    }
    if (h->impl.handle.stats == CCC_ENTRY_OCCUPIED)
    {
        void *const slot = data_at(h->impl.hrm, h->impl.handle.i);
        if (slot != key_val_type)
        {
            (void)memcpy(slot, key_val_type, h->impl.hrm->sizeof_type);
        }
        return h->impl.handle.i;
    }
    return maybe_alloc_insert(h->impl.hrm, h->impl.handle.i, h->impl.last_cmp,
                              key_val_type);
}

ccc_hromap_handle
ccc_hrm_handle(ccc_handle_realtime_ordered_map const *const hrm,
               void const *const key)
{
    if (!hrm || !key)
    {
        return (ccc_hromap_handle){{.handle = {.stats = CCC_ENTRY_ARG_ERROR}}};
    }
    return (ccc_hromap_handle){handle(hrm, key)};
}

ccc_handle
ccc_hrm_remove_handle(ccc_hromap_handle const *const h)
{
    if (!h)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (h->impl.handle.stats == CCC_ENTRY_OCCUPIED)
    {
        size_t const ret = remove_fixup(h->impl.hrm, h->impl.handle.i);
        return (ccc_handle){{
            .i = ret,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (ccc_handle){{
        .i = 0,
        .stats = CCC_ENTRY_VACANT,
    }};
}

ccc_handle
ccc_hrm_remove(ccc_handle_realtime_ordered_map *const hrm,
               void *const key_val_type_output)
{
    if (!hrm || !key_val_type_output)
    {
        return (ccc_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct hrm_query const q = find(hrm, key_in_slot(hrm, key_val_type_output));
    if (q.last_cmp != CCC_EQL)
    {
        return (ccc_handle){{
            .i = 0,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    size_t const removed = remove_fixup(hrm, q.found);
    assert(removed);
    void const *const r = data_at(hrm, removed);
    if (key_val_type_output != r)
    {
        (void)memcpy(key_val_type_output, r, hrm->sizeof_type);
    }
    return (ccc_handle){{
        .i = 0,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

ccc_range
ccc_hrm_equal_range(ccc_handle_realtime_ordered_map const *const hrm,
                    void const *const begin_key, void const *const end_key)
{
    if (!hrm || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(hrm, begin_key, end_key, INORDER)};
}

ccc_rrange
ccc_hrm_equal_rrange(ccc_handle_realtime_ordered_map const *const hrm,
                     void const *const rbegin_key, void const *const rend_key)
{
    if (!hrm || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){equal_range(hrm, rbegin_key, rend_key, R_INORDER)};
}

ccc_handle_i
ccc_hrm_unwrap(ccc_hromap_handle const *const h)
{
    if (h && h->impl.handle.stats & CCC_ENTRY_OCCUPIED && h->impl.handle.i > 0)
    {
        return h->impl.handle.i;
    }
    return 0;
}

ccc_tribool
ccc_hrm_insert_error(ccc_hromap_handle const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (h->impl.handle.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_hrm_occupied(ccc_hromap_handle const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (h->impl.handle.stats & CCC_ENTRY_OCCUPIED) != 0;
}

ccc_handle_status
ccc_hrm_handle_status(ccc_hromap_handle const *const h)
{
    return h ? h->impl.handle.stats : CCC_ENTRY_ARG_ERROR;
}

ccc_tribool
ccc_hrm_is_empty(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !ccc_hrm_size(hrm).count;
}

ccc_ucount
ccc_hrm_size(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    if (!hrm->count)
    {
        return (ccc_ucount){.count = 0};
    }
    /* The root slot is occupied at 0 but don't don't tell user. */
    return (ccc_ucount){.count = hrm->count - 1};
}

ccc_ucount
ccc_hrm_capacity(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm)
    {
        return (ccc_ucount){.error = CCC_RESULT_ARG_ERROR};
    }
    return (ccc_ucount){.count = hrm->capacity};
}

void *
ccc_hrm_data(ccc_handle_realtime_ordered_map const *const hrm)
{
    return hrm->data;
}

void *
ccc_hrm_begin(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm || !hrm->count)
    {
        return NULL;
    }
    size_t const n = min_max_from(hrm, hrm->root, MINDIR);
    return data_at(hrm, n);
}

void *
ccc_hrm_rbegin(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm || !hrm->count)
    {
        return NULL;
    }
    size_t const n = min_max_from(hrm, hrm->root, MAXDIR);
    return data_at(hrm, n);
}

void *
ccc_hrm_next(ccc_handle_realtime_ordered_map const *const hrm,
             void const *const key_val_type_iter)
{
    if (!hrm || !key_val_type_iter || !hrm->count)
    {
        return NULL;
    }
    size_t const n = next(hrm, index_of(hrm, key_val_type_iter), INORDER);
    return data_at(hrm, n);
}

void *
ccc_hrm_rnext(ccc_handle_realtime_ordered_map const *const hrm,
              void const *const key_val_type_iter)
{
    if (!hrm || !key_val_type_iter || !hrm->count)
    {
        return NULL;
    }
    size_t const n = next(hrm, index_of(hrm, key_val_type_iter), R_INORDER);
    return data_at(hrm, n);
}

void *
ccc_hrm_end(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm || !hrm->count)
    {
        return NULL;
    }
    return data_at(hrm, 0);
}

void *
ccc_hrm_rend(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm || !hrm->count)
    {
        return NULL;
    }
    return data_at(hrm, 0);
}

ccc_result
ccc_hrm_reserve(ccc_handle_realtime_ordered_map *const hrm, size_t const to_add,
                ccc_any_alloc_fn *const fn)
{
    if (!hrm || !fn)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    /* Once initialized the buffer always has a size of one for root node. */
    size_t const needed = hrm->count + to_add + (hrm->count == 0);
    if (needed <= hrm->capacity)
    {
        return CCC_RESULT_OK;
    }
    size_t const old_count = hrm->count;
    size_t old_cap = hrm->capacity;
    ccc_result const r = resize(hrm, needed, fn);
    if (r != CCC_RESULT_OK)
    {
        return r;
    }
    set_parity(hrm, 0, CCC_TRUE);
    if (!old_cap)
    {
        hrm->count = 1;
    }
    old_cap = old_count ? old_cap : 0;
    size_t const new_cap = hrm->capacity;
    size_t prev = 0;
    for (ptrdiff_t i = (ptrdiff_t)new_cap - 1; i > 0 && i >= (ptrdiff_t)old_cap;
         prev = i, --i)
    {
        node_at(hrm, i)->next_free = prev;
    }
    if (!hrm->free_list)
    {
        hrm->free_list = prev;
    }
    return CCC_RESULT_OK;
}

ccc_result
ccc_hrm_copy(ccc_handle_realtime_ordered_map *const dst,
             ccc_handle_realtime_ordered_map const *const src,
             ccc_any_alloc_fn *const fn)
{
    if (!dst || !src || src == dst || (dst->capacity < src->capacity && !fn))
    {
        return CCC_RESULT_ARG_ERROR;
    }
    void *const dst_mem = dst->data;
    struct ccc_hromap_elem *const dst_nodes = dst->nodes;
    hrm_block *const dst_parity = dst->parity;
    size_t const dst_cap = dst->capacity;
    ccc_any_alloc_fn *const dst_alloc = dst->alloc;
    *dst = *src;
    dst->data = dst_mem;
    dst->nodes = dst_nodes;
    dst->parity = dst_parity;
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
        dst->parity = parity_pos(dst->sizeof_type, dst->data, dst->capacity);
    }
    if (!dst->data || !src->data)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    copy_soa(src, dst->data, dst->capacity);
    return CCC_RESULT_OK;
}

ccc_result
ccc_hrm_clear(ccc_handle_realtime_ordered_map *const hrm,
              ccc_any_type_destructor_fn *const fn)
{
    if (!hrm)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!fn)
    {
        hrm->root = 0;
        hrm->count = 1;
        return CCC_RESULT_OK;
    }
    while (!ccc_hrm_is_empty(hrm))
    {
        size_t const i = remove_fixup(hrm, hrm->root);
        assert(i);
        fn((ccc_any_type){
            .any_type = data_at(hrm, i),
            .aux = hrm->aux,
        });
    }
    hrm->count = 1;
    hrm->root = 0;
    return CCC_RESULT_OK;
}

ccc_result
ccc_hrm_clear_and_free(ccc_handle_realtime_ordered_map *const hrm,
                       ccc_any_type_destructor_fn *const fn)
{
    if (!hrm || !hrm->alloc)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!fn)
    {
        hrm->root = 0;
        hrm->count = 0;
        hrm->capacity = 0;
        (void)hrm->alloc(hrm->data, 0, hrm->aux);
        return CCC_RESULT_OK;
    }
    while (!ccc_hrm_is_empty(hrm))
    {
        size_t const i = remove_fixup(hrm, hrm->root);
        assert(i);
        fn((ccc_any_type){
            .any_type = data_at(hrm, i),
            .aux = hrm->aux,
        });
    }
    hrm->root = 0;
    hrm->capacity = 0;
    (void)hrm->alloc(hrm->data, 0, hrm->aux);
    return CCC_RESULT_OK;
}

ccc_result
ccc_hrm_clear_and_free_reserve(ccc_handle_realtime_ordered_map *const hrm,
                               ccc_any_type_destructor_fn *const destructor,
                               ccc_any_alloc_fn *const alloc)
{
    if (!hrm || !alloc)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (!destructor)
    {
        hrm->root = 0;
        hrm->count = 0;
        hrm->capacity = 0;
        (void)alloc(hrm->data, 0, hrm->aux);
        return CCC_RESULT_OK;
    }
    while (!ccc_hrm_is_empty(hrm))
    {
        size_t const i = remove_fixup(hrm, hrm->root);
        assert(i);
        destructor((ccc_any_type){
            .any_type = data_at(hrm, i),
            .aux = hrm->aux,
        });
    }
    hrm->root = 0;
    hrm->capacity = 0;
    (void)alloc(hrm->data, 0, hrm->aux);
    return CCC_RESULT_OK;
}

ccc_tribool
ccc_hrm_validate(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(hrm);
}

/*========================  Private Interface  ==============================*/

void
ccc_impl_hrm_insert(struct ccc_hromap *const hrm, size_t const parent_i,
                    ccc_threeway_cmp const last_cmp, size_t const elem_i)
{
    insert(hrm, parent_i, last_cmp, elem_i);
}

struct ccc_hrtree_handle
ccc_impl_hrm_handle(struct ccc_hromap const *const hrm, void const *const key)
{
    return handle(hrm, key);
}

void *
ccc_impl_hrm_data_at(struct ccc_hromap const *const hrm, size_t const slot)
{
    return data_at(hrm, slot);
}

void *
ccc_impl_hrm_key_at(struct ccc_hromap const *const hrm, size_t const slot)
{
    return key_at(hrm, slot);
}

struct ccc_hromap_elem *
ccc_impl_hrm_elem_at(struct ccc_hromap const *hrm, size_t const i)
{
    return node_at(hrm, i);
}

size_t
ccc_impl_hrm_alloc_slot(struct ccc_hromap *const hrm)
{
    return alloc_slot(hrm);
}

/*==========================  Static Helpers   ==============================*/

static size_t
maybe_alloc_insert(struct ccc_hromap *const hrm, size_t const parent,
                   ccc_threeway_cmp const last_cmp, void const *const user_type)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const node = alloc_slot(hrm);
    if (!node)
    {
        return 0;
    }
    (void)memcpy(data_at(hrm, node), user_type, hrm->sizeof_type);
    insert(hrm, parent, last_cmp, node);
    return node;
}

static size_t
alloc_slot(struct ccc_hromap *const t)
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
            if (resize(t, old_cap ? old_cap * 2 : 8, t->alloc) != CCC_RESULT_OK)
            {
                return 0;
            }
        }
        else
        {
            t->nodes = node_pos(t->sizeof_type, t->data, t->capacity);
            t->parity = parity_pos(t->sizeof_type, t->data, t->capacity);
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
        set_parity(t, 0, CCC_TRUE);
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
resize(struct ccc_hromap *const hrm, size_t const new_capacity,
       ccc_any_alloc_fn *const fn)
{
    if (hrm->capacity && new_capacity <= hrm->capacity - 1)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOC;
    }
    void *const new_data
        = fn(NULL, total_bytes(hrm->sizeof_type, new_capacity), hrm->aux);
    if (!new_data)
    {
        return CCC_RESULT_MEM_ERROR;
    }
    copy_soa(hrm, new_data, new_capacity);
    hrm->nodes = node_pos(hrm->sizeof_type, new_data, new_capacity);
    hrm->parity = parity_pos(hrm->sizeof_type, new_data, new_capacity);
    fn(hrm->data, 0, hrm->aux);
    hrm->data = new_data;
    hrm->capacity = new_capacity;
    return CCC_RESULT_OK;
}

static void
insert(struct ccc_hromap *const hrm, size_t const parent_i,
       ccc_threeway_cmp const last_cmp, size_t const elem_i)
{
    struct ccc_hromap_elem *elem = node_at(hrm, elem_i);
    init_node(hrm, elem_i);
    if (hrm->count == SINGLE_TREE_NODE)
    {
        hrm->root = elem_i;
        return;
    }
    assert(last_cmp == CCC_GRT || last_cmp == CCC_LES);
    struct ccc_hromap_elem *parent = node_at(hrm, parent_i);
    ccc_tribool const rank_rule_break
        = !parent->branch[L] && !parent->branch[R];
    parent->branch[CCC_GRT == last_cmp] = elem_i;
    elem->parent = parent_i;
    if (rank_rule_break)
    {
        insert_fixup(hrm, parent_i, elem_i);
    }
}

static struct ccc_hrtree_handle
handle(struct ccc_hromap const *const hrm, void const *const key)
{
    struct hrm_query const q = find(hrm, key);
    if (CCC_EQL == q.last_cmp)
    {
        return (struct ccc_hrtree_handle){
            .hrm = (struct ccc_hromap *)hrm,
            .last_cmp = q.last_cmp,
            .handle = {.i = q.found, .stats = CCC_ENTRY_OCCUPIED},
        };
    }
    return (struct ccc_hrtree_handle){
        .hrm = (struct ccc_hromap *)hrm,
        .last_cmp = q.last_cmp,
        .handle
        = {.i = q.parent, .stats = CCC_ENTRY_NO_UNWRAP | CCC_ENTRY_VACANT},
    };
}

static struct hrm_query
find(struct ccc_hromap const *const hrm, void const *const key)
{
    size_t parent = 0;
    struct hrm_query q = {.last_cmp = CCC_CMP_ERROR, .found = hrm->root};
    while (q.found)
    {
        q.last_cmp = cmp_elems(hrm, key, q.found, hrm->cmp);
        if (CCC_EQL == q.last_cmp)
        {
            return q;
        }
        parent = q.found;
        q.found = branch_i(hrm, q.found, CCC_GRT == q.last_cmp);
    }
    /* Type punning here OK as both union members have same type and size. */
    q.parent = parent;
    return q;
}

static size_t
next(struct ccc_hromap const *const t, size_t n,
     enum hrm_branch const traversal)
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

static struct ccc_range_u
equal_range(struct ccc_hromap const *const t, void const *const begin_key,
            void const *const end_key, enum hrm_branch const traversal)
{
    if (ccc_hrm_is_empty(t))
    {
        return (struct ccc_range_u){};
    }
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    struct hrm_query b = find(t, begin_key);
    if (b.last_cmp == les_or_grt[traversal])
    {
        b.found = next(t, b.found, traversal);
    }
    struct hrm_query e = find(t, end_key);
    if (e.last_cmp != les_or_grt[!traversal])
    {
        e.found = next(t, e.found, traversal);
    }
    return (struct ccc_range_u){
        .begin = data_at(t, b.found),
        .end = data_at(t, e.found),
    };
}

static size_t
min_max_from(struct ccc_hromap const *const t, size_t start,
             enum hrm_branch const dir)
{
    if (!start)
    {
        return 0;
    }
    for (; branch_i(t, start, dir); start = branch_i(t, start, dir))
    {}
    return start;
}

static inline ccc_threeway_cmp
cmp_elems(struct ccc_hromap const *const hrm, void const *const key,
          size_t const node, ccc_any_key_cmp_fn *const fn)
{
    return fn((ccc_any_key_cmp){
        .any_key_lhs = key,
        .any_type_rhs = data_at(hrm, node),
        .aux = hrm->aux,
    });
}

static inline size_t
data_bytes(size_t const sizeof_type, size_t const capacity)
{
    return sizeof_type * capacity;
}

static inline size_t
node_bytes(size_t const capacity)
{
    return sizeof(typeof(*(struct ccc_hromap){}.nodes)) * capacity;
}

static inline size_t
parity_bytes(size_t capacity)
{
    return sizeof(hrm_block) * block_count(capacity);
}

static inline struct ccc_hromap_elem *
node_pos(size_t const sizeof_type, void const *const data,
         size_t const capacity)
{
    return (struct ccc_hromap_elem *)((char *)data
                                      + data_bytes(sizeof_type, capacity));
}

static inline hrm_block *
parity_pos(size_t const sizeof_type, void const *const data,
           size_t const capacity)
{
    return (hrm_block *)((char *)data + data_bytes(sizeof_type, capacity)
                         + node_bytes(capacity));
}

/** Copies over the Struct of Arrays contained within the one contiguous
allocation of the map to the new memory provided. Assumes the new_data pointer
points to the base of an allocation that has been allocated with sufficient
bytes to support the user data, nodes, and parity arrays for the provided new
capacity. */
static inline void
copy_soa(struct ccc_hromap const *const src, void *const dst_data_base,
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
    (void)memcpy(parity_pos(sizeof_type, dst_data_base, dst_capacity),
                 parity_pos(sizeof_type, src->data, src->capacity),
                 parity_bytes(src->capacity));
}

static inline void
init_node(struct ccc_hromap const *const t, size_t const node)
{
    set_parity(t, node, CCC_FALSE);
    struct ccc_hromap_elem *const e = node_at(t, node);
    e->branch[L] = e->branch[R] = e->parent = 0;
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

static inline struct ccc_hromap_elem *
node_at(struct ccc_hromap const *const t, size_t const i)
{
    return &t->nodes[i];
}

static inline void *
data_at(struct ccc_hromap const *const t, size_t const i)
{
    return (char *)t->data + (t->sizeof_type * i);
}

static inline hrm_block *
block_at(struct ccc_hromap const *const t, size_t const i)
{
    return &t->parity[i / HRM_BLOCK_BITS];
}

static inline hrm_block
bit_on(size_t const i)
{
    return ((hrm_block)1) << (i % HRM_BLOCK_BITS);
}

static inline size_t
branch_i(struct ccc_hromap const *const t, size_t const parent,
         enum hrm_branch const dir)
{
    return node_at(t, parent)->branch[dir];
}

static inline size_t
parent_i(struct ccc_hromap const *const t, size_t const child)
{
    return node_at(t, child)->parent;
}

static inline size_t
index_of(struct ccc_hromap const *const t, void const *const key_val_type)
{
    assert(key_val_type >= t->data
           && (char *)key_val_type
                  < ((char *)t->data + (t->capacity * t->sizeof_type)));
    return ((char *)key_val_type - (char *)t->data) / t->sizeof_type;
}

static inline ccc_tribool
parity(struct ccc_hromap const *const t, size_t const node)
{
    return (*block_at(t, node) & bit_on(node)) != 0;
}

static inline void
set_parity(struct ccc_hromap const *const t, size_t const node,
           ccc_tribool const status)
{
    if (status)
    {
        *block_at(t, node) |= bit_on(node);
    }
    else
    {
        *block_at(t, node) &= ~bit_on(node);
    }
}

static inline size_t
block_count(size_t const node_count)
{
    return (node_count + (HRM_BLOCK_BITS - 1)) / HRM_BLOCK_BITS;
}

static inline size_t
total_bytes(size_t sizeof_type, size_t const capacity)
{
    return (capacity * sizeof_type)
         + (capacity * sizeof(struct ccc_hromap_elem))
         + (block_count(capacity) * sizeof(hrm_block));
}

static inline size_t *
branch_r(struct ccc_hromap const *t, size_t const node,
         enum hrm_branch const branch)
{
    return &node_at(t, node)->branch[branch];
}

static inline size_t *
parent_r(struct ccc_hromap const *t, size_t const node)
{

    return &node_at(t, node)->parent;
}

static inline void *
key_at(struct ccc_hromap const *const t, size_t const i)
{
    return (char *)data_at(t, i) + t->key_offset;
}

static void *
key_in_slot(struct ccc_hromap const *t, void const *const user_struct)
{
    return (char *)user_struct + t->key_offset;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct ccc_hromap *const t, size_t z, size_t x)
{
    do
    {
        promote(t, z);
        x = z;
        z = parent_i(t, z);
        if (!z)
        {
            return;
        }
    }
    while (is_01_parent(t, x, z, sibling_of(t, x)));

    if (!is_02_parent(t, x, z, sibling_of(t, x)))
    {
        return;
    }
    assert(x);
    assert(is_0_child(t, z, x));
    enum hrm_branch const p_to_x_dir = branch_i(t, z, R) == x;
    size_t const y = branch_i(t, x, !p_to_x_dir);
    if (!y || is_2_child(t, z, y))
    {
        rotate(t, z, x, y, !p_to_x_dir);
        demote(t, z);
    }
    else
    {
        assert(is_1_child(t, z, y));
        double_rotate(t, z, x, y, p_to_x_dir);
        promote(t, y);
        demote(t, x);
        demote(t, z);
    }
}

static size_t
remove_fixup(struct ccc_hromap *const t, size_t const remove)
{
    size_t y = 0;
    size_t x = 0;
    size_t p = 0;
    ccc_tribool two_child = CCC_FALSE;
    if (!branch_i(t, remove, R) || !branch_i(t, remove, L))
    {
        y = remove;
        p = parent_i(t, y);
        x = branch_i(t, y, !branch_i(t, y, L));
        *parent_r(t, x) = parent_i(t, y);
        if (!p)
        {
            t->root = x;
        }
        two_child = is_2_child(t, p, y);
        *branch_r(t, p, branch_i(t, p, R) == y) = x;
    }
    else
    {
        y = min_max_from(t, branch_i(t, remove, R), MINDIR);
        p = parent_i(t, y);
        x = branch_i(t, y, !branch_i(t, y, L));
        *parent_r(t, x) = parent_i(t, y);

        /* Save if check and improve readability by assuming this is true. */
        assert(p);

        two_child = is_2_child(t, p, y);
        *branch_r(t, p, branch_i(t, p, R) == y) = x;
        transplant(t, remove, y);
        if (remove == p)
        {
            p = y;
        }
    }

    if (p)
    {
        if (two_child)
        {
            assert(p);
            rebalance_3_child(t, p, x);
        }
        else if (!x && branch_i(t, p, L) == branch_i(t, p, R))
        {
            assert(p);
            ccc_tribool const demote_makes_3_child
                = is_2_child(t, parent_i(t, p), p);
            demote(t, p);
            if (demote_makes_3_child)
            {
                rebalance_3_child(t, parent_i(t, p), p);
            }
        }
        assert(!is_leaf(t, p) || !parity(t, p));
    }
    node_at(t, remove)->next_free = t->free_list;
    t->free_list = remove;
    --t->count;
    return remove;
}

static void
transplant(struct ccc_hromap *const t, size_t const remove,
           size_t const replacement)
{
    assert(remove);
    assert(replacement);
    *parent_r(t, replacement) = parent_i(t, remove);
    if (!parent_i(t, remove))
    {
        t->root = replacement;
    }
    else
    {
        size_t const p = parent_i(t, remove);
        *branch_r(t, p, branch_i(t, p, R) == remove) = replacement;
    }
    struct ccc_hromap_elem *const remove_r = node_at(t, remove);
    struct ccc_hromap_elem *const replace_r = node_at(t, replacement);
    *parent_r(t, remove_r->branch[R]) = replacement;
    *parent_r(t, remove_r->branch[L]) = replacement;
    replace_r->branch[R] = remove_r->branch[R];
    replace_r->branch[L] = remove_r->branch[L];
    set_parity(t, replacement, parity(t, remove));
}

static void
rebalance_3_child(struct ccc_hromap *const t, size_t z, size_t x)
{
    assert(z);
    ccc_tribool made_3_child = CCC_FALSE;
    do
    {
        size_t const g = parent_i(t, z);
        size_t const y = branch_i(t, z, branch_i(t, z, L) == x);
        made_3_child = is_2_child(t, g, z);
        if (is_2_child(t, z, y))
        {
            demote(t, z);
        }
        else if (is_22_parent(t, branch_i(t, y, L), y, branch_i(t, y, R)))
        {
            demote(t, z);
            demote(t, y);
        }
        else /* p(x) is 1,3, y is not a 2,2 parent, and x is 3-child.*/
        {
            assert(is_3_child(t, z, x));
            enum hrm_branch const z_to_x_dir = branch_i(t, z, R) == x;
            size_t const w = branch_i(t, y, !z_to_x_dir);
            if (is_1_child(t, y, w))
            {
                rotate(t, z, y, branch_i(t, y, z_to_x_dir), z_to_x_dir);
                promote(t, y);
                demote(t, z);
                if (is_leaf(t, z))
                {
                    demote(t, z);
                }
            }
            else /* w is a 2-child and v will be a 1-child. */
            {
                size_t const v = branch_i(t, y, z_to_x_dir);
                assert(is_2_child(t, y, w));
                assert(is_1_child(t, y, v));
                double_rotate(t, z, y, v, !z_to_x_dir);
                double_promote(t, v);
                demote(t, y);
                double_demote(t, z);
                /* Optional "Rebalancing with Promotion," defined as follows:
                       if node z is a non-leaf 1,1 node, we promote it;
                       otherwise, if y is a non-leaf 1,1 node, we promote it.
                       (See Figure 4.) (Haeupler et. al. 2014, 17).
                   This reduces constants in some of theorems mentioned in the
                   paper but may not be worth doing. Rotations stay at 2 worst
                   case. Should revisit after more performance testing. */
                if (!is_leaf(t, z)
                    && is_11_parent(t, branch_i(t, z, L), z, branch_i(t, z, R)))
                {
                    promote(t, z);
                }
                else if (!is_leaf(t, y)
                         && is_11_parent(t, branch_i(t, y, L), y,
                                         branch_i(t, y, R)))
                {
                    promote(t, y);
                }
            }
            return;
        }
        x = z;
        z = g;
    }
    while (z && made_3_child);
}

/** A single rotation is symmetric. Here is the right case. Lowercase are nodes
and uppercase are arbitrary subtrees.
        z            x
     ╭──┴──╮      ╭──┴──╮
     x     C      A     z
   ╭─┴─╮      ->      ╭─┴─╮
   A   y              y   C
       │              │
       B              B */
static void
rotate(struct ccc_hromap *const t, size_t const z, size_t const x,
       size_t const y, enum hrm_branch const dir)
{
    assert(z);
    struct ccc_hromap_elem *const z_r = node_at(t, z);
    struct ccc_hromap_elem *const x_r = node_at(t, x);
    size_t const g = parent_i(t, z);
    x_r->parent = g;
    if (!g)
    {
        t->root = x;
    }
    else
    {
        struct ccc_hromap_elem *const g_r = node_at(t, g);
        g_r->branch[g_r->branch[R] == z] = x;
    }
    x_r->branch[dir] = z;
    z_r->parent = x;
    z_r->branch[!dir] = y;
    *parent_r(t, y) = z;
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
double_rotate(struct ccc_hromap *const t, size_t const z, size_t const x,
              size_t const y, enum hrm_branch const dir)
{
    assert(z && x && y);
    struct ccc_hromap_elem *const z_r = node_at(t, z);
    struct ccc_hromap_elem *const x_r = node_at(t, x);
    struct ccc_hromap_elem *const y_r = node_at(t, y);
    size_t const g = z_r->parent;
    y_r->parent = g;
    if (!g)
    {
        t->root = y;
    }
    else
    {
        struct ccc_hromap_elem *const g_r = node_at(t, g);
        g_r->branch[g_r->branch[R] == z] = y;
    }
    x_r->branch[!dir] = y_r->branch[dir];
    *parent_r(t, y_r->branch[dir]) = x;
    y_r->branch[dir] = x;
    x_r->parent = y;

    z_r->branch[dir] = y_r->branch[!dir];
    *parent_r(t, y_r->branch[!dir]) = z;
    y_r->branch[!dir] = z;
    z_r->parent = y;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
      0╭─╯
       x */
[[maybe_unused]] static inline ccc_tribool
is_0_child(struct ccc_hromap const *const t, size_t const p, size_t const x)
{
    return p && parity(t, p) == parity(t, x);
}

/* Returns true for rank difference 1 between the parent and node.
         p
      1╭─╯
       x */
static inline ccc_tribool
is_1_child(struct ccc_hromap const *const t, size_t const p, size_t const x)
{
    return p && parity(t, p) != parity(t, x);
}

/* Returns true for rank difference 2 between the parent and node.
         p
      2╭─╯
       x */
static inline ccc_tribool
is_2_child(struct ccc_hromap const *const t, size_t const p, size_t const x)
{
    return p && parity(t, p) == parity(t, x);
}

/* Returns true for rank difference 3 between the parent and node.
         p
      3╭─╯
       x */
[[maybe_unused]] static inline ccc_tribool
is_3_child(struct ccc_hromap const *const t, size_t const p, size_t const x)
{
    return p && parity(t, p) != parity(t, x);
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮1
       x   y */
static inline ccc_tribool
is_01_parent(struct ccc_hromap const *const t, size_t const x, size_t const p,
             size_t const y)
{
    assert(p);
    return (!parity(t, x) && !parity(t, p) && parity(t, y))
        || (parity(t, x) && parity(t, p) && !parity(t, y));
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
      1╭─┴─╮1
       x   y */
static inline ccc_tribool
is_11_parent(struct ccc_hromap const *const t, size_t const x, size_t const p,
             size_t const y)
{
    assert(p);
    return (!parity(t, x) && parity(t, p) && !parity(t, y))
        || (parity(t, x) && !parity(t, p) && parity(t, y));
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮2
       x   y */
static inline ccc_tribool
is_02_parent(struct ccc_hromap const *const t, size_t const x, size_t const p,
             size_t const y)
{
    assert(p);
    return (parity(t, x) == parity(t, p)) && (parity(t, p) == parity(t, y));
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
is_22_parent(struct ccc_hromap const *const t, size_t const x, size_t const p,
             size_t const y)
{
    assert(p);
    return (parity(t, x) == parity(t, p)) && (parity(t, p) == parity(t, y));
}

static inline void
promote(struct ccc_hromap const *const t, size_t const x)
{
    if (x)
    {
        *block_at(t, x) ^= bit_on(x);
    }
}

static inline void
demote(struct ccc_hromap const *const t, size_t const x)
{
    promote(t, x);
}

/* Parity based ranks mean this is no-op but leave in case implementation ever
   changes. Also, makes clear what sections of code are trying to do. */
static inline void
double_promote(struct ccc_hromap const *const, size_t const)
{}

/* Parity based ranks mean this is no-op but leave in case implementation ever
   changes. Also, makes clear what sections of code are trying to do. */
static inline void
double_demote(struct ccc_hromap const *const, size_t const)
{}

static inline ccc_tribool
is_leaf(struct ccc_hromap const *const t, size_t const x)
{
    return !branch_i(t, x, L) && !branch_i(t, x, R);
}

static inline size_t
sibling_of(struct ccc_hromap const *const t, size_t const x)
{
    size_t const p = parent_i(t, x);
    assert(p);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return node_at(t, p)->branch[branch_i(t, p, L) == x];
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
recursive_size(struct ccc_hromap const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_size(t, branch_i(t, r, R))
         + recursive_size(t, branch_i(t, r, L));
}

static ccc_tribool
are_subtrees_valid(struct ccc_hromap const *t, struct tree_range const r)
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
    return are_subtrees_valid(t,
                              (struct tree_range){
                                  .low = r.low,
                                  .root = branch_i(t, r.root, L),
                                  .high = r.root,
                              })
        && are_subtrees_valid(t, (struct tree_range){
                                     .low = r.root,
                                     .root = branch_i(t, r.root, R),
                                     .high = r.high,
                                 });
}

static ccc_tribool
is_storing_parent(struct ccc_hromap const *const t, size_t const p,
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
is_free_list_valid(struct ccc_hromap const *const t)
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

static inline ccc_tribool
validate(struct ccc_hromap const *const hrm)
{
    /* If we haven't lazily initialized we should not check anything. */
    if (hrm->data && (!hrm->nodes || !hrm->parity))
    {
        return CCC_TRUE;
    }
    if (!hrm->count && !parity(hrm, 0))
    {
        return CCC_FALSE;
    }
    if (!are_subtrees_valid(hrm, (struct tree_range){.root = hrm->root}))
    {
        return CCC_FALSE;
    }
    size_t const size = recursive_size(hrm, hrm->root);
    if (size && size != hrm->count - 1)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(hrm, 0, hrm->root))
    {
        return CCC_FALSE;
    }
    if (!is_free_list_valid(hrm))
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

The original implementation has be changed to eliminate left and right cases,
simplify deletion, and work within the C Container Collection memory framework.

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
