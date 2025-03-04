/** This file implements a splay tree that does not support duplicates.
The code to support a splay tree that does not allow duplicates is much simpler
than the code to support a multimap implementation. This implementation is
based on the following source.

    1. Daniel Sleator, Carnegie Mellon University. Sleator's implementation of a
       topdown splay tree was instrumental in starting things off, but required
       extensive modification. I had to update parent and child tracking, and
       unite the left and right cases for fun. See the code for a generalizable
       strategy to eliminate symmetric left and right cases for any binary tree
       code. https:www.link.cs.cmu.edulinkftp-sitesplayingtop-down-splay.c */
#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "flat_ordered_map.h"
#include "impl/impl_flat_ordered_map.h"
#include "impl/impl_types.h"
#include "types.h"

/** @private */
#define LR 2

/** @private */
enum fom_branch_
{
    L = 0,
    R,
};

static enum fom_branch_ const inorder_traversal = R;
static enum fom_branch_ const reverse_inorder_traversal = L;

static enum fom_branch_ const min = L;
static enum fom_branch_ const max = R;

/* Buffer allocates before insert. "Empty" has nil 0th slot and one more. */
static size_t const empty_tree = 2;

/*==============================  Prototypes   ==============================*/

/* Returning the internal elem type with stored offsets. */
static size_t splay(struct ccc_fomap_ *t, size_t root, void const *key,
                    ccc_key_cmp_fn *cmp_fn);
static struct ccc_fomap_elem_ *at(struct ccc_fomap_ const *, size_t);
static struct ccc_fomap_elem_ *elem_in_slot(struct ccc_fomap_ const *t,
                                            void const *slot);
/* Returning the user struct type with stored offsets. */
static struct ccc_ftree_entry_ entry(struct ccc_fomap_ *fom, void const *key);
static void *erase(struct ccc_fomap_ *t, void const *key);
static void *maybe_alloc_insert(struct ccc_fomap_ *fom,
                                struct ccc_fomap_elem_ *elem);
static void *find(struct ccc_fomap_ *, void const *key);
static void *connect_new_root(struct ccc_fomap_ *t, size_t new_root,
                              ccc_threeway_cmp cmp_result);
static void *struct_base(struct ccc_fomap_ const *,
                         struct ccc_fomap_elem_ const *);
static void *insert(struct ccc_fomap_ *t, size_t n);
static void *base_at(struct ccc_fomap_ const *, size_t);
static void *alloc_back(struct ccc_fomap_ *t);
static struct ccc_range_u_ equal_range(struct ccc_fomap_ *t,
                                       void const *begin_key,
                                       void const *end_key,
                                       enum fom_branch_ traversal);
/* Returning the user key with stored offsets. */
static void *key_from_node(struct ccc_fomap_ const *t,
                           struct ccc_fomap_elem_ const *);
static void *key_at(struct ccc_fomap_ const *t, size_t i);
/* Returning threeway comparison with user callback. */
static ccc_threeway_cmp cmp_elems(struct ccc_fomap_ const *fom, void const *key,
                                  size_t node, ccc_key_cmp_fn *fn);
/* Returning read only indices for tree nodes. */
static size_t remove_from_tree(struct ccc_fomap_ *t, size_t ret);
static size_t min_max_from(struct ccc_fomap_ const *t, size_t start,
                           enum fom_branch_ dir);
static size_t next(struct ccc_fomap_ const *t, size_t n,
                   enum fom_branch_ traversal);
static size_t branch_i(struct ccc_fomap_ const *t, size_t parent,
                       enum fom_branch_ dir);
static size_t parent_i(struct ccc_fomap_ const *t, size_t child);
static size_t index_of(struct ccc_fomap_ const *t,
                       struct ccc_fomap_elem_ const *elem);
/* Returning references to index fields for tree nodes. */
static size_t *branch_ref(struct ccc_fomap_ const *t, size_t node,
                          enum fom_branch_ branch);
