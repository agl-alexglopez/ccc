/** Author: Alexander G. Lopez
This file implements data compression algorithms over simple files, primarily
text files for demonstration purposes. The compression algorithm used now is
Huffman Encoding and Decoding but more methods could be added later. Such
algorithms use a wide range of data structures. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BITSET_USING_NAMESPACE_CCC
#define BUFFER_USING_NAMESPACE_CCC
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "alloc.h"
#include "ccc/bitset.h"
#include "ccc/buffer.h"
#include "ccc/flat_hash_map.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/traits.h"
#include "str_arena.h"
#include "str_view.h"

/*===========================   Type Declarations  ==========================*/

enum print_branch
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
};

struct bitq
{
    bitset bs;
    size_t front;
    size_t size;
};

struct ch_freq
{
    char ch;
    size_t freq;
};

enum : uint8_t
{
    LINK_SIZE = 2,
    ITER_END = LINK_SIZE,
};

/** Encode nodes will be allocated freely in the heap as tree nodes for now.
They will be referenced in the priority queue stage of the algorithm but need
not have any intrusive elements for now. Once the tree is built, it will be
traversed to complete message and tree compression. */
struct huffman_node
{
    /** The parent for backtracking during dfs. */
    struct huffman_node *parent;
    /** The necessary links needed to build the encoding tree. */
    struct huffman_node *link[LINK_SIZE];
    /** The key for frequency map counting. */
    char ch;
    /** The caching iterator used during dfs path building text encoding. */
    uint8_t iter;
};

/** Element intended for the flat priority queue during the tree building phase.
Having a small simple type in the contiguous flat priority queue is good for
performance and the entire buffer can be freed when the algorithm completes.
The priority queue is only needed while building the tree. */
struct fpq_elem
{
    size_t freq;
    struct huffman_node *node;
};

struct huffman_tree
{
    struct huffman_node *root;
    size_t num_nodes;
    size_t num_leaves;
};

/** As we build the bit queue representing the paths to all characters in the
text we encode, we can memoize known sequences we have already seen in the
bit queue. We will record the first encounter with every character in the bit
queue and simply append this range of bits to the end of the queue as we build
the sequence. */
struct path_memo
{
    char ch;
    size_t path_start_index;
    size_t path_len;
};

enum : size_t
{
    /** Number of ascii characters can be a good starting cap for the arena. */
    START_STR_ARENA_CAP = 256,
};

/** During the encoding phase we must build the sequence of leaves we encounter
in an in-order traversal of the encoding tree. This is done when we are encoding
a tree we have formed into its more compact representation. */
struct leaf_string
{
    str_ofs start;
    size_t len;
};

struct compressed_huffman_tree
{
    struct bitq tree_paths;
    struct leaf_string leaves;
};

struct huffman_encoding
{
    struct bitq text_bits;
    struct compressed_huffman_tree tree;
};

/*===========================      Prototypes      ==========================*/

static void compress_file(FILE *f, struct str_arena *arena);
static flat_priority_queue build_encoding_pq(FILE *f);
static void bitq_push_back(struct bitq *, ccc_tribool);
static ccc_tribool bitq_pop_back(struct bitq *bq);
static ccc_tribool bitq_pop_front(struct bitq *);
static ccc_tribool bitq_test(struct bitq const *, size_t i);
static size_t bitq_size(struct bitq const *bq);
static void bitq_clear_and_free(struct bitq *);
static ccc_result bitq_reserve(struct bitq *, size_t to_add);
static uint64_t hash_ch(ccc_any_key to_hash);
static ccc_tribool ch_eq(ccc_any_key_cmp);
static ccc_threeway_cmp cmp_freqs(ccc_any_type_cmp cmp);
static ccc_tribool path_memo_eq(ccc_any_key_cmp cmp);
static struct path_memo *memoize_path(struct huffman_tree *tree,
                                      flat_hash_map *fh, struct bitq *, char c);
static struct bitq build_encoding_bitq(FILE *f, struct huffman_tree *tree);
static struct huffman_tree build_encoding_tree(FILE *f);
static struct compressed_huffman_tree compress_tree(struct huffman_tree *tree,
                                                    struct str_arena *);
static void free_encode_tree(struct huffman_tree *);
static void print_tree(struct huffman_node const *tree);
static void print_inner_tree(struct huffman_node const *root, enum print_branch,
                             char const *prefix);
static void print_node(struct huffman_node const *root);
static bool is_leaf(struct huffman_node const *root);
static void print_bitq(struct bitq const *bq);

