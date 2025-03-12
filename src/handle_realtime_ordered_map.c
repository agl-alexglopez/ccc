/** Author: Alexander G. Lopez
    --------------------------
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
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "handle_realtime_ordered_map.h"
#include "impl/impl_handle_realtime_ordered_map.h"
#include "impl/impl_types.h"
#include "types.h"

/** @private */
enum hrm_branch_
{
    L = 0,
    R,
};

enum : uint8_t
{
    IN_FREE_LIST = (uint8_t)~0,
};

/** @private */
struct hrm_query_
{
    ccc_threeway_cmp last_cmp_;
    union
    {
        size_t found_;
        size_t parent_;
    };
};

static enum hrm_branch_ const inorder_traversal = R;
static enum hrm_branch_ const reverse_inorder_traversal = L;

static enum hrm_branch_ const mindir = L;
static enum hrm_branch_ const maxdir = R;

static size_t const single_tree_node = 2;

/*==============================  Prototypes   ==============================*/

/* Returning the user struct type with stored offsets. */
static void insert(struct ccc_hromap_ *hrm, size_t parent_i,
                   ccc_threeway_cmp last_cmp, size_t elem_i);
static void *struct_base(struct ccc_hromap_ const *,
                         struct ccc_hromap_elem_ const *);
static size_t maybe_alloc_insert(struct ccc_hromap_ *hrm, size_t parent,
                                 ccc_threeway_cmp last_cmp,
                                 struct ccc_hromap_elem_ *elem);
static size_t remove_fixup(struct ccc_hromap_ *t, size_t remove);
static void *base_at(struct ccc_hromap_ const *, size_t);
static size_t alloc_slot(struct ccc_hromap_ *t);
/* Returning the user key with stored offsets. */
static void *key_from_node(struct ccc_hromap_ const *t,
                           struct ccc_hromap_elem_ const *);
static void *key_at(struct ccc_hromap_ const *t, size_t i);
/* Returning the internal elem type with stored offsets. */
static struct ccc_hromap_elem_ *at(struct ccc_hromap_ const *, size_t);
static struct ccc_hromap_elem_ *elem_in_slot(struct ccc_hromap_ const *t,
                                             void const *slot);
/* Returning the internal query helper to aid in handle handling. */
static struct hrm_query_ find(struct ccc_hromap_ const *hrm, void const *key);
/* Returning the handle core to the Handle Interface. */
static inline struct ccc_hrtree_handle_ handle(struct ccc_hromap_ const *hrm,
                                               void const *key);
/* Returning a generic range that can be use for range or rrange. */
static struct ccc_range_u_ equal_range(struct ccc_hromap_ const *, void const *,
                                       void const *, enum hrm_branch_);
/* Returning threeway comparison with user callback. */
static ccc_threeway_cmp cmp_elems(struct ccc_hromap_ const *hrm,
                                  void const *key, size_t node,
                                  ccc_key_cmp_fn *fn);
/* Returning read only indices for tree nodes. */
static size_t sibling_of(struct ccc_hromap_ const *t, size_t x);
static size_t next(struct ccc_hromap_ const *t, size_t n,
                   enum hrm_branch_ traversal);
static size_t min_max_from(struct ccc_hromap_ const *t, size_t start,
                           enum hrm_branch_ dir);
static size_t branch_i(struct ccc_hromap_ const *t, size_t parent,
                       enum hrm_branch_ dir);
static size_t parent_i(struct ccc_hromap_ const *t, size_t child);
static size_t index_of(struct ccc_hromap_ const *t,
                       struct ccc_hromap_elem_ const *elem);
/* Returning references to index fields for tree nodes. */
static size_t *branch_r(struct ccc_hromap_ const *t, size_t node,
                        enum hrm_branch_ branch);
static size_t *parent_r(struct ccc_hromap_ const *t, size_t node);
/* Returning WAVL tree status. */
static ccc_tribool is_0_child(struct ccc_hromap_ const *, size_t p, size_t x);
static ccc_tribool is_1_child(struct ccc_hromap_ const *, size_t p, size_t x);
static ccc_tribool is_2_child(struct ccc_hromap_ const *, size_t p, size_t x);
static ccc_tribool is_3_child(struct ccc_hromap_ const *, size_t p, size_t x);
static ccc_tribool is_01_parent(struct ccc_hromap_ const *, size_t x, size_t p,
                                size_t y);
static ccc_tribool is_11_parent(struct ccc_hromap_ const *, size_t x, size_t p,
                                size_t y);
static ccc_tribool is_02_parent(struct ccc_hromap_ const *, size_t x, size_t p,
                                size_t y);
static ccc_tribool is_22_parent(struct ccc_hromap_ const *, size_t x, size_t p,
                                size_t y);
