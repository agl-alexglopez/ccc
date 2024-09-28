#include "flat_realtime_ordered_map.h"
#include "buffer.h"
#include "impl_flat_realtime_ordered_map.h"
#include "impl_types.h"
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

enum frm_branch_
{
    L = 0,
    R,
};

enum frm_print_branch_
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
};

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

/*==============================  Prototypes   ==============================*/

static void *struct_base(struct ccc_frm_ const *, struct ccc_frm_elem_ const *);
static void swap(char tmp[], void *a, void *b, size_t elem_sz);
static struct ccc_frm_elem_ *at(struct ccc_frm_ const *, size_t);
static void *base_at(struct ccc_frm_ const *, size_t);
static ccc_threeway_cmp cmp_elems(struct ccc_frm_ const *rom, void const *key,
                                  size_t node, ccc_key_cmp_fn *fn);
static inline struct frm_query_ find(struct ccc_frm_ const *frm,
                                     void const *key);
static void *maybe_alloc_insert(struct ccc_frm_ *frm, size_t parent,
                                ccc_threeway_cmp last_cmp,
                                struct ccc_frm_elem_ *elem);
static void init_node(struct ccc_frm_elem_ *e);
static void insert_fixup(struct ccc_frm_ *t, size_t z_p_of_xy, size_t x);

static bool is_0_child(struct ccc_frm_ const *, size_t p_of_x, size_t x);
static bool is_1_child(struct ccc_frm_ const *, size_t p_of_x, size_t x);
static bool is_2_child(struct ccc_frm_ const *, size_t p_of_x, size_t x);
static bool is_3_child(struct ccc_frm_ const *, size_t p_of_x, size_t x);
static bool is_01_parent(struct ccc_frm_ const *, size_t x, size_t p_of_xy,
                         size_t y);
static bool is_11_parent(struct ccc_frm_ const *, size_t x, size_t p_of_xy,
                         size_t y);
static bool is_02_parent(struct ccc_frm_ const *, size_t x, size_t p_of_xy,
                         size_t y);
static bool is_22_parent(struct ccc_frm_ const *, size_t x, size_t p_of_xy,
                         size_t y);
static bool is_leaf(struct ccc_frm_ const *t, size_t x);
static size_t sibling_of(struct ccc_frm_ const *t, size_t x);
static void promote(struct ccc_frm_ const *t, size_t x);
static void demote(struct ccc_frm_ const *t, size_t x);
static void double_promote(struct ccc_frm_ const *t, size_t x);
static void double_demote(struct ccc_frm_ const *t, size_t x);

static void rotate(struct ccc_frm_ *t, size_t z_p_of_x, size_t x_p_of_y,
                   size_t y, enum frm_branch_ dir);
static void double_rotate(struct ccc_frm_ *t, size_t z_p_of_x, size_t x_p_of_y,
                          size_t y, enum frm_branch_ dir);
static size_t next(struct ccc_frm_ const *t, size_t n,
                   enum frm_branch_ traversal);
static size_t min_max_from(struct ccc_frm_ const *t, size_t start,
                           enum frm_branch_ dir);
static size_t branch_i(struct ccc_frm_ const *t, size_t parent,
                       enum frm_branch_ dir);
static size_t parent_i(struct ccc_frm_ const *t, size_t child);
static size_t *branch_ref(struct ccc_frm_ const *t, size_t node,
                          enum frm_branch_ branch);
static size_t *parent_ref(struct ccc_frm_ const *t, size_t node);
static uint8_t parity(struct ccc_frm_ const *t, size_t node);
static size_t index_of(struct ccc_frm_ const *t,
                       struct ccc_frm_elem_ const *elem);
static bool validate(struct ccc_frm_ const *frm);
static void ccc_tree_print(struct ccc_frm_ const *t, size_t root,
                           ccc_print_fn *fn_print);

/*==============================  Interface    ==============================*/