#define prog_assert(cond, ...)                                                 \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            __VA_OPT__(__VA_ARGS__)                                            \
            printf("%s, %d, condition is false: %s\n", __FILE__, __LINE__,     \
                   #cond);                                                     \
            exit(1);                                                           \
        }                                                                      \
    }                                                                          \
    while (0)

/** Helper iterator to to turn a file pointer into a character iterator,
setting up iteration of each character in the file. Name the iterator and then
use it in the code block. Wrapping the code block in brackets is recommended for
formatting, though not required. Do not return early or use goto out of this
macro or memory will be leaked. */
#define foreach_filechar(file_ptr, char_iter_name, codeblock...)               \
    do                                                                         \
    {                                                                          \
        prog_assert(fseek(file_ptr, 0L, SEEK_SET) >= 0);                       \
        char *lineptr = NULL;                                                  \
        size_t len = 0;                                                        \
        ptrdiff_t read = 0;                                                    \
        while ((read = getline(&lineptr, &len, f)) > 0)                        \
        {                                                                      \
            str_view const line = {.s = lineptr, .len = read - 1};             \
            for (char const *char_iter_name = sv_begin(line);                  \
                 char_iter_name != sv_end(line);                               \
                 char_iter_name = sv_next(char_iter_name))                     \
            {                                                                  \
                codeblock                                                      \
            }                                                                  \
        }                                                                      \
        free(lineptr);                                                         \
    }                                                                          \
    while (0)

/*===========================   Argument Handling  ==========================*/

