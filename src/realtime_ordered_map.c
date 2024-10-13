/* Author: Alexander G. Lopez
   --------------------------
   This file contains my implementation of a realtime ordered map. The added
   realtime prefix is to indicate that this map meets specific runtime bounds
   that can be relied upon consistently. This is may not be the case if a map
   is implemented with some self-optimizing data structure like a Splay Tree.

   This map, however, promises O(lgN) search, insert, and remove as a true
   upper bound, inclusive. This is acheived through a Weak AVL (WAVL) tree
   that is derived from the following two sources.

   [1] Bernhard Haeupler, Siddhartha Sen, and Robert E. Tarjan, 2014.
   Rank-Balanced Trees, J.ACM Transactions on Algorithms 11, 4, Article 0
   (June 2015), 24 pages.
   https://sidsen.azurewebsites.net//papers/rb-trees-talg.pdf

   [2] Phil Vachon (pvachon) https://github.com/pvachon/wavl_tree
   This implementation is heavily influential throughout. However there have
   been some major adjustments and simplifications. Namely, the allocation has
   been adjusted to accomodate this library's ability to be an allocating or
   non-allocating container. All left-right symmetric cases have been united
   into one and I chose to tackle rotations and deletions slightly differently,
   shortening the code significantly. Finally, a few other changes and
   improvements suggested by the authors of the original paper are implemented.

   Overall a WAVL tree is quite impressive for it's simplicity and purported
   improvements over AVL and Red-Black trees. The rank framework is intuitive
   and flexible in how it can be implemented. */
#include "realtime_ordered_map.h"
#include "impl_realtime_ordered_map.h"
#include "impl_types.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>
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
#define PRINTER_INDENT 13
#define LR 2

enum rtom_link_
{
    L = 0,
    R,
};

enum rtom_print_link_
{
    BRANCH = 0, /* ├── */
    LEAF,       /* └── */
};

struct rtom_query_
{
    ccc_threeway_cmp last_cmp_;
    union
    {
        struct ccc_rtom_elem_ *found_;
        struct ccc_rtom_elem_ *parent_;
    };
};

static enum rtom_link_ const inorder_traversal = R;
static enum rtom_link_ const reverse_inorder_traversal = L;

static enum rtom_link_ const min = L;
static enum rtom_link_ const max = R;

/*==============================  Prototypes   ==============================*/

static void init_node(struct ccc_rtom_ *, struct ccc_rtom_elem_ *);
static ccc_threeway_cmp cmp(struct ccc_rtom_ const *, void const *key,
                            struct ccc_rtom_elem_ const *node,
                            ccc_key_cmp_fn *);
static void *struct_base(struct ccc_rtom_ const *,
                         struct ccc_rtom_elem_ const *);
static struct rtom_query_ find(struct ccc_rtom_ const *, void const *key);
static void swap(char tmp[], void *a, void *b, size_t elem_sz);
static void *maybe_alloc_insert(struct ccc_rtom_ *,
                                struct ccc_rtom_elem_ *parent,
                                ccc_threeway_cmp last_cmp,
                                struct ccc_rtom_elem_ *);
static void *remove_fixup(struct ccc_rtom_ *, struct ccc_rtom_elem_ *);
static void insert_fixup(struct ccc_rtom_ *, struct ccc_rtom_elem_ *z_p_of_x,
                         struct ccc_rtom_elem_ *x);
static void maintain_rank_rules(struct ccc_rtom_ *rom, bool two_child,
                                struct ccc_rtom_elem_ *p_of_xy,
                                struct ccc_rtom_elem_ *x);
static void transplant(struct ccc_rtom_ *, struct ccc_rtom_elem_ *remove,
                       struct ccc_rtom_elem_ *replacement);
static void rebalance_3_child(struct ccc_rtom_ *rom,
                              struct ccc_rtom_elem_ *p_of_x,
                              struct ccc_rtom_elem_ *x);
static void rebalance_via_rotation(struct ccc_rtom_ *rom,
                                   struct ccc_rtom_elem_ *z_p_of_xy,
                                   struct ccc_rtom_elem_ *x,
                                   struct ccc_rtom_elem_ *y);
static bool is_0_child(struct ccc_rtom_ const *,
                       struct ccc_rtom_elem_ const *p_of_x,
                       struct ccc_rtom_elem_ const *x);
static bool is_1_child(struct ccc_rtom_ const *,
                       struct ccc_rtom_elem_ const *p_of_x,
                       struct ccc_rtom_elem_ const *x);
static bool is_2_child(struct ccc_rtom_ const *,
                       struct ccc_rtom_elem_ const *p_of_x,
                       struct ccc_rtom_elem_ const *x);
static bool is_3_child(struct ccc_rtom_ const *,
                       struct ccc_rtom_elem_ const *p_of_x,
                       struct ccc_rtom_elem_ const *x);
static bool is_01_parent(struct ccc_rtom_ const *,
                         struct ccc_rtom_elem_ const *x,
                         struct ccc_rtom_elem_ const *p_of_xy,
                         struct ccc_rtom_elem_ const *y);
static bool is_11_parent(struct ccc_rtom_ const *,
                         struct ccc_rtom_elem_ const *x,
                         struct ccc_rtom_elem_ const *p_of_xy,
                         struct ccc_rtom_elem_ const *y);
static bool is_02_parent(struct ccc_rtom_ const *,
                         struct ccc_rtom_elem_ const *x,
                         struct ccc_rtom_elem_ const *p_of_xy,
                         struct ccc_rtom_elem_ const *y);
