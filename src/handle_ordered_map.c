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
#include "handle_ordered_map.h"
#include "impl/impl_handle_ordered_map.h"
#include "impl/impl_types.h"
#include "types.h"

/** @private */
#define LR 2

/** @private */
enum hom_branch_
{
    L = 0,
    R,
};

static enum hom_branch_ const inorder_traversal = R;
static enum hom_branch_ const reverse_inorder_traversal = L;

/* Buffer allocates before insert. "Empty" has nil 0th slot and one more. */
static size_t const empty_tree = 2;

/*==============================  Prototypes   ==============================*/

/* Returning the internal elem type with stored offsets. */
static size_t splay(struct ccc_homap_ *t, size_t root, void const *key,
                    ccc_key_cmp_fn *cmp_fn);
static struct ccc_homap_elem_ *at(struct ccc_homap_ const *, size_t);
static struct ccc_homap_elem_ *elem_in_slot(struct ccc_homap_ const *t,
                                            void const *slot);
/* Returning the user struct type with stored offsets. */
static struct ccc_htree_handle_ handle(struct ccc_homap_ *hom, void const *key);
static size_t erase(struct ccc_homap_ *t, void const *key);
static size_t maybe_alloc_insert(struct ccc_homap_ *hom,
                                 struct ccc_homap_elem_ *elem);
static size_t find(struct ccc_homap_ *, void const *key);
static size_t connect_new_root(struct ccc_homap_ *t, size_t new_root,
                               ccc_threeway_cmp cmp_result);
static void *struct_base(struct ccc_homap_ const *,
                         struct ccc_homap_elem_ const *);
static void insert(struct ccc_homap_ *t, size_t n);
static void *base_at(struct ccc_homap_ const *, size_t);
static size_t alloc_slot(struct ccc_homap_ *t);
static struct ccc_range_u_ equal_range(struct ccc_homap_ *t,
                                       void const *begin_key,
                                       void const *end_key,
                                       enum hom_branch_ traversal);
/* Returning the user key with stored offsets. */
static void *key_from_node(struct ccc_homap_ const *t,
                           struct ccc_homap_elem_ const *);
static void *key_at(struct ccc_homap_ const *t, size_t i);
/* Returning threeway comparison with user callback. */
static ccc_threeway_cmp cmp_elems(struct ccc_homap_ const *hom, void const *key,
                                  size_t node, ccc_key_cmp_fn *fn);
/* Returning read only indices for tree nodes. */
static size_t remove_from_tree(struct ccc_homap_ *t, size_t ret);
static size_t min_max_from(struct ccc_homap_ const *t, size_t start,
                           enum hom_branch_ dir);
static size_t next(struct ccc_homap_ const *t, size_t n,
                   enum hom_branch_ traversal);
static size_t branch_i(struct ccc_homap_ const *t, size_t parent,
                       enum hom_branch_ dir);
static size_t parent_i(struct ccc_homap_ const *t, size_t child);
static size_t index_of(struct ccc_homap_ const *t,
                       struct ccc_homap_elem_ const *elem);
/* Returning references to index fields for tree nodes. */
static size_t *branch_ref(struct ccc_homap_ const *t, size_t node,
                          enum hom_branch_ branch);
static size_t *parent_ref(struct ccc_homap_ const *t, size_t node);

static bool validate(struct ccc_homap_ const *hom);

/* Returning void as miscellaneous helpers. */
static void init_node(struct ccc_homap_elem_ *e);
static void swap(char tmp[], void *a, void *b, size_t elem_sz);
static void link(struct ccc_homap_ *t, size_t parent, enum hom_branch_ dir,
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
    return ccc_buf_at(&h->buf_, i);
}

bool
ccc_hom_contains(ccc_handle_ordered_map *const hom, void const *const key)
{
    if (!hom || !key)
    {
        return false;
    }
    hom->root_ = splay(hom, hom->root_, key, hom->cmp_);
    return cmp_elems(hom, key, hom->root_, hom->cmp_) == CCC_EQL;
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
        return (ccc_homap_handle){{.handle_ = {.stats_ = CCC_INPUT_ERROR}}};
    }
    return (ccc_homap_handle){handle(hom, key)};
}