static size_t *parent_ref(struct ccc_fomap_ const *t, size_t node);

static bool validate(struct ccc_fomap_ const *fom);

/* Returning void as miscellaneous helpers. */
static void swap_and_pop(struct ccc_fomap_ *t, size_t vacant_i);
static void init_node(struct ccc_fomap_elem_ *e);
static void swap(char tmp[], void *a, void *b, size_t elem_sz);
static void link_trees(struct ccc_fomap_ *t, size_t parent,
                       enum fom_branch_ dir, size_t subtree);

/*==============================  Interface    ==============================*/

bool
ccc_fom_contains(ccc_flat_ordered_map *const fom, void const *const key)
{
    if (!fom || !key)
    {
        return false;
    }
    fom->root_ = splay(fom, fom->root_, key, fom->cmp_);
    return cmp_elems(fom, key, fom->root_, fom->cmp_) == CCC_EQL;
}

void *
ccc_fom_get_key_val(ccc_flat_ordered_map *const fom, void const *const key)
{
    if (!fom || !key)
    {
        return NULL;
    }
    return find(fom, key);
}

ccc_fomap_entry
ccc_fom_entry(ccc_flat_ordered_map *const fom, void const *const key)
{
    if (!fom || !key)
    {
        return (ccc_fomap_entry){{.stats_ = CCC_INPUT_ERROR}};
    }
    return (ccc_fomap_entry){entry(fom, key)};
}

void *
ccc_fom_insert_entry(ccc_fomap_entry const *const e, ccc_fomap_elem *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->impl_.stats_ == CCC_OCCUPIED)
    {
        *elem = *at(e->impl_.fom_, e->impl_.i_);
        void *const ret = base_at(e->impl_.fom_, e->impl_.i_);
        void const *const e_base = struct_base(e->impl_.fom_, elem);
        if (e_base != ret)
        {
            memcpy(ret, e_base, ccc_buf_elem_size(&e->impl_.fom_->buf_));
        }
        return ret;
    }
    return maybe_alloc_insert(e->impl_.fom_, elem);
}

ccc_fomap_entry *
ccc_fom_and_modify(ccc_fomap_entry *const e, ccc_update_fn *const fn)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->impl_.stats_ & CCC_OCCUPIED)
    {
        fn((ccc_user_type){
            .user_type = base_at(e->impl_.fom_, e->impl_.i_),
            .aux = NULL,
        });
    }
    return e;
}

ccc_fomap_entry *
ccc_fom_and_modify_aux(ccc_fomap_entry *const e, ccc_update_fn *const fn,
                       void *const aux)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->impl_.stats_ & CCC_OCCUPIED)
    {
        fn((ccc_user_type){
            .user_type = base_at(e->impl_.fom_, e->impl_.i_),
            .aux = aux,
        });
    }
    return e;
}

void *
ccc_fom_or_insert(ccc_fomap_entry const *const e, ccc_fomap_elem *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->impl_.stats_ & CCC_OCCUPIED)
    {
        return base_at(e->impl_.fom_, e->impl_.i_);
    }
    return maybe_alloc_insert(e->impl_.fom_, elem);
}

ccc_entry
ccc_fom_swap_entry(ccc_flat_ordered_map *const fom,
                   ccc_fomap_elem *const out_handle)
{
    if (!fom || !out_handle)
    {
        return (ccc_entry){{.stats_ = CCC_INPUT_ERROR}};
    }
    void *const found = find(fom, key_from_node(fom, out_handle));
    if (found)
    {
        assert(fom->root_);
        *out_handle = *at(fom, fom->root_);
        void *const user_struct = struct_base(fom, out_handle);
        void *const ret = base_at(fom, fom->root_);
        void *const tmp = ccc_buf_at(&fom->buf_, 0);
        swap(tmp, user_struct, ret, ccc_buf_elem_size(&fom->buf_));
        return (ccc_entry){{.e_ = user_struct, .stats_ = CCC_OCCUPIED}};
    }
    void *const inserted = maybe_alloc_insert(fom, out_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_VACANT}};
}

