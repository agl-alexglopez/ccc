#include "realtime_ordered_map.h"
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

/* Because ranks are only tracked by odd/even parity. Double promoting or
   demoting does nothing and a macro won't produce any instructions if empty.
   However it is still good to insert these where the original research
   paper specifies they should occur in a WAVL tree for clarity. */
#define DOUBLE_PROMOTE(x) ((void)0)
#define DOUBLE_DEMOTE(x) DOUBLE_PROMOTE(x)

enum tree_link
{
    L = 0,
    R,
};

enum print_link
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
};

struct query
{
    ccc_threeway_cmp last_cmp;
    union {
        struct ccc_impl_r_om_elem *found;
        struct ccc_impl_r_om_elem *parent;
    };
};

enum tree_link const inorder_traversal = L;
enum tree_link const reverse_inorder_traversal = R;

/*==============================  Prototypes   ==============================*/

static void init_node(struct ccc_impl_realtime_ordered_map *,
                      struct ccc_impl_r_om_elem *);
static ccc_threeway_cmp cmp(struct ccc_impl_realtime_ordered_map const *,
                            void const *key,
                            struct ccc_impl_r_om_elem const *node,
                            ccc_key_cmp_fn *);
static void *struct_base(struct ccc_impl_realtime_ordered_map const *,
                         struct ccc_impl_r_om_elem const *);
static struct query find(struct ccc_impl_realtime_ordered_map const *,
                         void const *key);
static void swap(uint8_t tmp[], void *a, void *b, size_t elem_sz);
static void *maybe_alloc_insert(struct ccc_impl_realtime_ordered_map *,
                                struct query, struct ccc_impl_r_om_elem *);
static void *remove_fixup(struct ccc_impl_realtime_ordered_map *,
                          struct ccc_impl_r_om_elem *);
static void insert_fixup(struct ccc_impl_realtime_ordered_map *,
                         struct ccc_impl_r_om_elem *p_of_x,
                         struct ccc_impl_r_om_elem *x);
static void transplant(struct ccc_impl_realtime_ordered_map *,
                       struct ccc_impl_r_om_elem *remove,
                       struct ccc_impl_r_om_elem *replacement);
static void fixup_3_child(struct ccc_impl_realtime_ordered_map *rom,
                          struct ccc_impl_r_om_elem *p_of_x,
                          struct ccc_impl_r_om_elem *x);
static void fix_3_child_rank_rule(struct ccc_impl_realtime_ordered_map *rom,
                                  struct ccc_impl_r_om_elem *p_of_x,
                                  struct ccc_impl_r_om_elem *x,
                                  struct ccc_impl_r_om_elem *y);
static void fixup_22_leaf(struct ccc_impl_realtime_ordered_map *rom,
                          struct ccc_impl_r_om_elem *x);
static bool is_2_child(struct ccc_impl_r_om_elem const *p_of_x,
                       struct ccc_impl_r_om_elem const *x);
static bool is_parent_01(struct ccc_impl_r_om_elem *x,
                         struct ccc_impl_r_om_elem *p_of_xy,
                         struct ccc_impl_r_om_elem *y);
static bool is_parent_02(struct ccc_impl_r_om_elem *x,
                         struct ccc_impl_r_om_elem *p_of_xy,
                         struct ccc_impl_r_om_elem *y);
static bool is_leaf(struct ccc_impl_realtime_ordered_map const *rom,
                    struct ccc_impl_r_om_elem const *x);
static struct ccc_impl_r_om_elem *
sibling_of(struct ccc_impl_realtime_ordered_map const *rom,
           struct ccc_impl_r_om_elem const *x);
static void promote(struct ccc_impl_realtime_ordered_map const *rom,
                    struct ccc_impl_r_om_elem *x);
static void demote(struct ccc_impl_realtime_ordered_map const *rom,
                   struct ccc_impl_r_om_elem *x);

static void rotate(struct ccc_impl_realtime_ordered_map *rom,
                   struct ccc_impl_r_om_elem *p_of_x,
                   struct ccc_impl_r_om_elem *x_p_of_y,
                   struct ccc_impl_r_om_elem *y, enum tree_link dir);
