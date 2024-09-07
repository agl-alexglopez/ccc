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
        ccc_rtom_elem_ *found;
        ccc_rtom_elem_ *parent;
    };
};

enum rtom_link const inorder_traversal = L;
enum rtom_link const reverse_inorder_traversal = R;

/*==============================  Prototypes   ==============================*/

static void init_node(struct ccc_rtom_ *, ccc_rtom_elem_ *);
static ccc_threeway_cmp cmp(struct ccc_rtom_ const *, void const *key,
                            ccc_rtom_elem_ const *node, ccc_key_cmp_fn *);
static void *struct_base(struct ccc_rtom_ const *, ccc_rtom_elem_ const *);
static struct rtom_query find(struct ccc_rtom_ const *, void const *key);
static void swap(uint8_t tmp[], void *a, void *b, size_t elem_sz);
static void *maybe_alloc_insert(struct ccc_rtom_ *, struct rtom_query,
                                ccc_rtom_elem_ *);
static void *remove_fixup(struct ccc_rtom_ *, ccc_rtom_elem_ *);
static void insert_fixup(struct ccc_rtom_ *, ccc_rtom_elem_ *z_p_of_x,
                         ccc_rtom_elem_ *x);
static void maintain_rank_rules(struct ccc_rtom_ *rom, bool two_child,
                                ccc_rtom_elem_ *p_of_xy, ccc_rtom_elem_ *x);
static void transplant(struct ccc_rtom_ *, ccc_rtom_elem_ *remove,
                       ccc_rtom_elem_ *replacement);
static void rebalance_3_child(struct ccc_rtom_ *rom, ccc_rtom_elem_ *p_of_x,
                              ccc_rtom_elem_ *x);
static void rebalance_via_rotation(struct ccc_rtom_ *rom,
                                   ccc_rtom_elem_ *z_p_of_xy, ccc_rtom_elem_ *x,
                                   ccc_rtom_elem_ *y);
static bool is_0_child(struct ccc_rtom_ const *, ccc_rtom_elem_ const *p_of_x,
                       ccc_rtom_elem_ const *x);
static bool is_1_child(struct ccc_rtom_ const *, ccc_rtom_elem_ const *p_of_x,
                       ccc_rtom_elem_ const *x);
static bool is_2_child(struct ccc_rtom_ const *, ccc_rtom_elem_ const *p_of_x,
                       ccc_rtom_elem_ const *x);
static bool is_3_child(struct ccc_rtom_ const *, ccc_rtom_elem_ const *p_of_x,
                       ccc_rtom_elem_ const *x);
static bool is_01_parent(struct ccc_rtom_ const *, ccc_rtom_elem_ const *x,
                         ccc_rtom_elem_ const *p_of_xy,
                         ccc_rtom_elem_ const *y);
static bool is_11_parent(struct ccc_rtom_ const *, ccc_rtom_elem_ const *x,
                         ccc_rtom_elem_ const *p_of_xy,
                         ccc_rtom_elem_ const *y);
static bool is_02_parent(struct ccc_rtom_ const *, ccc_rtom_elem_ const *x,
                         ccc_rtom_elem_ const *p_of_xy,
                         ccc_rtom_elem_ const *y);
static bool is_22_parent(struct ccc_rtom_ const *, ccc_rtom_elem_ const *x,
                         ccc_rtom_elem_ const *p_of_xy,
                         ccc_rtom_elem_ const *y);
static bool is_leaf(struct ccc_rtom_ const *rom, ccc_rtom_elem_ const *x);
static ccc_rtom_elem_ *sibling_of(struct ccc_rtom_ const *rom,
                                  ccc_rtom_elem_ const *x);
static void promote(struct ccc_rtom_ const *rom, ccc_rtom_elem_ *x);
static void demote(struct ccc_rtom_ const *rom, ccc_rtom_elem_ *x);
static void double_promote(struct ccc_rtom_ const *rom, ccc_rtom_elem_ *x);
static void double_demote(struct ccc_rtom_ const *rom, ccc_rtom_elem_ *x);

