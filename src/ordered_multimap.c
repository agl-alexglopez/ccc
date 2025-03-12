/** This file implements a splay tree that supports duplicates.
The code to support a splay tree that allows duplicates is more complicated than
a traditional splay tree. It requires significant modification. This
implementation is based on the following source.

    1. Daniel Sleator, Carnegie Mellon University. Sleator's implementation of a
       topdown splay tree was instrumental in starting things off, but required
       extensive modification. I had to add the ability to track duplicates,
       update parent and child tracking, and unite the left and right cases for
       fun. See the code for a generalizable strategy to eliminate symmetric
       left and right cases for any binary tree code.
       https:www.link.cs.cmu.edulinkftp-sitesplayingtop-down-splay.c */
#include <assert.h>
#include <string.h>

#include "impl/impl_ordered_multimap.h"
#include "impl/impl_tree.h"
#include "impl/impl_types.h"
#include "ordered_multimap.h"
#include "types.h"

#define LR 2

/** @private Instead of thinking about left and right consider only links in
the abstract sense. Put them in an array and then flip this enum and left and
right code paths can be united into one */
enum tree_link_
{
    L = 0,
    R = 1
};

/** @private Trees are just a different interpretation of the same links used
for doubly linked lists. We take advantage of this for duplicates. */
enum list_link_
{
    P = 0,
    N = 1
};

static enum tree_link_ const inorder_traversal = R;
static enum tree_link_ const reverse_inorder_traversal = L;

/* =======================        Prototypes         ====================== */

/* No return value. */

static void init_node(struct ccc_tree_ *, struct ccc_node_ *);
static void link_trees(struct ccc_tree_ *, struct ccc_node_ *, enum tree_link_,
                       struct ccc_node_ *);
static void add_duplicate(struct ccc_tree_ *, struct ccc_node_ *,
                          struct ccc_node_ *, struct ccc_node_ *);

/* Boolean returns */

static ccc_tribool empty(struct ccc_tree_ const *);
static ccc_tribool contains(struct ccc_tree_ *, void const *);
static ccc_tribool has_dups(struct ccc_node_ const *, struct ccc_node_ const *);
static ccc_tribool ccc_tree_validate(struct ccc_tree_ const *t);

/* Returning the user type that is stored in data structure. */

static struct ccc_ent_ multimap_insert(struct ccc_tree_ *, struct ccc_node_ *);
static void *multimap_erase(struct ccc_tree_ *t, void const *key);
static void *find(struct ccc_tree_ *, void const *);
static void *struct_base(struct ccc_tree_ const *, struct ccc_node_ const *);
static void *multimap_erase_max_or_min(struct ccc_tree_ *, ccc_key_cmp_fn *);
static void *multimap_erase_node(struct ccc_tree_ *, struct ccc_node_ *);
static void *connect_new_root(struct ccc_tree_ *, struct ccc_node_ *,
                              ccc_threeway_cmp);
static void *max(struct ccc_tree_ const *);
static void *pop_max(struct ccc_tree_ *);
static void *pop_min(struct ccc_tree_ *);
static void *min(struct ccc_tree_ const *);
static void *key_in_slot(struct ccc_tree_ const *t, void const *slot);
static void *key_from_node(struct ccc_tree_ const *t,
                           struct ccc_node_ const *n);
static struct ccc_node_ *elem_in_slot(struct ccc_tree_ const *t,
                                      void const *slot);
static struct ccc_range_u_ equal_range(struct ccc_tree_ *, void const *,
                                       void const *, enum tree_link_);

/* Internal operations that take and return nodes for the tree. */

static struct ccc_node_ *pop_dup_node(struct ccc_tree_ *, struct ccc_node_ *,
                                      struct ccc_node_ *);
static struct ccc_node_ *pop_front_dup(struct ccc_tree_ *, struct ccc_node_ *,
                                       void const *old_key);
static struct ccc_node_ *remove_from_tree(struct ccc_tree_ *,
                                          struct ccc_node_ *);
static struct ccc_node_ const *next(struct ccc_tree_ const *,
                                    struct ccc_node_ const *, enum tree_link_);
static struct ccc_node_ const *multimap_next(struct ccc_tree_ const *,
                                             struct ccc_node_ const *,
                                             enum tree_link_);
static struct ccc_node_ *splay(struct ccc_tree_ *, struct ccc_node_ *,
                               void const *key, ccc_key_cmp_fn *);
static struct ccc_node_ const *next_tree_node(struct ccc_tree_ const *,
                                              struct ccc_node_ const *,
                                              enum tree_link_);