ccc_handle_i
ccc_hom_insert_handle(ccc_homap_handle const *const h,
                      ccc_homap_elem *const elem)
{
    if (!h || !elem)
    {
        return 0;
    }
    if (h->impl_.handle_.stats_ == CCC_OCCUPIED)
    {
        *elem = *at(h->impl_.hom_, h->impl_.handle_.i_);
        void *const ret = base_at(h->impl_.hom_, h->impl_.handle_.i_);
        void const *const e_base = struct_base(h->impl_.hom_, elem);
        if (e_base != ret)
        {
            memcpy(ret, e_base, ccc_buf_elem_size(&h->impl_.hom_->buf_));
        }
        return h->impl_.handle_.i_;
    }
    return maybe_alloc_insert(h->impl_.hom_, elem);
}

ccc_homap_handle *
ccc_hom_and_modify(ccc_homap_handle *const h, ccc_update_fn *const fn)
{
    if (!h)
    {
        return NULL;
    }
    if (fn && h->impl_.handle_.stats_ & CCC_OCCUPIED)
    {
        fn((ccc_user_type){
            .user_type = base_at(h->impl_.hom_, h->impl_.handle_.i_),
            .aux = NULL,
        });
    }
    return h;
}

ccc_homap_handle *
ccc_hom_and_modify_aux(ccc_homap_handle *const h, ccc_update_fn *const fn,
                       void *const aux)
{
    if (!h)
    {
        return NULL;
    }
    if (fn && h->impl_.handle_.stats_ & CCC_OCCUPIED)
    {
        fn((ccc_user_type){
            .user_type = base_at(h->impl_.hom_, h->impl_.handle_.i_),
            .aux = aux,
        });
    }
    return h;
}

ccc_handle_i
ccc_hom_or_insert(ccc_homap_handle const *const h, ccc_homap_elem *const elem)
{
    if (!h || !elem)
    {
        return 0;
    }
    if (h->impl_.handle_.stats_ & CCC_OCCUPIED)
    {
        return h->impl_.handle_.i_;
    }
    return maybe_alloc_insert(h->impl_.hom_, elem);
}

