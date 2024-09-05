#include "realtime_ordered_map.h"
#include "types.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

enum tree_link
{
    L = 0,
    R,
};

struct query
{
    ccc_threeway_cmp dir;
    union {
        struct ccc_impl_r_om_elem *found;
        struct ccc_impl_r_om_elem *parent;
    };
};

static void init_node(struct ccc_impl_realtime_ordered_map *,
                      struct ccc_impl_r_om_elem *);
static ccc_threeway_cmp cmp(struct ccc_impl_realtime_ordered_map const *,
                            void const *key,
                            struct ccc_impl_r_om_elem const *node,
                            ccc_key_cmp_fn *);
static void *struct_base(struct ccc_impl_realtime_ordered_map const *,
                         struct ccc_impl_r_om_elem const *);
static struct query find(struct ccc_impl_realtime_ordered_map *,
                         void const *key);
static void swap(uint8_t tmp[], void *a, void *b, size_t elem_sz);
static void *maybe_alloc_insert(struct ccc_impl_realtime_ordered_map *,
                                struct query *, struct ccc_impl_r_om_elem *);
static void insert_fixup(struct ccc_impl_realtime_ordered_map *,
                         struct ccc_impl_r_om_elem *parent,
                         struct ccc_impl_r_om_elem *x);
static bool is_0_1(uint8_t x_parity, uint8_t parent_parity,
                   uint8_t sibling_parity);
static bool is_0_2(uint8_t x_parity, uint8_t parent_parity,
                   uint8_t sibling_parity);
static struct ccc_impl_r_om_elem *
sibling(struct ccc_impl_r_om_elem const *parent,
        struct ccc_impl_r_om_elem const *x);
static void promote(struct ccc_impl_realtime_ordered_map const *rom,
                    struct ccc_impl_r_om_elem *x);
static void demote(struct ccc_impl_realtime_ordered_map const *rom,
                   struct ccc_impl_r_om_elem *x);

static void rotate(struct ccc_impl_realtime_ordered_map *rom,
                   struct ccc_impl_r_om_elem *parent,
                   struct ccc_impl_r_om_elem *x, struct ccc_impl_r_om_elem *y,
                   enum tree_link dir);
static bool validate(struct ccc_impl_realtime_ordered_map const *rom);

ccc_r_om_entry
ccc_rom_insert(ccc_realtime_ordered_map *const rom,
               ccc_r_om_elem *const out_handle)
{
    struct query q = find(
        &rom->impl, ccc_impl_rom_key_from_node(&rom->impl, &out_handle->impl));
    if (CCC_EQL == q.dir)
    {
        *out_handle = *(ccc_r_om_elem *)q.found;
        uint8_t tmp[rom->impl.elem_sz];
        void *user_struct = struct_base(&rom->impl, &out_handle->impl);
        void *ret = struct_base(&rom->impl, q.found);
        swap(tmp, user_struct, ret, rom->impl.elem_sz);
        return (ccc_r_om_entry) {{
            .rom = &rom->impl,
            .entry = {
                .entry = ret,
                .status = CCC_ROM_ENTRY_OCCUPIED,
            },
        }};
    }
    void *inserted = maybe_alloc_insert(&rom->impl, &q, &out_handle->impl);
    if (!inserted)
    {
        return (ccc_r_om_entry){{
            .rom = &rom->impl,
            .entry = {
                .entry = NULL, 
                .status = CCC_ROM_ENTRY_NULL | CCC_ROM_ENTRY_INSERT_ERROR,
            },
        }};
    }
    return (ccc_r_om_entry){{
        .rom = &rom->impl,
        .entry = {
            .entry = NULL, 
            .status = CCC_ROM_ENTRY_VACANT | CCC_ROM_ENTRY_NULL,
        },
    }};
}

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

void *
ccc_rom_begin(ccc_realtime_ordered_map const *rom)
{}

void *
ccc_rom_next(ccc_realtime_ordered_map *rom, ccc_r_om_elem const *)
{}

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

/*=========================   Static Helpers   ============================*/

