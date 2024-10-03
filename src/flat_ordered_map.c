#include "flat_ordered_map.h"
#include "impl_flat_ordered_map.h"

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

enum fom_branch_
{
    L = 0,
    R,
};

enum fom_print_branch_
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
};

static enum fom_branch_ const inorder_traversal = R;
static enum fom_branch_ const reverse_inorder_traversal = L;

static enum fom_branch_ const min = L;
static enum fom_branch_ const max = R;

static size_t const single_tree_node = 2;

/*==============================  Prototypes   ==============================*/

/* Returning the internal elem type with stored offsets. */
static size_t splay(struct ccc_fom_ *t, size_t root, void const *key,
                    ccc_key_cmp_fn *cmp_fn);
static struct ccc_fom_elem_ *at(struct ccc_fom_ const *, size_t);
static struct ccc_fom_elem_ *elem_in_slot(struct ccc_fom_ const *t,
                                          void const *slot);
/* Returning the user struct type with stored offsets. */
static void *maybe_alloc_insert(struct ccc_fom_ *fom,
                                struct ccc_fom_elem_ *elem);
static void *find(struct ccc_fom_ *, void const *key);
static void *connect_new_root(struct ccc_fom_ *t, size_t new_root,
                              ccc_threeway_cmp cmp_result);
static void *struct_base(struct ccc_fom_ const *, struct ccc_fom_elem_ const *);
static void *insert(struct ccc_fom_ *t, size_t n);
static void *base_at(struct ccc_fom_ const *, size_t);
static void *alloc_back(struct ccc_fom_ *t);
/* Returning the user key with stored offsets. */
static void *key_from_node(struct ccc_fom_ const *t,
                           struct ccc_fom_elem_ const *);
static void *key_at(struct ccc_fom_ const *t, size_t i);
/* Returning threeway comparison with user callback. */
static ccc_threeway_cmp cmp_elems(struct ccc_fom_ const *fom, void const *key,
                                  size_t node, ccc_key_cmp_fn *fn);
/* Returning read only indices for tree nodes. */
static size_t min_max_from(struct ccc_fom_ const *t, size_t start,
                           enum fom_branch_ dir);
static size_t next(struct ccc_fom_ const *t, size_t n,
                   enum fom_branch_ traversal);
static size_t branch_i(struct ccc_fom_ const *t, size_t parent,
                       enum fom_branch_ dir);
static size_t parent_i(struct ccc_fom_ const *t, size_t child);
static size_t index_of(struct ccc_fom_ const *t,
                       struct ccc_fom_elem_ const *elem);
/* Returning references to index fields for tree nodes. */
static size_t *branch_ref(struct ccc_fom_ const *t, size_t node,
                          enum fom_branch_ branch);
static size_t *parent_ref(struct ccc_fom_ const *t, size_t node);

static bool validate(struct ccc_fom_ const *fom);

/* Returning void as miscellaneous helpers. */
static void init_node(struct ccc_fom_elem_ *e);
static void swap(char tmp[], void *a, void *b, size_t elem_sz);
static void tree_print(struct ccc_fom_ const *t, size_t root,
                       ccc_print_fn *fn_print);
static void link_trees(struct ccc_fom_ *t, size_t parent, enum fom_branch_ dir,
                       size_t subtree);

/*==============================  Interface    ==============================*/

ccc_entry
ccc_fom_insert(ccc_flat_ordered_map *const t, ccc_f_om_elem *const out_handle,
               void *const tmp)
{
    void *found = find(t, key_from_node(t, out_handle));
    if (found)
    {
        assert(t->root_);
        *out_handle = *at(t, t->root_);
        void *user_struct = struct_base(t, out_handle);
        void *ret = base_at(t, t->root_);
        swap(tmp, user_struct, ret, ccc_buf_elem_size(&t->buf_));
        return (ccc_entry){{.e_ = ret, .stats_ = CCC_ENTRY_OCCUPIED}};
    }
    void *inserted = maybe_alloc_insert(t, out_handle);
    if (!inserted)
    {
        return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_INSERT_ERROR}};
    }
    return (ccc_entry){{.e_ = NULL, .stats_ = CCC_ENTRY_VACANT}};
}

bool
ccc_fom_empty(ccc_flat_ordered_map const *const fom)
{
    return !ccc_fom_size(fom);
}

size_t
ccc_fom_size(ccc_flat_ordered_map const *const fom)
{
    size_t const sz = ccc_buf_size(&fom->buf_);
    return !sz ? sz : sz - 1;
}

