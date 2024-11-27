/** Author: Alexander G. Lopez
    --------------------------
This file contains my implementation of a flat realtime ordered map. The added
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
shortening the code significantly. Finally, a few other changes and
improvements suggested by the authors of the original paper are implemented.
Finally, the data structure has been flattened into a buffer with relative
indices rather than pointers.

Overall a WAVL tree is quite impressive for it's simplicity and purported
improvements over AVL and Red-Black trees. The rank framework is intuitive
and flexible in how it can be implemented. */
#include "flat_realtime_ordered_map.h"
#include "buffer.h"
#include "impl/impl_flat_realtime_ordered_map.h"
#include "impl/impl_types.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/** @private */
enum frm_branch_
{
    L = 0,
    R,
};

/** @private */
struct frm_query_
{
    ccc_threeway_cmp last_cmp_;
    union
    {
        size_t found_;
        size_t parent_;
    };
};

static enum frm_branch_ const inorder_traversal = R;
static enum frm_branch_ const reverse_inorder_traversal = L;

static enum frm_branch_ const min = L;
static enum frm_branch_ const max = R;

static size_t const single_tree_node = 2;

/*==============================  Prototypes   ==============================*/

/* Returning the user struct type with stored offsets. */
static void *insert(struct ccc_fromap_ *frm, size_t parent_i,
                    ccc_threeway_cmp last_cmp, size_t elem_i);
static void *struct_base(struct ccc_fromap_ const *,
                         struct ccc_fromap_elem_ const *);
static void *maybe_alloc_insert(struct ccc_fromap_ *frm, size_t parent,
                                ccc_threeway_cmp last_cmp,
                                struct ccc_fromap_elem_ *elem);
static void *remove_fixup(struct ccc_fromap_ *t, size_t remove);
static void *base_at(struct ccc_fromap_ const *, size_t);
static void *alloc_back(struct ccc_fromap_ *t);
/* Returning the user key with stored offsets. */
static void *key_from_node(struct ccc_fromap_ const *t,
                           struct ccc_fromap_elem_ const *);
static void *key_at(struct ccc_fromap_ const *t, size_t i);
/* Returning the internal elem type with stored offsets. */
static struct ccc_fromap_elem_ *at(struct ccc_fromap_ const *, size_t);
static struct ccc_fromap_elem_ *elem_in_slot(struct ccc_fromap_ const *t,
                                             void const *slot);
/* Returning the internal query helper to aid in entry handling. */
static struct frm_query_ find(struct ccc_fromap_ const *frm, void const *key);
/* Returning the entry core to the Entry Interface. */
static inline struct ccc_frtree_entry_ entry(struct ccc_fromap_ const *frm,
                                             void const *key);
/* Returning a generic range that can be use for range or rrange. */
static struct ccc_range_u_ equal_range(struct ccc_fromap_ const *, void const *,
                                       void const *, enum frm_branch_);
/* Returning threeway comparison with user callback. */
static ccc_threeway_cmp cmp_elems(struct ccc_fromap_ const *frm,
                                  void const *key, size_t node,
                                  ccc_key_cmp_fn *fn);
/* Returning read only indices for tree nodes. */
static size_t sibling_of(struct ccc_fromap_ const *t, size_t x);
static size_t next(struct ccc_fromap_ const *t, size_t n,
                   enum frm_branch_ traversal);
static size_t min_max_from(struct ccc_fromap_ const *t, size_t start,
                           enum frm_branch_ dir);
static size_t branch_i(struct ccc_fromap_ const *t, size_t parent,
                       enum frm_branch_ dir);
static size_t parent_i(struct ccc_fromap_ const *t, size_t child);
static size_t index_of(struct ccc_fromap_ const *t,
                       struct ccc_fromap_elem_ const *elem);
/* Returning references to index fields for tree nodes. */
static size_t *branch_r(struct ccc_fromap_ const *t, size_t node,
                        enum frm_branch_ branch);
static size_t *parent_r(struct ccc_fromap_ const *t, size_t node);
/* Returning WAVL tree status. */
static bool is_0_child(struct ccc_fromap_ const *, size_t p_of_x, size_t x);
static bool is_1_child(struct ccc_fromap_ const *, size_t p_of_x, size_t x);
static bool is_2_child(struct ccc_fromap_ const *, size_t p_of_x, size_t x);
static bool is_3_child(struct ccc_fromap_ const *, size_t p_of_x, size_t x);
static bool is_01_parent(struct ccc_fromap_ const *, size_t x, size_t p_of_xy,
                         size_t y);
static bool is_11_parent(struct ccc_fromap_ const *, size_t x, size_t p_of_xy,
                         size_t y);
static bool is_02_parent(struct ccc_fromap_ const *, size_t x, size_t p_of_xy,
                         size_t y);
static bool is_22_parent(struct ccc_fromap_ const *, size_t x, size_t p_of_xy,
                         size_t y);