static void rotate(struct ccc_rtom_ *rom, ccc_rtom_elem_ *p_of_x,
                   ccc_rtom_elem_ *x_p_of_y, ccc_rtom_elem_ *y,
                   enum rtom_link dir);
static bool validate(struct ccc_rtom_ const *rom);
static void ccc_tree_print(struct ccc_rtom_ const *rom,
                           ccc_rtom_elem_ const *root, ccc_print_fn *fn_print);

static ccc_rtom_elem_ const *next(struct ccc_rtom_ *, ccc_rtom_elem_ const *,
                                  enum rtom_link);
static ccc_rtom_elem_ *min_from(struct ccc_rtom_ const *,
                                ccc_rtom_elem_ *start);

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
        uint8_t tmp[rom->impl.elem_sz];
        void *user_struct = struct_base(&rom->impl, &out_handle->impl);
        void *ret = struct_base(&rom->impl, q.found);
        swap(tmp, user_struct, ret, rom->impl.elem_sz);
        return (ccc_entry){
            .entry = ret,
            .status = CCC_ENTRY_OCCUPIED,
        };
    }
    void *inserted = maybe_alloc_insert(&rom->impl, q, &out_handle->impl);
    if (!inserted)
    {
        return (ccc_entry){
            .entry = NULL,
            .status = CCC_ENTRY_ERROR,
        };
    }
    return (ccc_entry){
        .entry = inserted,
        .status = CCC_ENTRY_VACANT,
    };
}

ccc_rtom_entry
ccc_rom_entry(ccc_realtime_ordered_map const *rom, void const *key)
{

    struct rtom_query q = find(&rom->impl, key);
    if (CCC_EQL == q.last_cmp)
    {
        return (ccc_rtom_entry){{
            .rom = (struct ccc_rtom_ *)&rom->impl,
            .last_cmp = q.last_cmp,
            .entry = {
                .entry = struct_base(&rom->impl, q.found), 
                .status = CCC_ROM_ENTRY_OCCUPIED,
            },
        }};
    }
    return (ccc_rtom_entry){{
        .rom = (struct ccc_rtom_ *)&rom->impl,
        .last_cmp = q.last_cmp,
        .entry = {
            .entry = struct_base(&rom->impl,q.parent), 
            .status = CCC_ROM_ENTRY_VACANT,
        },
    }};
}

void *
ccc_rom_insert_entry(ccc_rtom_entry e, ccc_rtom_elem *const elem)
{
    if (e.impl.entry.status == CCC_ROM_ENTRY_OCCUPIED)
    {
        elem->impl = *ccc_impl_rom_elem_in_slot(e.impl.rom, e.impl.entry.entry);
        memcpy((void *)e.impl.entry.entry, struct_base(e.impl.rom, &elem->impl),
               e.impl.rom->elem_sz);
        return (void *)e.impl.entry.entry;
    }
    return maybe_alloc_insert(e.impl.rom,
                              (struct rtom_query){
                                  .last_cmp = e.impl.last_cmp,
                                  .parent = ccc_impl_rom_elem_in_slot(
                                      e.impl.rom, (void *)e.impl.entry.entry),
                              },
                              &elem->impl);
}

bool
ccc_rom_remove_entry(ccc_rtom_entry e)
{
    if (e.impl.entry.status == CCC_ROM_ENTRY_OCCUPIED)
    {
        void *const erased = remove_fixup(
            e.impl.rom,
            ccc_impl_rom_elem_in_slot(e.impl.rom, e.impl.entry.entry));
        assert(erased);
        if (e.impl.rom->alloc)
        {
            e.impl.rom->alloc(erased, 0);
        }
        return true;
    }
    return false;
}

