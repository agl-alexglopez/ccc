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
#include "types.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
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

enum rtom_link
{
    L = 0,
    R,
};

enum rtom_print_link
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
};

struct rtom_query
{
    ccc_threeway_cmp last_cmp;
    union {
        struct ccc_rtom_elem_ *found;
        struct ccc_rtom_elem_ *parent;
    };
};

enum rtom_link const inorder_traversal = R;
enum rtom_link const reverse_inorder_traversal = L;

enum rtom_link const min = L;
enum rtom_link const max = R;

/*==============================  Prototypes   ==============================*/

static void init_node(struct ccc_rtom_ *, struct ccc_rtom_elem_ *);
static ccc_threeway_cmp cmp(struct ccc_rtom_ const *, void const *key,
                            struct ccc_rtom_elem_ const *node,
                            ccc_key_cmp_fn *);
static void *struct_base(struct ccc_rtom_ const *,
                         struct ccc_rtom_elem_ const *);
static struct rtom_query find(struct ccc_rtom_ const *, void const *key);
static void swap(uint8_t tmp[], void *a, void *b, size_t elem_sz);
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

static void rotate(struct ccc_rtom_ *rom, struct ccc_rtom_elem_ *p_of_x,
                   struct ccc_rtom_elem_ *x_p_of_y, struct ccc_rtom_elem_ *y,
                   enum rtom_link dir);
static bool validate(struct ccc_rtom_ const *rom);
static void ccc_tree_print(struct ccc_rtom_ const *rom,
                           struct ccc_rtom_elem_ const *root,
                           ccc_print_fn *fn_print);

static struct ccc_rtom_elem_ const *
next(struct ccc_rtom_ *, struct ccc_rtom_elem_ const *, enum rtom_link);
static struct ccc_rtom_elem_ *min_max_from(struct ccc_rtom_ const *,
                                           struct ccc_rtom_elem_ *start,
                                           enum rtom_link);

/*==============================  Interface    ==============================*/

bool
ccc_rom_contains(ccc_realtime_ordered_map const *const rom,
                 void const *const key)
{
    return CCC_EQL == find(&rom->impl, key).last_cmp;
}

void const *
ccc_rom_get(ccc_realtime_ordered_map const *rom, void const *key)
{
    struct rtom_query q = find(&rom->impl, key);
    if (CCC_EQL == q.last_cmp)
    {
        return struct_base(&rom->impl, q.found);
    }
    return NULL;
}

void *
ccc_rom_get_mut(ccc_realtime_ordered_map const *rom, void const *key)
{
    return (void *)ccc_rom_get(rom, key);
}