bool
ccc_frm_contains(ccc_flat_realtime_ordered_map const *const frm,
                 void const *const key)
{
    return CCC_EQL == find(frm, key).last_cmp_;
}

void *
ccc_frm_get_key_val(ccc_flat_realtime_ordered_map const *frm,
                    void const *const key)
{
    struct frm_query_ q = find(frm, key);
    return (CCC_EQL == q.last_cmp_) ? base_at(frm, q.found_) : NULL;
}

ccc_entry
ccc_frm_insert(ccc_flat_realtime_ordered_map *const frm,
               ccc_frtm_elem *const out_handle, void *const tmp)
{
    struct frm_query_ q
        = find(frm, ccc_impl_frm_key_from_node(frm, out_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        void *const slot = ccc_buf_at(&frm->buf_, q.found_);
        *out_handle = *ccc_impl_frm_elem_in_slot(frm, slot);
        void *user_struct = struct_base(frm, out_handle);
        swap(tmp, user_struct, slot, ccc_buf_elem_size(&frm->buf_));
        return (ccc_entry){{.e_ = slot, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted
        = maybe_alloc_insert(frm, q.parent_, q.last_cmp_, out_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

ccc_entry
ccc_rom_try_insert(ccc_flat_realtime_ordered_map *const rom,
                   ccc_frtm_elem *const key_val_handle)
{
    struct frm_query_ q
        = find(rom, ccc_impl_frm_key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        return (ccc_entry){
            {.e_ = base_at(rom, q.found_), .stats_ = CCC_ENTRY_OCCUPIED}};
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
ccc_rom_insert_or_assign(ccc_flat_realtime_ordered_map *const rom,
                         ccc_frtm_elem *const key_val_handle)
{
    struct frm_query_ q
        = find(rom, ccc_impl_frm_key_from_node(rom, key_val_handle));
    if (CCC_EQL == q.last_cmp_)
    {
        void *const found = base_at(rom, q.found_);
        ccc_buf_write(&rom->buf_, q.found_, struct_base(rom, key_val_handle));
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

bool
ccc_frm_empty(ccc_flat_realtime_ordered_map const *const frm)
{
    return !ccc_frm_size(frm);
}

size_t
ccc_frm_size(ccc_flat_realtime_ordered_map const *const frm)
{
    size_t const sz = ccc_buf_size(&frm->buf_);
    return !sz ? sz : sz - 1;
}

void *
ccc_frm_begin(ccc_flat_realtime_ordered_map const *frm)
{
    size_t const n = min_max_from(frm, frm->root_, min);
    return base_at(frm, n);
}

void *
ccc_frm_rbegin(ccc_flat_realtime_ordered_map const *frm)
{
    size_t const n = min_max_from(frm, frm->root_, max);
    return base_at(frm, n);
}

void *
ccc_frm_next(ccc_flat_realtime_ordered_map const *frm,
             ccc_frtm_elem const *const e)
{
    size_t const n = next(frm, index_of(frm, e), inorder_traversal);
    return base_at(frm, n);
}

void *
ccc_frm_rnext(ccc_flat_realtime_ordered_map const *const frm,
              ccc_frtm_elem const *const e)
{
    size_t const n = next(frm, index_of(frm, e), reverse_inorder_traversal);
    return base_at(frm, n);
}

void *
ccc_frm_end(ccc_flat_realtime_ordered_map const *const frm)
{
    return base_at(frm, 0);
}

void *
ccc_frm_rend(ccc_flat_realtime_ordered_map const *const frm)
{
    return base_at(frm, 0);
}

bool
ccc_frm_validate(ccc_flat_realtime_ordered_map const *const frm)
{
    return validate(frm);
}

void *
ccc_frm_root(ccc_flat_realtime_ordered_map const *const frm)
{
    return frm->root_ ? base_at(frm, frm->root_) : NULL;
}

void
ccc_frm_print(ccc_flat_realtime_ordered_map const *const frm,
              ccc_print_fn *const fn)
{
    ccc_tree_print(frm, frm->root_, fn);
}

/*========================  Private Interface  ==============================*/

void *
ccc_impl_frm_insert(struct ccc_frm_ *const frm, size_t const parent_i,
                    ccc_threeway_cmp const last_cmp, size_t const elem_i)
{
    struct ccc_frm_elem_ *elem = at(frm, elem_i);
    init_node(elem);
    if (ccc_buf_size(&frm->buf_) == 2)
    {
        frm->root_ = elem_i;
        return struct_base(frm, elem);
    }
    assert(last_cmp == CCC_GRT || last_cmp == CCC_LES);
    struct ccc_frm_elem_ *parent = at(frm, parent_i);
    bool const rank_rule_break = !parent->branch_[L] && !parent->branch_[R];
    parent->branch_[CCC_GRT == last_cmp] = elem_i;
    elem->parent_ = parent_i;
    if (rank_rule_break)
    {
        insert_fixup(frm, parent_i, elem_i);
    }
    return struct_base(frm, elem);
}

void *
ccc_impl_frm_key_from_node(struct ccc_frm_ const *const frm,
                           struct ccc_frm_elem_ const *const elem)
{
    return (char *)struct_base(frm, elem) + frm->key_offset_;
}

void *
ccc_impl_frm_key_in_slot(struct ccc_frm_ const *const frm,
                         void const *const slot)
{
    return (char *)slot + frm->key_offset_;
}

struct ccc_frm_elem_ *
ccc_impl_frm_elem_in_slot(struct ccc_frm_ const *const frm,
                          void const *const slot)
{
    return (struct ccc_frm_elem_ *)((char *)slot + frm->node_elem_offset_);
}

/*==========================  Static Helpers   ==============================*/

static void *
maybe_alloc_insert(struct ccc_frm_ *const frm, size_t const parent,
                   ccc_threeway_cmp const last_cmp,
                   struct ccc_frm_elem_ *const elem)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementor is always at least 1. */
    if (ccc_buf_empty(&frm->buf_))
    {
        void *const sentinel = ccc_buf_alloc(&frm->buf_);
        if (!sentinel)
        {
            return NULL;
        }
        ccc_impl_frm_elem_in_slot(frm, sentinel)->parity_ = 1;
    }
    if (!ccc_buf_alloc(&frm->buf_))
    {
        return NULL;
    }
    size_t const node = ccc_buf_size(&frm->buf_) - 1;
    ccc_buf_write(&frm->buf_, node, struct_base(frm, elem));
    return ccc_impl_frm_insert(frm, parent, last_cmp, node);
}

static inline struct frm_query_
find(struct ccc_frm_ const *const frm, void const *const key)
{
    size_t parent = 0;
    ccc_threeway_cmp link = CCC_CMP_ERR;
    for (size_t seek = frm->root_; seek;
         parent = seek, seek = branch_i(frm, seek, CCC_GRT == link))
    {
        link = cmp_elems(frm, key, seek, frm->cmp_);
        if (CCC_EQL == link)
        {
            return (struct frm_query_){.last_cmp_ = CCC_EQL, .found_ = seek};
        }
    }
    return (struct frm_query_){.last_cmp_ = link, .parent_ = parent};
}

static inline size_t
next(struct ccc_frm_ const *const t, size_t n, enum frm_branch_ const traversal)
{
    if (!n)
    {
        return 0;
    }
    assert(!parent_i(t, t->root_));
    /* The node is an internal one that has a subtree to explore first. */
    if (branch_i(t, n, traversal))
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = branch_i(t, n, traversal); branch_i(t, n, !traversal);
             n = branch_i(t, n, !traversal))
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
    for (; parent_i(t, n) && branch_i(t, parent_i(t, n), !traversal) != n;
         n = parent_i(t, n))
    {}
    return parent_i(t, n);
}

static inline size_t
min_max_from(struct ccc_frm_ const *const t, size_t start,
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
cmp_elems(struct ccc_frm_ const *const rom, void const *const key,
          size_t const node, ccc_key_cmp_fn *const fn)
{
    return fn(&(ccc_key_cmp){
        .key = key,
        .container = base_at(rom, node),
        .aux = rom->aux_,
    });
}

static inline void
init_node(struct ccc_frm_elem_ *const e)
{
    assert(e != NULL);
    e->branch_[L] = e->branch_[R] = 0;
    e->parent_ = 0;
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

static inline struct ccc_frm_elem_ *
at(struct ccc_frm_ const *const t, size_t const i)
{
    return ccc_impl_frm_elem_in_slot(t, ccc_buf_at(&t->buf_, i));
}

static inline size_t
branch_i(struct ccc_frm_ const *const t, size_t const parent,
         enum frm_branch_ const dir)
{
    return ccc_impl_frm_elem_in_slot(t, ccc_buf_at(&t->buf_, parent))
        ->branch_[dir];
}

static inline size_t
parent_i(struct ccc_frm_ const *const t, size_t const child)
{
    return ccc_impl_frm_elem_in_slot(t, ccc_buf_at(&t->buf_, child))->parent_;
}

static inline size_t
index_of(struct ccc_frm_ const *const t, struct ccc_frm_elem_ const *const elem)
{
    return ccc_buf_index_of(&t->buf_, struct_base(t, elem));
}

static inline uint8_t
parity(struct ccc_frm_ const *t, size_t const node)
{
    return ccc_impl_frm_elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parity_;
}

static inline size_t *
branch_ref(struct ccc_frm_ const *t, size_t const node,
           enum frm_branch_ const branch)
{
    return &ccc_impl_frm_elem_in_slot(t, ccc_buf_at(&t->buf_, node))
                ->branch_[branch];
}

static inline size_t *
parent_ref(struct ccc_frm_ const *t, size_t node)
{

    return &ccc_impl_frm_elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parent_;
}

static inline void *
base_at(struct ccc_frm_ const *const frm, size_t const i)
{
    return ccc_buf_at(&frm->buf_, i);
}

static inline void *
struct_base(struct ccc_frm_ const *const frm,
            struct ccc_frm_elem_ const *const e)
{
    return ((char *)e->branch_) - frm->node_elem_offset_;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct ccc_frm_ *const t, size_t z_p_of_xy, size_t x)
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
rotate(struct ccc_frm_ *const t, size_t const z_p_of_x, size_t const x_p_of_y,
       size_t const y, enum frm_branch_ const dir)
{
    assert(z_p_of_x);
    struct ccc_frm_elem_ *const z_ref = at(t, z_p_of_x);
    struct ccc_frm_elem_ *const x_ref = at(t, x_p_of_y);
    size_t const p_of_p_of_x = parent_i(t, z_p_of_x);
    x_ref->parent_ = p_of_p_of_x;
    if (!p_of_p_of_x)
    {
        t->root_ = x_p_of_y;
    }
    else
    {
        struct ccc_frm_elem_ *const g = at(t, p_of_p_of_x);
        g->branch_[g->branch_[R] == z_p_of_x] = x_p_of_y;
    }
    x_ref->branch_[dir] = z_p_of_x;
    z_ref->parent_ = x_p_of_y;
    z_ref->branch_[!dir] = y;
    *parent_ref(t, y) = z_p_of_x;
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
double_rotate(struct ccc_frm_ *const t, size_t const z_p_of_x,
              size_t const x_p_of_y, size_t const y, enum frm_branch_ const dir)
{
    assert(z_p_of_x && x_p_of_y && y);
    struct ccc_frm_elem_ *const z_ref = at(t, z_p_of_x);
    struct ccc_frm_elem_ *const x_ref = at(t, x_p_of_y);
    struct ccc_frm_elem_ *const y_ref = at(t, y);
    // struct ccc_frm_elem_ *const y = at(t, y);
    size_t const p_of_p_of_x = z_ref->parent_;
    y_ref->parent_ = p_of_p_of_x;
    if (!p_of_p_of_x)
    {
        t->root_ = y;
    }
    else
    {
        struct ccc_frm_elem_ *const g = at(t, p_of_p_of_x);
        g->branch_[g->branch_[R] == z_p_of_x] = y;
    }
    x_ref->branch_[!dir] = y_ref->branch_[dir];
    *parent_ref(t, y_ref->branch_[dir]) = x_p_of_y;
    y_ref->branch_[dir] = x_p_of_y;
    x_ref->parent_ = y;

    z_ref->branch_[dir] = y_ref->branch_[!dir];
    *parent_ref(t, y_ref->branch_[!dir]) = z_p_of_x;
    y_ref->branch_[!dir] = z_p_of_x;
    z_ref->parent_ = y;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
       0/
       x*/
[[maybe_unused]] static inline bool
is_0_child(struct ccc_frm_ const *const t, size_t p_of_x, size_t x)
{
    return p_of_x && parity(t, p_of_x) == parity(t, x);
}

/* Returns true for rank difference 1 between the parent and node.
         p
       1/
       x*/
static inline bool
is_1_child(struct ccc_frm_ const *const t, size_t p_of_x, size_t x)
{
    return p_of_x && parity(t, p_of_x) != parity(t, x);
}

/* Returns true for rank difference 2 between the parent and node.
         p
       2/
       x*/
static inline bool
is_2_child(struct ccc_frm_ const *const t, size_t const p_of_x, size_t const x)
{
    return p_of_x && parity(t, p_of_x) == parity(t, x);
}

/* Returns true for rank difference 3 between the parent and node.
         p
       3/
       x*/
[[maybe_unused]] static inline bool
is_3_child(struct ccc_frm_ const *const t, size_t const p_of_x, size_t const x)
{
    return p_of_x && parity(t, p_of_x) != parity(t, x);
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
       0/ \1
       x   y*/
static inline bool
is_01_parent(struct ccc_frm_ const *const t, size_t const x,
             size_t const p_of_xy, size_t const y)
{
    assert(p_of_xy);
    return (!parity(t, x) && !parity(t, p_of_xy) && parity(t, y))
           || (parity(t, x) && parity(t, p_of_xy) && !parity(t, y));
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
       1/ \1
       x   y*/
static inline bool
is_11_parent(struct ccc_frm_ const *const t, size_t const x,
             size_t const p_of_xy, size_t const y)
{
    assert(p_of_xy);
    return (!parity(t, x) && parity(t, p_of_xy) && !parity(t, y))
           || (parity(t, x) && !parity(t, p_of_xy) && parity(t, y));
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
       2/ \0
       x   y*/
static inline bool
is_02_parent(struct ccc_frm_ const *const t, size_t const x,
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
       2/ \2
       x   y*/
static inline bool
is_22_parent(struct ccc_frm_ const *const t, size_t const x,
             size_t const p_of_xy, size_t const y)
{
    assert(p_of_xy);
    return (parity(t, x) == parity(t, p_of_xy))
           && (parity(t, p_of_xy) == parity(t, y));
}

static inline void
promote(struct ccc_frm_ const *const t, size_t const x)
{
    if (x)
    {
        at(t, x)->parity_ = !parity(t, x);
    }
}

static inline void
demote(struct ccc_frm_ const *const t, size_t const x)
{
    promote(t, x);
}

static inline void
double_promote([[maybe_unused]] struct ccc_frm_ const *const t,
               [[maybe_unused]] size_t const x)
{}

static inline void
double_demote([[maybe_unused]] struct ccc_frm_ const *const t,
              [[maybe_unused]] size_t const x)
{}

static inline bool
is_leaf(struct ccc_frm_ const *const t, size_t const x)
{
    return !branch_i(t, x, L) && !branch_i(t, x, R);
}

static inline size_t
sibling_of(struct ccc_frm_ const *const t, size_t const x)
{
    assert(parent_i(t, x));
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return at(t, parent_i(t, x))->branch_[branch_i(t, parent_i(t, x), L) == x];
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

struct tree_range
{
    size_t low;
    size_t root;
    size_t high;
};

struct parent_status
{
    bool correct;
    size_t parent;
};

static size_t
recursive_size(struct ccc_frm_ const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_size(t, branch_i(t, r, R))
           + recursive_size(t, branch_i(t, r, L));
}

static bool
are_subtrees_valid(struct ccc_frm_ const *t, struct tree_range const r)
{
    if (!r.root)
    {
        return true;
    }
    if (r.low
        && cmp_elems(t, ccc_impl_frm_key_from_node(t, at(t, r.low)), r.root,
                     t->cmp_)
               != CCC_LES)
    {
        return false;
    }
    if (r.high
        && cmp_elems(t, ccc_impl_frm_key_from_node(t, at(t, r.high)), r.root,
                     t->cmp_)
               != CCC_GRT)
    {
        return false;
    }
    return are_subtrees_valid(
               t, (struct tree_range){.low = r.low,
                                      .root = branch_i(t, r.root, L),
                                      .high = r.root})
           && are_subtrees_valid(
               t, (struct tree_range){.low = r.root,
                                      .root = branch_i(t, r.root, R),
                                      .high = r.high});
}

static bool
is_storing_parent(struct ccc_frm_ const *const t, size_t const p,
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
validate(struct ccc_frm_ const *const frm)
{
    if (!at(frm, 0)->parity_)
    {
        return false;
    }
    if (!are_subtrees_valid(frm, (struct tree_range){.root = frm->root_}))
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

static void
print_node(struct ccc_frm_ const *const t, size_t const root,
           ccc_print_fn *const fn_print)
{
    printf("%s%u%s:", COLOR_CYN, at(t, root)->parity_, COLOR_NIL);
    fn_print(base_at(t, root));
    printf("\n");
}

static void
print_inner_tree(size_t const root, char const *const prefix,
                 enum frm_print_branch_ const node_type,
                 enum frm_branch_ const dir, struct ccc_frm_ const *const t,
                 ccc_print_fn *const fn_print)
{
    if (!root)
    {
        return;
    }
    printf("%s", prefix);
    printf("%s%s", node_type == LEAF ? " └──" : " ├──", COLOR_NIL);
    printf(COLOR_CYN);
    dir == L ? printf("L" COLOR_NIL) : printf("R" COLOR_NIL);
    print_node(t, root, fn_print);

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

    if (!branch_i(t, root, R))
    {
        print_inner_tree(branch_i(t, root, L), str, LEAF, L, t, fn_print);
    }
    else if (!branch_i(t, root, L))
    {
        print_inner_tree(branch_i(t, root, R), str, LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(branch_i(t, root, R), str, BRANCH, R, t, fn_print);
        print_inner_tree(branch_i(t, root, L), str, LEAF, L, t, fn_print);
    }
    free(str);
}

static void
ccc_tree_print(struct ccc_frm_ const *const t, size_t const root,
               ccc_print_fn *const fn_print)
{
    if (!root)
    {
        return;
    }
    print_node(t, root, fn_print);

    if (!branch_i(t, root, R))
    {
        print_inner_tree(branch_i(t, root, L), "", LEAF, L, t, fn_print);
    }
    else if (!branch_i(t, root, L))
    {
        print_inner_tree(branch_i(t, root, R), "", LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(branch_i(t, root, R), "", BRANCH, R, t, fn_print);
        print_inner_tree(branch_i(t, root, L), "", LEAF, L, t, fn_print);
    }
}

/* NOLINTEND(*misc-no-recursion) */