ccc_entry
ccc_rom_remove(ccc_realtime_ordered_map *const rom,
               ccc_rtom_elem *const out_handle)
{
    struct rtom_query q = find(
        &rom->impl, ccc_impl_rom_key_from_node(&rom->impl, &out_handle->impl));
    if (q.last_cmp != CCC_EQL)
    {
        return (ccc_entry){.entry = NULL, .status = CCC_ENTRY_VACANT};
    }
    void *const removed = remove_fixup(&rom->impl, q.found);
    if (rom->impl.alloc)
    {
        void *const user_struct = struct_base(&rom->impl, &out_handle->impl);
        memcpy(user_struct, removed, rom->impl.elem_sz);
        rom->impl.alloc(removed, 0);
        return (ccc_entry){.entry = user_struct, .status = CCC_ENTRY_OCCUPIED};
    }
    return (ccc_entry){.entry = removed, .status = CCC_ENTRY_OCCUPIED};
}

void const *
ccc_rom_unwrap(ccc_rtom_entry e)
{
    if (e.impl.entry.status & CCC_ROM_ENTRY_OCCUPIED)
    {
        return e.impl.entry.entry;
    }
    return NULL;
}

void *
ccc_rom_unwrap_mut(ccc_rtom_entry e)
{
    if (e.impl.entry.status & CCC_ROM_ENTRY_OCCUPIED)
    {
        return (void *)e.impl.entry.entry;
    }
    return NULL;
}

static ccc_rtom_elem_ *
min_from(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ *start)
{
    if (start == &rom->end)
    {
        return start;
    }
    for (; start->link[L] != &rom->end; start = start->link[L])
    {}
    return start;
}

void *
ccc_rom_begin(ccc_realtime_ordered_map const *rom)
{
    ccc_rtom_elem_ *m = min_from(&rom->impl, rom->impl.root);
    return m == &rom->impl.end ? NULL : struct_base(&rom->impl, m);
}

void *
ccc_rom_next(ccc_realtime_ordered_map *const rom, ccc_rtom_elem const *const e)
{
    ccc_rtom_elem_ const *const n
        = next(&rom->impl, &e->impl, inorder_traversal);
    if (n == &rom->impl.end)
    {
        return NULL;
    }
    return struct_base(&rom->impl, n);
}

size_t
ccc_rom_size(ccc_realtime_ordered_map const *const rom)
{
    return rom->impl.sz;
}

bool
ccc_rom_empty(ccc_realtime_ordered_map const *const rom)
{
    return !rom->impl.sz;
}

void *
ccc_rom_root(ccc_realtime_ordered_map const *rom)
{
    if (!rom || rom->impl.root == &rom->impl.end)
    {
        return NULL;
    }
    return struct_base(&rom->impl, rom->impl.root);
}

bool
ccc_rom_validate(ccc_realtime_ordered_map const *rom)
{
    return validate(&rom->impl);
}

void
ccc_rom_print(ccc_realtime_ordered_map const *const rom, ccc_print_fn *const fn)
{
    ccc_tree_print(&rom->impl, rom->impl.root, fn);
}

/*=========================   Private Interface  ============================*/

void *
ccc_impl_rom_key_from_node(struct ccc_rtom_ const *const rom,
                           ccc_rtom_elem_ const *const elem)
{
    return (uint8_t *)struct_base(rom, elem) + rom->key_offset;
}

ccc_rtom_elem_ *
ccc_impl_rom_elem_in_slot(struct ccc_rtom_ const *const rom,
                          void const *const slot)
{
    return (ccc_rtom_elem_ *)((uint8_t *)slot + rom->node_elem_offset);
}

/*=========================    Static Helpers    ============================*/