void *
ccc_fom_begin(ccc_flat_ordered_map const *const fom)
{
    if (ccc_buf_empty(&fom->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(fom, fom->root_, min);
    return base_at(fom, n);
}

void *
ccc_fom_rbegin(ccc_flat_ordered_map const *const fom)
{
    if (ccc_buf_empty(&fom->buf_))
    {
        return NULL;
    }
    size_t const n = min_max_from(fom, fom->root_, max);
    return base_at(fom, n);
}

void *
ccc_fom_next(ccc_flat_ordered_map const *const fom,
             ccc_f_om_elem const *const e)
{
    if (ccc_buf_empty(&fom->buf_))
    {
        return NULL;
    }
    size_t const n = next(fom, index_of(fom, e), inorder_traversal);
    return base_at(fom, n);
}

void *
ccc_fom_rnext(ccc_flat_ordered_map const *const fom,
              ccc_f_om_elem const *const e)
{
    if (ccc_buf_empty(&fom->buf_))
    {
        return NULL;
    }
    size_t const n = next(fom, index_of(fom, e), reverse_inorder_traversal);
    return base_at(fom, n);
}

void *
ccc_fom_end(ccc_flat_ordered_map const *const fom)
{
    if (ccc_buf_empty(&fom->buf_))
    {
        return NULL;
    }
    return base_at(fom, 0);
}

void *
ccc_fom_rend(ccc_flat_ordered_map const *const fom)
{
    if (ccc_buf_empty(&fom->buf_))
    {
        return NULL;
    }
    return base_at(fom, 0);
}

bool
ccc_fom_validate(ccc_flat_ordered_map const *const fom)
{
    return validate(fom);
}

void
ccc_fom_print(ccc_flat_ordered_map const *const fom, ccc_print_fn *const fn)
{
    tree_print(fom, fom->root_, fn);
}

/*===========================   Static Helpers    ===========================*/

static void *
maybe_alloc_insert(struct ccc_fom_ *const fom, struct ccc_fom_elem_ *const elem)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementor is always at least 1. */
    if (!alloc_back(fom))
    {
        return NULL;
    }
    size_t const node = ccc_buf_size(&fom->buf_) - 1;
    ccc_buf_write(&fom->buf_, node, struct_base(fom, elem));
    return insert(fom, node);
}

static inline void *
insert(struct ccc_fom_ *const t, size_t const n)
{
    struct ccc_fom_elem_ *const node = at(t, n);
    init_node(node);
    if (ccc_buf_size(&t->buf_) == single_tree_node)
    {
        t->root_ = n;
        return struct_base(t, node);
    }
    void const *const key = key_from_node(t, node);
    t->root_ = splay(t, t->root_, key, t->cmp_);
    ccc_threeway_cmp const root_cmp = cmp_elems(t, key, t->root_, t->cmp_);
    if (CCC_EQL == root_cmp)
    {
        return NULL;
    }
    return connect_new_root(t, n, root_cmp);
}

static inline void *
connect_new_root(struct ccc_fom_ *const t, size_t const new_root,
                 ccc_threeway_cmp const cmp_result)
{
    enum fom_branch_ const link = CCC_GRT == cmp_result;
    link_trees(t, new_root, link, branch_i(t, t->root_, link));
    link_trees(t, new_root, !link, t->root_);
    *branch_ref(t, t->root_, link) = 0;
    t->root_ = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link_trees(t, 0, 0, t->root_);
    return base_at(t, new_root);
}

static inline void *
find(struct ccc_fom_ *const t, void const *const key)
{
    if (!t->root_)
    {
        return NULL;
    }
    t->root_ = splay(t, t->root_, key, t->cmp_);
    return cmp_elems(t, key, t->root_, t->cmp_) == CCC_EQL
               ? base_at(t, t->root_)
               : NULL;
}

static inline size_t
splay(struct ccc_fom_ *const t, size_t root, void const *const key,
      ccc_key_cmp_fn *const cmp_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    struct ccc_fom_elem_ *const nil = at(t, 0);
    nil->branch_[L] = nil->branch_[R] = nil->parent_ = 0;
    size_t l_r_subtrees[LR] = {0, 0};
    for (;;)
    {
        ccc_threeway_cmp const root_cmp = cmp_elems(t, key, root, cmp_fn);
        enum fom_branch_ const dir = CCC_GRT == root_cmp;
        if (CCC_EQL == root_cmp || !branch_i(t, root, dir))
        {
            break;
        }
        ccc_threeway_cmp const child_cmp
            = cmp_elems(t, key, branch_i(t, root, dir), cmp_fn);
        enum fom_branch_ const dir_from_child = CCC_GRT == child_cmp;
        /* A straight line has formed from root->child->elem. An opportunity
           to splay and heal the tree arises. */
        if (CCC_EQL != child_cmp && dir == dir_from_child)
        {
            size_t const pivot = branch_i(t, root, dir);
            link_trees(t, root, dir, branch_i(t, pivot, !dir));
            link_trees(t, pivot, !dir, root);
            root = pivot;
            if (!branch_i(t, root, dir))
            {
                break;
            }
        }
        link_trees(t, l_r_subtrees[!dir], dir, root);
        l_r_subtrees[!dir] = root;
        root = branch_i(t, root, dir);
    }
    link_trees(t, l_r_subtrees[L], R, branch_i(t, root, L));
    link_trees(t, l_r_subtrees[R], L, branch_i(t, root, R));
    link_trees(t, root, L, nil->branch_[R]);
    link_trees(t, root, R, nil->branch_[L]);
    t->root_ = root;
    link_trees(t, 0, 0, t->root_);
    return root;
}

static inline void
link_trees(struct ccc_fom_ *const t, size_t const parent,
           enum fom_branch_ const dir, size_t const subtree)
{
    *branch_ref(t, parent, dir) = subtree;
    *parent_ref(t, subtree) = parent;
}

static inline size_t
min_max_from(struct ccc_fom_ const *const t, size_t start,
             enum fom_branch_ const dir)
{
    if (!start)
    {
        return 0;
    }
    for (; branch_i(t, start, dir); start = branch_i(t, start, dir))
    {}
    return start;
}

static inline size_t
next(struct ccc_fom_ const *const t, size_t n, enum fom_branch_ const traversal)
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

static inline ccc_threeway_cmp
cmp_elems(struct ccc_fom_ const *const fom, void const *const key,
          size_t const node, ccc_key_cmp_fn *const fn)
{
    return fn(&(ccc_key_cmp){
        .container = base_at(fom, node), .key = key, .aux = fom->aux_});
}

static inline void *
alloc_back(struct ccc_fom_ *const t)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementor is always at least 1. */
    if (ccc_buf_empty(&t->buf_))
    {
        void *const sentinel = ccc_buf_alloc_back(&t->buf_);
        if (!sentinel)
        {
            return NULL;
        }
    }
    return ccc_buf_alloc_back(&t->buf_);
}