static bool is_leaf(struct ccc_fromap_ const *t, size_t x);
static uint8_t parity(struct ccc_fromap_ const *t, size_t node);
static bool validate(struct ccc_fromap_ const *frm);
/* Returning void and maintaining the WAVL tree. */
static void init_node(struct ccc_fromap_elem_ *e);
static void insert_fixup(struct ccc_fromap_ *t, size_t z_p_of_xy, size_t x);
static void rebalance_3_child(struct ccc_fromap_ *t, size_t z_p_of_xy,
                              size_t x);
static void transplant(struct ccc_fromap_ *t, size_t remove,
                       size_t replacement);
static void swap_and_pop(struct ccc_fromap_ *t, size_t vacant_i);
static void promote(struct ccc_fromap_ const *t, size_t x);
static void demote(struct ccc_fromap_ const *t, size_t x);
static void double_promote(struct ccc_fromap_ const *t, size_t x);
static void double_demote(struct ccc_fromap_ const *t, size_t x);

static void rotate(struct ccc_fromap_ *t, size_t z_p_of_x, size_t x_p_of_y,
                   size_t y, enum frm_branch_ dir);
static void double_rotate(struct ccc_fromap_ *t, size_t z_p_of_x,
                          size_t x_p_of_y, size_t y, enum frm_branch_ dir);
/* Returning void as miscellaneous helpers. */
static void swap(char tmp[], void *a, void *b, size_t elem_sz);

/*==============================  Interface    ==============================*/

bool
ccc_frm_contains(ccc_flat_realtime_ordered_map const *const frm,
                 void const *const key)
{
    if (!frm || !key)
    {
        return false;
    }
    return CCC_EQL == find(frm, key).last_cmp_;
}

void *
ccc_frm_get_key_val(ccc_flat_realtime_ordered_map const *const frm,
                    void const *const key)
{
    if (!frm || !key)
    {
        return NULL;
    }
    struct frm_query_ q = find(frm, key);
    return (CCC_EQL == q.last_cmp_) ? base_at(frm, q.found_) : NULL;
}