static void *
maybe_alloc_insert(struct ccc_rtom_ *const rom, struct rtom_query const q,
                   ccc_rtom_elem_ *out_handle)
{
    init_node(rom, out_handle);
    if (rom->alloc)
    {
        void *new = rom->alloc(NULL, rom->elem_sz);
        if (!new)
        {
            return NULL;
        }
        memcpy(new, struct_base(rom, out_handle), rom->elem_sz);
        out_handle = ccc_impl_rom_elem_in_slot(rom, new);
    }
    if (!rom->sz)
    {
        rom->root = out_handle;
        ++rom->sz;
        return struct_base(rom, out_handle);
    }
    assert(q.last_cmp == CCC_GRT || q.last_cmp == CCC_LES);
    bool const rank_rule_break
        = q.parent->link[L] == &rom->end && q.parent->link[R] == &rom->end;
    q.parent->link[CCC_GRT == q.last_cmp] = out_handle;
    out_handle->parent = q.parent;
    if (rank_rule_break)
    {
        insert_fixup(rom, q.parent, out_handle);
    }
    ++rom->sz;
    return struct_base(rom, out_handle);
}

static struct rtom_query
find(struct ccc_rtom_ const *const rom, void const *const key)
{
    ccc_rtom_elem_ const *parent = &rom->end;
    ccc_threeway_cmp link = CCC_CMP_ERR;
    for (ccc_rtom_elem_ const *seek = rom->root; seek != &rom->end;
         parent = seek, seek = seek->link[CCC_GRT == link])
    {
        link = cmp(rom, key, seek, rom->cmp);
        if (CCC_EQL == link)
        {
            return (struct rtom_query){
                .last_cmp = CCC_EQL,
                .found = (ccc_rtom_elem_ *)seek,
            };
        }
    }
    return (struct rtom_query){
        .last_cmp = link,
        .parent = (ccc_rtom_elem_ *)parent,
    };
}

static ccc_rtom_elem_ const *
next(struct ccc_rtom_ *const t, ccc_rtom_elem_ const *n,
     enum rtom_link traversal)
{
    if (!n || n == &t->end)
    {
        return &t->end;
    }
    assert(t->root->parent == &t->end);
    /* This setup helps when once all nodes are visited. When ascending up to
       the root, traversal stops because the sentinel has root as child in the
       traversal direction. Iterator steps up to sentinel and terminates. */
    t->end.link[traversal] = t->root;
    t->end.link[!traversal] = &t->end;
    /* The node is an internal one that has a subtree to explore first. */
    if (n->link[!traversal] != &t->end)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->link[!traversal]; n->link[traversal] != &t->end;
             n = n->link[traversal])
        {}
        return n;
    }
    /* A leaf. Now it is time to visit the closest parent not yet visited. */
    ccc_rtom_elem_ *p = n->parent;
    for (; p->link[!traversal] == n; n = p, p = p->parent)
    {}
    /* This is an internal node, for example the root, or the end sentinel. */
    return p;
}

static inline void
init_node(struct ccc_rtom_ *const rom, ccc_rtom_elem_ *const e)
{
    assert(e != NULL);
    assert(rom != NULL);
    e->link[L] = &rom->end;
    e->link[R] = &rom->end;
    e->parent = &rom->end;
    e->parity = 0;
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
    ccc_rtom_elem_ const *const node, ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){
        .key = key,
        .container = struct_base(rom, node),
        .aux = rom->aux,
    });
}