static inline void
init_node(struct ccc_fom_elem_ *const e)
{
    assert(e != NULL);
    e->branch_[L] = e->branch_[R] = e->parent_ = 0;
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

static inline struct ccc_fom_elem_ *
at(struct ccc_fom_ const *const t, size_t const i)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, i));
}

static inline size_t
branch_i(struct ccc_fom_ const *const t, size_t const parent,
         enum fom_branch_ const dir)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, parent))->branch_[dir];
}

static inline size_t
parent_i(struct ccc_fom_ const *const t, size_t const child)
{
    return elem_in_slot(t, ccc_buf_at(&t->buf_, child))->parent_;
}

static inline size_t
index_of(struct ccc_fom_ const *const t, struct ccc_fom_elem_ const *const elem)
{
    return ccc_buf_index_of(&t->buf_, struct_base(t, elem));
}

static inline size_t *
branch_ref(struct ccc_fom_ const *t, size_t const node,
           enum fom_branch_ const branch)
{
    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->branch_[branch];
}

static inline size_t *
parent_ref(struct ccc_fom_ const *t, size_t node)
{

    return &elem_in_slot(t, ccc_buf_at(&t->buf_, node))->parent_;
}

static inline void *
base_at(struct ccc_fom_ const *const fom, size_t const i)
{
    return ccc_buf_at(&fom->buf_, i);
}

static inline void *
struct_base(struct ccc_fom_ const *const fom,
            struct ccc_fom_elem_ const *const e)
{
    return ((char *)e->branch_) - fom->node_elem_offset_;
}

static struct ccc_fom_elem_ *
elem_in_slot(struct ccc_fom_ const *const t, void const *const slot)
{

    return (struct ccc_fom_elem_ *)((char *)slot + t->node_elem_offset_);
}

static inline void *
key_from_node(struct ccc_fom_ const *const t,
              struct ccc_fom_elem_ const *const elem)
{
    return (char *)struct_base(t, elem) + t->key_offset_;
}