static struct ccc_node_ *parent_r(struct ccc_tree_ const *,
                                  struct ccc_node_ const *);

static struct ccc_tree_entry_ container_entry(struct ccc_tree_ *,
                                              void const *key);

/* Comparison function returns */

static ccc_threeway_cmp force_find_grt(ccc_key_cmp);
static ccc_threeway_cmp force_find_les(ccc_key_cmp);
/* The key comes first. It is the "left hand side" of the comparison. */
static ccc_threeway_cmp cmp(struct ccc_tree_ const *, void const *key,
                            struct ccc_node_ const *, ccc_key_cmp_fn *);

/* ===========================    Interface     ============================ */

void *
ccc_omm_get_key_val(ccc_ordered_multimap *const mm, void const *const key)
{
    if (!mm || !key)
    {
        return NULL;
    }
    return find(&mm->impl_, key);
}

ccc_tribool
ccc_omm_is_empty(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return CCC_BOOL_ERR;
    }
    return empty(&mm->impl_);
}

ccc_ommap_entry
ccc_omm_entry(ccc_ordered_multimap *const mm, void const *const key)
{
    if (!mm || !key)
    {
        return (ccc_ommap_entry){
            {.t_ = NULL, .entry_ = {.e_ = NULL, .stats_ = CCC_ARG_ERROR}}};
    }
    return (ccc_ommap_entry){container_entry(&mm->impl_, key)};
}

void *
ccc_omm_insert_entry(ccc_ommap_entry const *const e,
                     ccc_ommap_elem *const key_val_handle)
{
    if (!e || !key_val_handle)
    {
        return NULL;
    }
    return multimap_insert(e->impl_.t_, &key_val_handle->impl_).e_;
}

void *
ccc_omm_or_insert(ccc_ommap_entry const *const e,
                  ccc_ommap_elem *const key_val_handle)
{
    if (!e || !key_val_handle)
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ & CCC_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    return multimap_insert(e->impl_.t_, &key_val_handle->impl_).e_;
}

ccc_ommap_entry *
ccc_omm_and_modify(ccc_ommap_entry *const e, ccc_update_fn *const fn)
{
    if (!e || !fn)
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ & CCC_OCCUPIED)
    {
        fn((ccc_user_type){.user_type = e->impl_.entry_.e_, .aux = NULL});
    }
    return e;
}

ccc_ommap_entry *
ccc_omm_and_modify_aux(ccc_ommap_entry *const e, ccc_update_fn *const fn,
                       void *const aux)
{
    if (!e || !fn)
    {
        return NULL;
    }
    if (e->impl_.entry_.stats_ & CCC_OCCUPIED)
    {
        fn((ccc_user_type){.user_type = e->impl_.entry_.e_, .aux = aux});
    }
    return e;
}

ccc_entry
ccc_omm_swap_entry(ccc_ordered_multimap *const mm,
                   ccc_ommap_elem *const key_val_handle)
{
    if (!mm || !key_val_handle)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ARG_ERROR}};
    }
    struct ccc_node_ *n = &key_val_handle->impl_;
    if (mm->impl_.alloc_)
    {
        void *const mem
            = mm->impl_.alloc_(NULL, mm->impl_.elem_sz_, mm->impl_.aux_);
        if (!mem)
        {
            return (ccc_entry){{.e_ = NULL, .stats_ = CCC_INSERT_ERROR}};
        }
        (void)memcpy(mem, struct_base(&mm->impl_, &key_val_handle->impl_),
                     mm->impl_.elem_sz_);
        n = elem_in_slot(&mm->impl_, mem);
    }
    return (ccc_entry){multimap_insert(&mm->impl_, n)};
}

ccc_entry
ccc_omm_try_insert(ccc_ordered_multimap *const mm,
                   ccc_ommap_elem *const key_val_handle)
{
    if (!mm || !key_val_handle)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ARG_ERROR}};
    }
    void const *const key = key_from_node(&mm->impl_, &key_val_handle->impl_);
    void *const found = find(&mm->impl_, key);
    if (found)
    {
        return (ccc_entry){{.e_ = found, .stats_ = CCC_OCCUPIED}};
    }
    return (ccc_entry){multimap_insert(&mm->impl_, &key_val_handle->impl_)};
}