static bool is_22_parent(struct ccc_rtom_ const *,
                         struct ccc_rtom_elem_ const *x,
                         struct ccc_rtom_elem_ const *p_of_xy,
                         struct ccc_rtom_elem_ const *y);
static bool is_leaf(struct ccc_rtom_ const *rom,
                    struct ccc_rtom_elem_ const *x);
static struct ccc_rtom_elem_ *sibling_of(struct ccc_rtom_ const *rom,
                                         struct ccc_rtom_elem_ const *x);
static void promote(struct ccc_rtom_ const *rom, struct ccc_rtom_elem_ *x);
static void demote(struct ccc_rtom_ const *rom, struct ccc_rtom_elem_ *x);
static void double_promote(struct ccc_rtom_ const *rom,
                           struct ccc_rtom_elem_ *x);
static void double_demote(struct ccc_rtom_ const *rom,
                          struct ccc_rtom_elem_ *x);

static void rotate(struct ccc_rtom_ *rom, struct ccc_rtom_elem_ *z_p_of_x,
                   struct ccc_rtom_elem_ *x_p_of_y, struct ccc_rtom_elem_ *y,
                   enum rtom_link_ dir);
static void double_rotate(struct ccc_rtom_ *rom,
                          struct ccc_rtom_elem_ *z_p_of_x,
                          struct ccc_rtom_elem_ *x_p_of_y,
                          struct ccc_rtom_elem_ *y, enum rtom_link_ dir);
static bool validate(struct ccc_rtom_ const *rom);
static void ccc_tree_print(struct ccc_rtom_ const *rom,
                           struct ccc_rtom_elem_ const *root,
                           ccc_print_fn *fn_print);

static struct ccc_rtom_elem_ *
next(struct ccc_rtom_ const *, struct ccc_rtom_elem_ const *, enum rtom_link_);
static struct ccc_rtom_elem_ *min_max_from(struct ccc_rtom_ const *,
                                           struct ccc_rtom_elem_ *start,
                                           enum rtom_link_);
static struct ccc_range_ equal_range(struct ccc_rtom_ const *, void const *,
                                     void const *, enum rtom_link_);

/*==============================  Interface    ==============================*/

bool
ccc_rom_contains(ccc_realtime_ordered_map const *const rom,
                 void const *const key)
{
    return CCC_EQL == find(rom, key).last_cmp_;
}

void *
ccc_rom_get_key_val(ccc_realtime_ordered_map const *const rom,
                    void const *const key)
{
    struct rtom_query_ q = find(rom, key);
    return (CCC_EQL == q.last_cmp_) ? struct_base(rom, q.found_) : NULL;
}