ccc_entry
ccc_fom_try_insert(ccc_flat_ordered_map *const fom,
                   ccc_fomap_elem *const key_val_handle)
{
    if (!fom || !key_val_handle)
    {
        return (ccc_entry){{.stats_ = CCC_INPUT_ERROR}};
    }
    void *const found = find(fom, key_from_node(fom, key_val_handle));
    if (found)
    {
        assert(fom->root_);
        return (ccc_entry){{.e_ = found, .stats_ = CCC_OCCUPIED}};
    }
    void *const inserted = maybe_alloc_insert(fom, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_VACANT}};
}

ccc_entry
ccc_fom_insert_or_assign(ccc_flat_ordered_map *const fom,
                         ccc_fomap_elem *const key_val_handle)
{
    if (!fom || !key_val_handle)
    {
        return (ccc_entry){{.stats_ = CCC_INPUT_ERROR}};
    }
    void *const found = find(fom, key_from_node(fom, key_val_handle));
    if (found)
    {
        *key_val_handle = *elem_in_slot(fom, found);
        assert(fom->root_);
        void const *const e_base = struct_base(fom, key_val_handle);
        if (e_base != found)
        {
            memcpy(found, e_base, ccc_buf_elem_size(&fom->buf_));
        }
        return (ccc_entry){{.e_ = found, .stats_ = CCC_OCCUPIED}};
    }
    void *const inserted = maybe_alloc_insert(fom, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_VACANT}};
}

ccc_entry
ccc_fom_remove(ccc_flat_ordered_map *const fom,
               ccc_fomap_elem *const out_handle)
{
    if (!fom || !out_handle)
    {
        return (ccc_entry){{.stats_ = CCC_INPUT_ERROR}};
    }
    void *const n = erase(fom, key_from_node(fom, out_handle));
    if (!n)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_VACANT}};
    }
    return (ccc_entry){{.e_ = n, .stats_ = CCC_OCCUPIED}};
}

ccc_entry
ccc_fom_remove_entry(ccc_fomap_entry *const e)
{
    if (!e)
    {
        return (ccc_entry){{.stats_ = CCC_INPUT_ERROR}};
    }
    if (e->impl_.stats_ == CCC_OCCUPIED)
    {
        void *const erased
            = erase(e->impl_.fom_, key_at(e->impl_.fom_, e->impl_.i_));
        assert(erased);
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_VACANT}};
}

void *
ccc_fom_unwrap(ccc_fomap_entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->impl_.stats_ == CCC_OCCUPIED ? base_at(e->impl_.fom_, e->impl_.i_)
                                           : NULL;
}

bool
ccc_fom_insert_error(ccc_fomap_entry const *const e)
{
    return e ? e->impl_.stats_ & CCC_INSERT_ERROR : false;
}

bool
ccc_fom_occupied(ccc_fomap_entry const *const e)
{
    return e ? e->impl_.stats_ & CCC_OCCUPIED : false;
}

ccc_entry_status
ccc_fom_entry_status(ccc_fomap_entry const *const e)
{
    return e ? e->impl_.stats_ : CCC_INPUT_ERROR;
}

bool
ccc_fom_is_empty(ccc_flat_ordered_map const *const fom)
{
    return !ccc_fom_size(fom);
}

size_t
ccc_fom_size(ccc_flat_ordered_map const *const fom)
{
    if (!fom)
    {
        return 0;
    }
    size_t const sz = ccc_buf_size(&fom->buf_);
    return !sz ? sz : sz - 1;
}

size_t
ccc_fom_capacity(ccc_flat_ordered_map const *const fom)
{
    if (!fom)
    {
        return 0;
    }
    return ccc_buf_capacity(&fom->buf_);
}

