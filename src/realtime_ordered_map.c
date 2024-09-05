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
    ccc_threeway_cmp dir;
    union {
        struct ccc_impl_r_om_elem *found;
        struct ccc_impl_r_om_elem *parent;
    };
};

enum tree_link const inorder_traversal = L;
enum tree_link const reverse_inorder_traversal = R;

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
static bool parent_01(uint8_t x_parity, uint8_t parent_parity,
                      uint8_t sibling_parity);
static bool parent_02(uint8_t x_parity, uint8_t parent_parity,
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
static void ccc_tree_print(struct ccc_impl_realtime_ordered_map const *rom,
                           struct ccc_impl_r_om_elem const *root,
                           ccc_print_fn *fn_print);

static struct ccc_impl_r_om_elem const *
next(struct ccc_impl_realtime_ordered_map *, struct ccc_impl_r_om_elem const *,
     enum tree_link);
void *min(struct ccc_impl_realtime_ordered_map const *);

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
min(struct ccc_impl_realtime_ordered_map const *const rom)
{
    if (!rom->sz)
    {
        return NULL;
    }
    struct ccc_impl_r_om_elem *n = rom->root;
    for (; n->link[L] != &rom->end; n = n->link[L])
    {}
    return struct_base(rom, n);
}

void *
ccc_rom_begin(ccc_realtime_ordered_map const *rom)
{
    return min(&rom->impl);
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
    bool const rank_rule_break
        = q->parent->link[L] == &rom->end && q->parent->link[R] == &rom->end;
    q->parent->link[CCC_GRT == q->dir] = out_handle;
    out_handle->parent = q->parent;
    if (rank_rule_break)
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
    do
    {
        promote(rom, parent);
        x = parent;
        parent = parent->parent;
        if (parent == &rom->end)
        {
            return;
        }
    } while (parent_01(x->parity, parent->parity, sibling(parent, x)->parity));

    if (!parent_02(x->parity, parent->parity, sibling(parent, x)->parity))
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
            return (struct query){CCC_EQL, .found = seek};
        }
        parent = seek;
        seek = seek->link[CCC_GRT == dir];
    }
    return (struct query){dir, .parent = parent};
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

struct ccc_impl_r_om_elem const *
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

static inline bool
parent_01(uint8_t x_parity, uint8_t parent_parity, uint8_t sibling_parity)
{
    return (!x_parity && !parent_parity && sibling_parity)
           || (x_parity && parent_parity && !sibling_parity);
}

static inline bool
parent_02(uint8_t x_parity, uint8_t parent_parity, uint8_t sibling_parity)
{
    return (x_parity && parent_parity && sibling_parity)
           || (!x_parity && !parent_parity && !sibling_parity);
}

static inline struct ccc_impl_r_om_elem *
sibling(struct ccc_impl_r_om_elem const *parent,
        struct ccc_impl_r_om_elem const *x)
{
    /* We want the sibling so we need the truthy value to be opposite of x. */
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
    if (str == NULL)
    {
        printf(COLOR_ERR "memory exceeded. Cannot display tree." COLOR_NIL);
        return;
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