static ccc_tribool is_leaf(struct ccc_hromap_ const *t, size_t x);
static uint8_t parity(struct ccc_hromap_ const *t, size_t node);
static ccc_tribool validate(struct ccc_hromap_ const *hrm);
/* Returning void and maintaining the WAVL tree. */
static void init_node(struct ccc_hromap_elem_ *e);
static void insert_fixup(struct ccc_hromap_ *t, size_t z, size_t x);
static void rebalance_3_child(struct ccc_hromap_ *t, size_t z, size_t x);
static void transplant(struct ccc_hromap_ *t, size_t remove,
                       size_t replacement);
static void promote(struct ccc_hromap_ const *t, size_t x);
static void demote(struct ccc_hromap_ const *t, size_t x);
static void double_promote(struct ccc_hromap_ const *t, size_t x);
static void double_demote(struct ccc_hromap_ const *t, size_t x);

static void rotate(struct ccc_hromap_ *t, size_t z, size_t x, size_t y,
                   enum hrm_branch_ dir);
static void double_rotate(struct ccc_hromap_ *t, size_t z, size_t x, size_t y,
                          enum hrm_branch_ dir);
/* Returning void as miscellaneous helpers. */
static void swap(char tmp[], void *a, void *b, size_t elem_sz);
static size_t max(size_t, size_t);

/*==============================  Interface    ==============================*/

void *
ccc_hrm_at(ccc_handle_realtime_ordered_map const *const h, ccc_handle_i const i)
{
    if (!h || !i)
    {
        return 0;
    }
    return ccc_buf_at(&h->buf_, i);
}

ccc_tribool
ccc_hrm_contains(ccc_handle_realtime_ordered_map const *const hrm,
                 void const *const key)
{
    if (!hrm || !key)
    {
        return CCC_BOOL_ERR;
    }
    return CCC_EQL == find(hrm, key).last_cmp_;
}

ccc_handle_i
ccc_hrm_get_key_val(ccc_handle_realtime_ordered_map const *const hrm,
                    void const *const key)
{
    if (!hrm || !key)
    {
        return 0;
    }
    struct hrm_query_ const q = find(hrm, key);
    return (CCC_EQL == q.last_cmp_) ? q.found_ : 0;
}