static bool validate(struct ccc_impl_realtime_ordered_map const *rom);
static void ccc_tree_print(struct ccc_impl_realtime_ordered_map const *rom,
                           struct ccc_impl_r_om_elem const *root,
                           ccc_print_fn *fn_print);

static struct ccc_impl_r_om_elem const *
next(struct ccc_impl_realtime_ordered_map *, struct ccc_impl_r_om_elem const *,
     enum tree_link);
static struct ccc_impl_r_om_elem *
min_from(struct ccc_impl_realtime_ordered_map const *,
         struct ccc_impl_r_om_elem *start);

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
    struct query q = find(&rom->impl, key);
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

ccc_r_om_entry
ccc_rom_insert(ccc_realtime_ordered_map *const rom,
               ccc_r_om_elem *const out_handle)
{
    struct query q = find(
        &rom->impl, ccc_impl_rom_key_from_node(&rom->impl, &out_handle->impl));
    if (CCC_EQL == q.last_cmp)
    {
        *out_handle = *(ccc_r_om_elem *)q.found;
        uint8_t tmp[rom->impl.elem_sz];
        void *user_struct = struct_base(&rom->impl, &out_handle->impl);
        void *ret = struct_base(&rom->impl, q.found);
        swap(tmp, user_struct, ret, rom->impl.elem_sz);
        return (ccc_r_om_entry) {{
            .rom = &rom->impl,
            .last_cmp = q.last_cmp,
            .entry = {
                .entry = ret,
                .status = CCC_ROM_ENTRY_OCCUPIED,
            },
        }};
    }
    void *inserted = maybe_alloc_insert(&rom->impl, q, &out_handle->impl);
    if (!inserted)
    {
        return (ccc_r_om_entry){{
            .rom = &rom->impl,
            .last_cmp = CCC_CMP_ERR,
            .entry = {
                .entry = NULL, 
                .status = CCC_ROM_ENTRY_NULL | CCC_ROM_ENTRY_INSERT_ERROR,
            },
        }};
    }
    return (ccc_r_om_entry){{
        .rom = &rom->impl,
        .last_cmp = q.last_cmp,
        .entry = {
            .entry = NULL, 
            .status = CCC_ROM_ENTRY_VACANT | CCC_ROM_ENTRY_NULL,
        },
    }};
}

ccc_r_om_entry
ccc_rom_entry(ccc_realtime_ordered_map const *rom, void const *key)
{

    struct query q = find(&rom->impl, key);
    if (CCC_EQL == q.last_cmp)
    {
        return (ccc_r_om_entry){{
            .rom = (struct ccc_impl_realtime_ordered_map *)&rom->impl,
            .last_cmp = q.last_cmp,
            .entry = {
                .entry = struct_base(&rom->impl, q.found), 
                .status = CCC_ROM_ENTRY_OCCUPIED,
            },
        }};
    }
    return (ccc_r_om_entry){{
        .rom = (struct ccc_impl_realtime_ordered_map *)&rom->impl,
        .last_cmp = q.last_cmp,
        .entry = {
            .entry = struct_base(&rom->impl,q.parent), 
            .status = CCC_ROM_ENTRY_VACANT,
        },
    }};
}

void *
ccc_rom_insert_entry(ccc_r_om_entry e, ccc_r_om_elem *const elem)
{
    if (e.impl.entry.status == CCC_ROM_ENTRY_OCCUPIED)
    {
        elem->impl = *ccc_impl_rom_elem_in_slot(e.impl.rom, e.impl.entry.entry);
        memcpy((void *)e.impl.entry.entry, struct_base(e.impl.rom, &elem->impl),
               e.impl.rom->elem_sz);
        return (void *)e.impl.entry.entry;
    }
    return maybe_alloc_insert(e.impl.rom,
                              (struct query){
                                  .last_cmp = e.impl.last_cmp,
                                  .parent = ccc_impl_rom_elem_in_slot(
                                      e.impl.rom, (void *)e.impl.entry.entry),
                              },
                              &elem->impl);
}