static void *
maybe_alloc_insert(struct ccc_impl_realtime_ordered_map *const rom,
                   struct query *const q, struct ccc_impl_r_om_elem *out_handle)
{
    init_node(rom, out_handle);
    if (!rom->sz)
    {
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
        rom->root = out_handle;
        ++rom->sz;
        return struct_base(rom, out_handle);
    }
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
    q->parent->link[CCC_GRT == q->dir] = out_handle;
    out_handle->parent = q->parent;
    if (q->parent->link[L] == &rom->end && q->parent->link[R] == &rom->end)
    {
        insert_fixup(rom, q->parent, out_handle);
    }
    ++rom->sz;
    return struct_base(rom, out_handle);
}

static void
insert_fixup(struct ccc_impl_realtime_ordered_map *const rom,
             struct ccc_impl_r_om_elem *parent, struct ccc_impl_r_om_elem *x)
{
    for (; parent != &rom->end
           && is_0_1(x->parity, parent->parity, sibling(parent, x)->parity);
         x = parent, parent = parent->parent)
    {
        promote(rom, parent);
    }
    if (!is_0_2(x->parity, parent->parity, sibling(parent, x)->parity))
    {
        return;
    }
    enum tree_link const parent_to_x = parent->link[R] == x;
    struct ccc_impl_r_om_elem *y = x->link[!parent_to_x];
    if (y->parity == x->parity)
    {
        rotate(rom, parent, x, y, !parent_to_x);
        demote(rom, parent);
    }
    else
    {
        rotate(rom, x, y, y->link[!parent_to_x], parent_to_x);
        rotate(rom, y->parent, y, y->link[parent_to_x], !parent_to_x);
        promote(rom, y);
        demote(rom, x);
        demote(rom, parent);
    }
}

static struct query
find(struct ccc_impl_realtime_ordered_map *const rom, void const *const key)
{
    struct ccc_impl_r_om_elem *parent = &rom->end;
    struct ccc_impl_r_om_elem *seek = rom->root;
    ccc_threeway_cmp dir = CCC_CMP_ERR;
    while (seek != &rom->end)
    {
        dir = cmp(rom, key, seek, rom->cmp);
        if (CCC_EQL == dir)
        {
            return (struct query){CCC_EQL, {seek}};
        }
        parent = seek;
        seek = seek->link[CCC_GRT == dir];
    }
    return (struct query){dir, {parent}};
}

static inline void
rotate(struct ccc_impl_realtime_ordered_map *const rom,
       struct ccc_impl_r_om_elem *const parent,
       struct ccc_impl_r_om_elem *const x, struct ccc_impl_r_om_elem *const y,
       enum tree_link dir)
{
    struct ccc_impl_r_om_elem *const grandparent = parent->parent;
    x->parent = grandparent;
    if (grandparent == &rom->end)
    {
        rom->root = x;
    }
    grandparent->link[grandparent->link[R] == parent] = x;
    x->link[dir] = parent;
    parent->parent = x;
    parent->link[!dir] = y;
    y->parent = parent;
}

static inline bool
is_0_1(uint8_t x_parity, uint8_t parent_parity, uint8_t sibling_parity)
{
    return (!x_parity && !parent_parity && sibling_parity)
           || (x_parity && parent_parity && !sibling_parity);
}

static bool
is_0_2(uint8_t x_parity, uint8_t parent_parity, uint8_t sibling_parity)
{
    return (x_parity && parent_parity && sibling_parity)
           || (!x_parity && !parent_parity && !sibling_parity);
}

static inline struct ccc_impl_r_om_elem *
sibling(struct ccc_impl_r_om_elem const *parent,
        struct ccc_impl_r_om_elem const *x)
{
    /* Gives the opposite result, aka the sibling. */
    return parent->link[parent->link[L] == x];
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

static inline void
init_node(struct ccc_impl_realtime_ordered_map *const rom,
          struct ccc_impl_r_om_elem *const e)
{
    assert(e != NULL);
    assert(rom != NULL);
    e->link[L] = e->link[R] = e->parent = &rom->end;
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

/* NOLINTEND(*misc-no-recursion) */