ccc_entry
ccc_omm_insert_or_assign(ccc_ordered_multimap *const mm,
                         ccc_ommap_elem *const key_val_handle)
{
    if (!mm || !key_val_handle)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ARG_ERROR}};
    }
    void *const found
        = find(&mm->impl_, key_from_node(&mm->impl_, &key_val_handle->impl_));
    if (found)
    {
        key_val_handle->impl_ = *elem_in_slot(&mm->impl_, found);
        assert(mm->impl_.root_ != &mm->impl_.end_);
        memcpy(found, struct_base(&mm->impl_, &key_val_handle->impl_),
               mm->impl_.elem_sz_);
        return (ccc_entry){{.e_ = found, .stats_ = CCC_OCCUPIED}};
    }
    return (ccc_entry){multimap_insert(&mm->impl_, &key_val_handle->impl_)};
}

ccc_entry
ccc_omm_remove(ccc_ordered_multimap *const mm, ccc_ommap_elem *const out_handle)
{

    if (!mm || !out_handle)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ARG_ERROR}};
    }
    void *const n = multimap_erase(
        &mm->impl_, key_from_node(&mm->impl_, &out_handle->impl_));
    if (!n)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_VACANT}};
    }
    if (mm->impl_.alloc_)
    {
        void *const user_struct = struct_base(&mm->impl_, &out_handle->impl_);
        memcpy(user_struct, n, mm->impl_.elem_sz_);
        mm->impl_.alloc_(n, 0, mm->impl_.aux_);
        return (ccc_entry){{.e_ = user_struct, .stats_ = CCC_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = n, .stats_ = CCC_OCCUPIED}};
}

ccc_entry
ccc_omm_remove_entry(ccc_ommap_entry *const e)
{
    if (!e)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ARG_ERROR}};
    }
    if (e->impl_.entry_.stats_ == CCC_OCCUPIED)
    {
        void *const erased = multimap_erase(
            e->impl_.t_, key_in_slot(e->impl_.t_, e->impl_.entry_.e_));
        assert(erased);
        if (e->impl_.t_->alloc_)
        {
            e->impl_.t_->alloc_(erased, 0, e->impl_.t_->aux_);
            return (ccc_entry){{.e_ = NULL, .stats_ = CCC_OCCUPIED}};
        }
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_VACANT}};
}

void *
ccc_omm_max(ccc_ordered_multimap *const mm)
{
    if (!mm)
    {
        return NULL;
    }
    struct ccc_node_ const *const n
        = splay(&mm->impl_, mm->impl_.root_, NULL, force_find_grt);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&mm->impl_, n);
}

void *
ccc_omm_min(ccc_ordered_multimap *const mm)
{
    if (!mm)
    {
        return NULL;
    }
    struct ccc_node_ const *const n
        = splay(&mm->impl_, mm->impl_.root_, NULL, force_find_les);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&mm->impl_, n);
}

void *
ccc_omm_begin(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return NULL;
    }
    return max(&mm->impl_);
}

void *
ccc_omm_rbegin(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return NULL;
    }
    return min(&mm->impl_);
}

void *
ccc_omm_next(ccc_ordered_multimap const *const mm,
             ccc_ommap_elem const *const iter_handle)
{
    if (!mm || !iter_handle)
    {
        return NULL;
    }
    struct ccc_node_ const *const n = multimap_next(
        &mm->impl_, &iter_handle->impl_, reverse_inorder_traversal);
    return n == &mm->impl_.end_ ? NULL : struct_base(&mm->impl_, n);
}

void *
ccc_omm_rnext(ccc_ordered_multimap const *const mm,
              ccc_ommap_elem const *const iter_handle)
{
    if (!mm || !iter_handle)
    {
        return NULL;
    }
    struct ccc_node_ const *const n
        = multimap_next(&mm->impl_, &iter_handle->impl_, inorder_traversal);
    return n == &mm->impl_.end_ ? NULL : struct_base(&mm->impl_, n);
}

void *
ccc_omm_end(ccc_ordered_multimap const *const)
{
    return NULL;
}

void *
ccc_omm_rend(ccc_ordered_multimap const *const)
{
    return NULL;
}

ccc_range
ccc_omm_equal_range(ccc_ordered_multimap *const mm, void const *const begin_key,
                    void const *const end_key)
{
    if (!mm || !begin_key || !end_key)
    {
        return (ccc_range){};
    }
    return (ccc_range){
        equal_range(&mm->impl_, begin_key, end_key, reverse_inorder_traversal)};
}

ccc_rrange
ccc_omm_equal_rrange(ccc_ordered_multimap *const mm,
                     void const *const rbegin_key, void const *const rend_key)
{
    if (!mm || !rbegin_key || !rend_key)
    {
        return (ccc_rrange){};
    }
    return (ccc_rrange){
        equal_range(&mm->impl_, rbegin_key, rend_key, inorder_traversal)};
}