static void *
struct_base(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ const *const e)
{
    return ((uint8_t *)e->link) - rom->node_elem_offset;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct ccc_rtom_ *const rom, ccc_rtom_elem_ *z_p_of_xy,
             ccc_rtom_elem_ *x)
{
    do
    {
        promote(rom, z_p_of_xy);
        x = z_p_of_xy;
        z_p_of_xy = z_p_of_xy->parent;
        if (z_p_of_xy == &rom->end)
        {
            return;
        }
    } while (is_01_parent(rom, x, z_p_of_xy, sibling_of(rom, x)));

    if (!is_02_parent(rom, x, z_p_of_xy, sibling_of(rom, x)))
    {
        return;
    }
    assert(x != &rom->end);
    assert(is_0_child(rom, z_p_of_xy, x));
    enum rtom_link const p_to_x_dir = z_p_of_xy->link[R] == x;
    ccc_rtom_elem_ *y = x->link[!p_to_x_dir];
    if (y == &rom->end || is_2_child(rom, z_p_of_xy, y))
    {
        rotate(rom, z_p_of_xy, x, y, !p_to_x_dir);
        assert(x->link[!p_to_x_dir] == z_p_of_xy);
        assert(z_p_of_xy->link[p_to_x_dir] == y);
        demote(rom, z_p_of_xy);
    }
    else
    {
        assert(is_1_child(rom, z_p_of_xy, y));
        rotate(rom, x, y, y->link[p_to_x_dir], p_to_x_dir);
        rotate(rom, y->parent, y, y->link[!p_to_x_dir], !p_to_x_dir);
        assert(y->link[p_to_x_dir] == x);
        assert(y->link[!p_to_x_dir] == z_p_of_xy);
        promote(rom, y);
        demote(rom, x);
        demote(rom, z_p_of_xy);
    }
}

static void *
remove_fixup(struct ccc_rtom_ *const rom, ccc_rtom_elem_ *const remove)
{
    ccc_rtom_elem_ *y = NULL;
    ccc_rtom_elem_ *x = NULL;
    ccc_rtom_elem_ *p_of_xy = NULL;
    bool two_child = false;
    if (remove->link[L] == &rom->end || remove->link[R] == &rom->end)
    {
        y = remove;
        p_of_xy = y->parent;
        x = y->link[y->link[L] == &rom->end];
        x->parent = y->parent;
        if (p_of_xy == &rom->end)
        {
            rom->root = x;
        }
        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->link[p_of_xy->link[R] == y] = x;
    }
    else
    {
        y = min_from(rom, remove->link[R]);
        p_of_xy = y->parent;
        x = y->link[y->link[L] == &rom->end];
        x->parent = y->parent;

        /* Save if check and improve readability by assuming this is true. */
        assert(p_of_xy != &rom->end);

        two_child = is_2_child(rom, p_of_xy, y);
        p_of_xy->link[p_of_xy->link[R] == y] = x;
        transplant(rom, remove, y);
        if (remove == p_of_xy)
        {
            p_of_xy = y;
        }
    }

    if (p_of_xy != &rom->end)
    {
        maintain_rank_rules(rom, two_child, p_of_xy, x);
        assert(!is_leaf(rom, p_of_xy) || !p_of_xy->parity);
    }
    remove->link[L] = remove->link[R] = remove->parent = NULL;
    remove->parity = 0;
    --rom->sz;
    return struct_base(rom, remove);
}

static inline void
maintain_rank_rules(struct ccc_rtom_ *const rom, bool two_child,
                    ccc_rtom_elem_ *const p_of_xy, ccc_rtom_elem_ *const x)
{
    if (two_child)
    {
        assert(p_of_xy != &rom->end);
        rebalance_3_child(rom, p_of_xy, x);
    }
    else if (x == &rom->end && p_of_xy->link[L] == p_of_xy->link[R])
    {
        assert(p_of_xy != &rom->end);
        bool const demote_makes_3_child
            = is_2_child(rom, p_of_xy->parent, p_of_xy);
        demote(rom, p_of_xy);
        if (demote_makes_3_child)
        {
            rebalance_3_child(rom, p_of_xy->parent, p_of_xy);
        }
    }
    assert(!is_leaf(rom, p_of_xy) || !p_of_xy->parity);
}