ccc_entry
ccc_rom_insert(ccc_realtime_ordered_map *const rom,
               ccc_rtom_elem *const out_handle, void *const tmp)
{
    struct rtom_query_ q
        = find(rom, ccc_impl_rom_key_from_node(rom, out_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        *out_handle = *(ccc_rtom_elem *)q.found_;
        void *user_struct = struct_base(rom, out_handle);
        void *ret = struct_base(rom, q.found_);
        swap(tmp, user_struct, ret, rom->elem_sz_);
        return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted
        = maybe_alloc_insert(rom, q.parent_, q.last_cmp_, out_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_rom_try_insert(ccc_realtime_ordered_map *const rom,
                   ccc_rtom_elem *const key_val_handle)
{
    struct rtom_query_ q
        = find(rom, ccc_impl_rom_key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        return (ccc_entry){
            {.e_ = struct_base(rom, q.found_), .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted
        = maybe_alloc_insert(rom, q.parent_, q.last_cmp_, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_rom_insert_or_assign(ccc_realtime_ordered_map *const rom,
                         ccc_rtom_elem *const key_val_handle)
{
    struct rtom_query_ q
        = find(rom, ccc_impl_rom_key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        void *const found = struct_base(rom, q.found_);
        memcpy(found, struct_base(rom, key_val_handle), rom->elem_sz_);
        return (ccc_entry){{.e_ = found, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted
        = maybe_alloc_insert(rom, q.parent_, q.last_cmp_, key_val_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = inserted, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_rtom_entry
ccc_rom_entry(ccc_realtime_ordered_map const *const rom, void const *const key)
{
    return (ccc_rtom_entry){ccc_impl_rom_entry(rom, key)};
}

void *
ccc_rom_or_insert(ccc_rtom_entry const *const e, ccc_rtom_elem *const elem)
{
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    return maybe_alloc_insert(
        e->impl_.rom_,
        ccc_impl_rom_elem_in_slot(e->impl_.rom_, e->impl_.entry_.e_),
        e->impl_.last_cmp_, elem);
}

void *
ccc_rom_insert_entry(ccc_rtom_entry const *const e, ccc_rtom_elem *const elem)
{
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        *elem = *ccc_impl_rom_elem_in_slot(e->impl_.rom_, e->impl_.entry_.e_);
        memcpy(e->impl_.entry_.e_, struct_base(e->impl_.rom_, elem),
               e->impl_.rom_->elem_sz_);
        return e->impl_.entry_.e_;
    }
    return maybe_alloc_insert(
        e->impl_.rom_,
        ccc_impl_rom_elem_in_slot(e->impl_.rom_, e->impl_.entry_.e_),
        e->impl_.last_cmp_, elem);
}

ccc_entry
ccc_rom_remove_entry(ccc_rtom_entry const *const e)
{
    if (e->impl_.entry_.stats_ == CCC_ENTRY_OCCUPIED)
    {
        void *const erased = remove_fixup(
            e->impl_.rom_,
            ccc_impl_rom_elem_in_slot(e->impl_.rom_, e->impl_.entry_.e_));
        assert(erased);
        if (e->impl_.rom_->alloc_)
        {
            e->impl_.rom_->alloc_(erased, 0);
            return (ccc_entry){
                {.e_ = NULL, .stats_ = CCC_ENTRY_OCCUPIED_CONTAINS_NULL}};
        }
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_rom_remove(ccc_realtime_ordered_map *const rom,
               ccc_rtom_elem *const out_handle)
{
    struct rtom_query_ const q
        = find(rom, ccc_impl_rom_key_from_node(rom, out_handle));
    if (q.last_cmp_ != CCC_EQL)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    void *const removed = remove_fixup(rom, q.found_);
    if (rom->alloc_)
    {
        void *const user_struct = struct_base(rom, out_handle);
        memcpy(user_struct, removed, rom->elem_sz_);
        rom->alloc_(removed, 0);
        return (ccc_entry){{.e_ = user_struct, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = removed, .stats_ = CCC_ENTRY_OCCUPIED}};
}

ccc_rtom_entry *
ccc_rom_and_modify(ccc_rtom_entry *e, ccc_update_fn *fn)
{
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_user_type_mut){.user_type = e->impl_.entry_.e_, NULL});
    }
    return e;
}

ccc_rtom_entry *
ccc_rom_and_modify_aux(ccc_rtom_entry *e, ccc_update_fn *fn, void *aux)
{
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        fn((ccc_user_type_mut){.user_type = e->impl_.entry_.e_, aux});
    }
    return e;
}

void *
ccc_rom_unwrap(ccc_rtom_entry const *const e)
{
    if (e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED)
    {
        return e->impl_.entry_.e_;
    }
    return NULL;
}

bool
ccc_rom_occupied(ccc_rtom_entry const *const e)
{
    return e->impl_.entry_.stats_ & CCC_ENTRY_OCCUPIED;
}

bool
ccc_rom_insert_error(ccc_rtom_entry const *const e)
{
    return e->impl_.entry_.stats_ & CCC_ENTRY_INSERT_ERROR;
}

static struct ccc_rtom_elem_ *
min_max_from(struct ccc_rtom_ const *const rom, struct ccc_rtom_elem_ *start,
             enum rtom_link_ const dir)
{
    if (start == &rom->end_)
    {
        return start;
    }
    for (; start->branch_[dir] != &rom->end_; start = start->branch_[dir])
    {}
    return start;
}

void *
ccc_rom_begin(ccc_realtime_ordered_map const *rom)
{
    struct ccc_rtom_elem_ *m = min_max_from(rom, rom->root_, min);
    return m == &rom->end_ ? NULL : struct_base(rom, m);
}

void *
ccc_rom_next(ccc_realtime_ordered_map const *const rom,
             ccc_rtom_elem const *const e)
{
    struct ccc_rtom_elem_ const *const n = next(rom, e, inorder_traversal);
    if (n == &rom->end_)
    {
        return NULL;
    }
    return struct_base(rom, n);
}

void *
ccc_rom_rbegin(ccc_realtime_ordered_map const *const rom)
{
    struct ccc_rtom_elem_ *m = min_max_from(rom, rom->root_, max);
    return m == &rom->end_ ? NULL : struct_base(rom, m);
}

void *
ccc_rom_end(ccc_realtime_ordered_map const *const)
{
    return NULL;
}

void *
ccc_rom_rend(ccc_realtime_ordered_map const *const)
{
    return NULL;
}

void *
ccc_rom_rnext(ccc_realtime_ordered_map const *const rom,
              ccc_rtom_elem const *const e)
{
    struct ccc_rtom_elem_ const *const n
        = next(rom, e, reverse_inorder_traversal);
    return (n == &rom->end_) ? NULL : struct_base(rom, n);
}

ccc_range
ccc_rom_equal_range(ccc_realtime_ordered_map const *const rom,
                    void const *const begin_key, void const *const end_key)
{
    return (ccc_range){equal_range(rom, begin_key, end_key, inorder_traversal)};
}

ccc_rrange
ccc_rom_equal_rrange(ccc_realtime_ordered_map const *const rom,
                     void const *const rbegin_key, void const *const rend_key)
{
    return (ccc_rrange){
        equal_range(rom, rbegin_key, rend_key, reverse_inorder_traversal)};
}

size_t
ccc_rom_size(ccc_realtime_ordered_map const *const rom)
{
    return rom->sz_;
}

bool
ccc_rom_is_empty(ccc_realtime_ordered_map const *const rom)
{
    return !rom->sz_;
}

void *
ccc_rom_root(ccc_realtime_ordered_map const *rom)
{
    if (!rom || rom->root_ == &rom->end_)
    {
        return NULL;
    }
    return struct_base(rom, rom->root_);
}

bool
ccc_rom_validate(ccc_realtime_ordered_map const *rom)
{
    return validate(rom);
}

ccc_result
ccc_rom_print(ccc_realtime_ordered_map const *const rom, ccc_print_fn *const fn)
{
    if (!rom || !fn)
    {
        return CCC_INPUT_ERR;
    }
    ccc_tree_print(rom, rom->root_, fn);
    return CCC_OK;
}

ccc_result
ccc_rom_clear(ccc_realtime_ordered_map *const rom,
              ccc_destructor_fn *const destructor)
{
    if (!rom)
    {
        return CCC_INPUT_ERR;
    }
    while (!ccc_rom_is_empty(rom))
    {
        void *const deleted = remove_fixup(rom, rom->root_);
        if (destructor)
        {
            destructor(
                (ccc_user_type_mut){.user_type = deleted, .aux = rom->aux_});
        }
    }
    return CCC_OK;
}

ccc_result
ccc_rom_clear_and_free(ccc_realtime_ordered_map *const rom,
                       ccc_destructor_fn *const destructor)
{
    if (!rom->alloc_)
    {
        return CCC_NO_ALLOC;
    }
    while (!ccc_rom_is_empty(rom))
    {
        void *const deleted = remove_fixup(rom, rom->root_);
        if (destructor)
        {
            destructor(
                (ccc_user_type_mut){.user_type = deleted, .aux = rom->aux_});
        }
        (void)rom->alloc_(deleted, 0);
    }
    return CCC_OK;
}

/*=========================   Private Interface  ============================*/

struct ccc_rtom_entry_
ccc_impl_rom_entry(struct ccc_rtom_ const *const rom, void const *const key)
{
    struct rtom_query_ q = find(rom, key);
    if (CCC_EQL == q.last_cmp_)
    {
        return (struct ccc_rtom_entry_){
            .rom_ = (struct ccc_rtom_ *)rom,
            .last_cmp_ = q.last_cmp_,
            .entry_ = {
                .e_ = struct_base(rom, q.found_),
                .stats_ = CCC_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct ccc_rtom_entry_){
        .rom_ = (struct ccc_rtom_ *)rom,
        .last_cmp_ = q.last_cmp_,
        .entry_ = {
            .e_ = struct_base(rom, q.parent_),
            .stats_ = CCC_ENTRY_VACANT,
        },
    };
}

void *
ccc_impl_rom_insert(struct ccc_rtom_ *const rom,
                    struct ccc_rtom_elem_ *const parent,
                    ccc_threeway_cmp const last_cmp,
                    struct ccc_rtom_elem_ *const out_handle)
{
    init_node(rom, out_handle);
    if (!rom->sz_)
    {
        rom->root_ = out_handle;
        ++rom->sz_;
        return struct_base(rom, out_handle);
    }
    assert(last_cmp == CCC_GRT || last_cmp == CCC_LES);
    bool const rank_rule_break
        = parent->branch_[L] == &rom->end_ && parent->branch_[R] == &rom->end_;
    parent->branch_[CCC_GRT == last_cmp] = out_handle;
    out_handle->parent_ = parent;
    if (rank_rule_break)
    {
        insert_fixup(rom, parent, out_handle);
    }
    ++rom->sz_;
    return struct_base(rom, out_handle);
}

void *
ccc_impl_rom_key_from_node(struct ccc_rtom_ const *const rom,
                           struct ccc_rtom_elem_ const *const elem)
{
    return (char *)struct_base(rom, elem) + rom->key_offset_;
}

void *
ccc_impl_rom_key_in_slot(struct ccc_rtom_ const *const rom,
                         void const *const slot)
{
    return (char *)slot + rom->key_offset_;
}

struct ccc_rtom_elem_ *
ccc_impl_rom_elem_in_slot(struct ccc_rtom_ const *const rom,
                          void const *const slot)
{
    return (struct ccc_rtom_elem_ *)((char *)slot + rom->node_elem_offset_);
}

/*=========================    Static Helpers    ============================*/

static void *
maybe_alloc_insert(struct ccc_rtom_ *const rom,
                   struct ccc_rtom_elem_ *const parent,
                   ccc_threeway_cmp last_cmp, struct ccc_rtom_elem_ *out_handle)
{
    if (rom->alloc_)
    {
        void *new = rom->alloc_(NULL, rom->elem_sz_);
        if (!new)
        {
            return NULL;
        }
        memcpy(new, struct_base(rom, out_handle), rom->elem_sz_);
        out_handle = ccc_impl_rom_elem_in_slot(rom, new);
    }
    return ccc_impl_rom_insert(rom, parent, last_cmp, out_handle);
}

static inline struct rtom_query_
find(struct ccc_rtom_ const *const rom, void const *const key)
{
    struct ccc_rtom_elem_ *parent = (struct ccc_rtom_elem_ *)&rom->end_;
    ccc_threeway_cmp link = CCC_CMP_ERR;
    for (struct ccc_rtom_elem_ *seek = rom->root_; seek != &rom->end_;
         parent = seek, seek = seek->branch_[CCC_GRT == link])
    {
        link = cmp(rom, key, seek, rom->cmp_);
        if (CCC_EQL == link)
        {
            return (struct rtom_query_){.last_cmp_ = CCC_EQL, .found_ = seek};
        }
    }
    return (struct rtom_query_){.last_cmp_ = link, .parent_ = parent};
}

static struct ccc_rtom_elem_ *
next(struct ccc_rtom_ const *const rom, struct ccc_rtom_elem_ const *n,
     enum rtom_link_ const traversal)
{
    if (!n || n == &rom->end_)
    {
        return (struct ccc_rtom_elem_ *)&rom->end_;
    }
    assert(rom->root_->parent_ == &rom->end_);
    /* The node is an internal one that has a subtree to explore first. */
    if (n->branch_[traversal] != &rom->end_)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->branch_[traversal]; n->branch_[!traversal] != &rom->end_;
             n = n->branch_[!traversal])
        {}
        return (struct ccc_rtom_elem_ *)n;
    }
    /* A leaf. Now it is time to visit the closest parent not yet visited.
       The old stack overflow question I read about this type of iteration
       (Boost's method, can't find the post anymore?) had the sentinel node
       make the root its traversal child, but this means we would have to
       write to the sentinel on every call to next. I want multiple threads to
       iterate freely without undefined data race writes to memory locations.
       So more expensive loop.*/
    for (; n->parent_ != &rom->end_ && n->parent_->branch_[!traversal] != n;
         n = n->parent_)
    {}
    return n->parent_;
}

static struct ccc_range_
equal_range(struct ccc_rtom_ const *const rom, void const *const begin_key,
            void const *const end_key, enum rtom_link_ const traversal)
{
    if (!rom->sz_)
    {
        return (struct ccc_range_){};
    }
    ccc_threeway_cmp const les_or_grt[2] = {CCC_LES, CCC_GRT};
    struct rtom_query_ b = find(rom, begin_key);
    if (b.last_cmp_ == les_or_grt[traversal])
    {
        b.found_ = next(rom, b.found_, traversal);
    }
    struct rtom_query_ e = find(rom, end_key);
    if (e.last_cmp_ != les_or_grt[!traversal])
    {
        e.found_ = next(rom, e.found_, traversal);
    }
    return (struct ccc_range_){
        .begin_ = b.found_ == &rom->end_ ? NULL : struct_base(rom, b.found_),
        .end_ = e.found_ == &rom->end_ ? NULL : struct_base(rom, e.found_),
    };
}

static inline void
init_node(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ *const e)
{
    assert(e != NULL);
    assert(rom != NULL);
    e->branch_[L] = &rom->end_;
    e->branch_[R] = &rom->end_;
    e->parent_ = &rom->end_;
    e->parity_ = 0;
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const elem_sz)
{
    if (a == b)
    {
        return;
    }
    memcpy(tmp, a, elem_sz);
    memcpy(a, b, elem_sz);
    memcpy(b, tmp, elem_sz);
}

static inline ccc_threeway_cmp
cmp(struct ccc_rtom_ const *const rom, void const *const key,
    struct ccc_rtom_elem_ const *const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){
        .key_lhs = key,
        .user_type_rhs = struct_base(rom, node),
        .aux = rom->aux_,
    });
}

static inline void *
struct_base(struct ccc_rtom_ const *const rom,
            struct ccc_rtom_elem_ const *const e)
{
    return ((char *)e->branch_) - rom->node_elem_offset_;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ *z_p_of_xy,
             struct ccc_rtom_elem_ *x)
{
    do
    {
        promote(rom, z_p_of_xy);
        x = z_p_of_xy;
        z_p_of_xy = z_p_of_xy->parent_;
        if (z_p_of_xy == &rom->end_)
        {
            return;
        }
    } while (is_01_parent(rom, x, z_p_of_xy, sibling_of(rom, x)));

    if (!is_02_parent(rom, x, z_p_of_xy, sibling_of(rom, x)))
    {
        return;
    }
    assert(x != &rom->end_);
    assert(is_0_child(rom, z_p_of_xy, x));
    enum rtom_link_ const p_to_x_dir = z_p_of_xy->branch_[R] == x;
    struct ccc_rtom_elem_ *y = x->branch_[!p_to_x_dir];
    if (y == &rom->end_ || is_2_child(rom, z_p_of_xy, y))
    {
        rotate(rom, z_p_of_xy, x, y, !p_to_x_dir);
        demote(rom, z_p_of_xy);
    }
    else
    {
        assert(is_1_child(rom, z_p_of_xy, y));
        double_rotate(rom, z_p_of_xy, x, y, p_to_x_dir);
        promote(rom, y);
        demote(rom, x);
        demote(rom, z_p_of_xy);
    }
}

static void *
remove_fixup(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ *const remove)
{
    struct ccc_rtom_elem_ *y = NULL;
    struct ccc_rtom_elem_ *x = NULL;
    struct ccc_rtom_elem_ *p_of_xy = NULL;
    bool two_child = false;
    if (remove->branch_[L] == &rom->end_ || remove->branch_[R] == &rom->end_)
    {
        y = remove;
        p_of_xy = y->parent_;
        x = y->branch_[y->branch_[L] == &rom->end_];
        x->parent_ = y->parent_;
        if (p_of_xy == &rom->end_)
        {
            rom->root_ = x;
        }
        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->branch_[p_of_xy->branch_[R] == y] = x;
    }
    else
    {
        y = min_max_from(rom, remove->branch_[R], min);
        p_of_xy = y->parent_;
        x = y->branch_[y->branch_[L] == &rom->end_];
        x->parent_ = y->parent_;

        /* Save if check and improve readability by assuming this is true. */
        assert(p_of_xy != &rom->end_);

        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->branch_[p_of_xy->branch_[R] == y] = x;
        transplant(rom, remove, y);
        if (remove == p_of_xy)
        {
            p_of_xy = y;
        }
    }

    if (p_of_xy != &rom->end_)
    {
        maintain_rank_rules(rom, two_child, p_of_xy, x);
        assert(!is_leaf(rom, p_of_xy) || !p_of_xy->parity_);
    }
    remove->branch_[L] = remove->branch_[R] = remove->parent_ = NULL;
    remove->parity_ = 0;
    --rom->sz_;
    return struct_base(rom, remove);
}

static inline void
maintain_rank_rules(struct ccc_rtom_ *const rom, bool two_child,
                    struct ccc_rtom_elem_ *const p_of_xy,
                    struct ccc_rtom_elem_ *const x)
{
    if (two_child)
    {
        assert(p_of_xy != &rom->end_);
        rebalance_3_child(rom, p_of_xy, x);
    }
    else if (x == &rom->end_ && p_of_xy->branch_[L] == p_of_xy->branch_[R])
    {
        assert(p_of_xy != &rom->end_);
        bool const demote_makes_3_child
            = is_2_child(rom, p_of_xy->parent_, p_of_xy);
        demote(rom, p_of_xy);
        if (demote_makes_3_child)
        {
            rebalance_3_child(rom, p_of_xy->parent_, p_of_xy);
        }
    }
    assert(!is_leaf(rom, p_of_xy) || !p_of_xy->parity_);
}

static inline void
rebalance_3_child(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ *p_of_xy,
                  struct ccc_rtom_elem_ *x)
{
    assert(p_of_xy != &rom->end_);
    bool made_3_child = false;
    do
    {
        struct ccc_rtom_elem_ *const p_of_p_of_x = p_of_xy->parent_;
        struct ccc_rtom_elem_ *y = p_of_xy->branch_[p_of_xy->branch_[L] == x];
        made_3_child = is_2_child(rom, p_of_p_of_x, p_of_xy);
        if (is_2_child(rom, p_of_xy, y))
        {
            demote(rom, p_of_xy);
        }
        else if (is_22_parent(rom, y->branch_[L], y, y->branch_[R]))
        {
            demote(rom, p_of_xy);
            demote(rom, y);
        }
        else /* p(x) is 1,3, y is not a 2,2 parent, and x is 3-child.*/
        {
            assert(is_3_child(rom, p_of_xy, x));
            rebalance_via_rotation(rom, p_of_xy, x, y);
            return;
        }
        x = p_of_xy;
        p_of_xy = p_of_p_of_x;
    } while (p_of_xy != &rom->end_ && made_3_child);
}

static inline void
rebalance_via_rotation(struct ccc_rtom_ *const rom,
                       struct ccc_rtom_elem_ *const z,
                       struct ccc_rtom_elem_ *const x,
                       struct ccc_rtom_elem_ *const y)
{
    enum rtom_link_ const z_to_x_dir = z->branch_[R] == x;
    struct ccc_rtom_elem_ *const w = y->branch_[!z_to_x_dir];
    if (is_1_child(rom, y, w))
    {
        rotate(rom, z, y, y->branch_[z_to_x_dir], z_to_x_dir);
        promote(rom, y);
        demote(rom, z);
        if (is_leaf(rom, z))
        {
            demote(rom, z);
        }
    }
    else /* w is a 2-child and v will be a 1-child. */
    {
        struct ccc_rtom_elem_ *const v = y->branch_[z_to_x_dir];
        assert(is_2_child(rom, y, w));
        assert(is_1_child(rom, y, v));
        double_rotate(rom, z, y, v, !z_to_x_dir);
        double_promote(rom, v);
        demote(rom, y);
        double_demote(rom, z);
        /* Optional "Rebalancing with Promotion," defined as follows:
               if node z is a non-leaf 1,1 node, we promote it; otherwise, if y
               is a non-leaf 1,1 node, we promote it. (See Figure 4.)
               (Haeupler et. al. 2014, 17).
           This reduces constants in some of theorems mentioned in the paper
           but may not be worth doing. Rotations stay at 2 worst case. Should
           revisit after more performance testing. */
        if (!is_leaf(rom, z)
            && is_11_parent(rom, z->branch_[L], z, z->branch_[R]))
        {
            promote(rom, z);
        }
        else if (!is_leaf(rom, y)
                 && is_11_parent(rom, y->branch_[L], y, y->branch_[R]))
        {
            promote(rom, y);
        }
    }
}

static inline void
transplant(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ *const remove,
           struct ccc_rtom_elem_ *const replacement)
{
    assert(remove != &rom->end_);
    assert(replacement != &rom->end_);
    replacement->parent_ = remove->parent_;
    if (remove->parent_ == &rom->end_)
    {
        rom->root_ = replacement;
    }
    else
    {
        remove->parent_->branch_[remove->parent_->branch_[R] == remove]
            = replacement;
    }
    remove->branch_[R]->parent_ = replacement;
    remove->branch_[L]->parent_ = replacement;
    replacement->branch_[R] = remove->branch_[R];
    replacement->branch_[L] = remove->branch_[L];
    replacement->parity_ = remove->parity_;
}

/** A single rotation is symmetric. Here is the right case. Lowercase are nodes
and uppercase are arbitrary subtrees.
        z            x
     ╭──┴──╮      ╭──┴──╮
     x     C      A     z
   ╭─┴─╮      ->      ╭─┴─╮
   A   y              y   C
       │              │
       B              B*/
static inline void
rotate(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ *const z_p_of_x,
       struct ccc_rtom_elem_ *const x_p_of_y, struct ccc_rtom_elem_ *const y,
       enum rtom_link_ dir)
{
    assert(z_p_of_x != &rom->end_);
    struct ccc_rtom_elem_ *const p_of_p_of_x = z_p_of_x->parent_;
    x_p_of_y->parent_ = p_of_p_of_x;
    if (p_of_p_of_x == &rom->end_)
    {
        rom->root_ = x_p_of_y;
    }
    else
    {
        p_of_p_of_x->branch_[p_of_p_of_x->branch_[R] == z_p_of_x] = x_p_of_y;
    }
    x_p_of_y->branch_[dir] = z_p_of_x;
    z_p_of_x->parent_ = x_p_of_y;
    z_p_of_x->branch_[!dir] = y;
    y->parent_ = z_p_of_x;
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
double_rotate(struct ccc_rtom_ *const rom,
              struct ccc_rtom_elem_ *const z_p_of_x,
              struct ccc_rtom_elem_ *const x_p_of_y,
              struct ccc_rtom_elem_ *const y, enum rtom_link_ dir)
{
    assert(z_p_of_x != &rom->end_);
    assert(x_p_of_y != &rom->end_);
    assert(y != &rom->end_);
    struct ccc_rtom_elem_ *const p_of_p_of_x = z_p_of_x->parent_;
    y->parent_ = p_of_p_of_x;
    if (p_of_p_of_x == &rom->end_)
    {
        rom->root_ = y;
    }
    else
    {
        p_of_p_of_x->branch_[p_of_p_of_x->branch_[R] == z_p_of_x] = y;
    }
    x_p_of_y->branch_[!dir] = y->branch_[dir];
    y->branch_[dir]->parent_ = x_p_of_y;
    y->branch_[dir] = x_p_of_y;
    x_p_of_y->parent_ = y;

    z_p_of_x->branch_[dir] = y->branch_[!dir];
    y->branch_[!dir]->parent_ = z_p_of_x;
    y->branch_[!dir] = z_p_of_x;
    z_p_of_x->parent_ = y;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
       0/
       x*/
[[maybe_unused]] static inline bool
is_0_child(struct ccc_rtom_ const *const rom,
           struct ccc_rtom_elem_ const *p_of_x, struct ccc_rtom_elem_ const *x)
{
    return p_of_x != &rom->end_ && p_of_x->parity_ == x->parity_;
}

/* Returns true for rank difference 1 between the parent and node.
         p
       1/
       x*/
static inline bool
is_1_child(struct ccc_rtom_ const *const rom,
           struct ccc_rtom_elem_ const *p_of_x, struct ccc_rtom_elem_ const *x)
{
    return p_of_x != &rom->end_ && p_of_x->parity_ != x->parity_;
}

/* Returns true for rank difference 2 between the parent and node.
         p
       2/
       x*/
static inline bool
is_2_child(struct ccc_rtom_ const *const rom,
           struct ccc_rtom_elem_ const *const p_of_x,
           struct ccc_rtom_elem_ const *const x)
{
    return p_of_x != &rom->end_ && p_of_x->parity_ == x->parity_;
}

/* Returns true for rank difference 3 between the parent and node.
         p
       3/
       x*/
[[maybe_unused]] static inline bool
is_3_child(struct ccc_rtom_ const *const rom,
           struct ccc_rtom_elem_ const *const p_of_x,
           struct ccc_rtom_elem_ const *const x)
{
    return p_of_x != &rom->end_ && p_of_x->parity_ != x->parity_;
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
       0/ \1
       x   y*/
static inline bool
is_01_parent([[maybe_unused]] struct ccc_rtom_ const *const rom,
             struct ccc_rtom_elem_ const *const x,
             struct ccc_rtom_elem_ const *const p_of_xy,
             struct ccc_rtom_elem_ const *const y)
{
    assert(p_of_xy != &rom->end_);
    return (!x->parity_ && !p_of_xy->parity_ && y->parity_)
           || (x->parity_ && p_of_xy->parity_ && !y->parity_);
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
       1/ \1
       x   y*/
static inline bool
is_11_parent([[maybe_unused]] struct ccc_rtom_ const *const rom,
             struct ccc_rtom_elem_ const *const x,
             struct ccc_rtom_elem_ const *const p_of_xy,
             struct ccc_rtom_elem_ const *const y)
{
    assert(p_of_xy != &rom->end_);
    return (!x->parity_ && p_of_xy->parity_ && !y->parity_)
           || (x->parity_ && !p_of_xy->parity_ && y->parity_);
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
       2/ \0
       x   y*/
static inline bool
is_02_parent([[maybe_unused]] struct ccc_rtom_ const *const rom,
             struct ccc_rtom_elem_ const *const x,
             struct ccc_rtom_elem_ const *const p_of_xy,
             struct ccc_rtom_elem_ const *const y)
{
    assert(p_of_xy != &rom->end_);
    return (x->parity_ == p_of_xy->parity_) && (p_of_xy->parity_ == y->parity_);
}

/* Returns true if a parent is a 2,2 or 2,2 node, which is allowed. 2,2 nodes
   are allowed in a WAVL tree but the absence of any 2,2 nodes is the exact
   equivalent of a normal AVL tree which can occur if only insertions occur
   for a WAVL tree. Either child may be the sentinel node which has a parity of
   1 and rank -1.
         p
       2/ \2
       x   y*/
static inline bool
is_22_parent([[maybe_unused]] struct ccc_rtom_ const *const rom,
             struct ccc_rtom_elem_ const *const x,
             struct ccc_rtom_elem_ const *const p_of_xy,
             struct ccc_rtom_elem_ const *const y)
{
    assert(p_of_xy != &rom->end_);
    return (x->parity_ == p_of_xy->parity_) && (p_of_xy->parity_ == y->parity_);
}

static inline void
promote(struct ccc_rtom_ const *const rom, struct ccc_rtom_elem_ *const x)
{
    if (x != &rom->end_)
    {
        x->parity_ = !x->parity_;
    }
}

static inline void
demote(struct ccc_rtom_ const *const rom, struct ccc_rtom_elem_ *const x)
{
    promote(rom, x);
}

static inline void
double_promote(struct ccc_rtom_ const *const, struct ccc_rtom_elem_ *const)
{}

static inline void
double_demote(struct ccc_rtom_ const *const, struct ccc_rtom_elem_ *const)
{}

static inline bool
is_leaf(struct ccc_rtom_ const *const rom, struct ccc_rtom_elem_ const *const x)
{
    return x->branch_[L] == &rom->end_ && x->branch_[R] == &rom->end_;
}

static inline struct ccc_rtom_elem_ *
sibling_of([[maybe_unused]] struct ccc_rtom_ const *const rom,
           struct ccc_rtom_elem_ const *const x)
{
    assert(x->parent_ != &rom->end_);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return x->parent_->branch_[x->parent_->branch_[L] == x];
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

struct tree_range
{
    struct ccc_rtom_elem_ const *low;
    struct ccc_rtom_elem_ const *root;
    struct ccc_rtom_elem_ const *high;
};

struct parent_status
{
    bool correct;
    struct ccc_rtom_elem_ const *parent;
};

static size_t
recursive_size(struct ccc_rtom_ const *const rom,
               struct ccc_rtom_elem_ const *const r)
{
    if (r == &rom->end_)
    {
        return 0;
    }
    return 1 + recursive_size(rom, r->branch_[R])
           + recursive_size(rom, r->branch_[L]);
}

static bool
are_subtrees_valid(struct ccc_rtom_ const *t, struct tree_range const r,
                   struct ccc_rtom_elem_ const *const nil)
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
        && cmp(t, ccc_impl_rom_key_from_node(t, r.low), r.root, t->cmp_)
               != CCC_LES)
    {
        return false;
    }
    if (r.high != nil
        && cmp(t, ccc_impl_rom_key_from_node(t, r.high), r.root, t->cmp_)
               != CCC_GRT)
    {
        return false;
    }
    return are_subtrees_valid(t,
                              (struct tree_range){
                                  .low = r.low,
                                  .root = r.root->branch_[L],
                                  .high = r.root,
                              },
                              nil)
           && are_subtrees_valid(t,
                                 (struct tree_range){
                                     .low = r.root,
                                     .root = r.root->branch_[R],
                                     .high = r.high,
                                 },
                                 nil);
}

static bool
is_storing_parent(struct ccc_rtom_ const *const t,
                  struct ccc_rtom_elem_ const *const parent,
                  struct ccc_rtom_elem_ const *const root)
{
    if (root == &t->end_)
    {
        return true;
    }
    if (root->parent_ != parent)
    {
        return false;
    }
    return is_storing_parent(t, root, root->branch_[L])
           && is_storing_parent(t, root, root->branch_[R]);
}

static bool
validate(struct ccc_rtom_ const *const rom)
{
    if (!rom->end_.parity_)
    {
        return false;
    }
    if (!are_subtrees_valid(rom,
                            (struct tree_range){
                                .low = &rom->end_,
                                .root = rom->root_,
                                .high = &rom->end_,
                            },
                            &rom->end_))
    {
        return false;
    }
    if (recursive_size(rom, rom->root_) != rom->sz_)
    {
        return false;
    }
    if (!is_storing_parent(rom, &rom->end_, rom->root_))
    {
        return false;
    }
    return true;
}

static void
print_node(struct ccc_rtom_ const *const rom,
           struct ccc_rtom_elem_ const *const root,
           ccc_print_fn *const fn_print)
{
    printf("%s%u%s:", COLOR_CYN, root->parity_, COLOR_NIL);
    fn_print(
        (ccc_user_type){.user_type = struct_base(rom, root), .aux = rom->aux_});
    printf("\n");
}

static void
print_inner_tree(struct ccc_rtom_elem_ const *const root,
                 char const *const prefix,
                 enum rtom_print_link_ const node_type,
                 enum rtom_link_ const dir, struct ccc_rtom_ const *const rom,
                 ccc_print_fn *const fn_print)
{
    if (root == &rom->end_)
    {
        return;
    }
    printf("%s", prefix);
    printf("%s%s", node_type == LEAF ? " └──" : " ├──", COLOR_NIL);
    printf(COLOR_CYN);
    dir == L ? printf("L" COLOR_NIL) : printf("R" COLOR_NIL);
    print_node(rom, root, fn_print);

    char *str = NULL;
    /* NOLINTNEXTLINE */
    int const string_length = snprintf(NULL, 0, "%s%s", prefix,
                                       node_type == LEAF ? "     " : " │   ");
    if (string_length > 0)
    {
        /* NOLINTNEXTLINE */
        str = malloc(string_length + 1);
        assert(str);
        /* NOLINTNEXTLINE */
        (void)snprintf(str, string_length, "%s%s", prefix,
                       node_type == LEAF ? "     " : " │   ");
    }

    if (root->branch_[R] == &rom->end_)
    {
        print_inner_tree(root->branch_[L], str, LEAF, L, rom, fn_print);
    }
    else if (root->branch_[L] == &rom->end_)
    {
        print_inner_tree(root->branch_[R], str, LEAF, R, rom, fn_print);
    }
    else
    {
        print_inner_tree(root->branch_[R], str, BRANCH, R, rom, fn_print);
        print_inner_tree(root->branch_[L], str, LEAF, L, rom, fn_print);
    }
    free(str);
}

static void
ccc_tree_print(struct ccc_rtom_ const *const rom,
               struct ccc_rtom_elem_ const *const root,
               ccc_print_fn *const fn_print)
{
    if (root == &rom->end_)
    {
        return;
    }
    print_node(rom, root, fn_print);

    if (root->branch_[R] == &rom->end_)
    {
        print_inner_tree(root->branch_[L], "", LEAF, L, rom, fn_print);
    }
    else if (root->branch_[L] == &rom->end_)
    {
        print_inner_tree(root->branch_[R], "", LEAF, R, rom, fn_print);
    }
    else
    {
        print_inner_tree(root->branch_[R], "", BRANCH, R, rom, fn_print);
        print_inner_tree(root->branch_[L], "", LEAF, L, rom, fn_print);
    }
}

/* NOLINTEND(*misc-no-recursion) */