bool
ccc_rom_remove_entry(ccc_r_om_entry e)
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

void *
ccc_rom_remove(ccc_realtime_ordered_map *const rom,
               ccc_r_om_elem *const out_handle)
{
    struct query q = find(
        &rom->impl, ccc_impl_rom_key_from_node(&rom->impl, &out_handle->impl));
    if (q.last_cmp != CCC_EQL)
    {
        return NULL;
    }
    void *const removed = remove_fixup(&rom->impl, q.found);
    if (rom->impl.alloc)
    {
        void *const user_struct = struct_base(&rom->impl, &out_handle->impl);
        memcpy(user_struct, removed, rom->impl.elem_sz);
        rom->impl.alloc(removed, 0);
        return user_struct;
    }
    return removed;
}

void const *
ccc_rom_unwrap(ccc_r_om_entry e)
{
    if (e.impl.entry.status & CCC_ROM_ENTRY_OCCUPIED)
    {
        return e.impl.entry.entry;
    }
    return NULL;
}

void *
ccc_rom_unwrap_mut(ccc_r_om_entry e)
{
    if (e.impl.entry.status & CCC_ROM_ENTRY_OCCUPIED)
    {
        return (void *)e.impl.entry.entry;
    }
    return NULL;
}

static struct ccc_impl_r_om_elem *
min_from(struct ccc_impl_realtime_ordered_map const *const rom,
         struct ccc_impl_r_om_elem *start)
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
    struct ccc_impl_r_om_elem *m = min_from(&rom->impl, rom->impl.root);
    return m == &rom->impl.end ? NULL : struct_base(&rom->impl, m);
}

void *
ccc_rom_next(ccc_realtime_ordered_map *const rom, ccc_r_om_elem const *const e)
{
    struct ccc_impl_r_om_elem const *n
        = next(&rom->impl, &e->impl, inorder_traversal);
    if (!n)
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
ccc_impl_rom_key_from_node(
    struct ccc_impl_realtime_ordered_map const *const rom,
    struct ccc_impl_r_om_elem const *const elem)
{
    return (uint8_t *)struct_base(rom, elem) + rom->key_offset;
}

struct ccc_impl_r_om_elem *
ccc_impl_rom_elem_in_slot(struct ccc_impl_realtime_ordered_map const *const rom,
                          void const *const slot)
{
    return (struct ccc_impl_r_om_elem *)((uint8_t *)slot
                                         + rom->node_elem_offset);
}

/*=========================    Static Helpers    ============================*/

static void *
maybe_alloc_insert(struct ccc_impl_realtime_ordered_map *const rom,
                   struct query const q, struct ccc_impl_r_om_elem *out_handle)
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

static struct query
find(struct ccc_impl_realtime_ordered_map const *const rom,
     void const *const key)
{
    struct ccc_impl_r_om_elem const *parent = &rom->end;
    struct ccc_impl_r_om_elem const *seek = rom->root;
    ccc_threeway_cmp link = CCC_CMP_ERR;
    while (seek != &rom->end)
    {
        link = cmp(rom, key, seek, rom->cmp);
        if (CCC_EQL == link)
        {
            return (struct query){
                .last_cmp = CCC_EQL,
                .found = (struct ccc_impl_r_om_elem *)seek,
            };
        }
        parent = seek;
        seek = seek->link[CCC_GRT == link];
    }
    return (struct query){
        .last_cmp = link,
        .parent = (struct ccc_impl_r_om_elem *)parent,
    };
}

static struct ccc_impl_r_om_elem const *
next(struct ccc_impl_realtime_ordered_map *const t,
     struct ccc_impl_r_om_elem const *n, enum tree_link traversal)
{
    if (!n || n == &t->end)
    {
        return NULL;
    }
    assert(t->root->parent == &t->end);
    /* Using a helper node simplifies the code greatly. */
    t->end.link[traversal] = t->root;
    t->end.link[!traversal] = &t->end;
    /* The node is a parent, backtracked to, or the end. */
    if (n->link[!traversal] != &t->end)
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = n->link[!traversal]; n->link[traversal] != &t->end;
             n = n->link[traversal])
        {}
        return n;
    }
    /* A leaf. Work our way back up skpping nodes we already visited. */
    struct ccc_impl_r_om_elem *p = n->parent;
    for (; p->link[!traversal] == n; n = p, p = p->parent)
    {}
    /* This is where the end node is helpful. We get to it eventually. */
    return p == &t->end ? NULL : p;
}

