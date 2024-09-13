#include "ordered_map.h"
#include "impl_ordered_map.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_BLK "\033[34;1m"
#define COLOR_BLU_BOLD "\033[38;5;12m"
#define COLOR_RED_BOLD "\033[38;5;9m"
#define COLOR_RED "\033[31;1m"
#define COLOR_CYN "\033[36;1m"
#define COLOR_GRN "\033[32;1m"
#define COLOR_NIL "\033[0m"
#define COLOR_ERR COLOR_RED "Error: " COLOR_NIL
#define PRINTER_INDENT (short)13
#define LR 2

/* Instead of thinking about left and right consider only links
   in the abstract sense. Put them in an array and then flip
   this enum and left and right code paths can be united into one */
typedef enum
{
    L = 0,
    R = 1
} om_link;

/* Trees are just a different interpretation of the same links used
   for doubly linked lists. We take advantage of this for duplicates. */
typedef enum
{
    P = 0,
    N = 1
} om_list_link;

/* Printing enum for printing tree structures if heap available. */
typedef enum
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
} om_print_link;

static om_link const inorder_traversal = R;
static om_link const reverse_inorder_traversal = L;

/* No return value. */

static void init_node(struct ccc_tree_ *, struct ccc_node_ *);
static void swap(uint8_t tmp[], void *, void *, size_t);
static void link_trees(struct ccc_node_ *, om_link, struct ccc_node_ *);
static void ccc_tree_tree_print(struct ccc_tree_ const *t,
                                struct ccc_node_ const *root,
                                ccc_print_fn *fn_print);

/* Boolean returns */

static bool empty(struct ccc_tree_ const *);
static bool contains(struct ccc_tree_ *, void const *);
static bool ccc_tree_validate(struct ccc_tree_ const *);

/* Returning the user type that is stored in data structure. */

static void *struct_base(struct ccc_tree_ const *, struct ccc_node_ const *);
static void *find(struct ccc_tree_ *, void const *);
static void *erase(struct ccc_tree_ *, void const *key);
static void *alloc_insert(struct ccc_tree_ *, struct ccc_node_ *);
static void *connect_new_root(struct ccc_tree_ *, struct ccc_node_ *,
                              ccc_threeway_cmp);
static void *max(struct ccc_tree_ const *);
static void *min(struct ccc_tree_ const *);
static ccc_range equal_range(struct ccc_tree_ *, void const *, void const *,
                             om_link);

/* Internal operations that take and return nodes for the tree. */

static struct ccc_node_ *root(struct ccc_tree_ const *);
static struct ccc_node_ *remove_from_tree(struct ccc_tree_ *,
                                          struct ccc_node_ *);
static struct ccc_node_ const *next(struct ccc_tree_ const *,
                                    struct ccc_node_ const *, om_link);
static struct ccc_node_ *splay(struct ccc_tree_ *, struct ccc_node_ *,
                               void const *key, ccc_key_cmp_fn *);

/* The key comes first. It is the "left hand side" of the comparison. */
static ccc_threeway_cmp cmp(struct ccc_tree_ const *, void const *key,
                            struct ccc_node_ const *, ccc_key_cmp_fn *);

/* ======================        Map Interface      ====================== */

void
ccc_om_clear(ccc_ordered_map *const set, ccc_destructor_fn *const destructor)
{

    while (!ccc_om_empty(set))
    {
        void *popped
            = erase(&set->impl_,
                    ccc_impl_om_key_from_node(&set->impl_, set->impl_.root_));
        if (destructor)
        {
            destructor(popped);
        }
        if (set->impl_.alloc_)
        {
            set->impl_.alloc_(popped, 0);
        }
    }
}

bool
ccc_om_empty(ccc_ordered_map const *const s)
{
    return empty(&s->impl_);
}

size_t
ccc_om_size(ccc_ordered_map const *const s)
{
    return s->impl_.size_;
}

bool
ccc_om_contains(ccc_ordered_map *s, void const *const key)
{
    return contains(&s->impl_, key);
}