ccc_entry
ccc_rom_insert(ccc_realtime_ordered_map *const rom,
               ccc_rtom_elem *const out_handle)
{
    struct rtom_query q = find(
        &rom->impl, ccc_impl_rom_key_from_node(&rom->impl, &out_handle->impl));
    if (CCC_EQL == q.last_cmp)
    {
        *out_handle = *(ccc_rtom_elem *)q.found;
        uint8_t tmp[rom->impl.elem_sz_];
        void *user_struct = struct_base(&rom->impl, &out_handle->impl);
        void *ret = struct_base(&rom->impl, q.found);
        swap(tmp, user_struct, ret, rom->impl.elem_sz_);
        return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted = maybe_alloc_insert(&rom->impl, q.parent, q.last_cmp,
                                        &out_handle->impl);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_rtom_entry
ccc_rom_entry(ccc_realtime_ordered_map const *rom, void const *key)
{
    return (ccc_rtom_entry){ccc_impl_rom_entry(&rom->impl, key)};
}

void *
ccc_rom_or_insert(ccc_rtom_entry const *const e, ccc_rtom_elem *const elem)
{
    if (e->impl.entry_.stats_ == CCC_ROM_ENTRY_OCCUPIED)
    {
        return e->impl.entry_.e_;
    }
    return maybe_alloc_insert(
        e->impl.rom_,
        ccc_impl_rom_elem_in_slot(e->impl.rom_, e->impl.entry_.e_),
        e->impl.last_cmp_, &elem->impl);
}

void *
ccc_rom_insert_entry(ccc_rtom_entry const *const e, ccc_rtom_elem *const elem)
{
    if (e->impl.entry_.stats_ == CCC_ROM_ENTRY_OCCUPIED)
    {
        elem->impl
            = *ccc_impl_rom_elem_in_slot(e->impl.rom_, e->impl.entry_.e_);
        memcpy((void *)e->impl.entry_.e_,
               struct_base(e->impl.rom_, &elem->impl), e->impl.rom_->elem_sz_);
        return (void *)e->impl.entry_.e_;
    }
    return maybe_alloc_insert(
        e->impl.rom_,
        ccc_impl_rom_elem_in_slot(e->impl.rom_, e->impl.entry_.e_),
        e->impl.last_cmp_, &elem->impl);
}

ccc_entry
ccc_rom_remove_entry(ccc_rtom_entry const *const e)
{
    if (e->impl.entry_.stats_ == CCC_ROM_ENTRY_OCCUPIED)
    {
        void *const erased = remove_fixup(
            e->impl.rom_,
            ccc_impl_rom_elem_in_slot(e->impl.rom_, e->impl.entry_.e_));
        assert(erased);
        if (e->impl.rom_->alloc_)
        {
            e->impl.rom_->alloc_(erased, 0);
            return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_OCCUPIED}};
        }
        return (ccc_entry){{.e_ = erased, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_rom_remove(ccc_realtime_ordered_map *const rom,
               ccc_rtom_elem *const out_handle)
{
    struct rtom_query q = find(
        &rom->impl, ccc_impl_rom_key_from_node(&rom->impl, &out_handle->impl));
    if (q.last_cmp != CCC_EQL)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
    }
    void *const removed = remove_fixup(&rom->impl, q.found);
    if (rom->impl.alloc_)
    {
        void *const user_struct = struct_base(&rom->impl, &out_handle->impl);
        memcpy(user_struct, removed, rom->impl.elem_sz_);
        rom->impl.alloc_(removed, 0);
        return (ccc_entry){{.e_ = user_struct, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    return (ccc_entry){{.e_ = removed, .stats_ = CCC_ENTRY_OCCUPIED}};
}

void const *
ccc_rom_unwrap(ccc_rtom_entry const *const e)
{
    if (e->impl.entry_.stats_ & CCC_ROM_ENTRY_OCCUPIED)
    {
        return e->impl.entry_.e_;
    }
    return NULL;
}

void *
ccc_rom_unwrap_mut(ccc_rtom_entry const *const e)
{
    if (e->impl.entry_.stats_ & CCC_ROM_ENTRY_OCCUPIED)
    {
        return e->impl.entry_.e_;
    }
    return NULL;
}

static struct ccc_rtom_elem_ *
min_max_from(struct ccc_rtom_ const *const rom, struct ccc_rtom_elem_ *start,
             enum rtom_link const dir)
{
    if (start == &rom->end_)
    {
        return start;
    }
    for (; start->link_[dir] != &rom->end_; start = start->link_[dir])
    {}
    return start;
}

inline void *
ccc_rom_begin(ccc_realtime_ordered_map const *rom)
{
    struct ccc_rtom_elem_ *m = min_max_from(&rom->impl, rom->impl.root_, min);
    return m == &rom->impl.end_ ? NULL : struct_base(&rom->impl, m);
}

inline void *
ccc_rom_next(ccc_realtime_ordered_map *const rom, ccc_rtom_elem const *const e)
{
    struct ccc_rtom_elem_ const *const n
        = next(&rom->impl, &e->impl, inorder_traversal);
    if (n == &rom->impl.end_)
    {
        return NULL;
    }
    return struct_base(&rom->impl, n);
}

inline void *
ccc_rom_rbegin(ccc_realtime_ordered_map const *const rom)
{
    struct ccc_rtom_elem_ *m = min_max_from(&rom->impl, rom->impl.root_, max);
    return m == &rom->impl.end_ ? NULL : struct_base(&rom->impl, m);
}

inline void *
ccc_rom_end([[maybe_unused]] ccc_realtime_ordered_map const *const rom)
{
    return NULL;
}

inline void *
ccc_rom_rend([[maybe_unused]] ccc_realtime_ordered_map const *const rom)
{
    return NULL;
}

inline void *
ccc_rom_rnext(ccc_realtime_ordered_map *const rom, ccc_rtom_elem const *const e)
{
    struct ccc_rtom_elem_ const *const n
        = next(&rom->impl, &e->impl, reverse_inorder_traversal);
    if (n == &rom->impl.end_)
    {
        return NULL;
    }
    return struct_base(&rom->impl, n);
}

inline size_t
ccc_rom_size(ccc_realtime_ordered_map const *const rom)
{
    return rom->impl.sz_;
}

inline bool
ccc_rom_empty(ccc_realtime_ordered_map const *const rom)
{
    return !rom->impl.sz_;
}

inline void *
ccc_rom_root(ccc_realtime_ordered_map const *rom)
{
    if (!rom || rom->impl.root_ == &rom->impl.end_)
    {
        return NULL;
    }
    return struct_base(&rom->impl, rom->impl.root_);
}

bool
ccc_rom_validate(ccc_realtime_ordered_map const *rom)
{
    return validate(&rom->impl);
}

void
ccc_rom_print(ccc_realtime_ordered_map const *const rom, ccc_print_fn *const fn)
{
    ccc_tree_print(&rom->impl, rom->impl.root_, fn);
}

void
ccc_rom_clear(ccc_realtime_ordered_map *const rom,
              ccc_destructor_fn *const destructor)
{
    if (destructor)
    {
        while (!ccc_rom_empty(rom))
        {
            void *deleted = remove_fixup(&rom->impl, rom->impl.root_);
            destructor(deleted);
        }
    }
    else
    {
        while (!ccc_rom_empty(rom))
        {
            (void)remove_fixup(&rom->impl, rom->impl.root_);
        }
    }
}

ccc_result
ccc_rom_clear_and_free(ccc_realtime_ordered_map *const rom,
                       ccc_destructor_fn *const destructor)
{
    if (!rom->impl.alloc_)
    {
        return CCC_NO_REALLOC;
    }
    if (destructor)
    {
        while (!ccc_rom_empty(rom))
        {
            void *const deleted = remove_fixup(&rom->impl, rom->impl.root_);
            destructor(deleted);
            rom->impl.alloc_(deleted, 0);
        }
    }
    else
    {
        while (!ccc_rom_empty(rom))
        {
            rom->impl.alloc_(remove_fixup(&rom->impl, rom->impl.root_), 0);
        }
    }
    return CCC_OK;
}

/*=========================   Private Interface  ============================*/

struct ccc_rtom_entry_
ccc_impl_rom_entry(struct ccc_rtom_ const *const rom, void const *const key)
{
    struct rtom_query q = find(rom, key);
    if (CCC_EQL == q.last_cmp)
    {
        return (struct ccc_rtom_entry_){
            .rom_ = (struct ccc_rtom_ *)rom,
            .last_cmp_ = q.last_cmp,
            .entry_ = {
                .e_ = struct_base(rom, q.found),
                .stats_ = CCC_ROM_ENTRY_OCCUPIED,
            },
        };
    }
    return (struct ccc_rtom_entry_){
        .rom_ = (struct ccc_rtom_ *)rom,
        .last_cmp_ = q.last_cmp,
        .entry_ = {
            .e_ = struct_base(rom, q.parent),
            .stats_ = CCC_ROM_ENTRY_VACANT,
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
        = parent->link_[L] == &rom->end_ && parent->link_[R] == &rom->end_;
    parent->link_[CCC_GRT == last_cmp] = out_handle;
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
    return (uint8_t *)struct_base(rom, elem) + rom->key_offset_;
}

struct ccc_rtom_elem_ *
ccc_impl_rom_elem_in_slot(struct ccc_rtom_ const *const rom,
                          void const *const slot)
{
    return (struct ccc_rtom_elem_ *)((uint8_t *)slot + rom->node_elem_offset_);
}

/*=========================    Static Helpers    ============================*/

static void *
maybe_alloc_insert(struct ccc_rtom_ *const rom,
                   struct ccc_rtom_elem_ *const parent,
                   ccc_threeway_cmp last_cmp, struct ccc_rtom_elem_ *out_handle)
{
    init_node(rom, out_handle);
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
    if (!rom->sz_)
    {
        rom->root_ = out_handle;
        ++rom->sz_;
        return struct_base(rom, out_handle);
    }
    assert(last_cmp == CCC_GRT || last_cmp == CCC_LES);
    bool const rank_rule_break
        = parent->link_[L] == &rom->end_ && parent->link_[R] == &rom->end_;
    parent->link_[CCC_GRT == last_cmp] = out_handle;
    out_handle->parent_ = parent;
    if (rank_rule_break)
    {
        insert_fixup(rom, parent, out_handle);
    }
    ++rom->sz_;
    return struct_base(rom, out_handle);
}

static struct rtom_query
find(struct ccc_rtom_ const *const rom, void const *const key)
{
    struct ccc_rtom_elem_ const *parent = &rom->end_;
    ccc_threeway_cmp link = CCC_CMP_ERR;
    for (struct ccc_rtom_elem_ const *seek = rom->root_; seek != &rom->end_;
         parent = seek, seek = seek->link_[CCC_GRT == link])
    {
        link = cmp(rom, key, seek, rom->cmp_);
        if (CCC_EQL == link)
        {
            return (struct rtom_query){
                .last_cmp = CCC_EQL,
                .found = (struct ccc_rtom_elem_ *)seek,
            };
        }
    }
    return (struct rtom_query){
        .last_cmp = link,
        .parent = (struct ccc_rtom_elem_ *)parent,
    };
}

static struct ccc_rtom_elem_ const *
next(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ const *n,
     enum rtom_link const traversal)
{
    if (!n || n == &rom->end_)
    {
        return &rom->end_;
    }
    assert(rom->root_->parent_ == &rom->end_);
    /* The node is an internal one that has a subtree to explore first. */
    if (n->link_[traversal] != &rom->end_)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->link_[traversal]; n->link_[!traversal] != &rom->end_;
             n = n->link_[!traversal])
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
    for (; n->parent_ != &rom->end_ && n->parent_->link_[!traversal] != n;
         n = n->parent_)
    {}
    return n->parent_;
}

static inline void
init_node(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ *const e)
{
    assert(e != NULL);
    assert(rom != NULL);
    e->link_[L] = &rom->end_;
    e->link_[R] = &rom->end_;
    e->parent_ = &rom->end_;
    e->parity_ = 0;
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

static inline ccc_threeway_cmp
cmp(struct ccc_rtom_ const *const rom, void const *const key,
    struct ccc_rtom_elem_ const *const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){
        .key = key,
        .container = struct_base(rom, node),
        .aux = rom->aux_,
    });
}

static void *
struct_base(struct ccc_rtom_ const *const rom,
            struct ccc_rtom_elem_ const *const e)
{
    return ((uint8_t *)e->link_) - rom->node_elem_offset_;
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
    enum rtom_link const p_to_x_dir = z_p_of_xy->link_[R] == x;
    struct ccc_rtom_elem_ *y = x->link_[!p_to_x_dir];
    if (y == &rom->end_ || is_2_child(rom, z_p_of_xy, y))
    {
        rotate(rom, z_p_of_xy, x, y, !p_to_x_dir);
        assert(x->link_[!p_to_x_dir] == z_p_of_xy);
        assert(z_p_of_xy->link_[p_to_x_dir] == y);
        demote(rom, z_p_of_xy);
    }
    else
    {
        assert(is_1_child(rom, z_p_of_xy, y));
        rotate(rom, x, y, y->link_[p_to_x_dir], p_to_x_dir);
        rotate(rom, y->parent_, y, y->link_[!p_to_x_dir], !p_to_x_dir);
        assert(y->link_[p_to_x_dir] == x);
        assert(y->link_[!p_to_x_dir] == z_p_of_xy);
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
    if (remove->link_[L] == &rom->end_ || remove->link_[R] == &rom->end_)
    {
        y = remove;
        p_of_xy = y->parent_;
        x = y->link_[y->link_[L] == &rom->end_];
        x->parent_ = y->parent_;
        if (p_of_xy == &rom->end_)
        {
            rom->root_ = x;
        }
        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->link_[p_of_xy->link_[R] == y] = x;
    }
    else
    {
        y = min_max_from(rom, remove->link_[R], min);
        p_of_xy = y->parent_;
        x = y->link_[y->link_[L] == &rom->end_];
        x->parent_ = y->parent_;

        /* Save if check and improve readability by assuming this is true. */
        assert(p_of_xy != &rom->end_);

        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->link_[p_of_xy->link_[R] == y] = x;
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
    remove->link_[L] = remove->link_[R] = remove->parent_ = NULL;
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
    else if (x == &rom->end_ && p_of_xy->link_[L] == p_of_xy->link_[R])
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
        struct ccc_rtom_elem_ *y = p_of_xy->link_[p_of_xy->link_[L] == x];
        made_3_child = is_2_child(rom, p_of_p_of_x, p_of_xy);
        if (is_2_child(rom, p_of_xy, y))
        {
            demote(rom, p_of_xy);
        }
        else if (is_22_parent(rom, y->link_[L], y, y->link_[R]))
        {
            demote(rom, p_of_xy);
            demote(rom, y);
        }
        else /* p(x) is 1,3 and y is not a 2,2 parent and x is 3-child.*/
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
    enum rtom_link const z_to_x_dir = z->link_[R] == x;
    struct ccc_rtom_elem_ *const w = y->link_[!z_to_x_dir];
    if (is_1_child(rom, y, w))
    {
        rotate(rom, z, y, y->link_[z_to_x_dir], z_to_x_dir);
        assert(y->link_[z_to_x_dir] == z);
        assert(y->link_[!z_to_x_dir] == w);
        promote(rom, y);
        demote(rom, z);
        if (is_leaf(rom, z))
        {
            demote(rom, z);
        }
    }
    else /* w is a 2-child and v will be a 1-child. */
    {
        struct ccc_rtom_elem_ *const v = y->link_[z_to_x_dir];
        assert(is_2_child(rom, y, w));
        assert(is_1_child(rom, y, v));
        rotate(rom, y, v, v->link_[!z_to_x_dir], !z_to_x_dir);
        rotate(rom, v->parent_, v, v->link_[z_to_x_dir], z_to_x_dir);
        assert(v->link_[z_to_x_dir] == z);
        assert(v->link_[!z_to_x_dir] == y);
        double_promote(rom, v);
        demote(rom, y);
        double_demote(rom, z);
        /* At this point, the original paper mentions "Rebalancing with
           Promotion," and defines it as follows:
               if node z is a non-leaf 1,1 node, we promote it; otherwise, if y
               is a non-leaf 1,1 node, we promote it. (See Figure 4.)
               (Haeupler et. al. 2014, 17).
           This reduces constants in some of theorems mentioned in the paper
           but I am not sure if it is worth including as it does not ultimately
           improve the worst case rotations to less than 2. This should be
           revisited after more performance testing code is in place. */
        if (!is_leaf(rom, z) && is_11_parent(rom, z->link_[L], z, z->link_[R]))
        {
            promote(rom, z);
        }
        else if (!is_leaf(rom, y)
                 && is_11_parent(rom, y->link_[L], y, y->link_[R]))
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
        remove->parent_->link_[remove->parent_->link_[R] == remove]
            = replacement;
    }
    remove->link_[R]->parent_ = replacement;
    remove->link_[L]->parent_ = replacement;
    replacement->link_[R] = remove->link_[R];
    replacement->link_[L] = remove->link_[L];
    replacement->parity_ = remove->parity_;
}

static inline void
rotate(struct ccc_rtom_ *const rom, struct ccc_rtom_elem_ *const p_of_x,
       struct ccc_rtom_elem_ *const x_p_of_y, struct ccc_rtom_elem_ *const y,
       enum rtom_link dir)
{
    struct ccc_rtom_elem_ *const p_of_p_of_x = p_of_x->parent_;
    x_p_of_y->parent_ = p_of_p_of_x;
    if (p_of_p_of_x == &rom->end_)
    {
        rom->root_ = x_p_of_y;
    }
    else
    {
        p_of_p_of_x->link_[p_of_p_of_x->link_[R] == p_of_x] = x_p_of_y;
    }
    x_p_of_y->link_[dir] = p_of_x;
    p_of_x->parent_ = x_p_of_y;
    p_of_x->link_[!dir] = y;
    y->parent_ = p_of_x;
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
double_promote([[maybe_unused]] struct ccc_rtom_ const *const rom,
               [[maybe_unused]] struct ccc_rtom_elem_ *const x)
{}

static inline void
double_demote([[maybe_unused]] struct ccc_rtom_ const *const rom,
              [[maybe_unused]] struct ccc_rtom_elem_ *const x)
{}

static inline bool
is_leaf(struct ccc_rtom_ const *const rom, struct ccc_rtom_elem_ const *const x)
{
    return x->link_[L] == &rom->end_ && x->link_[R] == &rom->end_;
}

static inline struct ccc_rtom_elem_ *
sibling_of([[maybe_unused]] struct ccc_rtom_ const *const rom,
           struct ccc_rtom_elem_ const *const x)
{
    assert(x->parent_ != &rom->end_);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return x->parent_->link_[x->parent_->link_[L] == x];
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

struct ccc_tree_range
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
    return 1 + recursive_size(rom, r->link_[R])
           + recursive_size(rom, r->link_[L]);
}

static bool
are_subtrees_valid(struct ccc_rtom_ const *t, struct ccc_tree_range const r,
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
                              (struct ccc_tree_range){
                                  .low = r.low,
                                  .root = r.root->link_[L],
                                  .high = r.root,
                              },
                              nil)
           && are_subtrees_valid(t,
                                 (struct ccc_tree_range){
                                     .low = r.root,
                                     .root = r.root->link_[R],
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
    return is_storing_parent(t, root, root->link_[L])
           && is_storing_parent(t, root, root->link_[R]);
}

static bool
validate(struct ccc_rtom_ const *const rom)
{
    if (!rom->end_.parity_)
    {
        return false;
    }
    if (!are_subtrees_valid(rom,
                            (struct ccc_tree_range){
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
    fn_print(struct_base(rom, root));
    printf("\n");
}

static void
print_inner_tree(struct ccc_rtom_elem_ const *const root,
                 char const *const prefix, enum rtom_print_link const node_type,
                 enum rtom_link const dir, struct ccc_rtom_ const *const rom,
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

    if (root->link_[R] == &rom->end_)
    {
        print_inner_tree(root->link_[L], str, LEAF, L, rom, fn_print);
    }
    else if (root->link_[L] == &rom->end_)
    {
        print_inner_tree(root->link_[R], str, LEAF, R, rom, fn_print);
    }
    else
    {
        print_inner_tree(root->link_[R], str, BRANCH, R, rom, fn_print);
        print_inner_tree(root->link_[L], str, LEAF, L, rom, fn_print);
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

    if (root->link_[R] == &rom->end_)
    {
        print_inner_tree(root->link_[L], "", LEAF, L, rom, fn_print);
    }
    else if (root->link_[L] == &rom->end_)
    {
        print_inner_tree(root->link_[R], "", LEAF, R, rom, fn_print);
    }
    else
    {
        print_inner_tree(root->link_[R], "", BRANCH, R, rom, fn_print);
        print_inner_tree(root->link_[L], "", LEAF, L, rom, fn_print);
    }
}

/* NOLINTEND(*misc-no-recursion) */