ccc_handle
ccc_hom_swap_handle(ccc_handle_ordered_map *const hom,
                    ccc_homap_elem *const out_handle)
{
    if (!hom || !out_handle)
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    size_t const found = find(hom, key_from_node(hom, out_handle));
    if (found)
    {
        assert(hom->root_);
        *out_handle = *at(hom, hom->root_);
        void *const user_struct = struct_base(hom, out_handle);
        void *const ret = base_at(hom, hom->root_);
        void *const tmp = ccc_buf_at(&hom->buf_, 0);
        swap(tmp, user_struct, ret, ccc_buf_elem_size(&hom->buf_));
        return (ccc_handle){{.i_ = found, .stats_ = CCC_OCCUPIED}};
    }
    size_t const inserted = maybe_alloc_insert(hom, out_handle);
    if (!inserted)
    {
        return (ccc_handle){{.i_ = 0, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_handle){{.i_ = inserted, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hom_try_insert(ccc_handle_ordered_map *const hom,
                   ccc_homap_elem *const key_val_handle)
{
    if (!hom || !key_val_handle)
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    size_t const found = find(hom, key_from_node(hom, key_val_handle));
    if (found)
    {
        assert(hom->root_);
        return (ccc_handle){{.i_ = found, .stats_ = CCC_OCCUPIED}};
    }
    size_t const inserted = maybe_alloc_insert(hom, key_val_handle);
    if (!inserted)
    {
        return (ccc_handle){{.i_ = 0, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_handle){{.i_ = inserted, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hom_insert_or_assign(ccc_handle_ordered_map *const hom,
                         ccc_homap_elem *const key_val_handle)
{
    if (!hom || !key_val_handle)
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    size_t const found = find(hom, key_from_node(hom, key_val_handle));
    if (found)
    {
        *key_val_handle = *at(hom, found);
        assert(hom->root_);
        void const *const e_base = struct_base(hom, key_val_handle);
        void *const f_base = ccc_buf_at(&hom->buf_, found);
        if (e_base != f_base)
        {
            memcpy(f_base, e_base, ccc_buf_elem_size(&hom->buf_));
        }
        return (ccc_handle){{.i_ = found, .stats_ = CCC_OCCUPIED}};
    }
    size_t const inserted = maybe_alloc_insert(hom, key_val_handle);
    if (!inserted)
    {
        return (ccc_handle){{.i_ = 0, .stats_ = CCC_INSERT_ERROR}};
    }
    return (ccc_handle){{.i_ = inserted, .stats_ = CCC_VACANT}};
}

ccc_handle
ccc_hom_remove(ccc_handle_ordered_map *const hom,
               ccc_homap_elem *const out_handle)
{
    if (!hom || !out_handle)
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    size_t const n = erase(hom, key_from_node(hom, out_handle));
    if (!n)
    {
        return (ccc_handle){{.i_ = 0, .stats_ = CCC_VACANT}};
    }
    return (ccc_handle){{.i_ = n, .stats_ = CCC_OCCUPIED}};
}

ccc_handle
ccc_hom_remove_handle(ccc_homap_handle *const h)
{
    if (!h)
    {
        return (ccc_handle){{.stats_ = CCC_INPUT_ERROR}};
    }
    if (h->impl_.handle_.stats_ == CCC_OCCUPIED)
    {
        size_t const erased
            = erase(h->impl_.hom_, key_at(h->impl_.hom_, h->impl_.handle_.i_));
        assert(erased);
        return (ccc_handle){{.i_ = erased, .stats_ = CCC_OCCUPIED}};
    }
    return (ccc_handle){{.i_ = 0, .stats_ = CCC_VACANT}};
}

ccc_handle_i
ccc_hom_unwrap(ccc_homap_handle const *const h)
{
    if (!h)
    {
        return 0;
    }
    return h->impl_.handle_.stats_ == CCC_OCCUPIED ? h->impl_.handle_.i_ : 0;
}

bool
ccc_hom_insert_error(ccc_homap_handle const *const h)
{
    return h ? h->impl_.handle_.stats_ & CCC_INSERT_ERROR : false;
}

bool
ccc_hom_occupied(ccc_homap_handle const *const h)
{
    return h ? h->impl_.handle_.stats_ & CCC_OCCUPIED : false;
}

ccc_handle_status
ccc_hom_handle_status(ccc_homap_handle const *const h)
{
    return h ? h->impl_.handle_.stats_ : CCC_INPUT_ERROR;
}

bool
ccc_hom_is_empty(ccc_handle_ordered_map const *const hom)
{
    return !ccc_hom_size(hom);
}

size_t
ccc_hom_size(ccc_handle_ordered_map const *const hom)
{
    if (!hom)
    {
        return 0;
    }
    size_t const sz = ccc_buf_size(&hom->buf_);
    return !sz ? sz : sz - 1;
}

size_t
ccc_hom_capacity(ccc_handle_ordered_map const *const hom)
{
    if (!hom)
    {
        return 0;
    }
    return ccc_buf_capacity(&hom->buf_);
}

void *
ccc_hom_begin(ccc_handle_ordered_map const *const hom)
{
    if (!hom || ccc_buf_is_empty(&hom->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(hom, hom->root_, L);
    return base_at(hom, n);
}

void *
ccc_hom_rbegin(ccc_handle_ordered_map const *const hom)
{
    if (!hom || ccc_buf_is_empty(&hom->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(hom, hom->root_, R);
    return base_at(hom, n);
}

void *
ccc_hom_next(ccc_handle_ordered_map const *const hom,
             ccc_homap_elem const *const e)
{
    if (!hom || ccc_buf_is_empty(&hom->buf_))
    {
        return NULL;
    }
    size_t const n = next(hom, index_of(hom, e), inorder_traversal);
    return base_at(hom, n);
}

void *
ccc_hom_rnext(ccc_handle_ordered_map const *const hom,
              ccc_homap_elem const *const e)
{
    if (!hom || !e || ccc_buf_is_empty(&hom->buf_))
    {
        return NULL;
    }
    size_t const n = next(hom, index_of(hom, e), reverse_inorder_traversal);
    return base_at(hom, n);
}

void *
ccc_hom_end(ccc_handle_ordered_map const *const hom)
{
    if (!hom || ccc_buf_is_empty(&hom->buf_))
    {
        return NULL;
    }
    return base_at(hom, 0);
}

void *
ccc_hom_rend(ccc_handle_ordered_map const *const hom)
{
    if (!hom || ccc_buf_is_empty(&hom->buf_))
    {
        return NULL;
    }
    return base_at(hom, 0);
}

void *
ccc_hom_data(ccc_handle_ordered_map const *const hom)
{
    return hom ? ccc_buf_begin(&hom->buf_) : NULL;
}

ccc_range
ccc_hom_equal_range(ccc_handle_ordered_map *const hom,
                    void const *const begin_key, void const *const end_key)
{
    if (!hom || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){equal_range(hom, begin_key, end_key, inorder_traversal)};
}

ccc_rrange
ccc_hom_equal_rrange(ccc_handle_ordered_map *const hom,
                     void const *const rbegin_key, void const *const rend_key)

{
    if (!hom || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){
        equal_range(hom, rbegin_key, rend_key, reverse_inorder_traversal)};
}

ccc_result
ccc_hom_copy(ccc_handle_ordered_map *const dst,
             ccc_handle_ordered_map const *const src, ccc_alloc_fn *const fn)
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
ccc_hom_clear(ccc_handle_ordered_map *const hom, ccc_destructor_fn *const fn)
{
    if (!hom)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        (void)ccc_buf_size_set(&hom->buf_, 1);
        hom->root_ = 0;
        return CCC_OK;
    }
    while (!ccc_hom_is_empty(hom))
    {
        size_t const i = remove_from_tree(hom, hom->root_);
        assert(i);
        fn((ccc_user_type){.user_type = ccc_buf_at(&hom->buf_, i),
                           .aux = hom->buf_.aux_});
    }
    (void)ccc_buf_size_set(&hom->buf_, 1);
    hom->root_ = 0;
    return CCC_OK;
}

ccc_result
ccc_hom_clear_and_free(ccc_handle_ordered_map *const hom,
                       ccc_destructor_fn *const fn)
{
    if (!hom)
    {
        return CCC_INPUT_ERR;
    }
    if (!fn)
    {
        hom->root_ = 0;
        return ccc_buf_alloc(&hom->buf_, 0, hom->buf_.alloc_);
    }
    while (!ccc_hom_is_empty(hom))
    {
        size_t const i = remove_from_tree(hom, hom->root_);
        assert(i);
        fn((ccc_user_type){.user_type = ccc_buf_at(&hom->buf_, i),
                           .aux = hom->buf_.aux_});
    }
    hom->root_ = 0;
    return ccc_buf_alloc(&hom->buf_, 0, hom->buf_.alloc_);
}

bool
ccc_hom_validate(ccc_handle_ordered_map const *const hom)
{
    return hom ? validate(hom) : false;
}

/*===========================   Private Interface ===========================*/

void
ccc_impl_hom_insert(struct ccc_homap_ *const hom, size_t const elem_i)
{
    insert(hom, elem_i);
}

struct ccc_htree_handle_
ccc_impl_hom_handle(struct ccc_homap_ *const hom, void const *const key)
{
    return handle(hom, key);
}

void *
ccc_impl_hom_key_from_node(struct ccc_homap_ const *const hom,
                           struct ccc_homap_elem_ const *const elem)
{
    return key_from_node(hom, elem);
}

void *
ccc_impl_hom_key_at(struct ccc_homap_ const *const hom, size_t const slot)
{
    return key_at(hom, slot);
}

struct ccc_homap_elem_ *
ccc_impl_homap_elem_at(struct ccc_homap_ const *const hom, size_t const slot)
{
    return at(hom, slot);
}

size_t
ccc_impl_hom_alloc_slot(struct ccc_homap_ *const hom)
{
    return alloc_slot(hom);
}

/*===========================   Static Helpers    ===========================*/

static inline struct ccc_range_u_
equal_range(struct ccc_homap_ *const t, void const *const begin_key,
            void const *const end_key, enum hom_branch_ const traversal)
{
    if (ccc_hom_is_empty(t))
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

static inline struct ccc_htree_handle_
handle(struct ccc_homap_ *const hom, void const *const key)
{
    size_t const found = find(hom, key);
    if (found)
    {
        return (struct ccc_htree_handle_){
            .hom_ = hom,
            .handle_ = {.i_ = found, .stats_ = CCC_OCCUPIED},
        };
    }
    return (struct ccc_htree_handle_){
        .hom_ = hom,
        .handle_ = {.i_ = 0, .stats_ = CCC_VACANT},
    };
}

static size_t
maybe_alloc_insert(struct ccc_homap_ *const hom,
                   struct ccc_homap_elem_ *const elem)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const node = alloc_slot(hom);
    if (!node)
    {
        return 0;
    }
    (void)ccc_buf_write(&hom->buf_, node, struct_base(hom, elem));
    insert(hom, node);
    return node;
}

static inline void
insert(struct ccc_homap_ *const t, size_t const n)
{
    struct ccc_homap_elem_ *const node = at(t, n);
    init_node(node);
    if (ccc_buf_size(&t->buf_) == empty_tree)
    {
        t->root_ = n;
        return;
    }
    void const *const key = key_from_node(t, node);
    t->root_ = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const root_cmp = cmp_elems(t, key, t->root_, t->cmp_);
    if (CCC_EQL == root_cmp)
    {
        return;
    }
    (void)connect_new_root(t, n, root_cmp);
}

static inline size_t
erase(struct ccc_homap_ *const t, void const *const key)
{
    if (ccc_hom_is_empty(t))
    {
        return 0;
    }
    size_t ret = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const found = cmp_elems(t, key, ret, t->cmp_);
    if (found != CCC_EQL)
    {
        return 0;
    }
    ret = remove_from_tree(t, ret);
    return ret;
}

static inline size_t
remove_from_tree(struct ccc_homap_ *const t, size_t const ret)
{
    if (!branch_i(t, ret, L))
    {
        t->root_ = branch_i(t, ret, R);
        link(t, 0, 0, t->root_);
    }
    else
    {
        t->root_ = splay(t, branch_i(t, ret, L), key_from_node(t, at(t, ret)),
                         t->cmp_);
        link(t, t->root_, R, branch_i(t, ret, R));
    }
    at(t, ret)->next_free_ = t->free_list_;
    t->free_list_ = ret;
    [[maybe_unused]] ccc_result const r = ccc_buf_size_minus(&t->buf_, 1);
    assert(r == CCC_OK);
    return ret;
}

static inline size_t
connect_new_root(struct ccc_homap_ *const t, size_t const new_root,
                 ccc_threeway_cmp const cmp_result)
{
    enum hom_branch_ const dir = CCC_GRT == cmp_result;
    link(t, new_root, dir, branch_i(t, t->root_, dir));
    link(t, new_root, !dir, t->root_);
    *branch_ref(t, t->root_, dir) = 0;
    t->root_ = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link(t, 0, 0, t->root_);
    return new_root;
}

static inline size_t
find(struct ccc_homap_ *const t, void const *const key)
{
    if (!t->root_)
    {
        return 0;
    }
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp_elems(t, key, t->root_, t->cmp_) == CCC_EQL ? t->root_ : 0;
}

static inline size_t
splay(struct ccc_homap_ *const t, size_t root, void const *const key,
      ccc_key_cmp_fn *const cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    struct ccc_homap_elem_ *const nil = at(t, 0);
    nil->branch_[L] = nil->branch_[R] = nil->parent_ = 0;
    size_t l_r_subtrees[LR] = {0, 0};
    do
    {
        ccc_threeway_cmp const root_cmp = cmp_elems(t, key, root, cmp_fn);
        enum hom_branch_ const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || !branch_i(t, root, dir))
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp_elems(t, key, branch_i(t, root, dir), cmp_fn);
        enum hom_branch_ const dir_from_child = CCC_GRT == child_cmp;
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
    } while (true);
    link(t, l_r_subtrees[L], R, branch_i(t, root, L));
    link(t, l_r_subtrees[R], L, branch_i(t, root, R));
    link(t, root, L, nil->branch_[R]);
    link(t, root, R, nil->branch_[L]);
    t->root_ = root;
    link(t, 0, 0, t->root_);
    return root;
}

static inline void
link(struct ccc_homap_ *const t, size_t const parent,
     enum hom_branch_ const dir, size_t const subtree)
{
    *branch_ref(t, parent, dir) = subtree;
    *parent_ref(t, subtree) = parent;
}

static inline size_t
min_max_from(struct ccc_homap_ const *const t, size_t start,
             enum hom_branch_ const dir)
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
next(struct ccc_homap_ const *const t, size_t n,
     enum hom_branch_ const traversal)
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
cmp_elems(struct ccc_homap_ const *const hom, void const *const key,
          size_t const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){.key_lhs = key,
                            .user_type_rhs = base_at(hom, node),
                            .aux = hom->buf_.aux_});
}

static inline size_t
alloc_slot(struct ccc_homap_ *const t)
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
            at(t, i)->next_free_ = prev;
        }
        t->free_list_ = prev;
        if (ccc_buf_size_set(&t->buf_, max(old_sz, 1)) != CCC_OK)
        {
            return 0;
        }
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
init_node(struct ccc_homap_elem_ *const e)
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

static inline struct ccc_homap_elem_ *
at(struct ccc_homap_ const *const t, size_t const i)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, i));
}

static inline size_t
branch_i(struct ccc_homap_ const *const t, size_t const parent,
         enum hom_branch_ const dir)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, parent))->branch_[dir];
}

static inline size_t
parent_i(struct ccc_homap_ const *const t, size_t const child)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, child))->parent_;
}