ccc_handle
ccc_hrm_swap_handle(ccc_handle_realtime_ordered_map *const hrm,
                    ccc_hromap_elem *const out_handle)
{
    if (!hrm || !out_handle)
    {
        return (ccc_handle){{.stats_ = CCC_ARG_ERROR}};
    }
    struct hrm_query_ const q = find(hrm, key_from_node(hrm, out_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        void *const slot = ccc_buf_at(&hrm->buf_, q.found_);
        *out_handle = *elem_in_slot(hrm, slot);
        void *const user_struct = struct_base(hrm, out_handle);
        void *const tmp = ccc_buf_at(&hrm->buf_, 0);
        swap(tmp, user_struct, slot, ccc_buf_elem_size(&hrm->buf_));
        elem_in_slot(hrm, tmp)->parity_ = 1;
        return (ccc_handle){{.i_ = q.found_, .stats_ = CCC_OCCUPIED}};
    }
    size_t const i
        = maybe_alloc_insert(hrm, q.parent_, q.last_cmp_, out_handle);
    if (!i)
    {
        return (ccc_handle){{.i_ = 0, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_handle){{.i_ = i, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hrm_try_insert(ccc_handle_realtime_ordered_map *const hrm,
                   ccc_hromap_elem *const key_val_handle)
{
    if (!hrm || !key_val_handle)
    {
        return (ccc_handle){{.stats_ = CCC_ARG_ERROR}};
    }
    struct hrm_query_ const q = find(hrm, key_from_node(hrm, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        return (ccc_handle){{.i_ = q.found_, .stats_ = CCC_OCCUPIED}};
    }
    size_t const i
        = maybe_alloc_insert(hrm, q.parent_, q.last_cmp_, key_val_handle);
    if (!i)
    {
        return (ccc_handle){{.i_ = 0, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_handle){{.i_ = i, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hrm_insert_or_assign(ccc_handle_realtime_ordered_map *const hrm,
                         ccc_hromap_elem *const key_val_handle)
{
    if (!hrm || !key_val_handle)
    {
        return (ccc_handle){{.stats_ = CCC_ARG_ERROR}};
    }
    struct hrm_query_ const q = find(hrm, key_from_node(hrm, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        void *const found = base_at(hrm, q.found_);
        *key_val_handle = *elem_in_slot(hrm, found);
        (void)ccc_buf_write(&hrm->buf_, q.found_,
                            struct_base(hrm, key_val_handle));
        return (ccc_handle){{.i_ = q.found_, .stats_ = CCC_OCCUPIED}};
    }
    size_t const i
        = maybe_alloc_insert(hrm, q.parent_, q.last_cmp_, key_val_handle);
    if (!i)
    {
        return (ccc_handle){{.i_ = 0, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_handle){{.i_ = i, .stats_ = CCC_VACANT}};
}

ccc_hromap_handle *
ccc_hrm_and_modify(ccc_hromap_handle *const h, ccc_update_fn *const fn)
{
    if (h && fn && h->impl_.handle_.stats_ & CCC_OCCUPIED)
    {
        fn((ccc_user_type){
            .user_type = base_at(h->impl_.hrm_, h->impl_.handle_.i_), NULL});
    }
    return h;
}

ccc_hromap_handle *
ccc_hrm_and_modify_aux(ccc_hromap_handle *const h, ccc_update_fn *const fn,
                       void *const aux)
{
    if (h && fn && h->impl_.handle_.stats_ & CCC_OCCUPIED)
    {
        fn((ccc_user_type){
            .user_type = base_at(h->impl_.hrm_, h->impl_.handle_.i_), aux});
    }
    return h;
}

ccc_handle_i
ccc_hrm_or_insert(ccc_hromap_handle const *const h, ccc_hromap_elem *const elem)
{
    if (!h || !elem)
    {
        return 0;
    }
    if (h->impl_.handle_.stats_ == CCC_OCCUPIED)
    {
        return h->impl_.handle_.i_;
    }
    return maybe_alloc_insert(h->impl_.hrm_, h->impl_.handle_.i_,
                              h->impl_.last_cmp_, elem);
}

ccc_handle_i
ccc_hrm_insert_handle(ccc_hromap_handle const *const h,
                      ccc_hromap_elem *const elem)
{
    if (!h || !elem)
    {
        return 0;
    }
    if (h->impl_.handle_.stats_ == CCC_OCCUPIED)
    {
        void *const slot = base_at(h->impl_.hrm_, h->impl_.handle_.i_);
        *elem = *elem_in_slot(h->impl_.hrm_, slot);
        void const *const e_base = struct_base(h->impl_.hrm_, elem);
        if (slot != e_base)
        {
            (void)memcpy(slot, e_base, ccc_buf_elem_size(&h->impl_.hrm_->buf_));
        }
        return h->impl_.handle_.i_;
    }
    return maybe_alloc_insert(h->impl_.hrm_, h->impl_.handle_.i_,
                              h->impl_.last_cmp_, elem);
}

ccc_hromap_handle
ccc_hrm_handle(ccc_handle_realtime_ordered_map const *const hrm,
               void const *const key)
{
    if (!hrm || !key)
    {
        return (ccc_hromap_handle){{.handle_ = {.stats_ = CCC_ARG_ERROR}}};
    }
    return (ccc_hromap_handle){handle(hrm, key)};
}

ccc_handle
ccc_hrm_remove_handle(ccc_hromap_handle const *const h)
{
    if (!h)
    {
        return (ccc_handle){{.stats_ = CCC_ARG_ERROR}};
    }
    if (h->impl_.handle_.stats_ == CCC_OCCUPIED)
    {
        size_t const ret = remove_fixup(h->impl_.hrm_, h->impl_.handle_.i_);
        return (ccc_handle){{.i_ = ret, .stats_ = CCC_OCCUPIED}};
    }
    return (ccc_handle){{.i_ = 0, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hrm_remove(ccc_handle_realtime_ordered_map *const hrm,
               ccc_hromap_elem *const out_handle)
{
    if (!hrm || !out_handle)
    {
        return (ccc_handle){{.stats_ = CCC_ARG_ERROR}};
    }
    struct hrm_query_ const q = find(hrm, key_from_node(hrm, out_handle));
    if (q.last_cmp_ != CCC_EQL)
    {
        return (ccc_handle){{.i_ = 0, .stats_ = CCC_VACANT}};
    }
    size_t const removed = remove_fixup(hrm, q.found_);
    assert(removed);
    void *const user_struct = struct_base(hrm, out_handle);
    void const *const r = ccc_buf_at(&hrm->buf_, removed);
    if (user_struct != r)
    {
        (void)memcpy(user_struct, r, ccc_buf_elem_size(&hrm->buf_));
    }
    return (ccc_handle){{.i_ = 0, .stats_ = CCC_OCCUPIED}};
}

ccc_range
ccc_hrm_equal_range(ccc_handle_realtime_ordered_map const *const hrm,
                    void const *const begin_key, void const *const end_key)
{
    if (!hrm || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(hrm, begin_key, end_key, inorder_traversal)};
}

ccc_rrange
ccc_hrm_equal_rrange(ccc_handle_realtime_ordered_map const *const hrm,
                     void const *const rbegin_key, void const *const rend_key)
{
    if (!hrm || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){
        equal_range(hrm, rbegin_key, rend_key, reverse_inorder_traversal)};
}

ccc_handle_i
ccc_hrm_unwrap(ccc_hromap_handle const *const h)
{
    if (h && h->impl_.handle_.stats_ & CCC_OCCUPIED)
    {
        return h->impl_.handle_.i_;
    }
    return 0;
}

ccc_tribool
ccc_hrm_insert_error(ccc_hromap_handle const *const h)
{
    if (!h)
    {
        return CCC_BOOL_ERR;
    }
    return (h->impl_.handle_.stats_ & CCC_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_hrm_occupied(ccc_hromap_handle const *const h)
{
    if (!h)
    {
        return CCC_BOOL_ERR;
    }
    return (h->impl_.handle_.stats_ & CCC_OCCUPIED) != 0;
}

ccc_handle_status
ccc_hrm_handle_status(ccc_hromap_handle const *const h)
{
    return h ? h->impl_.handle_.stats_ : CCC_ARG_ERROR;
}

ccc_tribool
ccc_hrm_is_empty(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm)
    {
        return CCC_BOOL_ERR;
    }
    return !ccc_hrm_size(hrm);
}

size_t
ccc_hrm_size(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm)
    {
        return 0;
    }
    size_t const sz = ccc_buf_size(&hrm->buf_);
    return !sz ? sz : sz - 1;
}

size_t
ccc_hrm_capacity(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm)
    {
        return 0;
    }
    return ccc_buf_capacity(&hrm->buf_);
}

void *
ccc_hrm_data(ccc_handle_realtime_ordered_map const *const hrm)
{
    return hrm ? ccc_buf_begin(&hrm->buf_) : NULL;
}

void *
ccc_hrm_begin(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm || ccc_buf_is_empty(&hrm->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(hrm, hrm->root_, mindir);
    return base_at(hrm, n);
}

void *
ccc_hrm_rbegin(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm || ccc_buf_is_empty(&hrm->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(hrm, hrm->root_, maxdir);
    return base_at(hrm, n);
}

void *
ccc_hrm_next(ccc_handle_realtime_ordered_map const *const hrm,
             ccc_hromap_elem const *const e)
{
    if (!hrm || !e || ccc_buf_is_empty(&hrm->buf_))
    {
        return NULL;
    }
    size_t const n = next(hrm, index_of(hrm, e), inorder_traversal);
    return base_at(hrm, n);
}

void *
ccc_hrm_rnext(ccc_handle_realtime_ordered_map const *const hrm,
              ccc_hromap_elem const *const e)
{
    if (!hrm || !e || ccc_buf_is_empty(&hrm->buf_))
    {
        return NULL;
    }
    size_t const n = next(hrm, index_of(hrm, e), reverse_inorder_traversal);
    return base_at(hrm, n);
}

void *
ccc_hrm_end(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm || ccc_buf_is_empty(&hrm->buf_))
    {
        return NULL;
    }
    return base_at(hrm, 0);
}

void *
ccc_hrm_rend(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm || ccc_buf_is_empty(&hrm->buf_))
    {
        return NULL;
    }
    return base_at(hrm, 0);
}

ccc_result
ccc_hrm_copy(ccc_handle_realtime_ordered_map *const dst,
             ccc_handle_realtime_ordered_map const *const src,
             ccc_alloc_fn *const fn)
{
    if (!dst || !src || src == dst
        || (dst->buf_.capacity_ < src->buf_.capacity_ && !fn))
    {
        return CCC_INPUT_ERR;
    }
    /* Copy everything so we don't worry about staying in sync with future
       changes to buf container. But we have to give back original destination
       memory in case it has already been allocated. Alloc will remain the
       same as in dst initialization because that controls permission. */
    void *const dst_mem = dst->buf_.mem_;
    size_t const dst_cap = dst->buf_.capacity_;
    ccc_alloc_fn *const dst_alloc = dst->buf_.alloc_;
    *dst = *src;
    dst->buf_.mem_ = dst_mem;
    dst->buf_.capacity_ = dst_cap;
    dst->buf_.alloc_ = dst_alloc;
    if (dst->buf_.capacity_ < src->buf_.capacity_)
    {
        ccc_result const resize_res
            = ccc_buf_alloc(&dst->buf_, src->buf_.capacity_, fn);
        if (resize_res != CCC_OK)
        {
            return resize_res;
        }
        dst->buf_.capacity_ = src->buf_.capacity_;
    }
    (void)memcpy(dst->buf_.mem_, src->buf_.mem_,
                 src->buf_.capacity_ * src->buf_.elem_sz_);
    return CCC_OK;
}

ccc_result
ccc_hrm_clear(ccc_handle_realtime_ordered_map *const hrm,
              ccc_destructor_fn *const fn)
{
    if (!hrm)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        hrm->root_ = 0;
        return ccc_buf_size_set(&hrm->buf_, 1);
    }
    while (!ccc_hrm_is_empty(hrm))
    {
        size_t const i = remove_fixup(hrm, hrm->root_);
        assert(i);
        fn((ccc_user_type){.user_type = ccc_buf_at(&hrm->buf_, i),
                           .aux = hrm->buf_.aux_});
    }
    (void)ccc_buf_size_set(&hrm->buf_, 1);
    hrm->root_ = 0;
    return CCC_OK;
}

ccc_result
ccc_hrm_clear_and_free(ccc_handle_realtime_ordered_map *const hrm,
                       ccc_destructor_fn *const fn)
{
    if (!hrm)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        hrm->root_ = 0;
        return ccc_buf_alloc(&hrm->buf_, 0, hrm->buf_.alloc_);
    }
    while (!ccc_hrm_is_empty(hrm))
    {
        size_t const i = remove_fixup(hrm, hrm->root_);
        assert(i);
        fn((ccc_user_type){.user_type = ccc_buf_at(&hrm->buf_, i),
                           .aux = hrm->buf_.aux_});
    }
    hrm->root_ = 0;
    return ccc_buf_alloc(&hrm->buf_, 0, hrm->buf_.alloc_);
}

ccc_tribool
ccc_hrm_validate(ccc_handle_realtime_ordered_map const *const hrm)
{
    if (!hrm)
    {
        return CCC_BOOL_ERR;
    }
    return validate(hrm);
}

/*========================  Private Interface  ==============================*/

void
ccc_impl_hrm_insert(struct ccc_hromap_ *const hrm, size_t const parent_i,
                    ccc_threeway_cmp const last_cmp, size_t const elem_i)
{
    insert(hrm, parent_i, last_cmp, elem_i);
}

struct ccc_hrtree_handle_
ccc_impl_hrm_handle(struct ccc_hromap_ const *const hrm, void const *const key)
{
    return handle(hrm, key);
}

void *
ccc_impl_hrm_key_at(struct ccc_hromap_ const *const hrm, size_t const slot)
{
    return key_at(hrm, slot);
}

struct ccc_hromap_elem_ *
ccc_impl_hrm_elem_at(struct ccc_hromap_ const *hrm, size_t const i)
{
    return at(hrm, i);
}

size_t
ccc_impl_hrm_alloc_slot(struct ccc_hromap_ *const hrm)
{
    return alloc_slot(hrm);
}

/*==========================  Static Helpers   ==============================*/

static inline size_t
maybe_alloc_insert(struct ccc_hromap_ *const hrm, size_t const parent,
                   ccc_threeway_cmp const last_cmp,
                   struct ccc_hromap_elem_ *const elem)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const node = alloc_slot(hrm);
    if (!node)
    {
        return 0;
    }
    (void)ccc_buf_write(&hrm->buf_, node, struct_base(hrm, elem));
    insert(hrm, parent, last_cmp, node);
    return node;
}

static inline void
insert(struct ccc_hromap_ *const hrm, size_t const parent_i,
       ccc_threeway_cmp const last_cmp, size_t const elem_i)
{
    struct ccc_hromap_elem_ *elem = at(hrm, elem_i);
    init_node(elem);
    if (ccc_buf_size(&hrm->buf_) == single_tree_node)
    {
        hrm->root_ = elem_i;
        return;
    }
    assert(last_cmp == CCC_GRT || last_cmp == CCC_LES);
    struct ccc_hromap_elem_ *parent = at(hrm, parent_i);
    ccc_tribool const rank_rule_break
        = !parent->branch_[L] && !parent->branch_[R];
    parent->branch_[CCC_GRT == last_cmp] = elem_i;
    elem->parent_ = parent_i;
    if (rank_rule_break)
    {
        insert_fixup(hrm, parent_i, elem_i);
    }
}

static inline struct ccc_hrtree_handle_
handle(struct ccc_hromap_ const *const hrm, void const *const key)
{
    struct hrm_query_ const q = find(hrm, key);
    if (CCC_EQL == q.last_cmp_)
    {
        return (struct ccc_hrtree_handle_){
            .hrm_ = (struct ccc_hromap_ *)hrm,
            .last_cmp_ = q.last_cmp_,
            .handle_ = {.i_ = q.found_, .stats_ = CCC_OCCUPIED},
        };
    }
    return (struct ccc_hrtree_handle_){
        .hrm_ = (struct ccc_hromap_ *)hrm,
        .last_cmp_ = q.last_cmp_,
        .handle_ = {.i_ = q.parent_, .stats_ = CCC_NO_UNWRAP | CCC_VACANT},
    };
}

static inline struct hrm_query_
find(struct ccc_hromap_ const *const hrm, void const *const key)
{
    size_t parent = 0;
    struct hrm_query_ q = {.last_cmp_ = CCC_CMP_ERR, .found_ = hrm->root_};
    for (; q.found_; parent = q.found_,
                     q.found_ = branch_i(hrm, q.found_, CCC_GRT == q.last_cmp_))
    {
        q.last_cmp_ = cmp_elems(hrm, key, q.found_, hrm->cmp_);
        if (CCC_EQL == q.last_cmp_)
        {
            return q;
        }
    }
    /* Type punning here OK as both union members have same type and size. */
    q.parent_ = parent;
    return q;
}

static inline size_t
next(struct ccc_hromap_ const *const t, size_t n,
     enum hrm_branch_ const traversal)
{
    if (!n)
    {
        return 0;
    }
    assert(!parent_i(t, t->root_));
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

static inline struct ccc_range_u_
equal_range(struct ccc_hromap_ const *const t, void const *const begin_key,
            void const *const end_key, enum hrm_branch_ const traversal)
{
    if (ccc_hrm_is_empty(t))
    {
        return (struct ccc_range_u_){};
    }
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    struct hrm_query_ b = find(t, begin_key);
    if (b.last_cmp_ == les_or_grt[traversal])
    {
        b.found_ = next(t, b.found_, traversal);
    }
    struct hrm_query_ e = find(t, end_key);
    if (e.last_cmp_ != les_or_grt[!traversal])
    {
        e.found_ = next(t, e.found_, traversal);
    }
    return (struct ccc_range_u_){
        .begin_ = base_at(t, b.found_),
        .end_ = base_at(t, e.found_),
    };
}

static inline size_t
min_max_from(struct ccc_hromap_ const *const t, size_t start,
             enum hrm_branch_ const dir)
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
cmp_elems(struct ccc_hromap_ const *const hrm, void const *const key,
          size_t const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){.key_lhs = key,
                            .user_type_rhs = base_at(hrm, node),
                            .aux = hrm->buf_.aux_});
}

static inline size_t
alloc_slot(struct ccc_hromap_ *const t)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const old_sz = ccc_buf_size(&t->buf_);
    size_t old_cap = ccc_buf_capacity(&t->buf_);
    if (!old_sz || old_sz == old_cap)
    {
        assert(!t->free_list_);
        if (old_sz == old_cap
            && ccc_buf_alloc(&t->buf_, old_cap ? old_cap * 2 : 8,
                             t->buf_.alloc_)
                   != CCC_OK)
        {
            return 0;
        }
        old_cap = old_sz ? old_cap : 0;
        size_t const new_cap = ccc_buf_capacity(&t->buf_);
        size_t prev = 0;
        for (size_t i = new_cap - 1; i > 0 && i >= old_cap; prev = i, --i)
        {
            at(t, i)->parity_ = IN_FREE_LIST;
            at(t, i)->next_free_ = prev;
        }
        t->free_list_ = prev;
        if (ccc_buf_size_set(&t->buf_, max(old_sz, 1)) != CCC_OK)
        {
            return 0;
        }
        at(t, 0)->parity_ = 1;
    }
    if (!t->free_list_ || ccc_buf_size_plus(&t->buf_, 1) != CCC_OK)
    {
        return 0;
    }
    size_t const slot = t->free_list_;
    t->free_list_ = at(t, slot)->next_free_;
    return slot;
}

static inline void
init_node(struct ccc_hromap_elem_ *const e)
{
    assert(e != NULL);
    e->parity_ = 0;
    e->branch_[L] = e->branch_[R] = e->parent_ = 0;
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const elem_sz)
{
    if (a == b)
    {
        return;
    }
    (void)memcpy(tmp, a, elem_sz);
    (void)memcpy(a, b, elem_sz);
    (void)memcpy(b, tmp, elem_sz);
}

static inline struct ccc_hromap_elem_ *
at(struct ccc_hromap_ const *const t, size_t const i)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, i));
}

static inline size_t
branch_i(struct ccc_hromap_ const *const t, size_t const parent,
         enum hrm_branch_ const dir)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, parent))->branch_[dir];
}

static inline size_t
parent_i(struct ccc_hromap_ const *const t, size_t const child)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, child))->parent_;
}

static inline size_t
index_of(struct ccc_hromap_ const *const t,
         struct ccc_hromap_elem_ const *const elem)
{
    return ccc_buf_i(&t->buf_, struct_base(t, elem));
}

static inline uint8_t
parity(struct ccc_hromap_ const *t, size_t const node)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parity_;
}

static inline size_t *
branch_r(struct ccc_hromap_ const *t, size_t const node,
         enum hrm_branch_ const branch)
{
    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->branch_[branch];
}

static inline size_t *
parent_r(struct ccc_hromap_ const *t, size_t const node)
{

    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parent_;
}

static inline void *
base_at(struct ccc_hromap_ const *const hrm, size_t const i)
{
    return ccc_buf_at(&hrm->buf_, i);
}

static inline void *
struct_base(struct ccc_hromap_ const *const hrm,
            struct ccc_hromap_elem_ const *const e)
{
    return ((char *)e->branch_) - hrm->node_elem_offset_;
}

static struct ccc_hromap_elem_ *
elem_in_slot(struct ccc_hromap_ const *const t, void const *const slot)
{

    return (struct ccc_hromap_elem_ *)((char *)slot + t->node_elem_offset_);
}

static inline void *
key_from_node(struct ccc_hromap_ const *const t,
              struct ccc_hromap_elem_ const *const elem)
{
    return (char *)struct_base(t, elem) + t->key_offset_;
}

static inline void *
key_at(struct ccc_hromap_ const *const t, size_t const i)
{
    return (char *)ccc_buf_at(&t->buf_, i) + t->key_offset_;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static inline void
insert_fixup(struct ccc_hromap_ *const t, size_t z, size_t x)
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
    } while (is_01_parent(t, x, z, sibling_of(t, x)));

    if (!is_02_parent(t, x, z, sibling_of(t, x)))
    {
        return;
    }
    assert(x);
    assert(is_0_child(t, z, x));
    enum hrm_branch_ const p_to_x_dir = branch_i(t, z, R) == x;
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

static inline size_t
remove_fixup(struct ccc_hromap_ *const t, size_t const remove)
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
            t->root_ = x;
        }
        two_child = is_2_child(t, p, y);
        *branch_r(t, p, branch_i(t, p, R) == y) = x;
    }
    else
    {
        y = min_max_from(t, branch_i(t, remove, R), mindir);
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
    at(t, remove)->next_free_ = t->free_list_;
    at(t, remove)->parity_ = IN_FREE_LIST;
    t->free_list_ = remove;
    [[maybe_unused]] ccc_result const r = ccc_buf_size_minus(&t->buf_, 1);
    assert(r == CCC_OK);
    return remove;
}