void *
ccc_fom_begin(ccc_flat_ordered_map const *const fom)
{
    if (!fom || ccc_buf_is_empty(&fom->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(fom, fom->root_, min);
    return base_at(fom, n);
}

void *
ccc_fom_rbegin(ccc_flat_ordered_map const *const fom)
{
    if (!fom || ccc_buf_is_empty(&fom->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(fom, fom->root_, max);
    return base_at(fom, n);
}

void *
ccc_fom_next(ccc_flat_ordered_map const *const fom,
             ccc_fomap_elem const *const e)
{
    if (!fom || ccc_buf_is_empty(&fom->buf_))
    {
        return NULL;
    }
    size_t const n = next(fom, index_of(fom, e), inorder_traversal);
    return base_at(fom, n);
}

void *
ccc_fom_rnext(ccc_flat_ordered_map const *const fom,
              ccc_fomap_elem const *const e)
{
    if (!fom || !e || ccc_buf_is_empty(&fom->buf_))
    {
        return NULL;
    }
    size_t const n = next(fom, index_of(fom, e), reverse_inorder_traversal);
    return base_at(fom, n);
}

void *
ccc_fom_end(ccc_flat_ordered_map const *const fom)
{
    if (!fom || ccc_buf_is_empty(&fom->buf_))
    {
        return NULL;
    }
    return base_at(fom, 0);
}

void *
ccc_fom_rend(ccc_flat_ordered_map const *const fom)
{
    if (!fom || ccc_buf_is_empty(&fom->buf_))
    {
        return NULL;
    }
    return base_at(fom, 0);
}

void *
ccc_fom_data(ccc_flat_ordered_map const *const fom)
{
    return fom ? ccc_buf_begin(&fom->buf_) : NULL;
}

ccc_range
ccc_fom_equal_range(ccc_flat_ordered_map *const fom,
                    void const *const begin_key, void const *const end_key)
{
    if (!fom || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(fom, begin_key, end_key, inorder_traversal)};
}

ccc_rrange
ccc_fom_equal_rrange(ccc_flat_ordered_map *const fom,
                     void const *const rbegin_key, void const *const rend_key)

{
    if (!fom || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){
        equal_range(fom, rbegin_key, rend_key, reverse_inorder_traversal)};
}

ccc_result
ccc_fom_copy(ccc_flat_ordered_map *const dst,
             ccc_flat_ordered_map const *const src, ccc_alloc_fn *const fn)
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
ccc_fom_clear(ccc_flat_ordered_map *const fom, ccc_destructor_fn *const fn)
{
    if (!fom)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        (void)ccc_buf_size_set(&fom->buf_, 1);
        fom->root_ = 0;
        return CCC_OK;
    }
    for (void *e = ccc_buf_at(&fom->buf_, 1); e != ccc_buf_end(&fom->buf_);
         e = ccc_buf_next(&fom->buf_, e))
    {
        fn((ccc_user_type){.user_type = e, .aux = fom->buf_.aux_});
    }
    (void)ccc_buf_size_set(&fom->buf_, 1);
    fom->root_ = 0;
    return CCC_OK;
}

ccc_result
ccc_fom_clear_and_free(ccc_flat_ordered_map *const fom,
                       ccc_destructor_fn *const fn)
{
    if (!fom)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        fom->root_ = 0;
        return ccc_buf_alloc(&fom->buf_, 0, fom->buf_.alloc_);
    }
    for (void *e = ccc_buf_at(&fom->buf_, 1); e != ccc_buf_end(&fom->buf_);
         e = ccc_buf_next(&fom->buf_, e))
    {
        fn((ccc_user_type){.user_type = e, .aux = fom->buf_.aux_});
    }
    fom->root_ = 0;
    return ccc_buf_alloc(&fom->buf_, 0, fom->buf_.alloc_);
}

bool
ccc_fom_validate(ccc_flat_ordered_map const *const fom)
{
    return fom ? validate(fom) : false;
}

/*===========================   Private Interface ===========================*/

void *
ccc_impl_fom_insert(struct ccc_fomap_ *const fom, size_t const elem_i)
{
    return insert(fom, elem_i);
}

struct ccc_ftree_entry_
ccc_impl_fom_entry(struct ccc_fomap_ *const fom, void const *const key)
{
    return entry(fom, key);
}

void *
ccc_impl_fom_key_from_node(struct ccc_fomap_ const *const fom,
                           struct ccc_fomap_elem_ const *const elem)
{
    return key_from_node(fom, elem);
}

void *
ccc_impl_fom_key_in_slot(struct ccc_fomap_ const *const fom,
                         void const *const slot)
{
    return (char *)slot + fom->key_offset_;
}

struct ccc_fomap_elem_ *
ccc_impl_fomap_elem_in_slot(struct ccc_fomap_ const *const fom,
                            void const *const slot)
{
    return elem_in_slot(fom, slot);
}

void *
ccc_impl_fom_alloc_back(struct ccc_fomap_ *const fom)
{
    return alloc_back(fom);
}

/*===========================   Static Helpers    ===========================*/

static inline struct ccc_range_u_
equal_range(struct ccc_fomap_ *const t, void const *const begin_key,
            void const *const end_key, enum fom_branch_ const traversal)
{
    if (ccc_fom_is_empty(t))
    {
        return (struct ccc_range_u_){};
    }
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    size_t b = splay(t, t->root_, begin_key, t->cmp_);
    if (cmp_elems(t, begin_key, b, t->cmp_) == les_or_grt[traversal])
    {
        b = next(t, b, traversal);
    }
    size_t e = splay(t, t->root_, end_key, t->cmp_);
    if (cmp_elems(t, end_key, e, t->cmp_) != les_or_grt[!traversal])
    {
        e = next(t, e, traversal);
    }
    return (struct ccc_range_u_){
        .begin_ = base_at(t, b),
        .end_ = base_at(t, e),
    };
}

static inline struct ccc_ftree_entry_
entry(struct ccc_fomap_ *const fom, void const *const key)
{
    void *const found = find(fom, key);
    if (found)
    {
        return (struct ccc_ftree_entry_){
            .fom_ = fom,
            .i_ = ccc_buf_i(&fom->buf_, found),
            .stats_ = CCC_OCCUPIED,
        };
    }
    return (struct ccc_ftree_entry_){
        .fom_ = fom,
        .i_ = 0,
        .stats_ = CCC_VACANT,
    };
}

static void *
maybe_alloc_insert(struct ccc_fomap_ *const fom,
                   struct ccc_fomap_elem_ *const elem)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    if (!alloc_back(fom))
    {
        return NULL;
    }
    size_t const node = ccc_buf_size(&fom->buf_) - 1;
    (void)ccc_buf_write(&fom->buf_, node, struct_base(fom, elem));
    return insert(fom, node);
}

static inline void *
insert(struct ccc_fomap_ *const t, size_t const n)
{
    struct ccc_fomap_elem_ *const node = at(t, n);
    init_node(node);
    if (ccc_buf_size(&t->buf_) == empty_tree)
    {
        t->root_ = n;
        return struct_base(t, node);
    }
    void const *const key = key_from_node(t, node);
    t->root_ = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const root_cmp = cmp_elems(t, key, t->root_, t->cmp_);
    if (CCC_EQL == root_cmp)
    {
        return NULL;
    }
    return connect_new_root(t, n, root_cmp);
}

static inline void *
erase(struct ccc_fomap_ *const t, void const *const key)
{
    if (ccc_fom_is_empty(t))
    {
        return NULL;
    }
    size_t ret = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const found = cmp_elems(t, key, ret, t->cmp_);
    if (found != CCC_EQL)
    {
        return NULL;
    }
    ret = remove_from_tree(t, ret);
    swap_and_pop(t, ret);
    return base_at(t, ccc_buf_size(&t->buf_));
}

/** Swaps in the back buffer element into vacated slot*/
static inline void
swap_and_pop(struct ccc_fomap_ *const t, size_t const vacant_i)
{
    (void)ccc_buf_size_minus(&t->buf_, 1);
    size_t const x_i = ccc_buf_size(&t->buf_);
    if (vacant_i == x_i)
    {
        return;
    }
    struct ccc_fomap_elem_ *const x = at(t, x_i);
    assert(vacant_i);
    assert(x_i);
    assert(x);
    if (x_i == t->root_)
    {
        t->root_ = vacant_i;
    }
    else
    {
        struct ccc_fomap_elem_ *const p = at(t, x->parent_);
        p->branch_[p->branch_[R] == x_i] = vacant_i;
    }
    *parent_ref(t, x->branch_[R]) = vacant_i;
    *parent_ref(t, x->branch_[L]) = vacant_i;
    /* Code may not allocate (i.e Variable Length Array) so 0 slot is tmp. */
    (void)ccc_buf_swap(&t->buf_, base_at(t, 0), vacant_i, x_i);
    /* Clear out the back elements fields just as precaution. */
    x->branch_[L] = x->branch_[R] = x->parent_ = 0;
}

static inline size_t
remove_from_tree(struct ccc_fomap_ *const t, size_t const ret)
{
    if (!branch_i(t, ret, L))
    {
        t->root_ = branch_i(t, ret, R);
        link_trees(t, 0, 0, t->root_);
    }
    else
    {
        t->root_ = splay(t, branch_i(t, ret, L), key_from_node(t, at(t, ret)),
                         t->cmp_);
        link_trees(t, t->root_, R, branch_i(t, ret, R));
    }
    return ret;
}

static inline void *
connect_new_root(struct ccc_fomap_ *const t, size_t const new_root,
                 ccc_threeway_cmp const cmp_result)
{
    enum fom_branch_ const link = CCC_GRT == cmp_result;
    link_trees(t, new_root, link, branch_i(t, t->root_, link));
    link_trees(t, new_root, !link, t->root_);
    *branch_ref(t, t->root_, link) = 0;
    t->root_ = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link_trees(t, 0, 0, t->root_);
    return base_at(t, new_root);
}

static inline void *
find(struct ccc_fomap_ *const t, void const *const key)
{
    if (!t->root_)
    {
        return NULL;
    }
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp_elems(t, key, t->root_, t->cmp_) == CCC_EQL
               ? base_at(t, t->root_)
               : NULL;
}

static inline size_t
splay(struct ccc_fomap_ *const t, size_t root, void const *const key,
      ccc_key_cmp_fn *const cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    struct ccc_fomap_elem_ *const nil = at(t, 0);
    nil->branch_[L] = nil->branch_[R] = nil->parent_ = 0;
    size_t l_r_subtrees[LR] = {0, 0};
    do
    {
        ccc_threeway_cmp const root_cmp = cmp_elems(t, key, root, cmp_fn);
        enum fom_branch_ const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || !branch_i(t, root, dir))
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp_elems(t, key, branch_i(t, root, dir), cmp_fn);
        enum fom_branch_ const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            size_t const pivot = branch_i(t, root, dir);
            link_trees(t, root, dir, branch_i(t, pivot, !dir));
            link_trees(t, pivot, !dir, root);
            root = pivot;
            if (!branch_i(t, root, dir))
            {
                break;
            }
        }
        link_trees(t, l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = branch_i(t, root, dir);
    } while (true);
    link_trees(t, l_r_subtrees[L], R, branch_i(t, root, L));
    link_trees(t, l_r_subtrees[R], L, branch_i(t, root, R));
    link_trees(t, root, L, nil->branch_[R]);
    link_trees(t, root, R, nil->branch_[L]);
    t->root_ = root;
    link_trees(t, 0, 0, t->root_);
    return root;
}