static inline void *
key_at(struct ccc_fom_ const *const t, size_t const i)
{
    return (char *)ccc_buf_at(&t->buf_, i) + t->key_offset_;
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
recursive_size(struct ccc_fom_ const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_size(t, branch_i(t, r, R))
           + recursive_size(t, branch_i(t, r, L));
}

static bool
are_subtrees_valid(struct ccc_fom_ const *t, struct tree_range const r)
{
    if (!r.root)
    {
        return true;
    }
    if (r.low && cmp_elems(t, key_at(t, r.low), r.root, t->cmp_) != CCC_LES)
    {
        return false;
    }
    if (r.high && cmp_elems(t, key_at(t, r.high), r.root, t->cmp_) != CCC_GRT)
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
is_storing_parent(struct ccc_fom_ const *const t, size_t const p,
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
validate(struct ccc_fom_ const *const fom)
{
    if (!are_subtrees_valid(fom, (struct tree_range){.root = fom->root_}))
    {
        return false;
    }
    size_t const size = recursive_size(fom, fom->root_);
    if (size && size != ccc_buf_size(&fom->buf_) - 1)
    {
        return false;
    }
    if (!is_storing_parent(fom, 0, fom->root_))
    {
        return false;
    }
    return true;
}

static struct parent_status
child_tracks_parent(struct ccc_fom_ const *const t, size_t const parent,
                    size_t const root)
{
    if (parent_i(t, root) != parent)
    {
        size_t const p = parent_i(t, parent_i(t, root));
        return (struct parent_status){false, p};
    }
    return (struct parent_status){true, parent};
}

static size_t
get_subtree_size(struct ccc_fom_ const *const t, size_t const root)
{
    if (!root)
    {
        return 0;
    }
    return 1 + get_subtree_size(t, branch_i(t, root, L))
           + get_subtree_size(t, branch_i(t, root, R));
}

static char const *
get_edge_color(struct ccc_fom_ const *const t, size_t const root,
               size_t const parent_size)
{
    if (!root)
    {
        return "";
    }
    return get_subtree_size(t, root) <= parent_size / 2 ? COLOR_BLU_BOLD
                                                        : COLOR_RED_BOLD;
}

static void
print_node(struct ccc_fom_ const *const t, size_t const parent,
           size_t const root, ccc_print_fn *const fn_print)
{
    fn_print(base_at(t, root));
    struct parent_status stat = child_tracks_parent(t, parent, root);
    if (!stat.correct)
    {
        printf("%s", COLOR_RED);
        fn_print(base_at(t, stat.parent));
        printf("%s", COLOR_NIL);
    }
    printf("\n");
}

/* I know this function is rough but it's tricky to focus on edge color rather
   than node color. Don't care about pretty code here, need thorough debug.
   I want to convert to iterative stack when I get the chance. */
static void
print_inner_tree(size_t const root, size_t const parent_size,
                 size_t const parent, char const *const prefix,
                 char const *const prefix_color,
                 enum fom_print_branch_ const node_type,
                 enum fom_branch_ const dir, struct ccc_fom_ const *const t,
                 ccc_print_fn *const fn_print)
{
    if (!root)
    {
        return;
    }
    size_t subtree_size = get_subtree_size(t, root);
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
        = get_edge_color(t, branch_i(t, root, L), subtree_size);
    if (!branch_i(t, root, R))
    {
        print_inner_tree(branch_i(t, root, L), subtree_size, root, str,
                         left_edge_color, LEAF, L, t, fn_print);
    }
    else if (!branch_i(t, root, L))
    {
        print_inner_tree(branch_i(t, root, R), subtree_size, root, str,
                         left_edge_color, LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(branch_i(t, root, R), subtree_size, root, str,
                         left_edge_color, LEAF, R, t, fn_print);
        print_inner_tree(branch_i(t, root, L), subtree_size, root, str,
                         left_edge_color, LEAF, L, t, fn_print);
    }
    free(str);
}

/* Should be pretty straightforward output. Red node means there
   is an error in parent tracking. The child does not track the parent
   correctly if this occurs and this will cause subtle delayed bugs. */
static void
tree_print(struct ccc_fom_ const *const t, size_t const root,
           ccc_print_fn *const fn_print)
{
    if (!root)
    {
        return;
    }
    size_t subtree_size = get_subtree_size(t, root);
    printf("\n%s(%zu)%s", COLOR_CYN, subtree_size, COLOR_NIL);
    print_node(t, 0, root, fn_print);

    char const *left_edge_color
        = get_edge_color(t, branch_i(t, root, L), subtree_size);
    if (!branch_i(t, root, R))
    {
        print_inner_tree(branch_i(t, root, L), subtree_size, root, "",
                         left_edge_color, LEAF, L, t, fn_print);
    }
    else if (!branch_i(t, root, L))
    {
        print_inner_tree(branch_i(t, root, R), subtree_size, root, "",
                         left_edge_color, LEAF, R, t, fn_print);
    }
    else
    {
        print_inner_tree(branch_i(t, root, L), subtree_size, root, "",
                         left_edge_color, LEAF, L, t, fn_print);
        print_inner_tree(branch_i(t, root, R), subtree_size, root, "",
                         left_edge_color, LEAF, R, t, fn_print);
    }
}

/* NOLINTEND(*misc-no-recursion) */