static inline void
rebalance_3_child(struct ccc_rtom_ *const rom, ccc_rtom_elem_ *p_of_xy,
                  ccc_rtom_elem_ *x)
{
    assert(p_of_xy != &rom->end);
    bool made_3_child = false;
    do
    {
        ccc_rtom_elem_ *const p_of_p_of_x = p_of_xy->parent;
        ccc_rtom_elem_ *y = p_of_xy->link[p_of_xy->link[L] == x];
        made_3_child = is_2_child(rom, p_of_p_of_x, p_of_xy);
        if (is_2_child(rom, p_of_xy, y))
        {
            demote(rom, p_of_xy);
        }
        else if (is_22_parent(rom, y->link[L], y, y->link[R]))
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
    } while (p_of_xy != &rom->end && made_3_child);
}

static inline void
rebalance_via_rotation(struct ccc_rtom_ *const rom, ccc_rtom_elem_ *const z,
                       ccc_rtom_elem_ *const x, ccc_rtom_elem_ *const y)
{
    enum rtom_link const z_to_x_dir = z->link[R] == x;
    ccc_rtom_elem_ *const w = y->link[!z_to_x_dir];
    if (is_1_child(rom, y, w))
    {
        rotate(rom, z, y, y->link[z_to_x_dir], z_to_x_dir);
        assert(y->link[z_to_x_dir] == z);
        assert(y->link[!z_to_x_dir] == w);
        promote(rom, y);
        demote(rom, z);
        if (is_leaf(rom, z))
        {
            demote(rom, z);
        }
    }
    else /* w is a 2-child and v will be a 1-child. */
    {
        ccc_rtom_elem_ *const v = y->link[z_to_x_dir];
        assert(is_2_child(rom, y, w));
        assert(is_1_child(rom, y, v));
        rotate(rom, y, v, v->link[!z_to_x_dir], !z_to_x_dir);
        rotate(rom, v->parent, v, v->link[z_to_x_dir], z_to_x_dir);
        assert(v->link[z_to_x_dir] == z);
        assert(v->link[!z_to_x_dir] == y);
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
        if (!is_leaf(rom, z) && is_11_parent(rom, z->link[L], z, z->link[R]))
        {
            promote(rom, z);
        }
        else if (!is_leaf(rom, y)
                 && is_11_parent(rom, y->link[L], y, y->link[R]))
        {
            promote(rom, y);
        }
    }
}

static inline void
transplant(struct ccc_rtom_ *const rom, ccc_rtom_elem_ *const remove,
           ccc_rtom_elem_ *const replacement)
{
    assert(remove != &rom->end);
    assert(replacement != &rom->end);
    replacement->parent = remove->parent;
    if (remove->parent == &rom->end)
    {
        rom->root = replacement;
    }
    else
    {
        remove->parent->link[remove->parent->link[R] == remove] = replacement;
    }
    remove->link[R]->parent = replacement;
    remove->link[L]->parent = replacement;
    replacement->link[R] = remove->link[R];
    replacement->link[L] = remove->link[L];
    replacement->parity = remove->parity;
}

