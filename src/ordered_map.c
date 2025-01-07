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

#include "impl/impl_ordered_map.h"
#include "impl/impl_tree.h"
#include "impl/impl_types.h"
#include "ordered_map.h"
#include "types.h"

#define LR 2

/** @private Instead of thinking about left and right consider only links
    in the abstract sense. Put them in an array and then flip
    this enum and left and right code paths can be united into one */
typedef enum
{
    L = 0,
    R,
} om_branch_;

static om_branch_ const inorder_traversal = R;
static om_branch_ const reverse_inorder_traversal = L;

/* Container entry return value. */

static struct ccc_tree_entry_ container_entry(struct ccc_tree_ *t,
                                              void const *key);

/* No return value. */

static void init_node(struct ccc_tree_ *, struct ccc_node_ *);
static void swap(char tmp[], void *, void *, size_t);
static void link_trees(struct ccc_node_ *, om_branch_, struct ccc_node_ *);

/* Boolean returns */

static bool empty(struct ccc_tree_ const *);
static bool contains(struct ccc_tree_ *, void const *);
static bool ccc_tree_validate(struct ccc_tree_ const *);

/* Returning the user type that is stored in data structure. */

static void *struct_base(struct ccc_tree_ const *, struct ccc_node_ const *);
static void *find(struct ccc_tree_ *, void const *);
static void *erase(struct ccc_tree_ *, void const *key);
static void *alloc_insert(struct ccc_tree_ *, struct ccc_node_ *);
static void *insert(struct ccc_tree_ *, struct ccc_node_ *);
static void *connect_new_root(struct ccc_tree_ *, struct ccc_node_ *,
                              ccc_threeway_cmp);
static void *max(struct ccc_tree_ const *);
static void *min(struct ccc_tree_ const *);
static void *key_in_slot(struct ccc_tree_ const *t, void const *slot);
static void *key_from_node(struct ccc_tree_ const *, ccc_node_ const *);
static struct ccc_range_u_ equal_range(struct ccc_tree_ *, void const *,
                                       void const *, om_branch_);

/* Internal operations that take and return nodes for the tree. */

static struct ccc_node_ *remove_from_tree(struct ccc_tree_ *,
                                          struct ccc_node_ *);
static struct ccc_node_ const *next(struct ccc_tree_ const *,
                                    struct ccc_node_ const *, om_branch_);
static struct ccc_node_ *splay(struct ccc_tree_ *, struct ccc_node_ *,
                               void const *key, ccc_key_cmp_fn *);
static struct ccc_node_ *elem_in_slot(struct ccc_tree_ const *t,
                                      void const *slot);

/* The key comes first. It is the "left hand side" of the comparison. */
static ccc_threeway_cmp cmp(struct ccc_tree_ const *, void const *key,
                            struct ccc_node_ const *, ccc_key_cmp_fn *);

/* ======================        Map Interface      ====================== */

bool
ccc_om_is_empty(ccc_ordered_map const *const om)
{
    return om ? empty(&om->impl_) : true;
}

size_t
ccc_om_size(ccc_ordered_map const *const om)
{
    return om ? om->impl_.size_ : 0;
}

bool
ccc_om_contains(ccc_ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return false;
    }
    return contains(&om->impl_, key);
}

ccc_omap_entry
ccc_om_entry(ccc_ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return (ccc_omap_entry){{.entry_ = {.stats_ = CCC_ENTRY_INPUT_ERROR}}};
    }
    return (ccc_omap_entry){container_entry(&om->impl_, key)};
}

void *
ccc_om_insert_entry(ccc_omap_entry const *const e, ccc_omap_elem *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        elem->impl_ = *elem_in_slot(e->impl_.t_, e->impl_.entry_.e_);
        (void)memcpy(e->impl_.entry_.e_, struct_base(e->impl_.t_, &elem->impl_),
                     e->impl_.t_->elem_sz_);
        return e->impl_.entry_.e_;
    }
    return alloc_insert(e->impl_.t_, &elem->impl_);
}

void *
ccc_om_or_insert(ccc_omap_entry const *const e, ccc_omap_elem *const elem)
{
    if (!e || !elem)
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    return alloc_insert(e->impl_.t_, &elem->impl_);
}