static inline void
link_trees(struct ccc_fomap_ *const t, size_t const parent,
           enum fom_branch_ const dir, size_t const subtree)
{
    *branch_ref(t, parent, dir) = subtree;
    *parent_ref(t, subtree) = parent;
}

static inline size_t
min_max_from(struct ccc_fomap_ const *const t, size_t start,
             enum fom_branch_ const dir)
{
    if (!start)
    {
        return 0;
    }
    for (; branch_i(t, start, dir); start = branch_i(t, start, dir))
    {}
    return start;
}

static inline size_t
next(struct ccc_fomap_ const *const t, size_t n,
     enum fom_branch_ const traversal)
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

static inline ccc_threeway_cmp
cmp_elems(struct ccc_fomap_ const *const fom, void const *const key,
          size_t const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){.key_lhs = key,
                            .user_type_rhs = base_at(fom, node),
                            .aux = fom->buf_.aux_});
}

static inline void *
alloc_back(struct ccc_fomap_ *const t)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    if (ccc_buf_is_empty(&t->buf_) && !ccc_buf_alloc_back(&t->buf_))
    {
        return NULL;
    }
    return ccc_buf_alloc_back(&t->buf_);
}

static inline void
init_node(struct ccc_fomap_elem_ *const e)
{
    assert(e != NULL);
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

static inline struct ccc_fomap_elem_ *
at(struct ccc_fomap_ const *const t, size_t const i)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, i));
}