static inline void
rotate(struct ccc_rtom_ *const rom, ccc_rtom_elem_ *const p_of_x,
       ccc_rtom_elem_ *const x_p_of_y, ccc_rtom_elem_ *const y,
       enum rtom_link dir)
{
    ccc_rtom_elem_ *const p_of_p_of_x = p_of_x->parent;
    x_p_of_y->parent = p_of_p_of_x;
    if (p_of_p_of_x == &rom->end)
    {
        rom->root = x_p_of_y;
    }
    else
    {
        p_of_p_of_x->link[p_of_p_of_x->link[R] == p_of_x] = x_p_of_y;
    }
    x_p_of_y->link[dir] = p_of_x;
    p_of_x->parent = x_p_of_y;
    p_of_x->link[!dir] = y;
    y->parent = p_of_x;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
       0/
       x*/
[[maybe_unused]] static inline bool
is_0_child(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ const *p_of_x,
           ccc_rtom_elem_ const *x)
{
    return p_of_x != &rom->end && p_of_x->parity == x->parity;
}

/* Returns true for rank difference 1 between the parent and node.
         p
       1/
       x*/
static inline bool
is_1_child(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ const *p_of_x,
           ccc_rtom_elem_ const *x)
{
    return p_of_x != &rom->end && p_of_x->parity != x->parity;
}

/* Returns true for rank difference 2 between the parent and node.
         p
       2/
       x*/
static inline bool
is_2_child(struct ccc_rtom_ const *const rom,
           ccc_rtom_elem_ const *const p_of_x, ccc_rtom_elem_ const *const x)
{
    return p_of_x != &rom->end && p_of_x->parity == x->parity;
}

/* Returns true for rank difference 3 between the parent and node.
         p
       3/
       x*/
[[maybe_unused]] static inline bool
is_3_child(struct ccc_rtom_ const *const rom,
           ccc_rtom_elem_ const *const p_of_x, ccc_rtom_elem_ const *const x)
{
    return p_of_x != &rom->end && p_of_x->parity != x->parity;
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
       0/ \1
       x   y*/
static inline bool
is_01_parent([[maybe_unused]] struct ccc_rtom_ const *const rom,
             ccc_rtom_elem_ const *const x, ccc_rtom_elem_ const *const p_of_xy,
             ccc_rtom_elem_ const *const y)
{
    assert(p_of_xy != &rom->end);
    return (!x->parity && !p_of_xy->parity && y->parity)
           || (x->parity && p_of_xy->parity && !y->parity);
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
       1/ \1
       x   y*/
static inline bool
is_11_parent([[maybe_unused]] struct ccc_rtom_ const *const rom,
             ccc_rtom_elem_ const *const x, ccc_rtom_elem_ const *const p_of_xy,
             ccc_rtom_elem_ const *const y)
{
    assert(p_of_xy != &rom->end);
    return (!x->parity && p_of_xy->parity && !y->parity)
           || (x->parity && !p_of_xy->parity && y->parity);
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
       2/ \0
       x   y*/
static inline bool
is_02_parent([[maybe_unused]] struct ccc_rtom_ const *const rom,
             ccc_rtom_elem_ const *const x, ccc_rtom_elem_ const *const p_of_xy,
             ccc_rtom_elem_ const *const y)
{
    assert(p_of_xy != &rom->end);
    return (x->parity == p_of_xy->parity) && (p_of_xy->parity == y->parity);
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
             ccc_rtom_elem_ const *const x, ccc_rtom_elem_ const *const p_of_xy,
             ccc_rtom_elem_ const *const y)
{
    assert(p_of_xy != &rom->end);
    return (x->parity == p_of_xy->parity) && (p_of_xy->parity == y->parity);
}

static inline void
promote(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ *const x)
{
    if (x != &rom->end)
    {
        x->parity = !x->parity;
    }
}

static inline void
demote(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ *const x)
{
    promote(rom, x);
}

static inline void
double_promote([[maybe_unused]] struct ccc_rtom_ const *const rom,
               [[maybe_unused]] ccc_rtom_elem_ *const x)
{}

static inline void
double_demote([[maybe_unused]] struct ccc_rtom_ const *const rom,
              [[maybe_unused]] ccc_rtom_elem_ *const x)
{}

static inline bool
is_leaf(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ const *const x)
{
    return x->link[L] == &rom->end && x->link[R] == &rom->end;
}

static inline ccc_rtom_elem_ *
sibling_of([[maybe_unused]] struct ccc_rtom_ const *const rom,
           ccc_rtom_elem_ const *const x)
{
    assert(x->parent != &rom->end);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return x->parent->link[x->parent->link[L] == x];
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

struct ccc_tree_range
{
    ccc_rtom_elem_ const *low;
    ccc_rtom_elem_ const *root;
    ccc_rtom_elem_ const *high;
};

struct parent_status
{
    bool correct;
    ccc_rtom_elem_ const *parent;
};

static size_t
recursive_size(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ const *const r)
{
    if (r == &rom->end)
    {
        return 0;
    }
    return 1 + recursive_size(rom, r->link[R])
           + recursive_size(rom, r->link[L]);
}

static bool
are_subtrees_valid(struct ccc_rtom_ const *t, struct ccc_tree_range const r,
                   ccc_rtom_elem_ const *const nil)
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
        && cmp(t, ccc_impl_rom_key_from_node(t, r.low), r.root, t->cmp)
               != CCC_LES)
    {
        return false;
    }
    if (r.high != nil
        && cmp(t, ccc_impl_rom_key_from_node(t, r.high), r.root, t->cmp)
               != CCC_GRT)
    {
        return false;
    }
    return are_subtrees_valid(t,
                              (struct ccc_tree_range){
                                  .low = r.low,
                                  .root = r.root->link[L],
                                  .high = r.root,
                              },
                              nil)
           && are_subtrees_valid(t,
                                 (struct ccc_tree_range){
                                     .low = r.root,
                                     .root = r.root->link[R],
                                     .high = r.high,
                                 },
                                 nil);
}

static bool
is_storing_parent(struct ccc_rtom_ const *const t,
                  ccc_rtom_elem_ const *const parent,
                  ccc_rtom_elem_ const *const root)
{
    if (root == &t->end)
    {
        return true;
    }
    if (root->parent != parent)
    {
        return false;
    }
    return is_storing_parent(t, root, root->link[L])
           && is_storing_parent(t, root, root->link[R]);
}

static bool
validate(struct ccc_rtom_ const *const rom)
{
    if (!rom->end.parity)
    {
        return false;
    }
    if (!are_subtrees_valid(rom,
                            (struct ccc_tree_range){
                                .low = &rom->end,
                                .root = rom->root,
                                .high = &rom->end,
                            },
                            &rom->end))
    {
        return false;
    }
    if (recursive_size(rom, rom->root) != rom->sz)
    {
        return false;
    }
    if (!is_storing_parent(rom, &rom->end, rom->root))
    {
        return false;
    }
    return true;
}

static void
print_node(struct ccc_rtom_ const *const rom, ccc_rtom_elem_ const *const root,
           ccc_print_fn *const fn_print)
{
    printf("%s%u%s:", COLOR_CYN, root->parity, COLOR_NIL);
    fn_print(struct_base(rom, root));
    printf("\n");
}

static void
print_inner_tree(ccc_rtom_elem_ const *const root, char const *const prefix,
                 enum rtom_print_link const node_type, enum rtom_link const dir,
                 struct ccc_rtom_ const *const rom,
                 ccc_print_fn *const fn_print)
{
    if (root == &rom->end)
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

    if (root->link[R] == &rom->end)
    {
        print_inner_tree(root->link[L], str, LEAF, L, rom, fn_print);
    }
    else if (root->link[L] == &rom->end)
    {
        print_inner_tree(root->link[R], str, LEAF, R, rom, fn_print);
    }
    else
    {
        print_inner_tree(root->link[R], str, BRANCH, R, rom, fn_print);
        print_inner_tree(root->link[L], str, LEAF, L, rom, fn_print);
    }
    free(str);
}

static void
ccc_tree_print(struct ccc_rtom_ const *const rom,
               ccc_rtom_elem_ const *const root, ccc_print_fn *const fn_print)
{
    if (root == &rom->end)
    {
        return;
    }
    print_node(rom, root, fn_print);

    if (root->link[R] == &rom->end)
    {
        print_inner_tree(root->link[L], "", LEAF, L, rom, fn_print);
    }
    else if (root->link[L] == &rom->end)
    {
        print_inner_tree(root->link[R], "", LEAF, R, rom, fn_print);
    }
    else
    {
        print_inner_tree(root->link[R], "", BRANCH, R, rom, fn_print);
        print_inner_tree(root->link[L], "", LEAF, L, rom, fn_print);
    }
}

/* NOLINTEND(*misc-no-recursion) */