static inline void
init_node(struct ccc_impl_realtime_ordered_map *const rom,
          struct ccc_impl_r_om_elem *const e)
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
cmp(struct ccc_impl_realtime_ordered_map const *const rom,
    void const *const key, struct ccc_impl_r_om_elem const *const node,
    ccc_key_cmp_fn *const fn)
{
    return fn((ccc_key_cmp){
        .key = key,
        .container = struct_base(rom, node),
        .aux = rom->aux,
    });
}

static void *
struct_base(struct ccc_impl_realtime_ordered_map const *const rom,
            struct ccc_impl_r_om_elem const *const e)
{
    return ((uint8_t *)e->link) - rom->node_elem_offset;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct ccc_impl_realtime_ordered_map *const rom,
             struct ccc_impl_r_om_elem *p_of_x, struct ccc_impl_r_om_elem *x)
{
    do
    {
        promote(rom, p_of_x);
        x = p_of_x;
        p_of_x = p_of_x->parent;
        if (p_of_x == &rom->end)
        {
            return;
        }
    } while (is_parent_01(x, p_of_x, sibling_of(rom, x)));

    if (!is_parent_02(x, p_of_x, sibling_of(rom, x)))
    {
        return;
    }
    enum tree_link const p_to_x = p_of_x->link[R] == x;
    struct ccc_impl_r_om_elem *y = x->link[!p_to_x];
    if (y->parity == x->parity)
    {
        rotate(rom, p_of_x, x, y, !p_to_x);
        demote(rom, p_of_x);
    }
    else
    {
        rotate(rom, x, y, y->link[p_to_x], p_to_x);
        rotate(rom, y->parent, y, y->link[!p_to_x], !p_to_x);
        promote(rom, y);
        demote(rom, x);
        demote(rom, p_of_x);
    }
}

static void *
remove_fixup(struct ccc_impl_realtime_ordered_map *const rom,
             struct ccc_impl_r_om_elem *const remove)
{
    struct ccc_impl_r_om_elem *y = NULL;
    struct ccc_impl_r_om_elem *x = NULL;
    struct ccc_impl_r_om_elem *p_of_x = NULL;
    bool two_child = false;
    if (remove->link[L] == &rom->end || remove->link[R] == &rom->end)
    {
        y = remove;
        p_of_x = y->parent;
        x = y->link[y->link[L] == &rom->end];
        x->parent = y->parent;
        if (p_of_x == &rom->end)
        {
            rom->root = x;
        }
        else
        {
            two_child = is_2_child(p_of_x, y);
            p_of_x->link[p_of_x->link[R] == y] = x;
        }
    }
    else
    {
        y = min_from(rom, remove->link[R]);
        x = y->link[y->link[L] == &rom->end];
        x->parent = y->parent;
        p_of_x = y->parent;

        /* Save if check and improve readability by assuming this is true. */
        assert(p_of_x != &rom->end);

        two_child = is_2_child(p_of_x, y);
        p_of_x->link[p_of_x->link[R] == y] = x;
        transplant(rom, remove, y);
        if (remove == p_of_x)
        {
            p_of_x = y;
        }
    }

    if (p_of_x != &rom->end)
    {
        if (two_child)
        {
            fixup_3_child(rom, p_of_x, x);
        }
        else if (x == &rom->end && p_of_x->link[L] == p_of_x->link[R])
        {
            fixup_22_leaf(rom, p_of_x);
        }
        assert(!is_leaf(rom, p_of_x) || !p_of_x->parity);
    }
    remove->link[L] = remove->link[R] = remove->parent = NULL;
    remove->parity = 0;
    --rom->sz;
    return struct_base(rom, remove);
}