int
main(int argc, char **argv)
{
    if (argc < 2)
    {
        return 0;
    }
    if (argc == 2 && sv_starts_with(sv(argv[1]), SV("-h")))
    {
        return 0;
    }

    str_view file = {};
    for (int arg = 1; arg < argc; ++arg)
    {
        str_view const sv_arg = sv(argv[arg]);
        if (sv_starts_with(sv_arg, SV("-f=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            prog_assert(!sv_empty(raw_file),
                        (void)fprintf(stderr, "file string is empty\n"););
            file = raw_file;
        }
    }
    FILE *const f = fopen(sv_begin(file), "r");
    prog_assert(
        f, (void)fprintf(stderr, "could not open file %s", sv_begin(file)););
    struct str_arena arena = str_arena_create(START_STR_ARENA_CAP);
    prog_assert(arena.arena);
    compress_file(f, &arena);
    (void)fclose(f);
    str_arena_free(&arena);
    return 0;
}

/*=========================     Huffman Encoding    =========================*/

static void
compress_file(FILE *const f, struct str_arena *const arena)
{
    /* Encode characters in alphabet. */
    struct huffman_tree tree = build_encoding_tree(f);

    /* Encode message and compress alphabet tree relative to message encode. */
    struct huffman_encoding encoding = {
        .text_bits = build_encoding_bitq(f, &tree),
        .tree = compress_tree(&tree, arena),
    };

    /* Write compression to file. */

    /* Free in memory resources. */
    bitq_clear_and_free(&encoding.text_bits);
    bitq_clear_and_free(&encoding.tree.tree_paths);
    free_encode_tree(&tree);
}

/** Compresses the Huffman tree by creating its representation as a Pre-Order
traversal of the tree. The traversal is entered into a bit queue. We also push
the leaves to a string as they are encountered during this traversal. By the
end of the operation, we have a bit queue of our traversal where every internal
node encountered on the way down is a 1 and every leaf is a 0. We also have a
string of leaf characters that we encountered in order. */
static struct compressed_huffman_tree
compress_tree(struct huffman_tree *const tree, struct str_arena *const arena)
{
    struct compressed_huffman_tree ret = {
        .tree_paths = {.bs = bs_init(NULL, std_alloc, NULL, 0)},
    };
    prog_assert(bitq_reserve(&ret.tree_paths, tree->num_nodes)
                == CCC_RESULT_OK);
    struct huffman_node *cur = tree->root;
    /* To properly emulate a recursive Pre-Order traversal with iteration we
       use the parent field for backtracking and an iterator for caching and
       progression. */
    while (cur)
    {
        if (!cur->link[1])
        {
            /* A leaf is always pushed because it is only seen once. */
            bitq_push_back(&ret.tree_paths, CCC_FALSE);
            str_arena_push_back(arena, ret.leaves.start, ret.leaves.len,
                                cur->ch);
            ++ret.leaves.len;
            cur = cur->parent;
        }
        else if (cur->iter < ITER_END)
        {
            /* Be wary of this during backtracking. We only push internal 1
               nodes the first time they are encountered on the way down. We
               still need to access the second child so don't push a bit when
               we are simply progressing to the next child subtree. */
            if (cur->iter == 0)
            {
                bitq_push_back(&ret.tree_paths, CCC_TRUE);
            }
            cur = cur->link[cur->iter++];
        }
        else
        {
            /* Both child subtrees have been explored, so we backtrack. */
            cur->iter = 0;
            cur = cur->parent;
        }
    }
    print_bitq(&ret.tree_paths);
    return ret;
}

static struct bitq
build_encoding_bitq(FILE *const f, struct huffman_tree *const tree)
{
    struct bitq ret = {.bs = bs_init(NULL, std_alloc, NULL, 0)};
    /* By memoizing known bit sequences we can save significant time by not
       performing a DFS over the tree. This is especially helpful for large
       alphabets aka trees with many leaves. */
    flat_hash_map memo = ccc_fhm_init(NULL, struct path_memo, ch, hash_ch,
                                      path_memo_eq, NULL, NULL, 0);
    prog_assert(fhm_reserve(&memo, tree->num_leaves, std_alloc)
                == CCC_RESULT_OK);
    foreach_filechar(f, c, {
        struct path_memo const *path = fhm_get_key_val(&memo, c);
        if (!path)
        {
            path = memoize_path(tree, &memo, &ret, *c);
        }
        for (size_t i = path->path_start_index,
                    end = path->path_start_index + path->path_len;
             i < end; ++i)
        {
            ccc_tribool const bit = bitq_test(&ret, i);
            prog_assert(bit != CCC_TRIBOOL_ERROR);
            bitq_push_back(&ret, bit);
        }
    });
    prog_assert(fhm_clear_and_free_reserve(&memo, NULL, std_alloc)
                == CCC_RESULT_OK);
    return ret;
}

/** Finds the path to the current character in the encoding tree and adds it
as an entry in the path memo map. This function modifies the tree nodes by
altering their iterator field during the DFS, but it restores all nodes to their
original state before returning. */
static struct path_memo *
memoize_path(struct huffman_tree *const tree, flat_hash_map *const fh,
             struct bitq *const bq, char const c)
{
    struct path_memo *const path = fhm_insert_entry_w(
        fhm_entry_r(fh, &c), (struct path_memo){
                                 .ch = c,
                                 .path_start_index = bitq_size(bq),
                             });
    struct huffman_node *cur = tree->root;
    /* An iterative depth first search is convenient because the bit path in
       the queue can represent the exact path we are currently on. Just be
       sure to backtrack up the path to cleanup iterators. */
    while (cur)
    {
        /* This is the leaf we want. */
        if (!cur->link[1] && cur->ch == c)
        {
            break;
        }
        /* Wrong leaf or we have explored both subtrees of an internal node. */
        if (!cur->link[1] || cur->iter >= ITER_END)
        {
            cur->iter = 0;
            cur = cur->parent;
            bitq_pop_back(bq);
            continue;
        }
        /* Depth progression of depth first search. */
        prog_assert(cur->iter <= CCC_TRUE);
        bitq_push_back(bq, cur->iter);
        /* During backtracking this helps us know which child subtree needs to
           be explored or if we are done and can continue backtracking. */
        cur = cur->link[cur->iter++];
    }
    /* Cleanup because we now have the correct path. */
    for (; cur; cur = cur->parent)
    {
        cur->iter = 0;
    }
    path->path_len = bitq_size(bq) - path->path_start_index;
    return path;
}

static struct huffman_tree
build_encoding_tree(FILE *const f)
{
    flat_priority_queue pq = build_encoding_pq(f);
    struct huffman_tree ret = {
        .root = NULL,
        .num_nodes = size(&pq).count,
        .num_leaves = size(&pq).count,
    };
    while (size(&pq).count >= 2)
    {
        struct fpq_elem zero = *(struct fpq_elem *)front(&pq);
        prog_assert(pop(&pq) == CCC_RESULT_OK);
        struct fpq_elem one = *(struct fpq_elem *)front(&pq);
        prog_assert(pop(&pq) == CCC_RESULT_OK);
        struct huffman_node *const subtree_root
            = malloc(sizeof(struct huffman_node));
        prog_assert(subtree_root);
        zero.node->parent = subtree_root;
        one.node->parent = subtree_root;
        *subtree_root = (struct huffman_node){
            .parent = NULL,
            .link = {zero.node, one.node},
            .ch = '\0',
            .iter = 0,
        };
        ++ret.num_nodes;
        struct fpq_elem const *const pushed
            = fpq_emplace(&pq, (struct fpq_elem){
                                   .freq = zero.freq + one.freq,
                                   .node = subtree_root,
                               });
        prog_assert(pushed);
        ret.root = subtree_root;
    }
    print_tree(ret.root);
    /* The flat pq was given no allocation permission because the memory it
       needs was already allocated by the buffer it stole from. The priority
       queue only gets smaller as the algorithms progresses so we didn't need
       to worry about growth. Provide same function used to reserve buffer. */
    prog_assert(fpq_clear_and_free_reserve(&pq, NULL, std_alloc)
                == CCC_RESULT_OK);
    return ret;
}

/** Returns a min priority queue sorted by frequency meaning the least frequent
character will be the root. The priority queue is built in O(N) time. */
static flat_priority_queue
build_encoding_pq(FILE *const f)
{
    flat_hash_map fh = fhm_init(NULL, struct ch_freq, ch, hash_ch, ch_eq,
                                std_alloc, NULL, 0);
    foreach_filechar(f, c, {
        struct ch_freq const *const cf = fhm_or_insert_w(
            fhm_and_modify_w(entry_r(&fh, c), struct ch_freq, ++T->freq;),
            (struct ch_freq){.ch = *c, .freq = 1});
        prog_assert(cf);
    });
    prog_assert(size(&fh).count >= 2);
    /* Use a buffer to simply push back elements we will heapify at the end. */
    buffer buf = buf_init((struct fpq_elem *)NULL, NULL, NULL, 0);
    /* Add one to reservation for the flat priority queue swap slot. */
    prog_assert(buf_alloc(&buf, size(&fh).count + 1, std_alloc)
                == CCC_RESULT_OK);
    for (struct ch_freq const *map_slot = fhm_begin(&fh);
         map_slot != fhm_end(&fh); map_slot = fhm_next(&fh, map_slot))
    {
        struct huffman_node *const tree_node
            = malloc(sizeof(struct huffman_node));
        prog_assert(tree_node);
        *tree_node = (struct huffman_node){.ch = map_slot->ch};
        struct fpq_elem const *const pushed
            = buf_push_back(&buf, &(struct fpq_elem){
                                      .freq = map_slot->freq,
                                      .node = tree_node,
                                  });
        prog_assert(pushed);
    }
    /* The buffer had no allocation permission and set up all the elements we
       needed to be in the flat priority queue. Now we take its memory and
       heapify the data in O(N) time rather than pushing each element. */
    flat_priority_queue pq = fpq_heapify_init(
        (struct fpq_elem *)buf_begin(&buf), CCC_LES, cmp_freqs, NULL, NULL,
        buf_capacity(&buf).count, buf_size(&buf).count);
    /* Free map but not the buffer because the priority queue took buffer. */
    prog_assert(fhm_clear_and_free(&fh, NULL) == CCC_RESULT_OK);
    return pq;
}

/** Frees all encoding nodes from the tree provided. */
static void
free_encode_tree(struct huffman_tree *tree)
{
    /* Freeing via left rotations is an easy iterative way to destroy tree. */
    struct huffman_node *cur = tree->root;
    while (cur)
    {
        if (cur->link[0])
        {
            struct huffman_node *const zero = cur->link[0];
            cur->link[0] = cur->link[0]->link[1];
            zero->link[1] = cur;
            cur = zero;
            continue;
        }
        struct huffman_node *const next = cur->link[1];
        free(cur);
        cur = next;
    }
    tree->num_leaves = 0;
    tree->num_nodes = 0;
    tree->root = NULL;
}

/* NOLINTBEGIN(*misc-no-recursion) */

[[maybe_unused]] static void
print_tree(struct huffman_node const *const tree)
{
    if (!tree)
    {
        return;
    }
    print_node(tree);
    print_inner_tree(tree->link[1], BRANCH, "");
    print_inner_tree(tree->link[0], LEAF, "");
}

[[maybe_unused]] static void
print_inner_tree(struct huffman_node const *const root,
                 enum print_branch const branch_type, char const *const prefix)
{
    if (!root)
    {
        return;
    }
    printf("%s", prefix);
    printf("%s", branch_type == LEAF ? " └──" : " ├──");

    print_node(root);

    char *str = NULL;
    int const string_length = snprintf(NULL, 0, "%s%s", prefix,
                                       branch_type == LEAF ? "     " : " │   ");
    if (string_length > 0)
    {
        str = malloc(string_length + 1);
        (void)snprintf(str, string_length, "%s%s", prefix,
                       branch_type == LEAF ? "     " : " │   ");
    }
    if (str == NULL)
    {
        printf("memory exceeded. Cannot display tree.\n");
        return;
    }

    if (!root->link[1])
    {
        print_inner_tree(root->link[0], LEAF, str);
    }
    else if (!root->link[0])
    {
        print_inner_tree(root->link[1], LEAF, str);
    }
    else
    {
        print_inner_tree(root->link[1], BRANCH, str);
        print_inner_tree(root->link[0], LEAF, str);
    }
    free(str);
}

[[maybe_unused]] static void
print_node(struct huffman_node const *const root)
{
    if (is_leaf(root))
    {
        printf("(%c)\n", root->ch);
    }
    else
    {
        printf("1┐\n");
    }
}

[[maybe_unused]] static bool
is_leaf(struct huffman_node const *const root)
{
    return !root->link[0] && !root->link[1];
}

[[maybe_unused]] static void
print_bitq(struct bitq const *const bq)
{
    for (size_t i = 0, col = 1, end = bitq_size(bq); i < end; ++i, ++col)
    {
        bitq_test(bq, i) ? printf("1") : printf("0");
        if (col % 50 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

/* NOLINTEND(*misc-no-recursion) */

/*=========================     Huffman Decoding    =========================*/

/*=====================       Bit Queue Helper Code     =====================*/

static void
bitq_push_back(struct bitq *const bq, ccc_tribool const bit)
{
    if (bq->size == bs_size(&bq->bs).count)
    {
        ccc_result const r = bs_push_back(&bq->bs, bit);
        prog_assert(r == CCC_RESULT_OK);
    }
    else
    {
        ccc_tribool const was = bs_set(
            &bq->bs, ((bq->front + bq->size) % bs_capacity(&bq->bs).count),
            bit);
        prog_assert(was != CCC_TRIBOOL_ERROR);
    }
    ++bq->size;
}

static ccc_tribool
bitq_pop_back(struct bitq *const bq)
{
    size_t const i = (bq->front + bq->size - 1) % bs_capacity(&bq->bs).count;
    ccc_tribool const bit = bs_test(&bq->bs, i);
    prog_assert(bit != CCC_TRIBOOL_ERROR);
    --bq->size;
    return bit;
}

static ccc_tribool
bitq_pop_front(struct bitq *const bq)
{
    ccc_tribool const bit = bs_test(&bq->bs, bq->front);
    prog_assert(bit != CCC_TRIBOOL_ERROR);
    bq->front = (bq->front + 1) % bs_size(&bq->bs).count;
    --bq->size;
    return bit;
}

static ccc_tribool
bitq_test(struct bitq const *const bq, size_t const i)
{
    return bs_test(&bq->bs, (bq->front + i) % bs_capacity(&bq->bs).count);
}

static size_t
bitq_size(struct bitq const *const bq)
{
    return bq->size;
}

static void
bitq_clear_and_free(struct bitq *const bq)
{
    prog_assert(bq);
    ccc_result const r = bs_clear_and_free(&bq->bs);
    prog_assert(r == CCC_RESULT_OK);
}

static ccc_result
bitq_reserve(struct bitq *const bq, size_t const to_add)
{
    return bs_reserve(&bq->bs, to_add, std_alloc);
}

/*=====================       Hash Map Helper Code      =====================*/

/** The flat hash map documentation mentions we should have good bit diversity
in the high and low byte of our hash but we are only hashing characters which
are their own unique value across all characters we will encounter. So the
hashed value will just be the character repeated at the high and low bit. */
static uint64_t
hash_ch(ccc_any_key const to_hash)
{
    char const key = *(char *)to_hash.any_key;
    return ((uint64_t)key << 55) | key;
}

static ccc_tribool
ch_eq(ccc_any_key_cmp const cmp)
{
    struct ch_freq const *const type = (struct ch_freq *)cmp.any_type_rhs;
    return *(char *)cmp.any_key_lhs == type->ch;
}

static ccc_threeway_cmp
cmp_freqs(ccc_any_type_cmp const cmp)
{
    struct fpq_elem const *const lhs = (struct fpq_elem *)cmp.any_type_lhs;
    struct fpq_elem const *const rhs = (struct fpq_elem *)cmp.any_type_rhs;
    return (lhs->freq > rhs->freq) - (lhs->freq < rhs->freq);
}

static ccc_tribool
path_memo_eq(ccc_any_key_cmp const cmp)
{
    struct path_memo const *const type = (struct path_memo *)cmp.any_type_rhs;
    return *(char *)cmp.any_key_lhs == type->ch;
}