void *
ccc_omm_extract(ccc_ordered_multimap *const mm,
                ccc_ommap_elem *const key_val_handle)
{
    if (!mm || !key_val_handle)
    {
        return NULL;
    }
    return multimap_erase_node(&mm->impl_, &key_val_handle->impl_);
}

ccc_tribool
ccc_omm_update(ccc_ordered_multimap *const mm,
               ccc_ommap_elem *const key_val_handle, ccc_update_fn *const fn,
               void *const aux)
{
    if (!mm || !key_val_handle || !fn || !key_val_handle->impl_.branch_[L]
        || !key_val_handle->impl_.branch_[R])
    {
        return CCC_BOOL_ERR;
    }
    void *const e = multimap_erase_node(&mm->impl_, &key_val_handle->impl_);
    if (!e)
    {
        return CCC_FALSE;
    }
    fn((ccc_user_type){e, aux});
    (void)multimap_insert(&mm->impl_, &key_val_handle->impl_);
    return CCC_TRUE;
}

ccc_tribool
ccc_omm_increase(ccc_ordered_multimap *const mm,
                 ccc_ommap_elem *const key_val_handle, ccc_update_fn *const fn,
                 void *const aux)
{
    return ccc_omm_update(mm, key_val_handle, fn, aux);
}

ccc_tribool
ccc_omm_decrease(ccc_ordered_multimap *const mm,
                 ccc_ommap_elem *const key_val_handle, ccc_update_fn *const fn,
                 void *const aux)
{
    return ccc_omm_update(mm, key_val_handle, fn, aux);
}

ccc_tribool
ccc_omm_contains(ccc_ordered_multimap *const mm, void const *const key)
{
    if (!mm || !key)
    {
        return CCC_BOOL_ERR;
    }
    return contains(&mm->impl_, key);
}

ccc_result
ccc_omm_pop_max(ccc_ordered_multimap *const mm)
{
    if (!mm)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    void *const n = pop_max(&mm->impl_);
    if (!n)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (mm->impl_.alloc_)
    {
        mm->impl_.alloc_(n, 0, mm->impl_.aux_);
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
    struct ccc_node_ *const n = pop_min(&mm->impl_);
    if (!n)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    if (mm->impl_.alloc_)
    {
        mm->impl_.alloc_(struct_base(&mm->impl_, n), 0, mm->impl_.aux_);
    }
    return CCC_RESULT_OK;
}

size_t
ccc_omm_size(ccc_ordered_multimap const *const mm)
{
    return mm ? mm->impl_.size_ : 0;
}

void *
ccc_omm_unwrap(ccc_ommap_entry const *const e)
{
    return ccc_entry_unwrap(&(ccc_entry){e->impl_.entry_});
}

ccc_tribool
ccc_omm_insert_error(ccc_ommap_entry const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.entry_.stats_ & CCC_INSERT_ERROR) != 0;
}

ccc_tribool
ccc_omm_input_error(ccc_ommap_entry const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.entry_.stats_ & CCC_ARG_ERROR) != 0;
}

ccc_tribool
ccc_omm_occupied(ccc_ommap_entry const *const e)
{
    if (!e)
    {
        return CCC_BOOL_ERR;
    }
    return (e->impl_.entry_.stats_ & CCC_OCCUPIED) != 0;
}

ccc_entry_status
ccc_omm_entry_status(ccc_ommap_entry const *const e)
{
    return e ? e->impl_.entry_.stats_ : CCC_ARG_ERROR;
}

ccc_tribool
ccc_omm_validate(ccc_ordered_multimap const *const mm)
{
    if (!mm)
    {
        return CCC_BOOL_ERR;
    }
    return ccc_tree_validate(&mm->impl_);
}

ccc_result
ccc_omm_clear(ccc_ordered_multimap *const mm,
              ccc_destructor_fn *const destructor)
{
    if (!mm)
    {
        return CCC_RESULT_ARG_ERROR;
    }
    while (!ccc_omm_is_empty(mm))
    {
        void *const popped = pop_min(&mm->impl_);
        if (destructor)
        {
            destructor(
                (ccc_user_type){.user_type = popped, .aux = mm->impl_.aux_});
        }
        if (mm->impl_.alloc_)
        {
            (void)mm->impl_.alloc_(popped, 0, mm->impl_.aux_);
        }
    }
    return CCC_RESULT_OK;
}

/*==========================  Private Interface  ============================*/