ccc_omap_entry *
ccc_om_and_modify(ccc_omap_entry *const e, ccc_update_fn *const fn)
{
    if (!e)
    {
        return NULL;
    }
    if (fn && e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_user_type){.user_type = e->impl_.entry_.e_, .aux = NULL});
    }
    return e;
}

ccc_omap_entry *
ccc_om_and_modify_aux(ccc_omap_entry *const e, ccc_update_fn *const fn,
                      void *const aux)
{
    if (e && fn && e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_user_type){.user_type = e->impl_.entry_.e_, .aux = aux});
    }
    return e;
}

ccc_entry
ccc_om_insert(ccc_ordered_map *const om, ccc_omap_elem *const key_val_handle,
              ccc_omap_elem *const tmp)
{
    if (!om || !key_val_handle || !tmp)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const found
        = find(&om->impl_, key_from_node(&om->impl_, &key_val_handle->impl_));
    if (found)
    {
        assert(om->impl_.root_ != &om->impl_.end_);
        key_val_handle->impl_ = *om->impl_.root_;
        void *const user_struct
            = struct_base(&om->impl_, &key_val_handle->impl_);
        void *const in_tree = struct_base(&om->impl_, om->impl_.root_);
        void *const old_val = struct_base(&om->impl_, &tmp->impl_);
        swap(old_val, in_tree, user_struct, om->impl_.elem_sz_);
        key_val_handle->impl_.branch_[L] = key_val_handle->impl_.branch_[R]
            = key_val_handle->impl_.parent_ = NULL;
        tmp->impl_.branch_[L] = tmp->impl_.branch_[R] = tmp->impl_.parent_
            = NULL;
        return (ccc_entry){{.e_ = old_val, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *const inserted = alloc_insert(&om->impl_, &key_val_handle->impl_);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_om_try_insert(ccc_ordered_map *const om,
                  ccc_omap_elem *const key_val_handle)
{
    if (!om || !key_val_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const found
        = find(&om->impl_, key_from_node(&om->impl_, &key_val_handle->impl_));
    if (found)
    {
        assert(om->impl_.root_ != &om->impl_.end_);
        return (ccc_entry){{.e_ = struct_base(&om->impl_, om->impl_.root_),
                            .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *const inserted = alloc_insert(&om->impl_, &key_val_handle->impl_);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_om_insert_or_assign(ccc_ordered_map *const om,
                        ccc_omap_elem *const key_val_handle)
{
    if (!om || !key_val_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const found
        = find(&om->impl_, key_from_node(&om->impl_, &key_val_handle->impl_));
    if (found)
    {
        key_val_handle->impl_ = *elem_in_slot(&om->impl_, found);
        assert(om->impl_.root_ != &om->impl_.end_);
        memcpy(found, struct_base(&om->impl_, &key_val_handle->impl_),
               om->impl_.elem_sz_);
        return (ccc_entry){{.e_ = found, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *const inserted = alloc_insert(&om->impl_, &key_val_handle->impl_);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_om_remove(ccc_ordered_map *const om, ccc_omap_elem *const out_handle)
{
    if (!om || !out_handle)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    void *const n
        = erase(&om->impl_, key_from_node(&om->impl_, &out_handle->impl_));
    if (!n)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    if (om->impl_.alloc_)
    {
        void *const user_struct = struct_base(&om->impl_, &out_handle->impl_);
        memcpy(user_struct, n, om->impl_.elem_sz_);
        om->impl_.alloc_(n, 0, om->impl_.aux_);
        return (ccc_entry){{.e_ = user_struct, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = n, .stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_entry
ccc_om_remove_entry(ccc_omap_entry *const e)
{
    if (!e)
    {
        return (ccc_entry){{.stats_ = CCC_ENTRY_INPUT_ERROR}};
    }
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        void *const erased
            = erase(e->impl_.t_, key_in_slot(e->impl_.t_, e->impl_.entry_.e_));
        assert(erased);
        if (e->impl_.t_->alloc_)
        {
            e->impl_.t_->alloc_(erased, 0, e->impl_.t_->aux_);
            return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_OCCUPIED}};
        }
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

void *
ccc_om_get_key_val(ccc_ordered_map *const om, void const *const key)
{
    if (!om || !key)
    {
        return NULL;
    }
    return find(&om->impl_, key);
}

void *
ccc_om_unwrap(ccc_omap_entry const *const e)
{
    if (!e)
    {
        return NULL;
    }
    return e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED ? e->impl_.entry_.e_
                                                        : NULL;
}

bool
ccc_om_insert_error(ccc_omap_entry const *const e)
{
    return e ? e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR : false;
}

bool
ccc_om_occupied(ccc_omap_entry const *const e)
{
    return e ? e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED : false;
}

ccc_entry_status
ccc_om_entry_status(ccc_omap_entry const *const e)
{
    return e ? e->impl_.entry_.stats_ : CCC_ENTRY_INPUT_ERROR;
}

void *
ccc_om_begin(ccc_ordered_map const *const om)
{
    return om ? min(&om->impl_) : NULL;
}

void *
ccc_om_rbegin(ccc_ordered_map const *const om)
{
    return om ? max(&om->impl_) : NULL;
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
    struct ccc_node_ const *n = next(&om->impl_, &e->impl_, inorder_traversal);
    return n == &om->impl_.end_ ? NULL : struct_base(&om->impl_, n);
}

void *
ccc_om_rnext(ccc_ordered_map const *const om, ccc_omap_elem const *const e)
{
    if (!om || !e)
    {
        return NULL;
    }
    struct ccc_node_ const *n
        = next(&om->impl_, &e->impl_, reverse_inorder_traversal);
    return n == &om->impl_.end_ ? NULL : struct_base(&om->impl_, n);
}

ccc_range
ccc_om_equal_range(ccc_ordered_map *const om, void const *const begin_key,
                   void const *const end_key)
{
    if (!om || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){
        equal_range(&om->impl_, begin_key, end_key, inorder_traversal)};
}

ccc_rrange
ccc_om_equal_rrange(ccc_ordered_map *const om, void const *const rbegin_key,
                    void const *const rend_key)

{
    if (!om || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){equal_range(&om->impl_, rbegin_key, rend_key,
                                    reverse_inorder_traversal)};
}

ccc_result
ccc_om_clear(ccc_ordered_map *const om, ccc_destructor_fn *const destructor)
{
    if (!om)
    {
        return CCC_INPUT_ERR;
    }
    while (!ccc_om_is_empty(om))
    {
        void *const popped
            = erase(&om->impl_, key_from_node(&om->impl_, om->impl_.root_));
        if (destructor)
        {
            destructor(
                (ccc_user_type){.user_type = popped, .aux = om->impl_.aux_});
        }
        if (om->impl_.alloc_)
        {
            (void)om->impl_.alloc_(popped, 0, om->impl_.aux_);
        }
    }
    return CCC_OK;
}

bool
ccc_om_validate(ccc_ordered_map const *const om)
{
    if (!om)
    {
        return false;
    }
    return ccc_tree_validate(&om->impl_);
}

/*==========================  Private Interface  ============================*/

struct ccc_tree_entry_
ccc_impl_om_entry(struct ccc_tree_ *const t, void const *const key)
{
    return container_entry(t, key);
}

void *
ccc_impl_om_insert(struct ccc_tree_ *const t, struct ccc_node_ *n)
{
    return insert(t, n);
}

void *
ccc_impl_om_key_in_slot(struct ccc_tree_ const *const t, void const *const slot)
{
    return key_in_slot(t, slot);
}

void *
ccc_impl_om_key_from_node(struct ccc_tree_ const *const t,
                          struct ccc_node_ const *const n)
{
    return key_from_node(t, n);
}

struct ccc_node_ *
ccc_impl_omap_elem_in_slot(struct ccc_tree_ const *const t, void const *slot)
{

    return elem_in_slot(t, slot);
}

/*======================  Static Splay Tree Helpers  ========================*/

static inline struct ccc_tree_entry_
container_entry(struct ccc_tree_ *const t, void const *const key)
{
    void *const found = find(t, key);
    if (found)
    {
        return (struct ccc_tree_entry_){
            .t_ = t, .entry_ = {.e_ = found, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (struct ccc_tree_entry_){
        .t_ = t, .entry_ = {.e_ = found, .stats_ = CCC_ENTRY_VACANT}};
}

static inline void *
key_from_node(struct ccc_tree_ const *const t, ccc_node_ const *const n)
{
    return (char *)struct_base(t, n) + t->key_offset_;
}

static inline void *
key_in_slot(struct ccc_tree_ const *const t, void const *const slot)
{
    return (char *)slot + t->key_offset_;
}

static inline struct ccc_node_ *
elem_in_slot(struct ccc_tree_ const *const t, void const *const slot)
{

    return (struct ccc_node_ *)((char *)slot + t->node_elem_offset_);
}

static inline void
init_node(struct ccc_tree_ *const t, struct ccc_node_ *const n)
{
    n->branch_[L] = &t->end_;
    n->branch_[R] = &t->end_;
    n->parent_ = &t->end_;
}

static inline bool
empty(struct ccc_tree_ const *const t)
{
    return !t->size_ || !t->root_ || t->root_ == &t->end_;
}

static inline void *
max(struct ccc_tree_ const *const t)
{
    if (!t->size_)
    {
        return NULL;
    }
    struct ccc_node_ *m = t->root_;
    for (; m->branch_[R] != &t->end_; m = m->branch_[R])
    {}
    return struct_base(t, m);
}

static inline void *
min(struct ccc_tree_ const *t)
{
    if (!t->size_)
    {
        return NULL;
    }
    struct ccc_node_ *m = t->root_;
    for (; m->branch_[L] != &t->end_; m = m->branch_[L])
    {}
    return struct_base(t, m);
}

static inline struct ccc_node_ const *
next(struct ccc_tree_ const *const t, struct ccc_node_ const *n,
     om_branch_ const traversal)
{
    if (!n || n == &t->end_)
    {
        return NULL;
    }
    assert(t->root_->parent_ == &t->end_);
    /* The node is a parent, backtracked to, or the end. */
    if (n->branch_[traversal] != &t->end_)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->branch_[traversal]; n->branch_[!traversal] != &t->end_;
             n = n->branch_[!traversal])
        {}
        return n;
    }
    /* This is how to return internal nodes on the way back up from a leaf. */
    struct ccc_node_ const *p = n->parent_;
    for (; p != &t->end_ && p->branch_[!traversal] != n; n = p, p = n->parent_)
    {}
    return p;
}

static inline struct ccc_range_u_
equal_range(struct ccc_tree_ *const t, void const *const begin_key,
            void const *const end_key, om_branch_ const traversal)
{
    if (!t->size_)
    {
        return (struct ccc_range_u_){};
    }
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    struct ccc_node_ const *b = splay(t, t->root_, begin_key, t->cmp_);
    if (cmp(t, begin_key, b, t->cmp_) == les_or_grt[traversal])
    {
        b = next(t, b, traversal);
    }
    struct ccc_node_ const *e = splay(t, t->root_, end_key, t->cmp_);
    if (cmp(t, end_key, e, t->cmp_) != les_or_grt[!traversal])
    {
        e = next(t, e, traversal);
    }
    return (struct ccc_range_u_){
        .begin_ = b == &t->end_ ? NULL : struct_base(t, b),
        .end_ = e == &t->end_ ? NULL : struct_base(t, e),
    };
}

static inline void *
find(struct ccc_tree_ *const t, void const *const key)
{
    if (t->root_ == &t->end_)
    {
        return NULL;
    }
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp(t, key, t->root_, t->cmp_) == CCC_EQL ? struct_base(t, t->root_)
                                                     : NULL;
}

static inline bool
contains(struct ccc_tree_ *const t, void const *key)
{
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp(t, key, t->root_, t->cmp_) == CCC_EQL;
}

static inline void *
alloc_insert(struct ccc_tree_ *const t, struct ccc_node_ *out_handle)
{
    init_node(t, out_handle);
    ccc_threeway_cmp root_cmp = CCC_CMP_ERR;
    if (!empty(t))
    {
        void const *const key = key_from_node(t, out_handle);
        t->root_ = splay(t, t->root_, key, t->cmp_);
        root_cmp = cmp(t, key, t->root_, t->cmp_);
        if (CCC_EQL == root_cmp)
        {
            return NULL;
        }
    }
    if (t->alloc_)
    {
        void *const node = t->alloc_(NULL, t->elem_sz_, t->aux_);
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, struct_base(t, out_handle), t->elem_sz_);
        out_handle = elem_in_slot(t, node);
    }
    if (empty(t))
    {
        t->root_ = out_handle;
        t->size_ = 1;
        return struct_base(t, out_handle);
    }
    assert(root_cmp != CCC_CMP_ERR);
    t->size_++;
    return connect_new_root(t, out_handle, root_cmp);
}

static inline void *
insert(struct ccc_tree_ *const t, struct ccc_node_ *const n)
{
    init_node(t, n);
    if (empty(t))
    {
        t->root_ = n;
        t->size_ = 1;
        return struct_base(t, n);
    }
    void const *const key = key_from_node(t, n);
    t->root_ = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const root_cmp = cmp(t, key, t->root_, t->cmp_);
    if (CCC_EQL == root_cmp)
    {
        return NULL;
    }
    t->size_++;
    return connect_new_root(t, n, root_cmp);
}

static inline void *
connect_new_root(struct ccc_tree_ *const t, struct ccc_node_ *const new_root,
                 ccc_threeway_cmp const cmp_result)
{
    om_branch_ const link = CCC_GRT == cmp_result;
    link_trees(new_root, link, t->root_->branch_[link]);
    link_trees(new_root, !link, t->root_);
    t->root_->branch_[link] = &t->end_;
    t->root_ = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link_trees(&t->end_, 0, t->root_);
    return struct_base(t, new_root);
}

static inline void *
erase(struct ccc_tree_ *const t, void const *const key)
{
    if (empty(t))
    {
        return NULL;
    }
    struct ccc_node_ *ret = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const found = cmp(t, key, ret, t->cmp_);
    if (found != CCC_EQL)
    {
        return NULL;
    }
    ret = remove_from_tree(t, ret);
    ret->branch_[L] = ret->branch_[R] = ret->parent_ = NULL;
    t->size_--;
    return struct_base(t, ret);
}

static inline struct ccc_node_ *
remove_from_tree(struct ccc_tree_ *const t, struct ccc_node_ *const ret)
{
    if (ret->branch_[L] == &t->end_)
    {
        t->root_ = ret->branch_[R];
        link_trees(&t->end_, 0, t->root_);
    }
    else
    {
        t->root_ = splay(t, ret->branch_[L], key_from_node(t, ret), t->cmp_);
        link_trees(t->root_, R, ret->branch_[R]);
    }
    return ret;
}

static inline struct ccc_node_ *
splay(struct ccc_tree_ *const t, struct ccc_node_ *root, void const *const key,
      ccc_key_cmp_fn *const cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    t->end_.branch_[L] = t->end_.branch_[R] = t->end_.parent_ = &t->end_;
    struct ccc_node_ *l_r_subtrees[LR] = {&t->end_, &t->end_};
    for (;;)
    {
        ccc_threeway_cmp const root_cmp = cmp(t, key, root, cmp_fn);
        om_branch_ const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || root->branch_[dir] == &t->end_)
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp(t, key, root->branch_[dir], cmp_fn);
        om_branch_ const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            struct ccc_node_ *const pivot = root->branch_[dir];
            link_trees(root, dir, pivot->branch_[!dir]);
            link_trees(pivot, !dir, root);
            root = pivot;
            if (root->branch_[dir] == &t->end_)
            {
                break;
            }
        }
        link_trees(l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = root->branch_[dir];
    }
    link_trees(l_r_subtrees[L], R, root->branch_[L]);
    link_trees(l_r_subtrees[R], L, root->branch_[R]);
    link_trees(root, L, t->end_.branch_[R]);
    link_trees(root, R, t->end_.branch_[L]);
    t->root_ = root;
    link_trees(&t->end_, 0, t->root_);
    return root;
}

static inline void *
struct_base(struct ccc_tree_ const *const t, struct ccc_node_ const *const n)
{
    /* Link is the first field of the struct and is an array so no need to get
       pointer address of [0] element of array. That's the same as just the
       array field. */
    return ((char *)n->branch_) - t->node_elem_offset_;
}

static inline ccc_threeway_cmp
cmp(struct ccc_tree_ const *const t, void const *const key,
    struct ccc_node_ const *const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){
        .key_lhs = key,
        .user_type_rhs = struct_base(t, node),
        .aux = t->aux_,
    });
}

static inline void
swap(char tmp[], void *const a, void *const b, size_t elem_sz)
{
    if (a == b)
    {
        return;
    }
    (void)memcpy(tmp, a, elem_sz);
    (void)memcpy(a, b, elem_sz);
    (void)memcpy(b, tmp, elem_sz);
}

static inline void
link_trees(struct ccc_node_ *const parent, om_branch_ const dir,
           struct ccc_node_ *const subtree)
{
    parent->branch_[dir] = subtree;
    subtree->parent_ = parent;
}

/* NOLINTBEGIN(*misc-no-recursion) */

/* ======================        Debugging           ====================== */

/** @private Validate binary tree invariants with ranges. Use a recursive method
that does not rely upon the implementation of iterators or any other possibly
buggy implementation. A pure functional range check will provide the most
reliable check regardless of implementation changes throughout code base. */
struct tree_range_
{
    struct ccc_node_ const *low;
    struct ccc_node_ const *root;
    struct ccc_node_ const *high;
};

/** @private */
struct parent_status_
{
    bool correct;
    struct ccc_node_ const *parent;
};

static size_t
recursive_size(struct ccc_tree_ const *const t, struct ccc_node_ const *const r)
{
    if (r == &t->end_)
    {
        return 0;
    }
    return 1 + recursive_size(t, r->branch_[R])
           + recursive_size(t, r->branch_[L]);
}

static bool
are_subtrees_valid(struct ccc_tree_ const *const t, struct tree_range_ const r,
                   struct ccc_node_ const *const nil)
{
    if (!r.root)
    {
        return false;
    }
    if (r.root == nil)
    {
        return true;
    }
    if (r.low != nil
        && cmp(t, ccc_impl_om_key_from_node(t, r.low), r.root, t->cmp_)
               != CCC_LES)
    {
        return false;
    }
    if (r.high != nil
        && cmp(t, ccc_impl_om_key_from_node(t, r.high), r.root, t->cmp_)
               != CCC_GRT)
    {
        return false;
    }
    return are_subtrees_valid(t,
                              (struct tree_range_){.low = r.low,
                                                   .root = r.root->branch_[L],
                                                   .high = r.root},
                              nil)
           && are_subtrees_valid(
               t,
               (struct tree_range_){
                   .low = r.root, .root = r.root->branch_[R], .high = r.high},
               nil);
}

static struct parent_status_
child_tracks_parent(struct ccc_node_ const *const parent,
                    struct ccc_node_ const *const root)
{
    if (root->parent_ != parent)
    {
        struct ccc_node_ *p = root->parent_->parent_;
        return (struct parent_status_){false, p};
    }
    return (struct parent_status_){true, parent};
}

static bool
is_duplicate_storing_parent(struct ccc_tree_ const *const t,
                            struct ccc_node_ const *const parent,
                            struct ccc_node_ const *const root)
{
    if (root == &t->end_)
    {
        return true;
    }
    if (!child_tracks_parent(parent, root).correct)
    {
        return false;
    }
    return is_duplicate_storing_parent(t, root, root->branch_[L])
           && is_duplicate_storing_parent(t, root, root->branch_[R]);
}

/* Validate tree prefers to use recursion to examine the tree over the
   provided iterators of any implementation so as to avoid using a
   flawed implementation of such iterators. This should help be more
   sure that the implementation is correct because it follows the
   truth of the provided pointers with its own stack as backtracking
   information. */
static bool
ccc_tree_validate(struct ccc_tree_ const *const t)
{
    if (!are_subtrees_valid(t,
                            (struct tree_range_){
                                .low = &t->end_,
                                .root = t->root_,
                                .high = &t->end_,
                            },
                            &t->end_))
    {
        return false;
    }
    if (!is_duplicate_storing_parent(t, &t->end_, t->root_))
    {
        return false;
    }
    if (recursive_size(t, t->root_) != t->size_)
    {
        return false;
    }
    return true;
}

/* NOLINTEND(*misc-no-recursion) */