static inline size_t
index_of(struct ccc_homap_ const *const t,
         struct ccc_homap_elem_ const *const elem)
{
    return ccc_buf_i(&t->buf_, struct_base(t, elem));
}

static inline size_t *
branch_ref(struct ccc_homap_ const *t, size_t const node,
           enum hom_branch_ const branch)
{
    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->branch_[branch];
}

static inline size_t *
parent_ref(struct ccc_homap_ const *t, size_t node)
{
    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parent_;
}

static inline void *
base_at(struct ccc_homap_ const *const hom, size_t const i)
{
    return ccc_buf_at(&hom->buf_, i);
}

static inline void *
struct_base(struct ccc_homap_ const *const hom,
            struct ccc_homap_elem_ const *const e)
{
    return ((char *)e->branch_) - hom->node_elem_offset_;
}

static struct ccc_homap_elem_ *
elem_in_slot(struct ccc_homap_ const *const t, void const *const slot)
{
    return (struct ccc_homap_elem_ *)((char *)slot + t->node_elem_offset_);
}

static inline void *
key_from_node(struct ccc_homap_ const *const t,
              struct ccc_homap_elem_ const *const elem)
{
    return (char *)struct_base(t, elem) + t->key_offset_;
}

static inline void *
key_at(struct ccc_homap_ const *const t, size_t const i)
{
    return (char *)ccc_buf_at(&t->buf_, i) + t->key_offset_;
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
recursive_size(struct ccc_homap_ const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_size(t, branch_i(t, r, R))
           + recursive_size(t, branch_i(t, r, L));
}

static bool
are_subtrees_valid(struct ccc_homap_ const *t, struct tree_range_ const r)
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
is_storing_parent(struct ccc_homap_ const *const t, size_t const p,
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

static inline bool
is_free_list_valid(struct ccc_homap_ const *const t)
{
    if (!ccc_buf_size(&t->buf_))
    {
        return true;
    }
    size_t list_check = 0;
    for (size_t cur = t->free_list_;
         cur && list_check < ccc_buf_capacity(&t->buf_);
         cur = at(t, cur)->next_free_, ++list_check)
    {}
    return (list_check + ccc_buf_size(&t->buf_) == ccc_buf_capacity(&t->buf_));
}

static bool
validate(struct ccc_homap_ const *const hom)
{
    if (!are_subtrees_valid(hom, (struct tree_range_){.root = hom->root_}))
    {
        return false;
    }
    size_t const size = recursive_size(hom, hom->root_);
    if (size && size != ccc_buf_size(&hom->buf_) - 1)
    {
        return false;
    }
    if (!is_storing_parent(hom, 0, hom->root_))
    {
        return false;
    }
    if (!is_free_list_valid(hom))
    {
        return false;
    }
    return true;
}

/* NOLINTEND(*misc-no-recursion) */