struct ccc_tree_entry_
ccc_impl_omm_entry(struct ccc_tree_ *const t, void const *const key)
{
    return container_entry(t, key);
}

void *
ccc_impl_omm_multimap_insert(struct ccc_tree_ *const t,
                             struct ccc_node_ *const n)
{
    return multimap_insert(t, n).e_;
}

void *
ccc_impl_omm_key_in_slot(struct ccc_tree_ const *const t,
                         void const *const slot)
{
    return key_in_slot(t, slot);
}

struct ccc_node_ *
ccc_impl_ommap_elem_in_slot(struct ccc_tree_ const *const t,
                            void const *const slot)
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
            .t_ = t, .entry_ = {.e_ = found, .stats_ = CCC_OCCUPIED}};
    }
    return (struct ccc_tree_entry_){
        .t_ = t, .entry_ = {.e_ = NULL, .stats_ = CCC_VACANT}};
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

static inline void
init_node(struct ccc_tree_ *const t, struct ccc_node_ *const n)
{
    n->branch_[L] = &t->end_;
    n->branch_[R] = &t->end_;
    n->parent_ = &t->end_;
}

static inline ccc_tribool
empty(struct ccc_tree_ const *const t)
{
    return !t->size_;
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
min(struct ccc_tree_ const *const t)
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

static inline void *
pop_max(struct ccc_tree_ *const t)
{
    return multimap_erase_max_or_min(t, force_find_grt);
}

static inline void *
pop_min(struct ccc_tree_ *const t)
{
    return multimap_erase_max_or_min(t, force_find_les);
}

static inline ccc_tribool
is_dup_head_next(struct ccc_node_ const *const i)
{
    return i->link_[R]->parent_ != NULL;
}

static inline ccc_tribool
is_dup_head(struct ccc_node_ const *const end, struct ccc_node_ const *const i)
{
    return i != end && i->link_[P] != end && i->link_[P]->link_[N] == i;
}

static inline struct ccc_node_ const *
multimap_next(struct ccc_tree_ const *const t, struct ccc_node_ const *const i,
              enum tree_link_ const traversal)
{
    /* An arbitrary node in a doubly linked list of duplicates. */
    if (NULL == i->parent_)
    {
        /* We have finished the lap around the duplicate list. */
        if (is_dup_head_next(i))
        {
            return next_tree_node(t, i->link_[N], traversal);
        }
        return i->link_[N];
    }
    /* The special head node of a doubly linked list of duplicates. */
    if (is_dup_head(&t->end_, i))
    {
        /* The duplicate head can be the only node in the list. */
        if (is_dup_head_next(i))
        {
            return next_tree_node(t, i, traversal);
        }
        return i->link_[N];
    }
    if (has_dups(&t->end_, i))
    {
        return i->dup_head_;
    }
    return next(t, i, traversal);
}

static inline struct ccc_node_ const *
next_tree_node(struct ccc_tree_ const *const t,
               struct ccc_node_ const *const head,
               enum tree_link_ const traversal)
{
    if (head->parent_ == &t->end_)
    {
        return next(t, t->root_, traversal);
    }
    struct ccc_node_ const *parent = head->parent_;
    if (parent->branch_[L] != &t->end_ && parent->branch_[L]->dup_head_ == head)
    {
        return next(t, parent->branch_[L], traversal);
    }
    if (parent->branch_[R] != &t->end_ && parent->branch_[R]->dup_head_ == head)
    {
        return next(t, parent->branch_[R], traversal);
    }
    return &t->end_;
}

static inline struct ccc_node_ const *
next(struct ccc_tree_ const *const t, struct ccc_node_ const *n,
     enum tree_link_ const traversal)
{
    if (!n || n == &t->end_)
    {
        return NULL;
    }
    assert(parent_r(t, t->root_) == &t->end_);
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
    struct ccc_node_ const *p = parent_r(t, n);
    for (; p != &t->end_ && p->branch_[!traversal] != n;
         n = p, p = parent_r(t, n))
    {}
    return p;
}

static inline struct ccc_range_u_
equal_range(struct ccc_tree_ *const t, void const *const begin_key,
            void const *const end_key, enum tree_link_ const traversal)
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

static inline ccc_tribool
contains(struct ccc_tree_ *const t, void const *const key)
{
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp(t, key, t->root_, t->cmp_) == CCC_EQL;
}

static inline struct ccc_ent_
multimap_insert(struct ccc_tree_ *const t, struct ccc_node_ *const out_handle)
{
    init_node(t, out_handle);
    if (empty(t))
    {
        t->root_ = out_handle;
        t->size_ = 1;
        return (struct ccc_ent_){.e_ = struct_base(t, out_handle),
                                 .stats_ = CCC_VACANT};
    }
    t->size_++;
    void const *const key = key_from_node(t, out_handle);
    t->root_ = splay(t, t->root_, key, t->cmp_);

    ccc_threeway_cmp const root_cmp = cmp(t, key, t->root_, t->cmp_);
    if (CCC_EQL == root_cmp)
    {
        add_duplicate(t, t->root_, out_handle, &t->end_);
        return (struct ccc_ent_){.e_ = struct_base(t, out_handle),
                                 .stats_ = CCC_OCCUPIED};
    }
    return (struct ccc_ent_){.e_ = connect_new_root(t, out_handle, root_cmp),
                             .stats_ = CCC_VACANT};
}

static inline void *
connect_new_root(struct ccc_tree_ *const t, struct ccc_node_ *const new_root,
                 ccc_threeway_cmp const cmp_result)
{
    enum tree_link_ const link = CCC_GRT == cmp_result;
    link_trees(t, new_root, link, t->root_->branch_[link]);
    link_trees(t, new_root, !link, t->root_);
    t->root_->branch_[link] = &t->end_;
    t->root_ = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link_trees(t, &t->end_, 0, t->root_);
    return struct_base(t, new_root);
}

static inline void
add_duplicate(struct ccc_tree_ *const t, struct ccc_node_ *const tree_node,
              struct ccc_node_ *const add, struct ccc_node_ *const parent)
{
    /* This is a circular doubly linked list with O(1) append to back
       to maintain round robin fairness for any use of this queue.
       The oldest duplicate should be in the tree so we will add new dup
       to the back. The head then needs to point to new tail and new
       tail points to already in place head that tree points to.
       This operation still works if we previously had size 1 list. */
    if (!has_dups(&t->end_, tree_node))
    {
        add->parent_ = parent;
        tree_node->dup_head_ = add;
        add->link_[N] = add;
        add->link_[P] = add;
        return;
    }
    add->parent_ = NULL;
    struct ccc_node_ *const list_head = tree_node->dup_head_;
    struct ccc_node_ *const tail = list_head->link_[P];
    tail->link_[N] = add;
    list_head->link_[P] = add;
    add->link_[N] = list_head;
    add->link_[P] = tail;
}

static inline void *
multimap_erase(struct ccc_tree_ *const t, void const *const key)
{
    if (!t || !key)
    {
        return NULL;
    }
    if (empty(t))
    {
        return NULL;
    }
    struct ccc_node_ *ret = splay(t, t->root_, key, t->cmp_);
    if (cmp(t, key, ret, t->cmp_) != CCC_EQL)
    {
        return NULL;
    }
    --t->size_;
    if (has_dups(&t->end_, ret))
    {
        ret = pop_front_dup(t, ret, key_from_node(t, ret));
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->branch_[L] = ret->branch_[R] = ret->parent_ = NULL;
    return struct_base(t, ret);
}

/* We need to mindful of what the user is asking for. If they want any
   max or min, we have provided a dummy node and dummy compare function
   that will force us to return the max or min. So this operation
   simply grabs the first node available in the tree for round robin.
   This function expects to be passed the t->impl_il as the node and a
   comparison function that forces either the max or min to be searched. */
static inline void *
multimap_erase_max_or_min(struct ccc_tree_ *const t,
                          ccc_key_cmp_fn *const force_max_or_min)
{
    if (!t || !force_max_or_min)
    {
        return NULL;
    }
    if (empty(t))
    {
        return NULL;
    }
    t->size_--;
    struct ccc_node_ *ret = splay(t, t->root_, NULL, force_max_or_min);
    if (has_dups(&t->end_, ret))
    {
        ret = pop_front_dup(t, ret, key_from_node(t, ret));
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->branch_[L] = ret->branch_[R] = ret->parent_ = NULL;
    return struct_base(t, ret);
}

/* We need to mindful of what the user is asking for. This is a request
   to erase the exact node provided in the argument. So extra care is
   taken to only delete that node, especially if a different node with
   the same size is splayed to the root and we are a duplicate in the
   list. */
static inline void *
multimap_erase_node(struct ccc_tree_ *const t, struct ccc_node_ *const node)
{
    /* This is what we set removed nodes to so this is a mistaken query */
    if (NULL == node->branch_[R] || NULL == node->branch_[L])
    {
        return NULL;
    }
    if (empty(t))
    {
        return NULL;
    }
    t->size_--;
    /* Special case that this must be a duplicate that is in the
       linked list but it is not the special head node. So, it
       is a quick snip to get it out. */
    if (!node->parent_)
    {
        assert(node->link_[P] && node->link_[N]
               && node->link_[N]->link_[P] == node
               && node->link_[P]->link_[N] == node);
        node->link_[P]->link_[N] = node->link_[N];
        node->link_[N]->link_[P] = node->link_[P];
        node->link_[N] = node->link_[P] = node->parent_ = NULL;
        return struct_base(t, node);
    }
    void const *const key = key_from_node(t, node);
    struct ccc_node_ *ret = splay(t, t->root_, key, t->cmp_);
    if (cmp(t, key, ret, t->cmp_) != CCC_EQL)
    {
        return NULL;
    }
    if (has_dups(&t->end_, ret))
    {
        ret = pop_dup_node(t, node, ret);
    }
    else
    {
        ret = remove_from_tree(t, ret);
    }
    ret->branch_[L] = ret->branch_[R] = ret->parent_ = NULL;
    return struct_base(t, ret);
}

/* This function assumes that splayed is the new root of the tree */
static inline struct ccc_node_ *
pop_dup_node(struct ccc_tree_ *const t, struct ccc_node_ *const dup,
             struct ccc_node_ *const splayed)
{
    if (dup == splayed)
    {
        return pop_front_dup(t, splayed, key_from_node(t, splayed));
    }
    /* This is the head of the list of duplicates and no dups left. */
    if (dup->link_[N] == dup)
    {
        splayed->parent_ = &t->end_;
        return dup;
    }
    /* The dup is the head. There is an arbitrary number of dups after the
       head so replace head. Update the tail at back of the list. Easy to
       forget hard to catch because bugs are often delayed. */
    dup->link_[P]->link_[N] = dup->link_[N];
    dup->link_[N]->link_[P] = dup->link_[P];
    dup->link_[N]->parent_ = dup->parent_;
    splayed->dup_head_ = dup->link_[N];
    return dup;
}

/* A node is in the tree and has a trailing list of duplicates. Many updates
must occur to remove the first duplicate from the list and make it part of the
splay tree to replace the old node to be deleted. */
static inline struct ccc_node_ *
pop_front_dup(struct ccc_tree_ *const t, struct ccc_node_ *const old,
              void const *const old_key)
{
    struct ccc_node_ *const parent = old->dup_head_->parent_;
    struct ccc_node_ *const tree_replacement = old->dup_head_;
    /* Comparing sizes with the root's parent is undefined. */
    if (old == t->root_)
    {
        t->root_ = tree_replacement;
    }
    else
    {
        parent->branch_[CCC_GRT == cmp(t, old_key, parent, t->cmp_)]
            = tree_replacement;
    }

    struct ccc_node_ *const new_list_head = old->dup_head_->link_[N];
    struct ccc_node_ *const list_tail = old->dup_head_->link_[P];
    ccc_tribool const circular_list_empty
        = new_list_head->link_[N] == new_list_head;

    new_list_head->link_[P] = list_tail;
    new_list_head->parent_ = parent;
    list_tail->link_[N] = new_list_head;
    tree_replacement->branch_[L] = old->branch_[L];
    tree_replacement->branch_[R] = old->branch_[R];
    tree_replacement->dup_head_ = new_list_head;

    link_trees(t, tree_replacement, L, tree_replacement->branch_[L]);
    link_trees(t, tree_replacement, R, tree_replacement->branch_[R]);
    if (circular_list_empty)
    {
        tree_replacement->parent_ = parent;
    }
    return old;
}

static inline struct ccc_node_ *
remove_from_tree(struct ccc_tree_ *const t, struct ccc_node_ *const ret)
{
    if (ret->branch_[L] == &t->end_)
    {
        t->root_ = ret->branch_[R];
        link_trees(t, &t->end_, 0, t->root_);
    }
    else
    {
        t->root_ = splay(t, ret->branch_[L], key_from_node(t, ret), t->cmp_);
        link_trees(t, t->root_, R, ret->branch_[R]);
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
    do
    {
        ccc_threeway_cmp const root_cmp = cmp(t, key, root, cmp_fn);
        enum tree_link_ const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || root->branch_[dir] == &t->end_)
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp(t, key, root->branch_[dir], cmp_fn);
        enum tree_link_ const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            struct ccc_node_ *const pivot = root->branch_[dir];
            link_trees(t, root, dir, pivot->branch_[!dir]);
            link_trees(t, pivot, !dir, root);
            root = pivot;
            if (root->branch_[dir] == &t->end_)
            {
                break;
            }
        }
        link_trees(t, l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = root->branch_[dir];
    } while (1);
    link_trees(t, l_r_subtrees[L], R, root->branch_[L]);
    link_trees(t, l_r_subtrees[R], L, root->branch_[R]);
    link_trees(t, root, L, t->end_.branch_[R]);
    link_trees(t, root, R, t->end_.branch_[L]);
    t->root_ = root;
    link_trees(t, &t->end_, 0, t->root_);
    return root;
}

/* This function has proven to be VERY important. The nil node often
   has garbage values associated with real nodes in our tree and if we access
   them by mistake it's bad! But the nil is also helpful for some invariant
   coding patters and reducing if checks all over the place. Links a parent
   to a subtree updating the parents child pointer in the direction specified
   and updating the subtree parent field to point back to parent. This last
   step is critical and easy to miss or mess up. */
static inline void
link_trees(struct ccc_tree_ *const t, struct ccc_node_ *const parent,
           enum tree_link_ const dir, struct ccc_node_ *const subtree)
{
    parent->branch_[dir] = subtree;
    if (has_dups(&t->end_, subtree))
    {
        subtree->dup_head_->parent_ = parent;
        return;
    }
    subtree->parent_ = parent;
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
has_dups(struct ccc_node_ const *const end, struct ccc_node_ const *const n)
{
    return n != end && n->dup_head_ != end && n->dup_head_->link_[P] != end
           && n->dup_head_->link_[P]->link_[N] == n->dup_head_;
}

static inline struct ccc_node_ *
parent_r(struct ccc_tree_ const *const t, struct ccc_node_ const *const n)
{
    return has_dups(&t->end_, n) ? n->dup_head_->parent_ : n->parent_;
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

static inline void *
key_in_slot(struct ccc_tree_ const *const t, void const *const slot)
{
    return (char *)slot + t->key_offset_;
}

static inline void *
key_from_node(struct ccc_tree_ const *const t, struct ccc_node_ const *const n)
{
    return (char *)struct_base(t, n) + t->key_offset_;
}

static inline struct ccc_node_ *
elem_in_slot(struct ccc_tree_ const *const t, void const *const slot)
{
    return (struct ccc_node_ *)((char *)slot + t->node_elem_offset_);
}

/* We can trick our splay tree into giving us the max via splaying
   without any input from the user. Our seach evaluates a threeway
   comparison to decide which branch to take in the tree or if we
   found the desired element. Simply force the function to always
   return one or the other and we will end up at the max or min */
static inline ccc_threeway_cmp
force_find_grt(ccc_key_cmp const)
{
    return CCC_GRT;
}

static inline ccc_threeway_cmp
force_find_les(ccc_key_cmp const)
{
    return CCC_LES;
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
    ccc_tribool correct;
    struct ccc_node_ const *parent;
};

static inline size_t
count_dups(struct ccc_tree_ const *const t, struct ccc_node_ const *const n)
{
    if (!has_dups(&t->end_, n))
    {
        return 0;
    }
    size_t dups = 1;
    for (struct ccc_node_ *cur = n->dup_head_->link_[N]; cur != n->dup_head_;
         cur = cur->link_[N])
    {
        ++dups;
    }
    return dups;
}

static size_t
recursive_size(struct ccc_tree_ const *const t, struct ccc_node_ const *const r)
{
    if (r == &t->end_)
    {
        return 0;
    }
    size_t s = count_dups(t, r) + 1;
    return s + recursive_size(t, r->branch_[R])
           + recursive_size(t, r->branch_[L]);
}

static ccc_tribool
are_subtrees_valid(struct ccc_tree_ const *const t, struct tree_range_ const r,
                   struct ccc_node_ const *const nil)
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
is_duplicate_storing_parent(struct ccc_tree_ const *const t,
                            struct ccc_node_ const *const parent,
                            struct ccc_node_ const *const root)
{
    if (root == &t->end_)
    {
        return CCC_TRUE;
    }
    if (has_dups(&t->end_, root))
    {
        if (root->dup_head_->parent_ != parent)
        {
            return CCC_FALSE;
        }
    }
    else if (root->parent_ != parent)
    {
        return CCC_FALSE;
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
static inline ccc_tribool
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
        return CCC_FALSE;
    }
    if (!is_duplicate_storing_parent(t, &t->end_, t->root_))
    {
        return CCC_FALSE;
    }
    if (recursive_size(t, t->root_) != t->size_)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