static inline void
fixup_22_leaf(struct ccc_impl_realtime_ordered_map *const rom,
              struct ccc_impl_r_om_elem *const x)
{
    if (x->parent->parity == x->parity)
    {
        demote(rom, x);
        fixup_3_child(rom, x->parent, x);
    }
    else
    {
        demote(rom, x);
    }
}

static inline void
fixup_3_child(struct ccc_impl_realtime_ordered_map *const rom,
              struct ccc_impl_r_om_elem *p_of_x, struct ccc_impl_r_om_elem *x)
{
    bool x_is_3_child = false;
    do
    {
        struct ccc_impl_r_om_elem *const grandparent = p_of_x->parent;
        struct ccc_impl_r_om_elem *y = sibling_of(rom, x);
        x_is_3_child = grandparent != &rom->end
                       && (p_of_x->parity == grandparent->parity);
        /* Sanity check for the second case. It would be undefined behavior
           to access a node stored in the end node as those are often written
           to invariantly to cut down on lines of code. */
        assert(is_2_child(p_of_x, y) || y != &rom->end);
        if (is_2_child(p_of_x, y))
        {
            demote(rom, p_of_x);
        }
        else if (y->parity == y->link[L]->parity
                 && y->parity == y->link[R]->parity)
        {
            demote(rom, p_of_x);
            demote(rom, y);
        }
        else
        {
            fix_3_child_rank_rule(rom, p_of_x, x, y);
            return;
        }
        x = p_of_x;
        p_of_x = p_of_x->parent;
    } while (p_of_x != &rom->end && x_is_3_child);
}

static inline void
fix_3_child_rank_rule(struct ccc_impl_realtime_ordered_map *const rom,
                      struct ccc_impl_r_om_elem *p_of_xy,
                      struct ccc_impl_r_om_elem *x,
                      struct ccc_impl_r_om_elem *y)
{
    enum tree_link const p_to_x_dir = p_of_xy->link[R] == x;
    struct ccc_impl_r_om_elem *const w = y->link[!p_to_x_dir];
    if (w->parity != y->parity)
    {
        rotate(rom, p_of_xy, y, y->link[p_to_x_dir], p_to_x_dir);
        promote(rom, y);
        demote(rom, p_of_xy);
        if (is_leaf(rom, p_of_xy))
        {
            demote(rom, p_of_xy);
        }
    }
    else
    {
        struct ccc_impl_r_om_elem *const v = y->link[p_to_x_dir];
        assert(y->parity != v->parity);
        rotate(rom, y, v, v->link[!p_to_x_dir], !p_to_x_dir);
        rotate(rom, v->parent, v, v->link[p_to_x_dir], p_to_x_dir);
        DOUBLE_PROMOTE(v);
        demote(rom, y);
        DOUBLE_DEMOTE(pxy);
    }
}

static inline void
transplant(struct ccc_impl_realtime_ordered_map *const rom,
           struct ccc_impl_r_om_elem *const remove,
           struct ccc_impl_r_om_elem *const replacement)
{
    assert(remove != &rom->end);
    replacement->parent = remove->parent;
    if (remove->parent == &rom->end)
    {
        rom->root = replacement;
    }
    else
    {
        remove->parent->link[remove->parent->link[R] == remove] = replacement;
    }
    replacement->link[R] = remove->link[R];
    remove->link[R]->parent = replacement;
    replacement->link[L] = remove->link[L];
    remove->link[L]->parent = replacement;
    replacement->parity = remove->parity;
}