static inline void
transplant(struct ccc_hromap_ *const t, size_t const remove,
           size_t const replacement)
{
    assert(remove);
    assert(replacement);
    *parent_r(t, replacement) = parent_i(t, remove);
    if (!parent_i(t, remove))
    {
        t->root_ = replacement;
    }
    else
    {
        size_t const p = parent_i(t, remove);
        *branch_r(t, p, branch_i(t, p, R) == remove) = replacement;
    }
    struct ccc_hromap_elem_ *const remove_r = at(t, remove);
    struct ccc_hromap_elem_ *const replace_r = at(t, replacement);
    *parent_r(t, remove_r->branch_[R]) = replacement;
    *parent_r(t, remove_r->branch_[L]) = replacement;
    replace_r->branch_[R] = remove_r->branch_[R];
    replace_r->branch_[L] = remove_r->branch_[L];
    replace_r->parity_ = remove_r->parity_;
}

static inline void
rebalance_3_child(struct ccc_hromap_ *const t, size_t z, size_t x)
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
            enum hrm_branch_ const z_to_x_dir = branch_i(t, z, R) == x;
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
    } while (z && made_3_child);
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
static inline void
rotate(struct ccc_hromap_ *const t, size_t const z, size_t const x,
       size_t const y, enum hrm_branch_ const dir)
{
    assert(z);
    struct ccc_hromap_elem_ *const z_r = at(t, z);
    struct ccc_hromap_elem_ *const x_r = at(t, x);
    size_t const g = parent_i(t, z);
    x_r->parent_ = g;
    if (!g)
    {
        t->root_ = x;
    }
    else
    {
        struct ccc_hromap_elem_ *const g_r = at(t, g);
        g_r->branch_[g_r->branch_[R] == z] = x;
    }
    x_r->branch_[dir] = z;
    z_r->parent_ = x;
    z_r->branch_[!dir] = y;
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
static inline void
double_rotate(struct ccc_hromap_ *const t, size_t const z, size_t const x,
              size_t const y, enum hrm_branch_ const dir)
{
    assert(z && x && y);
    struct ccc_hromap_elem_ *const z_r = at(t, z);
    struct ccc_hromap_elem_ *const x_r = at(t, x);
    struct ccc_hromap_elem_ *const y_r = at(t, y);
    size_t const g = z_r->parent_;
    y_r->parent_ = g;
    if (!g)
    {
        t->root_ = y;
    }
    else
    {
        struct ccc_hromap_elem_ *const g_r = at(t, g);
        g_r->branch_[g_r->branch_[R] == z] = y;
    }
    x_r->branch_[!dir] = y_r->branch_[dir];
    *parent_r(t, y_r->branch_[dir]) = x;
    y_r->branch_[dir] = x;
    x_r->parent_ = y;

    z_r->branch_[dir] = y_r->branch_[!dir];
    *parent_r(t, y_r->branch_[!dir]) = z;
    y_r->branch_[!dir] = z;
    z_r->parent_ = y;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
      0╭─╯
       x */
[[maybe_unused]] static inline ccc_tribool
is_0_child(struct ccc_hromap_ const *const t, size_t const p, size_t const x)
{
    return p && parity(t, p) == parity(t, x);
}

/* Returns true for rank difference 1 between the parent and node.
         p
      1╭─╯
       x */
static inline ccc_tribool
is_1_child(struct ccc_hromap_ const *const t, size_t const p, size_t const x)
{
    return p && parity(t, p) != parity(t, x);
}

/* Returns true for rank difference 2 between the parent and node.
         p
      2╭─╯
       x */
static inline ccc_tribool
is_2_child(struct ccc_hromap_ const *const t, size_t const p, size_t const x)
{
    return p && parity(t, p) == parity(t, x);
}

/* Returns true for rank difference 3 between the parent and node.
         p
      3╭─╯
       x */
[[maybe_unused]] static inline ccc_tribool
is_3_child(struct ccc_hromap_ const *const t, size_t const p, size_t const x)
{
    return p && parity(t, p) != parity(t, x);
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮1
       x   y */
static inline ccc_tribool
is_01_parent(struct ccc_hromap_ const *const t, size_t const x, size_t const p,
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
is_11_parent(struct ccc_hromap_ const *const t, size_t const x, size_t const p,
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
is_02_parent(struct ccc_hromap_ const *const t, size_t const x, size_t const p,
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
is_22_parent(struct ccc_hromap_ const *const t, size_t const x, size_t const p,
             size_t const y)
{
    assert(p);
    return (parity(t, x) == parity(t, p)) && (parity(t, p) == parity(t, y));
}

static inline void
promote(struct ccc_hromap_ const *const t, size_t const x)
{
    if (x)
    {
        at(t, x)->parity_ = !parity(t, x);
    }
}

static inline void
demote(struct ccc_hromap_ const *const t, size_t const x)
{
    promote(t, x);
}

/* Parity based ranks mean this is no-op but leave in case implementation ever
   changes. Also, makes clear what sections of code are trying to do. */
static inline void
double_promote(struct ccc_hromap_ const *const, size_t const)
{}

/* Parity based ranks mean this is no-op but leave in case implementation ever
   changes. Also, makes clear what sections of code are trying to do. */
static inline void
double_demote(struct ccc_hromap_ const *const, size_t const)
{}

static inline ccc_tribool
is_leaf(struct ccc_hromap_ const *const t, size_t const x)
{
    return !branch_i(t, x, L) && !branch_i(t, x, R);
}

static inline size_t
sibling_of(struct ccc_hromap_ const *const t, size_t const x)
{
    size_t const p = parent_i(t, x);
    assert(p);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return at(t, p)->branch_[branch_i(t, p, L) == x];
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

/** @private */
struct tree_range_
{
    size_t low;
    size_t root;
    size_t high;
};

static size_t
recursive_size(struct ccc_hromap_ const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_size(t, branch_i(t, r, R))
           + recursive_size(t, branch_i(t, r, L));
}

static ccc_tribool
are_subtrees_valid(struct ccc_hromap_ const *t, struct tree_range_ const r)
{
    if (!r.root)
    {
        return CCC_TRUE;
    }
    if (r.low && cmp_elems(t, key_at(t, r.low), r.root, t->cmp_) != CCC_LES)
    {
        return CCC_FALSE;
    }
    if (r.high && cmp_elems(t, key_at(t, r.high), r.root, t->cmp_) != CCC_GRT)
    {
        return CCC_FALSE;
    }
    return are_subtrees_valid(
               t, (struct tree_range_){.low = r.low,
                                       .root = branch_i(t, r.root, L),
                                       .high = r.root})
           && are_subtrees_valid(
               t, (struct tree_range_){.low = r.root,
                                       .root = branch_i(t, r.root, R),
                                       .high = r.high});
}

static ccc_tribool
is_storing_parent(struct ccc_hromap_ const *const t, size_t const p,
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

static inline ccc_tribool
is_free_list_valid(struct ccc_hromap_ const *const t)
{
    if (!ccc_buf_size(&t->buf_))
    {
        return CCC_TRUE;
    }
    size_t list_check1 = 0;
    for (size_t i = 0; i < ccc_buf_capacity(&t->buf_); ++i)
    {
        if (at(t, i)->parity_ == IN_FREE_LIST)
        {
            ++list_check1;
        }
    }
    if (list_check1 + ccc_buf_size(&t->buf_) != ccc_buf_capacity(&t->buf_))
    {
        return CCC_FALSE;
    }
    size_t list_check2 = 0;
    for (size_t cur = t->free_list_;
         cur && list_check2 < ccc_buf_capacity(&t->buf_);
         cur = at(t, cur)->next_free_, ++list_check2)
    {}
    return list_check2 == list_check1;
}

static inline ccc_tribool
validate(struct ccc_hromap_ const *const hrm)
{
    if (!ccc_buf_is_empty(&hrm->buf_) && !at(hrm, 0)->parity_)
    {
        return CCC_FALSE;
    }
    if (!are_subtrees_valid(hrm, (struct tree_range_){.root = hrm->root_}))
    {
        return CCC_FALSE;
    }
    size_t const size = recursive_size(hrm, hrm->root_);
    if (size && size != ccc_buf_size(&hrm->buf_) - 1)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(hrm, 0, hrm->root_))
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
implementation of a WAVL tree in this repository for some map containers. This
license is identical to that used for the entire repository that I provide at
the root of the repository.

The original repository can be found here:

https://github.com/pvachon/wavl_tree

See the detailed changes to the code described at the top of the file. The
copyright notice follows.

Copyright (c) 2021, Phil Vachon <phil@security-embedded.com>
All rights reserved.

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