static inline size_t
branch_i(struct ccc_fomap_ const *const t, size_t const parent,
         enum fom_branch_ const dir)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, parent))->branch_[dir];
}

static inline size_t
parent_i(struct ccc_fomap_ const *const t, size_t const child)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, child))->parent_;
}

static inline size_t
index_of(struct ccc_fomap_ const *const t,
         struct ccc_fomap_elem_ const *const elem)
{
    return ccc_buf_i(&t->buf_, struct_base(t, elem));
}

static inline size_t *
branch_ref(struct ccc_fomap_ const *t, size_t const node,
           enum fom_branch_ const branch)
{
    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->branch_[branch];
}

static inline size_t *
parent_ref(struct ccc_fomap_ const *t, size_t node)
{
    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parent_;
}

static inline void *
base_at(struct ccc_fomap_ const *const fom, size_t const i)
{
    return ccc_buf_at(&fom->buf_, i);
}

static inline void *
struct_base(struct ccc_fomap_ const *const fom,
            struct ccc_fomap_elem_ const *const e)
{
    return ((char *)e->branch_) - fom->node_elem_offset_;
}

static struct ccc_fomap_elem_ *
elem_in_slot(struct ccc_fomap_ const *const t, void const *const slot)
{
    return (struct ccc_fomap_elem_ *)((char *)slot + t->node_elem_offset_);
}

static inline void *
key_from_node(struct ccc_fomap_ const *const t,
              struct ccc_fomap_elem_ const *const elem)
{
    return (char *)struct_base(t, elem) + t->key_offset_;
}

static inline void *
key_at(struct ccc_fomap_ const *const t, size_t const i)
{
    return (char *)ccc_buf_at(&t->buf_, i) + t->key_offset_;
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
recursive_size(struct ccc_fomap_ const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_size(t, branch_i(t, r, R))
           + recursive_size(t, branch_i(t, r, L));
}

static bool
are_subtrees_valid(struct ccc_fomap_ const *t, struct tree_range_ const r)
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
is_storing_parent(struct ccc_fomap_ const *const t, size_t const p,
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
validate(struct ccc_fomap_ const *const fom)
{
    if (!are_subtrees_valid(fom, (struct tree_range_){.root = fom->root_}))
    {
        return false;
    }
    size_t const size = recursive_size(fom, fom->root_);
    if (size && size != ccc_buf_size(&fom->buf_) - 1)
    {
        return false;
    }
    if (!is_storing_parent(fom, 0, fom->root_))
    {
        return false;
    }
    return true;
}

/* NOLINTEND(*misc-no-recursion) */