static inline void
rotate(struct ccc_impl_realtime_ordered_map *const rom,
       struct ccc_impl_r_om_elem *const p_of_x,
       struct ccc_impl_r_om_elem *const x_p_of_y,
       struct ccc_impl_r_om_elem *const y, enum tree_link dir)
{
    struct ccc_impl_r_om_elem *const p_of_p_of_x = p_of_x->parent;
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

static inline bool
is_2_child(struct ccc_impl_r_om_elem const *const p_of_x,
           struct ccc_impl_r_om_elem const *const x)
{
    return p_of_x->parity == x->parity;
}

static inline bool
is_parent_01(struct ccc_impl_r_om_elem *x, struct ccc_impl_r_om_elem *p_of_xy,
             struct ccc_impl_r_om_elem *y)
{
    return (!x->parity && !p_of_xy->parity && y->parity)
           || (x->parity && p_of_xy->parity && !y->parity);
}

static inline bool
is_parent_02(struct ccc_impl_r_om_elem *x, struct ccc_impl_r_om_elem *p_of_xy,
             struct ccc_impl_r_om_elem *y)
{
    return (x->parity && p_of_xy->parity && y->parity)
           || (!x->parity && !p_of_xy->parity && !y->parity);
}

static inline struct ccc_impl_r_om_elem *
sibling_of(struct ccc_impl_realtime_ordered_map const *const rom,
           struct ccc_impl_r_om_elem const *const x)
{
    /* We want the sibling so we need the truthy value to be opposite of x. */
    if (x->parent == &rom->end)
    {
        return (struct ccc_impl_r_om_elem *)&rom->end;
    }
    return x->parent->link[x->parent->link[L] == x];
}

static inline void
promote(struct ccc_impl_realtime_ordered_map const *const rom,
        struct ccc_impl_r_om_elem *x)
{
    if (x != &rom->end)
    {
        x->parity = !x->parity;
    }
}

static inline void
demote(struct ccc_impl_realtime_ordered_map const *const rom,
       struct ccc_impl_r_om_elem *x)
{
    promote(rom, x);
}

static inline bool
is_leaf(struct ccc_impl_realtime_ordered_map const *const rom,
        struct ccc_impl_r_om_elem const *const x)
{
    return x->link[L] == &rom->end && x->link[R] == &rom->end;
}

/*===========================   Validation   ===============================*/
/* NOLINTBEGIN(*misc-no-recursion) */

struct ccc_tree_range
{
    struct ccc_impl_r_om_elem const *low;
    struct ccc_impl_r_om_elem const *root;
    struct ccc_impl_r_om_elem const *high;
};

struct parent_status
{
    bool correct;
    struct ccc_impl_r_om_elem const *parent;
};

static size_t
recursive_size(struct ccc_impl_realtime_ordered_map const *const rom,
               struct ccc_impl_r_om_elem const *const r)
{
    if (r == &rom->end)
    {
        return 0;
    }
    return 1 + recursive_size(rom, r->link[R])
           + recursive_size(rom, r->link[L]);
}

static bool
are_subtrees_valid(struct ccc_impl_realtime_ordered_map const *t,
                   struct ccc_tree_range const r,
                   struct ccc_impl_r_om_elem const *const nil)
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
is_storing_parent(struct ccc_impl_realtime_ordered_map const *const t,
                  struct ccc_impl_r_om_elem const *const parent,
                  struct ccc_impl_r_om_elem const *const root)
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
validate(struct ccc_impl_realtime_ordered_map const *const rom)
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
print_node(struct ccc_impl_realtime_ordered_map const *const rom,
           struct ccc_impl_r_om_elem const *const root,
           ccc_print_fn *const fn_print)
{
    printf("%s%u%s:", COLOR_CYN, root->parity, COLOR_NIL);
    fn_print(struct_base(rom, root));
    printf("\n");
}

static void
print_inner_tree(struct ccc_impl_r_om_elem const *const root,
                 char const *const prefix, enum print_link const node_type,
                 enum tree_link const dir,
                 struct ccc_impl_realtime_ordered_map const *const rom,
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
        /* NOLINTNEXTLINE */
        (void)snprintf(str, string_length, "%s%s", prefix,
                       node_type == LEAF ? "     " : " │   ");
    }

    assert(str);

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
ccc_tree_print(struct ccc_impl_realtime_ordered_map const *const rom,
               struct ccc_impl_r_om_elem const *const root,
               ccc_print_fn *const fn_print)
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