ccc_entry
ccc_frm_insert(ccc_flat_realtime_ordered_map *const frm,
               ccc_fromap_elem *const out_handle)
{
    if (!frm || !out_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    struct frm_query_ q = find(frm, key_from_node(frm, out_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        void *const slot = ccc_buf_at(&frm->buf_, q.found_);
        *out_handle = *elem_in_slot(frm, slot);
        void *const user_struct = struct_base(frm, out_handle);
        void *const tmp = ccc_buf_at(&frm->buf_, 0);
        swap(tmp, user_struct, slot, ccc_buf_elem_size(&frm->buf_));
        elem_in_slot(frm, tmp)->parity_ = 1;
        return (ccc_entry){{.e_ = slot, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *const inserted
        = maybe_alloc_insert(frm, q.parent_, q.last_cmp_, out_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_frm_try_insert(ccc_flat_realtime_ordered_map *const frm,
                   ccc_fromap_elem *const key_val_handle)
{
    if (!frm || !key_val_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    struct frm_query_ q = find(frm, key_from_node(frm, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        return (ccc_entry){
            {.e_ = base_at(frm, q.found_), .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted
        = maybe_alloc_insert(frm, q.parent_, q.last_cmp_, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_frm_insert_or_assign(ccc_flat_realtime_ordered_map *const frm,
                         ccc_fromap_elem *const key_val_handle)
{
    if (!frm || !key_val_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    struct frm_query_ q = find(frm, key_from_node(frm, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        void *const found = base_at(frm, q.found_);
        *key_val_handle = *elem_in_slot(frm, found);
        (void)ccc_buf_write(&frm->buf_, q.found_,
                            struct_base(frm, key_val_handle));
        return (ccc_entry){{.e_ = found, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted
        = maybe_alloc_insert(frm, q.parent_, q.last_cmp_, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_fromap_entry *
ccc_frm_and_modify(ccc_fromap_entry *const e, ccc_update_fn *const fn)
{
    if (e && fn && e->impl_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_user_type){.user_type = base_at(e->impl_.frm_, e->impl_.i_),
                           NULL});
    }
    return e;
}

ccc_fromap_entry *
ccc_frm_and_modify_aux(ccc_fromap_entry *const e, ccc_update_fn *const fn,
                       void *const aux)
{
    if (e && fn && e->impl_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_user_type){.user_type = base_at(e->impl_.frm_, e->impl_.i_),
                           aux});
    }
    return e;
}

void *
ccc_frm_or_insert(ccc_fromap_entry const *const e, ccc_fromap_elem *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->impl_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        return base_at(e->impl_.frm_, e->impl_.i_);
    }
    return maybe_alloc_insert(e->impl_.frm_, e->impl_.i_, e->impl_.last_cmp_,
                              elem);
}

void *
ccc_frm_insert_entry(ccc_fromap_entry const *const e,
                     ccc_fromap_elem *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->impl_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        void *const ret = base_at(e->impl_.frm_, e->impl_.i_);
        *elem = *elem_in_slot(e->impl_.frm_, ret);
        (void)memcpy(ret, struct_base(e->impl_.frm_, elem),
                     ccc_buf_elem_size(&e->impl_.frm_->buf_));
        return ret;
    }
    return maybe_alloc_insert(e->impl_.frm_, e->impl_.i_, e->impl_.last_cmp_,
                              elem);
}

ccc_fromap_entry
ccc_frm_entry(ccc_flat_realtime_ordered_map const *const frm,
              void const *const key)
{
    if (!frm || !key)
    {
        return (ccc_fromap_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    return (ccc_fromap_entry){entry(frm, key)};
}

ccc_entry
ccc_frm_remove_entry(ccc_fromap_entry const *const e)
{
    if (!e)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    if (e->impl_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        void *const erased = remove_fixup(e->impl_.frm_, e->impl_.i_);
        assert(erased);
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_frm_remove(ccc_flat_realtime_ordered_map *const frm,
               ccc_fromap_elem *const out_handle)
{
    if (!frm || !out_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    struct frm_query_ const q = find(frm, key_from_node(frm, out_handle));
    if (q.last_cmp_ != CCC_EQL)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    void const *const removed = remove_fixup(frm, q.found_);
    assert(removed);
    void *const user_struct = struct_base(frm, out_handle);
    (void)memcpy(user_struct, removed, ccc_buf_elem_size(&frm->buf_));
    return (ccc_entry){{.e_ = user_struct, .stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_range
ccc_frm_equal_range(ccc_flat_realtime_ordered_map const *const frm,
                    void const *const begin_key, void const *const end_key)
{
    if (!frm || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(frm, begin_key, end_key, inorder_traversal)};
}

ccc_rrange
ccc_frm_equal_rrange(ccc_flat_realtime_ordered_map const *const frm,
                     void const *const rbegin_key, void const *const rend_key)
{
    if (!frm || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){
        equal_range(frm, rbegin_key, rend_key, reverse_inorder_traversal)};
}

void *
ccc_frm_unwrap(ccc_fromap_entry const *const e)
{
    if (e && e->impl_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return ccc_buf_at(&e->impl_.frm_->buf_, e->impl_.i_);
    }
    return NULL;
}

bool
ccc_frm_insert_error(ccc_fromap_entry const *const e)
{
    return e ? e->impl_.stats_ & CCC_ENTRY_INSERT_ERROR : false;
}

bool
ccc_frm_occupied(ccc_fromap_entry const *const e)
{
    return e ? e->impl_.stats_ & CCC_ENTRY_OCCUPIED : false;
}

ccc_entry_status
ccc_frm_entry_status(ccc_fromap_entry const *const e)
{
    return e ? e->impl_.stats_ : CCC_ENTRY_INPUT_ERROR;
}

bool
ccc_frm_is_empty(ccc_flat_realtime_ordered_map const *const frm)
{
    return !ccc_frm_size(frm);
}

size_t
ccc_frm_size(ccc_flat_realtime_ordered_map const *const frm)
{
    if (!frm)
    {
        return 0;
    }
    size_t const sz = ccc_buf_size(&frm->buf_);
    return !sz ? sz : sz - 1;
}

size_t
ccc_frm_capacity(ccc_flat_realtime_ordered_map const *const frm)
{
    if (!frm)
    {
        return 0;
    }
    return ccc_buf_capacity(&frm->buf_);
}

void *
ccc_frm_data(ccc_flat_realtime_ordered_map const *const frm)
{
    return frm ? ccc_buf_begin(&frm->buf_) : NULL;
}

void *
ccc_frm_begin(ccc_flat_realtime_ordered_map const *const frm)
{
    if (!frm || ccc_buf_is_empty(&frm->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(frm, frm->root_, min);
    return base_at(frm, n);
}

void *
ccc_frm_rbegin(ccc_flat_realtime_ordered_map const *const frm)
{
    if (!frm || ccc_buf_is_empty(&frm->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(frm, frm->root_, max);
    return base_at(frm, n);
}

void *
ccc_frm_next(ccc_flat_realtime_ordered_map const *const frm,
             ccc_fromap_elem const *const e)
{
    if (!frm || !e || ccc_buf_is_empty(&frm->buf_))
    {
        return NULL;
    }
    size_t const n = next(frm, index_of(frm, e), inorder_traversal);
    return base_at(frm, n);
}

void *
ccc_frm_rnext(ccc_flat_realtime_ordered_map const *const frm,
              ccc_fromap_elem const *const e)
{
    if (!frm || !e || ccc_buf_is_empty(&frm->buf_))
    {
        return NULL;
    }
    size_t const n = next(frm, index_of(frm, e), reverse_inorder_traversal);
    return base_at(frm, n);
}

void *
ccc_frm_end(ccc_flat_realtime_ordered_map const *const frm)
{
    if (!frm || ccc_buf_is_empty(&frm->buf_))
    {
        return NULL;
    }
    return base_at(frm, 0);
}

void *
ccc_frm_rend(ccc_flat_realtime_ordered_map const *const frm)
{
    if (!frm || ccc_buf_is_empty(&frm->buf_))
    {
        return NULL;
    }
    return base_at(frm, 0);
}

ccc_result
ccc_frm_copy(ccc_flat_realtime_ordered_map *const dst,
             ccc_flat_realtime_ordered_map const *const src,
             ccc_alloc_fn *const fn)
{
    if (!dst || !src || (dst->buf_.capacity_ < src->buf_.capacity_ && !fn))
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
        ccc_result resize_res
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
ccc_frm_clear(ccc_flat_realtime_ordered_map *const frm,
              ccc_destructor_fn *const fn)
{
    if (!frm)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        frm->root_ = 0;
        return ccc_buf_size_set(&frm->buf_, 1);
    }
    for (void *e = ccc_buf_at(&frm->buf_, 1); e != ccc_buf_end(&frm->buf_);
         e = ccc_buf_next(&frm->buf_, e))
    {
        fn((ccc_user_type){.user_type = e, .aux = frm->buf_.aux_});
    }
    (void)ccc_buf_size_set(&frm->buf_, 1);
    frm->root_ = 0;
    return CCC_OK;
}

ccc_result
ccc_frm_clear_and_free(ccc_flat_realtime_ordered_map *const frm,
                       ccc_destructor_fn *const fn)
{
    if (!frm)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        frm->root_ = 0;
        return ccc_buf_alloc(&frm->buf_, 0, frm->buf_.alloc_);
    }
    for (void *e = ccc_buf_at(&frm->buf_, 1); e != ccc_buf_end(&frm->buf_);
         e = ccc_buf_next(&frm->buf_, e))
    {
        fn((ccc_user_type){.user_type = e, .aux = frm->buf_.aux_});
    }
    frm->root_ = 0;
    return ccc_buf_alloc(&frm->buf_, 0, frm->buf_.alloc_);
}

bool
ccc_frm_validate(ccc_flat_realtime_ordered_map const *const frm)
{
    return frm ? validate(frm) : false;
}

/*========================  Private Interface  ==============================*/

void *
ccc_impl_frm_insert(struct ccc_fromap_ *const frm, size_t const parent_i,
                    ccc_threeway_cmp const last_cmp, size_t const elem_i)
{
    return insert(frm, parent_i, last_cmp, elem_i);
}

struct ccc_frtree_entry_
ccc_impl_frm_entry(struct ccc_fromap_ const *const frm, void const *const key)
{
    return entry(frm, key);
}

void *
ccc_impl_frm_key_from_node(struct ccc_fromap_ const *const frm,
                           struct ccc_fromap_elem_ const *const elem)
{
    return key_from_node(frm, elem);
}

void *
ccc_impl_frm_key_in_slot(struct ccc_fromap_ const *const frm,
                         void const *const slot)
{
    return (char *)slot + frm->key_offset_;
}

struct ccc_fromap_elem_ *
ccc_impl_frm_elem_in_slot(struct ccc_fromap_ const *const frm,
                          void const *const slot)
{
    return elem_in_slot(frm, slot);
}

void *
ccc_impl_frm_alloc_back(struct ccc_fromap_ *const frm)
{
    return alloc_back(frm);
}

/*==========================  Static Helpers   ==============================*/

static inline void *
maybe_alloc_insert(struct ccc_fromap_ *const frm, size_t const parent,
                   ccc_threeway_cmp const last_cmp,
                   struct ccc_fromap_elem_ *const elem)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    if (!alloc_back(frm))
    {
        return NULL;
    }
    size_t const node = ccc_buf_size(&frm->buf_) - 1;
    (void)ccc_buf_write(&frm->buf_, node, struct_base(frm, elem));
    return insert(frm, parent, last_cmp, node);
}

static inline void *
insert(struct ccc_fromap_ *const frm, size_t const parent_i,
       ccc_threeway_cmp const last_cmp, size_t const elem_i)
{
    struct ccc_fromap_elem_ *elem = at(frm, elem_i);
    init_node(elem);
    if (ccc_buf_size(&frm->buf_) == single_tree_node)
    {
        frm->root_ = elem_i;
        return struct_base(frm, elem);
    }
    assert(last_cmp == CCC_GRT || last_cmp == CCC_LES);
    struct ccc_fromap_elem_ *parent = at(frm, parent_i);
    bool const rank_rule_break = !parent->branch_[L] && !parent->branch_[R];
    parent->branch_[CCC_GRT == last_cmp] = elem_i;
    elem->parent_ = parent_i;
    if (rank_rule_break)
    {
        insert_fixup(frm, parent_i, elem_i);
    }
    return struct_base(frm, elem);
}

static inline struct ccc_frtree_entry_
entry(struct ccc_fromap_ const *const frm, void const *const key)
{
    struct frm_query_ q = find(frm, key);
    if (CCC_EQL == q.last_cmp_)
    {
        return (struct ccc_frtree_entry_){
            .frm_ = (struct ccc_fromap_ *)frm,
            .last_cmp_ = q.last_cmp_,
            .i_ = q.found_,
            .stats_ = CCC_ENTRY_OCCUPIED,
        };
    }
    return (struct ccc_frtree_entry_){
        .frm_ = (struct ccc_fromap_ *)frm,
        .last_cmp_ = q.last_cmp_,
        .i_ = q.parent_,
        .stats_ = CCC_ENTRY_NO_UNWRAP | CCC_ENTRY_VACANT,
    };
}

static inline struct frm_query_
find(struct ccc_fromap_ const *const frm, void const *const key)
{
    size_t parent = 0;
    struct frm_query_ q = {.last_cmp_ = CCC_CMP_ERR, .found_ = frm->root_};
    for (; q.found_; parent = q.found_,
                     q.found_ = branch_i(frm, q.found_, CCC_GRT == q.last_cmp_))
    {
        q.last_cmp_ = cmp_elems(frm, key, q.found_, frm->cmp_);
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
next(struct ccc_fromap_ const *const t, size_t n,
     enum frm_branch_ const traversal)
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

static struct ccc_range_u_
equal_range(struct ccc_fromap_ const *const t, void const *const begin_key,
            void const *const end_key, enum frm_branch_ const traversal)
{
    if (ccc_frm_is_empty(t))
    {
        return (struct ccc_range_u_){};
    }
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    struct frm_query_ b = find(t, begin_key);
    if (b.last_cmp_ == les_or_grt[traversal])
    {
        b.found_ = next(t, b.found_, traversal);
    }
    struct frm_query_ e = find(t, end_key);
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
min_max_from(struct ccc_fromap_ const *const t, size_t start,
             enum frm_branch_ const dir)
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
cmp_elems(struct ccc_fromap_ const *const frm, void const *const key,
          size_t const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){.key_lhs = key,
                            .user_type_rhs = base_at(frm, node),
                            .aux = frm->buf_.aux_});
}

static inline void *
alloc_back(struct ccc_fromap_ *const t)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    if (ccc_buf_is_empty(&t->buf_))
    {
        void *const sentinel = ccc_buf_alloc_back(&t->buf_);
        if (!sentinel)
        {
            return NULL;
        }
        elem_in_slot(t, sentinel)->parity_ = 1;
    }
    return ccc_buf_alloc_back(&t->buf_);
}

static inline void
init_node(struct ccc_fromap_elem_ *const e)
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

static inline struct ccc_fromap_elem_ *
at(struct ccc_fromap_ const *const t, size_t const i)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, i));
}

static inline size_t
branch_i(struct ccc_fromap_ const *const t, size_t const parent,
         enum frm_branch_ const dir)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, parent))->branch_[dir];
}

static inline size_t
parent_i(struct ccc_fromap_ const *const t, size_t const child)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, child))->parent_;
}

static inline size_t
index_of(struct ccc_fromap_ const *const t,
         struct ccc_fromap_elem_ const *const elem)
{
    return ccc_buf_i(&t->buf_, struct_base(t, elem));
}

static inline uint8_t
parity(struct ccc_fromap_ const *t, size_t const node)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parity_;
}

static inline size_t *
branch_r(struct ccc_fromap_ const *t, size_t const node,
         enum frm_branch_ const branch)
{
    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->branch_[branch];
}

static inline size_t *
parent_r(struct ccc_fromap_ const *t, size_t node)
{

    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parent_;
}

static inline void *
base_at(struct ccc_fromap_ const *const frm, size_t const i)
{
    return ccc_buf_at(&frm->buf_, i);
}

static inline void *
struct_base(struct ccc_fromap_ const *const frm,
            struct ccc_fromap_elem_ const *const e)
{
    return ((char *)e->branch_) - frm->node_elem_offset_;
}

static struct ccc_fromap_elem_ *
elem_in_slot(struct ccc_fromap_ const *const t, void const *const slot)
{

    return (struct ccc_fromap_elem_ *)((char *)slot + t->node_elem_offset_);
}

static inline void *
key_from_node(struct ccc_fromap_ const *const t,
              struct ccc_fromap_elem_ const *const elem)
{
    return (char *)struct_base(t, elem) + t->key_offset_;
}

static inline void *
key_at(struct ccc_fromap_ const *const t, size_t const i)
{
    return (char *)ccc_buf_at(&t->buf_, i) + t->key_offset_;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct ccc_fromap_ *const t, size_t z_p_of_xy, size_t x)
{
    do
    {
        promote(t, z_p_of_xy);
        x = z_p_of_xy;
        z_p_of_xy = parent_i(t, z_p_of_xy);
        if (!z_p_of_xy)
        {
            return;
        }
    } while (is_01_parent(t, x, z_p_of_xy, sibling_of(t, x)));

    if (!is_02_parent(t, x, z_p_of_xy, sibling_of(t, x)))
    {
        return;
    }
    assert(x);
    assert(is_0_child(t, z_p_of_xy, x));
    enum frm_branch_ const p_to_x_dir = branch_i(t, z_p_of_xy, R) == x;
    size_t const y = branch_i(t, x, !p_to_x_dir);
    if (!y || is_2_child(t, z_p_of_xy, y))
    {
        rotate(t, z_p_of_xy, x, y, !p_to_x_dir);
        demote(t, z_p_of_xy);
    }
    else
    {
        assert(is_1_child(t, z_p_of_xy, y));
        double_rotate(t, z_p_of_xy, x, y, p_to_x_dir);
        promote(t, y);
        demote(t, x);
        demote(t, z_p_of_xy);
    }
}

static void *
remove_fixup(struct ccc_fromap_ *const t, size_t const remove)
{
    size_t y = 0;
    size_t x = 0;
    size_t p_of_xy = 0;
    bool two_child = false;
    if (!branch_i(t, remove, R) || !branch_i(t, remove, L))
    {
        y = remove;
        p_of_xy = parent_i(t, y);
        x = branch_i(t, y, !branch_i(t, y, L));
        *parent_r(t, x) = parent_i(t, y);
        if (!p_of_xy)
        {
            t->root_ = x;
        }
        two_child = is_2_child(t, p_of_xy, y);
        *branch_r(t, p_of_xy, branch_i(t, p_of_xy, R) == y) = x;
    }
    else
    {
        y = min_max_from(t, branch_i(t, remove, R), min);
        p_of_xy = parent_i(t, y);
        x = branch_i(t, y, !branch_i(t, y, L));
        *parent_r(t, x) = parent_i(t, y);

        /* Save if check and improve readability by assuming this is true. */
        assert(p_of_xy);

        two_child = is_2_child(t, p_of_xy, y);
        *branch_r(t, p_of_xy, branch_i(t, p_of_xy, R) == y) = x;
        transplant(t, remove, y);
        if (remove == p_of_xy)
        {
            p_of_xy = y;
        }
    }

    if (p_of_xy)
    {
        if (two_child)
        {
            assert(p_of_xy);
            rebalance_3_child(t, p_of_xy, x);
        }
        else if (!x && branch_i(t, p_of_xy, L) == branch_i(t, p_of_xy, R))
        {
            assert(p_of_xy);
            bool const demote_makes_3_child
                = is_2_child(t, parent_i(t, p_of_xy), p_of_xy);
            demote(t, p_of_xy);
            if (demote_makes_3_child)
            {
                rebalance_3_child(t, parent_i(t, p_of_xy), p_of_xy);
            }
        }
        assert(!is_leaf(t, p_of_xy) || !parity(t, p_of_xy));
    }
    swap_and_pop(t, remove);
    return base_at(t, ccc_buf_size(&t->buf_));
}

static inline void
transplant(struct ccc_fromap_ *const t, size_t const remove,
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
    struct ccc_fromap_elem_ *const remove_r = at(t, remove);
    struct ccc_fromap_elem_ *const replace_r = at(t, replacement);
    *parent_r(t, remove_r->branch_[R]) = replacement;
    *parent_r(t, remove_r->branch_[L]) = replacement;
    replace_r->branch_[R] = remove_r->branch_[R];
    replace_r->branch_[L] = remove_r->branch_[L];
    replace_r->parity_ = remove_r->parity_;
}

static inline void
rebalance_3_child(struct ccc_fromap_ *const t, size_t z_p_of_xy, size_t x)
{
    assert(z_p_of_xy);
    bool made_3_child = false;
    do
    {
        size_t const p_of_p_of_x = parent_i(t, z_p_of_xy);
        size_t const y = branch_i(t, z_p_of_xy, branch_i(t, z_p_of_xy, L) == x);
        made_3_child = is_2_child(t, p_of_p_of_x, z_p_of_xy);
        if (is_2_child(t, z_p_of_xy, y))
        {
            demote(t, z_p_of_xy);
        }
        else if (is_22_parent(t, branch_i(t, y, L), y, branch_i(t, y, R)))
        {
            demote(t, z_p_of_xy);
            demote(t, y);
        }
        else /* p(x) is 1,3, y is not a 2,2 parent, and x is 3-child.*/
        {
            assert(is_3_child(t, z_p_of_xy, x));
            enum frm_branch_ const z_to_x_dir = branch_i(t, z_p_of_xy, R) == x;
            size_t const w = branch_i(t, y, !z_to_x_dir);
            if (is_1_child(t, y, w))
            {
                rotate(t, z_p_of_xy, y, branch_i(t, y, z_to_x_dir), z_to_x_dir);
                promote(t, y);
                demote(t, z_p_of_xy);
                if (is_leaf(t, z_p_of_xy))
                {
                    demote(t, z_p_of_xy);
                }
            }
            else /* w is a 2-child and v will be a 1-child. */
            {
                size_t const v = branch_i(t, y, z_to_x_dir);
                assert(is_2_child(t, y, w));
                assert(is_1_child(t, y, v));
                double_rotate(t, z_p_of_xy, y, v, !z_to_x_dir);
                double_promote(t, v);
                demote(t, y);
                double_demote(t, z_p_of_xy);
                /* Optional "Rebalancing with Promotion," defined as follows:
                       if node z is a non-leaf 1,1 node, we promote it;
                       otherwise, if y is a non-leaf 1,1 node, we promote it.
                       (See Figure 4.) (Haeupler et. al. 2014, 17).
                   This reduces constants in some of theorems mentioned in the
                   paper but may not be worth doing. Rotations stay at 2 worst
                   case. Should revisit after more performance testing. */
                if (!is_leaf(t, z_p_of_xy)
                    && is_11_parent(t, branch_i(t, z_p_of_xy, L), z_p_of_xy,
                                    branch_i(t, z_p_of_xy, R)))
                {
                    promote(t, z_p_of_xy);
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
        x = z_p_of_xy;
        z_p_of_xy = p_of_p_of_x;
    } while (z_p_of_xy && made_3_child);
}

/** Swaps in the back buffer element into vacated slot*/
static inline void
swap_and_pop(struct ccc_fromap_ *const t, size_t const vacant_i)
{
    (void)ccc_buf_size_minus(&t->buf_, 1);
    size_t const x_i = ccc_buf_size(&t->buf_);
    if (vacant_i == x_i)
    {
        return;
    }
    struct ccc_fromap_elem_ *const x = at(t, x_i);
    assert(vacant_i);
    assert(x_i);
    assert(x);
    if (x_i == t->root_)
    {
        t->root_ = vacant_i;
    }
    else
    {
        struct ccc_fromap_elem_ *const p = at(t, x->parent_);
        p->branch_[p->branch_[R] == x_i] = vacant_i;
    }
    *parent_r(t, x->branch_[R]) = vacant_i;
    *parent_r(t, x->branch_[L]) = vacant_i;
    /* Code may not allocate (i.e Variable Length Array) so 0 slot is tmp. */
    (void)ccc_buf_swap(&t->buf_, base_at(t, 0), vacant_i, x_i);
    at(t, 0)->parity_ = 1;
    /* Clear back elements fields as precaution. */
    x->parity_ = 0;
    x->branch_[L] = x->branch_[R] = x->parent_ = 0;
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
rotate(struct ccc_fromap_ *const t, size_t const z_p_of_x,
       size_t const x_p_of_y, size_t const y, enum frm_branch_ const dir)
{
    assert(z_p_of_x);
    struct ccc_fromap_elem_ *const z_r = at(t, z_p_of_x);
    struct ccc_fromap_elem_ *const x_r = at(t, x_p_of_y);
    size_t const p_of_p_of_x = parent_i(t, z_p_of_x);
    x_r->parent_ = p_of_p_of_x;
    if (!p_of_p_of_x)
    {
        t->root_ = x_p_of_y;
    }
    else
    {
        struct ccc_fromap_elem_ *const g = at(t, p_of_p_of_x);
        g->branch_[g->branch_[R] == z_p_of_x] = x_p_of_y;
    }
    x_r->branch_[dir] = z_p_of_x;
    z_r->parent_ = x_p_of_y;
    z_r->branch_[!dir] = y;
    *parent_r(t, y) = z_p_of_x;
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
double_rotate(struct ccc_fromap_ *const t, size_t const z_p_of_x,
              size_t const x_p_of_y, size_t const y, enum frm_branch_ const dir)
{
    assert(z_p_of_x && x_p_of_y && y);
    struct ccc_fromap_elem_ *const z_r = at(t, z_p_of_x);
    struct ccc_fromap_elem_ *const x_r = at(t, x_p_of_y);
    struct ccc_fromap_elem_ *const y_r = at(t, y);
    size_t const p_of_p_of_x = z_r->parent_;
    y_r->parent_ = p_of_p_of_x;
    if (!p_of_p_of_x)
    {
        t->root_ = y;
    }
    else
    {
        struct ccc_fromap_elem_ *const g = at(t, p_of_p_of_x);
        g->branch_[g->branch_[R] == z_p_of_x] = y;
    }
    x_r->branch_[!dir] = y_r->branch_[dir];
    *parent_r(t, y_r->branch_[dir]) = x_p_of_y;
    y_r->branch_[dir] = x_p_of_y;
    x_r->parent_ = y;

    z_r->branch_[dir] = y_r->branch_[!dir];
    *parent_r(t, y_r->branch_[!dir]) = z_p_of_x;
    y_r->branch_[!dir] = z_p_of_x;
    z_r->parent_ = y;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
      0╭─╯
       x */
[[maybe_unused]] static inline bool
is_0_child(struct ccc_fromap_ const *const t, size_t const p_of_x,
           size_t const x)
{
    return p_of_x && parity(t, p_of_x) == parity(t, x);
}

/* Returns true for rank difference 1 between the parent and node.
         p
      1╭─╯
       x */
static inline bool
is_1_child(struct ccc_fromap_ const *const t, size_t const p_of_x,
           size_t const x)
{
    return p_of_x && parity(t, p_of_x) != parity(t, x);
}

/* Returns true for rank difference 2 between the parent and node.
         p
      2╭─╯
       x */
static inline bool
is_2_child(struct ccc_fromap_ const *const t, size_t const p_of_x,
           size_t const x)
{
    return p_of_x && parity(t, p_of_x) == parity(t, x);
}

/* Returns true for rank difference 3 between the parent and node.
         p
      3╭─╯
       x */
[[maybe_unused]] static inline bool
is_3_child(struct ccc_fromap_ const *const t, size_t const p_of_x,
           size_t const x)
{
    return p_of_x && parity(t, p_of_x) != parity(t, x);
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮1
       x   y */
static inline bool
is_01_parent(struct ccc_fromap_ const *const t, size_t const x,
             size_t const p_of_xy, size_t const y)
{
    assert(p_of_xy);
    return (!parity(t, x) && !parity(t, p_of_xy) && parity(t, y))
           || (parity(t, x) && parity(t, p_of_xy) && !parity(t, y));
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
      1╭─┴─╮1
       x   y */
static inline bool
is_11_parent(struct ccc_fromap_ const *const t, size_t const x,
             size_t const p_of_xy, size_t const y)
{
    assert(p_of_xy);
    return (!parity(t, x) && parity(t, p_of_xy) && !parity(t, y))
           || (parity(t, x) && !parity(t, p_of_xy) && parity(t, y));
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮2
       x   y */
static inline bool
is_02_parent(struct ccc_fromap_ const *const t, size_t const x,
             size_t const p_of_xy, size_t const y)
{
    assert(p_of_xy);
    return (parity(t, x) == parity(t, p_of_xy))
           && (parity(t, p_of_xy) == parity(t, y));
}

/* Returns true if a parent is a 2,2 or 2,2 node, which is allowed. 2,2 nodes
   are allowed in a WAVL tree but the absence of any 2,2 nodes is the exact
   equivalent of a normal AVL tree which can occur if only insertions occur
   for a WAVL tree. Either child may be the sentinel node which has a parity of
   1 and rank -1.
         p
      2╭─┴─╮2
       x   y */
static inline bool
is_22_parent(struct ccc_fromap_ const *const t, size_t const x,
             size_t const p_of_xy, size_t const y)
{
    assert(p_of_xy);
    return (parity(t, x) == parity(t, p_of_xy))
           && (parity(t, p_of_xy) == parity(t, y));
}

static inline void
promote(struct ccc_fromap_ const *const t, size_t const x)
{
    if (x)
    {
        at(t, x)->parity_ = !parity(t, x);
    }
}

static inline void
demote(struct ccc_fromap_ const *const t, size_t const x)
{
    promote(t, x);
}

static inline void
double_promote(struct ccc_fromap_ const *const, size_t const)
{}

static inline void
double_demote(struct ccc_fromap_ const *const, size_t const)
{}

static inline bool
is_leaf(struct ccc_fromap_ const *const t, size_t const x)
{
    return !branch_i(t, x, L) && !branch_i(t, x, R);
}

static inline size_t
sibling_of(struct ccc_fromap_ const *const t, size_t const x)
{
    size_t const p = parent_i(t, x);
    assert(p);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return at(t, p)->branch_[branch_i(t, p, L) == x];
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
recursive_size(struct ccc_fromap_ const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_size(t, branch_i(t, r, R))
           + recursive_size(t, branch_i(t, r, L));
}

static bool
are_subtrees_valid(struct ccc_fromap_ const *t, struct tree_range_ const r)
{
    if (!r.root)
    {
        return true;
    }
    if (r.low && cmp_elems(t, key_at(t, r.low), r.root, t->cmp_) != CCC_LES)
    {
        return false;
    }
    if (r.high && cmp_elems(t, key_at(t, r.high), r.root, t->cmp_) != CCC_GRT)
    {
        return false;
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

static bool
is_storing_parent(struct ccc_fromap_ const *const t, size_t const p,
                  size_t const root)
{
    if (!root)
    {
        return true;
    }
    if (parent_i(t, root) != p)
    {
        return false;
    }
    return is_storing_parent(t, root, branch_i(t, root, L))
           && is_storing_parent(t, root, branch_i(t, root, R));
}

static bool
validate(struct ccc_fromap_ const *const frm)
{
    if (!ccc_buf_is_empty(&frm->buf_) && !at(frm, 0)->parity_)
    {
        return false;
    }
    if (!are_subtrees_valid(frm, (struct tree_range_){.root = frm->root_}))
    {
        return false;
    }
    size_t const size = recursive_size(frm, frm->root_);
    if (size && size != ccc_buf_size(&frm->buf_) - 1)
    {
        return false;
    }
    if (!is_storing_parent(frm, 0, frm->root_))
    {
        return false;
    }
    return true;
}

/* NOLINTEND(*misc-no-recursion) */