struct ccc_tree_entry_
ccc_impl_om_entry(struct ccc_tree_ *t, void const *key)
{
    void *found = find(t, key);
    if (found)
    {
        return (struct ccc_tree_entry_){
            .t_ = t,
            .entry_ = {
                .e_ = found,
                .stats_ = CCC_TREE_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct ccc_tree_entry_){
        .t_ = t,
        .entry_ = {
            .e_ = found,
            .stats_ = CCC_TREE_ENTRY_VACANT,
        },
    };
}

ccc_o_map_entry
ccc_om_entry(ccc_ordered_map *const s, void const *const key)
{
    return (ccc_o_map_entry){ccc_impl_om_entry(&s->impl_, key)};
}

void *
ccc_om_insert_entry(ccc_o_map_entry const *const e, ccc_o_map_elem *const elem)
{
    if (e->impl_.entry_.stats_ == CCC_TREE_ENTRY_OCCUPIED)
    {
        elem->impl_
            = *ccc_impl_om_elem_in_slot(e->impl_.t_, e->impl_.entry_.e_);
        memcpy(e->impl_.entry_.e_, struct_base(e->impl_.t_, &elem->impl_),
               e->impl_.t_->elem_sz_);
        return e->impl_.entry_.e_;
    }
    return alloc_insert(e->impl_.t_, &elem->impl_);
}

void *
ccc_om_or_insert(ccc_o_map_entry const *const e, ccc_o_map_elem *const elem)
{
    if (e->impl_.entry_.stats_ & CCC_TREE_ENTRY_OCCUPIED)
    {
        return NULL;
    }
    return alloc_insert(e->impl_.t_, &elem->impl_);
}

ccc_o_map_entry
ccc_om_and_modify(ccc_o_map_entry const *const e, ccc_update_fn *const fn)
{
    if (e->impl_.entry_.stats_ & CCC_TREE_ENTRY_OCCUPIED)
    {
        fn((ccc_update){
            .container = e->impl_.entry_.e_,
            .aux = NULL,
        });
    }
    return *e;
}

ccc_o_map_entry
ccc_om_and_modify_with(ccc_o_map_entry const *const e, ccc_update_fn *fn,
                       void *aux)
{
    if (e->impl_.entry_.stats_ & CCC_TREE_ENTRY_OCCUPIED)
    {
        fn((ccc_update){
            .container = e->impl_.entry_.e_,
            .aux = aux,
        });
    }
    return *e;
}

ccc_entry
ccc_om_insert(ccc_ordered_map *const s, ccc_o_map_elem *const out_handle)
{
    void *found = find(
        &s->impl_, ccc_impl_om_key_from_node(&s->impl_, &out_handle->impl_));
    if (found)
    {
        assert(s->impl_.root_ != &s->impl_.end_);
        *out_handle = *(ccc_o_map_elem *)s->impl_.root_;
        uint8_t tmp[s->impl_.elem_sz_];
        void *user_struct = struct_base(&s->impl_, &out_handle->impl_);
        void *ret = struct_base(&s->impl_, s->impl_.root_);
        swap(tmp, user_struct, ret, s->impl_.elem_sz_);
        return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted = alloc_insert(&s->impl_, &out_handle->impl_);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_om_remove(ccc_ordered_map *const s, ccc_o_map_elem *const out_handle)
{
    void *n = erase(&s->impl_,
                    ccc_impl_om_key_from_node(&s->impl_, &out_handle->impl_));
    if (!n)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    if (s->impl_.alloc_)
    {
        void *user_struct = struct_base(&s->impl_, &out_handle->impl_);
        memcpy(user_struct, n, s->impl_.elem_sz_);
        s->impl_.alloc_(n, 0);
        return (ccc_entry){{.e_ = user_struct, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = n, .stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_entry
ccc_om_remove_entry(ccc_o_map_entry *const e)
{
    if (e->impl_.entry_.stats_ == CCC_TREE_ENTRY_OCCUPIED)
    {
        void *erased
            = erase(e->impl_.t_,
                    ccc_impl_om_key_in_slot(e->impl_.t_, e->impl_.entry_.e_));
        assert(erased);
        if (e->impl_.t_->alloc_)
        {
            e->impl_.t_->alloc_(erased, 0);
            return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_OCCUPIED}};
        }
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

void *
ccc_om_get_mut(ccc_ordered_map *s, void const *const key)
{
    return find(&s->impl_, key);
}

void const *
ccc_om_get(ccc_ordered_map *s, void const *const key)
{
    return find(&s->impl_, key);
}

void *
ccc_om_unwrap_mut(ccc_o_map_entry const *const e)
{
    return (void *)ccc_om_unwrap(e);
}

void const *
ccc_om_unwrap(ccc_o_map_entry const *const e)
{
    if (e->impl_.entry_.stats_ == CCC_TREE_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    return NULL;
}

void *
ccc_om_begin(ccc_ordered_map const *const s)
{
    return min(&s->impl_);
}

void *
ccc_om_rbegin(ccc_ordered_map const *const s)
{
    return max(&s->impl_);
}

void *
ccc_om_end([[maybe_unused]] ccc_ordered_map const *const s)
{
    return NULL;
}

void *
ccc_om_rend([[maybe_unused]] ccc_ordered_map const *const s)
{
    return NULL;
}

void *
ccc_om_next(ccc_ordered_map const *const s, ccc_o_map_elem const *const e)
{
    struct ccc_node_ const *n = next(&s->impl_, &e->impl_, inorder_traversal);
    return n == &s->impl_.end_ ? NULL : struct_base(&s->impl_, n);
}

void *
ccc_om_rnext(ccc_ordered_map const *const s, ccc_o_map_elem const *const e)
{
    struct ccc_node_ const *n
        = next(&s->impl_, &e->impl_, reverse_inorder_traversal);
    return n == &s->impl_.end_ ? NULL : struct_base(&s->impl_, n);
}

ccc_range
ccc_om_equal_range(ccc_ordered_map *s, void const *const begin_key,
                   void const *const end_key)
{
    return equal_range(&s->impl_, begin_key, end_key, inorder_traversal);
}

ccc_rrange
ccc_om_equal_rrange(ccc_ordered_map *s, void const *const rbegin_key,
                    void const *const end_key)

{
    return (ccc_rrange){
        equal_range(&s->impl_, rbegin_key, end_key, reverse_inorder_traversal)
            .impl_};
}

void *
ccc_om_root(ccc_ordered_map const *const s)
{
    struct ccc_node_ *n = root(&s->impl_);
    if (!n)
    {
        return NULL;
    }
    return struct_base(&s->impl_, n);
}

void
ccc_om_print(ccc_ordered_map const *const s, ccc_o_map_elem const *const root,
             ccc_print_fn *const fn)
{
    ccc_tree_tree_print(&s->impl_, &root->impl_, fn);
}

bool
ccc_om_insert_error(ccc_o_map_entry const *const e)
{
    return e->impl_.entry_.stats_ & CCC_TREE_ENTRY_INSERT_ERROR;
}

bool
ccc_om_validate(ccc_ordered_map const *const s)
{
    return ccc_tree_validate(&s->impl_);
}

/*==========================  Private Interface  ============================*/

void *
ccc_impl_om_key_in_slot(struct ccc_tree_ const *const t, void const *const slot)
{
    return (uint8_t *)slot + t->key_offset_;
}

void *
ccc_impl_om_key_from_node(struct ccc_tree_ const *const t,
                          struct ccc_node_ const *const n)
{
    return (uint8_t *)struct_base(t, n) + t->key_offset_;
}

struct ccc_node_ *
ccc_impl_om_elem_in_slot(struct ccc_tree_ const *t, void const *slot)
{

    return (struct ccc_node_ *)((uint8_t *)slot + t->node_elem_offset_);
}

/*======================  Static Splay Tree Helpers  ========================*/

static void
init_node(struct ccc_tree_ *t, struct ccc_node_ *n)
{
    n->branch_[L] = &t->end_;
    n->branch_[R] = &t->end_;
    n->parent_ = &t->end_;
}

static bool
empty(struct ccc_tree_ const *const t)
{
    return !t->size_;
}

static struct ccc_node_ *
root(struct ccc_tree_ const *const t)
{
    return t->root_;
}

static void *
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

static void *
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

static struct ccc_node_ const *
next(struct ccc_tree_ const *const t, struct ccc_node_ const *n,
     om_link const traversal)
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
    /* A leaf. Now it is time to visit the closest parent not yet visited.
       The old stack overflow question I read about this type of iteration
       (Boost's method, can't find the post anymore?) had the sentinel node
       make the root its traversal child, but this means we would have to
       write to the sentinel on every call to next. I want multiple threads to
       iterate freely without undefined data race writes to memory locations.
       So more expensive loop.*/
    struct ccc_node_ *p = n->parent_;
    for (; p != &t->end_ && p->branch_[!traversal] != n; n = p, p = n->parent_)
    {}
    return p;
}

static ccc_range
equal_range(struct ccc_tree_ *t, void const *begin_key, void const *end_key,
            om_link const traversal)
{
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    ccc_threeway_cmp const grt_or_les[2] = {CCC_LES, CCC_GRT};
    struct ccc_node_ const *b = splay(t, t->root_, begin_key, t->cmp_);
    if (cmp(t, begin_key, b, t->cmp_) == grt_or_les[traversal])
    {
        b = next(t, b, traversal);
    }
    struct ccc_node_ const *e = splay(t, t->root_, end_key, t->cmp_);
    if (cmp(t, end_key, e, t->cmp_) == grt_or_les[traversal])
    {
        e = next(t, e, traversal);
    }
    return (ccc_range){{
        .begin_ = b == &t->end_ ? NULL : struct_base(t, b),
        .end_ = e == &t->end_ ? NULL : struct_base(t, e),
    }};
}

static void *
find(struct ccc_tree_ *t, void const *const key)
{
    if (t->root_ == &t->end_)
    {
        return NULL;
    }
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp(t, key, t->root_, t->cmp_) == CCC_EQL ? struct_base(t, t->root_)
                                                     : NULL;
}

static bool
contains(struct ccc_tree_ *t, void const *key)
{
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp(t, key, t->root_, t->cmp_) == CCC_EQL;
}

static void *
alloc_insert(struct ccc_tree_ *t, struct ccc_node_ *out_handle)
{
    init_node(t, out_handle);
    ccc_threeway_cmp root_cmp = CCC_CMP_ERR;
    if (!empty(t))
    {
        void const *const key = ccc_impl_om_key_from_node(t, out_handle);
        t->root_ = splay(t, t->root_, key, t->cmp_);
        root_cmp = cmp(t, key, t->root_, t->cmp_);
        if (CCC_EQL == root_cmp)
        {
            return NULL;
        }
    }
    if (t->alloc_)
    {
        void *node = t->alloc_(NULL, t->elem_sz_);
        if (!node)
        {
            return NULL;
        }
        memcpy(node, struct_base(t, out_handle), t->elem_sz_);
        out_handle = ccc_impl_om_elem_in_slot(t, node);
    }
    if (empty(t))
    {
        t->root_ = out_handle;
        t->size_++;
        return struct_base(t, out_handle);
    }
    assert(root_cmp != CCC_CMP_ERR);
    t->size_++;
    return connect_new_root(t, out_handle, root_cmp);
}

void *
ccc_impl_om_insert(struct ccc_tree_ *t, struct ccc_node_ *n)
{
    init_node(t, n);
    if (empty(t))
    {
        t->root_ = n;
        t->size_++;
        return struct_base(t, n);
    }
    void const *const key = ccc_impl_om_key_from_node(t, n);
    t->root_ = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const root_cmp = cmp(t, key, t->root_, t->cmp_);
    if (CCC_EQL == root_cmp)
    {
        return NULL;
    }
    t->size_++;
    return connect_new_root(t, n, root_cmp);
}

static void *
connect_new_root(struct ccc_tree_ *t, struct ccc_node_ *new_root,
                 ccc_threeway_cmp cmp_result)
{
    om_link const link = CCC_GRT == cmp_result;
    link_trees(new_root, link, t->root_->branch_[link]);
    link_trees(new_root, !link, t->root_);
    t->root_->branch_[link] = &t->end_;
    t->root_ = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link_trees(&t->end_, 0, t->root_);
    return struct_base(t, new_root);
}

static void *
erase(struct ccc_tree_ *t, void const *const key)
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
remove_from_tree(struct ccc_tree_ *t, struct ccc_node_ *ret)
{
    if (ret->branch_[L] == &t->end_)
    {
        t->root_ = ret->branch_[R];
        link_trees(&t->end_, 0, t->root_);
    }
    else
    {
        t->root_ = splay(t, ret->branch_[L], ccc_impl_om_key_from_node(t, ret),
                         t->cmp_);
        link_trees(t->root_, R, ret->branch_[R]);
    }
    return ret;
}

static struct ccc_node_ *
splay(struct ccc_tree_ *t, struct ccc_node_ *root, void const *const key,
      ccc_key_cmp_fn *cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    t->end_.branch_[L] = t->end_.branch_[R] = t->end_.parent_ = &t->end_;
    struct ccc_node_ *l_r_subtrees[LR] = {&t->end_, &t->end_};
    for (;;)
    {
        ccc_threeway_cmp const root_cmp = cmp(t, key, root, cmp_fn);
        om_link const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || root->branch_[dir] == &t->end_)
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp(t, key, root->branch_[dir], cmp_fn);
        om_link const dir_from_child = CCC_GRT == child_cmp;
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
    return ((uint8_t *)n->branch_) - t->node_elem_offset_;
}

static inline ccc_threeway_cmp
cmp(struct ccc_tree_ const *const t, void const *const key,
    struct ccc_node_ const *const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){
        .container = struct_base(t, node),
        .key = key,
        .aux = t->aux_,
    });
}

static inline void
swap(uint8_t tmp[], void *const a, void *const b, size_t elem_sz)
{
    if (a == b)
    {
        return;
    }
    memcpy(tmp, a, elem_sz);
    memcpy(a, b, elem_sz);
    memcpy(b, tmp, elem_sz);
}

static inline void
link_trees(struct ccc_node_ *parent, om_link dir, struct ccc_node_ *subtree)
{
    parent->branch_[dir] = subtree;
    subtree->parent_ = parent;
}

/* NOLINTEND(*swappable-parameters) NOLINTBEGIN(*misc-no-recursion) */

/* ======================        Debugging           ====================== */

/* This section has recursion so it should probably not be used in
   a custom operating system environment with constrained stack space.
   Needless to mention the stdlib.h heap implementation that would need
   to be replaced with the custom OS drop in. */

/* Validate binary tree invariants with ranges. Use a recursive method that
   does not rely upon the implementation of iterators or any other possibly
   buggy implementation. A pure functional range check will provide the most
   reliable check regardless of implementation changes throughout code base. */
struct ccc_tree_range
{
    struct ccc_node_ const *low;
    struct ccc_node_ const *root;
    struct ccc_node_ const *high;
};

struct parent_status
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
are_subtrees_valid(struct ccc_tree_ const *t, struct ccc_tree_range const r,
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
                              (struct ccc_tree_range){
                                  .low = r.low,
                                  .root = r.root->branch_[L],
                                  .high = r.root,
                              },
                              nil)
           && are_subtrees_valid(t,
                                 (struct ccc_tree_range){
                                     .low = r.root,
                                     .root = r.root->branch_[R],
                                     .high = r.high,
                                 },
                                 nil);
}

static struct parent_status
child_tracks_parent(struct ccc_node_ const *const parent,
                    struct ccc_node_ const *const root)
{
    if (root->parent_ != parent)
    {
        struct ccc_node_ *p = root->parent_->parent_;
        return (struct parent_status){false, p};
    }
    return (struct parent_status){true, parent};
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
                            (struct ccc_tree_range){
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

static size_t
get_subtree_size(struct ccc_node_ const *const root, void const *const nil)
{
    if (root == nil)
    {
        return 0;
    }
    return 1 + get_subtree_size(root->branch_[L], nil)
           + get_subtree_size(root->branch_[R], nil);
}

static char const *
get_edge_color(struct ccc_node_ const *const root, size_t const parent_size,
               struct ccc_node_ const *const nil)
{
    if (root == nil)
    {
        return "";
    }
    return get_subtree_size(root, nil) <= parent_size / 2 ? COLOR_BLU_BOLD
                                                          : COLOR_RED_BOLD;
}

static void
print_node(struct ccc_tree_ const *const t,
           struct ccc_node_ const *const parent,
           struct ccc_node_ const *const root, ccc_print_fn *const fn_print)
{
    fn_print(struct_base(t, root));
    struct parent_status stat = child_tracks_parent(parent, root);
    if (!stat.correct)
    {
        printf("%s", COLOR_RED);
        fn_print(struct_base(t, stat.parent));
        printf("%s", COLOR_NIL);
    }
    printf("\n");
}

/* I know this function is rough but it's tricky to focus on edge color rather
   than node color. Don't care about pretty code here, need thorough debug.
   I want to convert to iterative stack when I get the chance. */
static void
print_inner_tree(struct ccc_node_ const *const root, size_t const parent_size,
                 struct ccc_node_ const *const parent, char const *const prefix,
                 char const *const prefix_color, om_print_link const node_type,
                 om_link const dir, struct ccc_tree_ const *const t,
                 ccc_print_fn *const fn_print)
{
    if (root == &t->end_)
    {
        return;
    }
    size_t subtree_size = get_subtree_size(root, &t->end_);
    printf("%s", prefix);
    printf("%s%s%s",
           subtree_size <= parent_size / 2 ? COLOR_BLU_BOLD : COLOR_RED_BOLD,
           node_type == LEAF ? " └──" : " ├──", COLOR_NIL);
    printf(COLOR_CYN);
    printf("(%zu)", subtree_size);
    dir == L ? printf("L:" COLOR_NIL) : printf("R:" COLOR_NIL);

    print_node(t, parent, root, fn_print);

    char *str = NULL;
    int const string_length
        = snprintf(NULL, 0, "%s%s%s", prefix, prefix_color, /* NOLINT */
                   node_type == LEAF ? "     " : " │   ");
    if (string_length > 0)
    {
        /* NOLINTNEXTLINE */
        str = malloc(string_length + 1);
        /* NOLINTNEXTLINE */
        (void)snprintf(str, string_length, "%s%s%s", prefix, prefix_color,
                       node_type == LEAF ? "     " : " │   ");
    }
    if (str == NULL)
    {
        printf(COLOR_ERR "memory exceeded. Cannot display tree." COLOR_NIL);
        return;
    }

    char const *left_edge_color
        = get_edge_color(root->branch_[L], subtree_size, &t->end_);
    if (root->branch_[R] == &t->end_)
    {
        print_inner_tree(root->branch_[L], subtree_size, root, str,
                         left_edge_color, LEAF, L, t, fn_print);
    }
    else if (root->branch_[L] == &t->end_)
    {
        print_inner_tree(root->branch_[R], subtree_size, root, str,
                         left_edge_color, LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(root->branch_[R], subtree_size, root, str,
                         left_edge_color, BRANCH, R, t, fn_print);
        print_inner_tree(root->branch_[L], subtree_size, root, str,
                         left_edge_color, LEAF, L, t, fn_print);
    }
    free(str);
}

/* Should be pretty straightforward output. Red node means there
   is an error in parent tracking. The child does not track the parent
   correctly if this occurs and this will cause subtle delayed bugs. */
static void
ccc_tree_tree_print(struct ccc_tree_ const *const t,
                    struct ccc_node_ const *const root,
                    ccc_print_fn *const fn_print)
{
    if (root == &t->end_)
    {
        return;
    }
    size_t subtree_size = get_subtree_size(root, &t->end_);
    printf("\n%s(%zu)%s", COLOR_CYN, subtree_size, COLOR_NIL);
    print_node(t, &t->end_, root, fn_print);

    char const *left_edge_color
        = get_edge_color(root->branch_[L], subtree_size, &t->end_);
    if (root->branch_[R] == &t->end_)
    {
        print_inner_tree(root->branch_[L], subtree_size, root, "",
                         left_edge_color, LEAF, L, t, fn_print);
    }
    else if (root->branch_[L] == &t->end_)
    {
        print_inner_tree(root->branch_[R], subtree_size, root, "",
                         left_edge_color, LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(root->branch_[R], subtree_size, root, "",
                         left_edge_color, BRANCH, R, t, fn_print);
        print_inner_tree(root->branch_[L], subtree_size, root, "",
                         left_edge_color, LEAF, L, t, fn_print);
    }
}

/* NOLINTEND(*misc-no-recursion) */
