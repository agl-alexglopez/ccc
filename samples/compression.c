/** Author: Alexander G. Lopez
This file implements data compression algorithms over simple files, primarily
text files for demonstration purposes. The compression algorithm used now is
Huffman Encoding and Decoding but more methods could be added later. Such
algorithms use a wide range of data structures. */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifdef __linux__
#    include <linux/limits.h>
#    define FILESYS_MAX_PATH PATH_MAX
#endif
#ifdef __APPLE__
#    include <sys/syslimits.h>
#    define FILESYS_MAX_PATH NAME_MAX
#endif
/*===========================   Type Declarations  ==========================*/

enum print_branch
{
    BRANCH = 0, /* ├── */
    LEAF = 1    /* └── */
};

/** A thin wrapper around a bit set to provide queue/deq like behavior. For
Huffman Compression we only need to push and pop from back and pop from the
front so we don't need the full suite of deq functionality. The bit set provides
almost all the functionality needed we just need to manage what happens when
we pop from bit set but do not manually decrease the size of the underlying bit
set container. Instead we manage our own front and size fields. */
struct bitq
{
    bitset bs;
    size_t front;
    size_t size;
};

/** Simple entry in the flat hash map while counting character occurrences. */
struct ch_freq
{
    char ch;
    size_t freq;
};

enum : uint8_t
{
    /** There are two children for every Huffman tree node. */
    LINK_SIZE = 2,
    /** Caching for iterative traversals uses this position as end sentinel. */
    ITER_END = LINK_SIZE,
};

/** Tree nodes will be pushed into a ccc_buffer. This is the same concept as
freely allocating them in the heap, but much more efficient and convenient. We
push elements to the back of the buffer to allocate and because we never free
any nodes until we are done with the entire tree this is an optimal bump
allocator. All memory is freed at once in one contiguous allocation. The only
detail to manage is that due to resizing of the buffer elements must track each
other through indices not pointers. */
struct huffman_node
{
    /** The parent for backtracking during DFS and Pre-Order traversal. */
    size_t parent;
    /** The necessary links needed to build the encoding tree. */
    size_t link[LINK_SIZE];
    /** The leaf character if this node is a leaf. */
    char ch;
    /** The caching iterator to help emulate recursion with iteration. */
    uint8_t iter;
};

/** Element intended for the flat priority queue during the tree building phase.
Having a small simple type in the contiguous flat priority queue is good for
performance and the entire buffer can be freed when the algorithm completes.
The priority queue is only needed while building the tree. */
struct fpq_elem
{
    size_t freq;
    size_t node;
};

/** It is helpful to know how many leaves and total nodes there are for
reserving the appropriate space for helper data structures. */
struct huffman_tree
{
    ccc_buffer nodes;
    size_t root;
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
    /** The key for the path memo. */
    char ch;
    /** The index in the bit queue where we have seen this path before. */
    size_t path_start_index;
    /** The length of the known path we have already pushed to the queue. */
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
    /** The string arena hands out offsets not pointers to strings. */
    str_ofs start;
    /** We should know the length of the leaf string for safety. */
    size_t len;
};

/** The compact representation of the tree structure we will encode to a
compressed file for later reconstruction. */
struct compressed_huffman_tree
{
    /** The Pre-Order traversal of internal nodes and leaves. Every internal
    node encountered on the way down is a 1 and every leaf is a 0. */
    struct bitq tree_paths;
    /** The inorder sequence of leaf character nodes. */
    char const *leaf_string;
};

/** The completed encoding of a file. This can be written to disk and retrieved
later to be reconstructed. This method loses no information by compression. */
struct huffman_encoding
{
    /** The path to every character encountered in the file text in order. */
    struct bitq text_bits;
    /** The compact representation of the tree to be reconstructed later. */
    struct compressed_huffman_tree blueprint;
};

struct huffman_header
{
    uint8_t magic;
    uint8_t leaves_len;
    char leaves[256];
};

struct file_action_pack
{
    str_view to_compress;
    str_view to_decompress;
};

/** File format ".cccz" in header. */
static uint32_t const cccz_magic = 0x2E63637A;
static str_view const relative_output_dir = SV("samples/output/");
static str_view const compression_file_suffix = SV(".cccz");

/*===========================      Prototypes      ==========================*/

static struct huffman_encoding compress_file(str_view to_compress,
                                             struct str_arena *arena);
static flat_priority_queue build_encoding_pq(FILE *f, struct huffman_tree *);
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
static void memoize_path(struct huffman_tree *tree, flat_hash_map *fh,
                         struct bitq *, char c);
static struct bitq build_encoding_bitq(FILE *f, struct huffman_tree *tree);
static struct huffman_tree build_encoding_tree(FILE *f);
static struct compressed_huffman_tree compress_tree(struct huffman_tree *tree,
                                                    struct str_arena *);
static void free_encode_tree(struct huffman_tree *);
static void print_tree(struct huffman_tree const *tree, size_t node);
static void print_inner_tree(struct huffman_tree const *tree, size_t node,
                             enum print_branch, char const *prefix);
static void print_node(struct huffman_tree const *tree, size_t node);
static bool is_leaf(struct huffman_tree const *tree, size_t node);
static void print_bitq(struct bitq const *bq);
static ccc_tribool bitq_front(struct bitq const *bq);
static void decompress_file(str_view to_decompress,
                            struct huffman_encoding *he);
static struct huffman_tree
reconstruct_tree(struct compressed_huffman_tree *blueprint);
static void reconstruct_text(str_view to_decompress,
                             struct huffman_tree const *, struct bitq *);
static void print_help(void);
size_t branch_i(struct huffman_tree const *t, size_t node, uint8_t dir);
size_t parent_i(struct huffman_tree const *t, size_t node);
char char_i(struct huffman_tree const *t, size_t node);
struct huffman_node *node_at(struct huffman_tree const *t, size_t node);
void write_to_file(str_view original_filename, struct huffman_encoding *);

/** Asserts even in release mode. Run code in the second argument if needed. */
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
formatting, though not required.

Do not return early or use goto out of this macro or memory will be leaked. */
#define foreach_filechar(file_ptr, char_iter_name, codeblock...)               \
    do                                                                         \
    {                                                                          \
        prog_assert(fseek(file_ptr, 0L, SEEK_SET) >= 0);                       \
        char *lineptr = NULL;                                                  \
        size_t len = 0;                                                        \
        ptrdiff_t read = 0;                                                    \
        while ((read = getline(&lineptr, &len, f)) > 0)                        \
        {                                                                      \
            str_view const line = {.s = lineptr, .len = read};                 \
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
    struct file_action_pack todo = {};
    for (int arg = 1; arg < argc; ++arg)
    {
        str_view const sv_arg = sv(argv[arg]);
        if (sv_starts_with(sv_arg, SV("-h")))
        {
            print_help();
            return 0;
        }
        if (sv_starts_with(sv_arg, SV("-c=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            prog_assert(!sv_empty(raw_file),
                        (void)fprintf(stderr, "file string is empty\n"););
            todo.to_compress = raw_file;
        }
        else if (sv_starts_with(sv_arg, SV("-d=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            prog_assert(!sv_empty(raw_file),
                        (void)fprintf(stderr, "file string is empty\n"););
            todo.to_decompress = raw_file;
        }
    }
    struct str_arena arena = str_arena_create(START_STR_ARENA_CAP);
    prog_assert(arena.arena);
    struct huffman_encoding encode = compress_file(todo.to_compress, &arena);
    if (!sv_empty(todo.to_decompress))
    {
        decompress_file(todo.to_decompress, &encode);
    }

    str_arena_free(&arena);
    return 0;
}

/*=========================     Huffman Encoding    =========================*/

static struct huffman_encoding
compress_file(str_view const to_compress, struct str_arena *const arena)
{
    FILE *const f = fopen(sv_begin(to_compress), "r");
    prog_assert(f, (void)fprintf(stderr, "could not open file %s",
                                 sv_begin(to_compress)););
    /* Encode characters in alphabet. */
    struct huffman_tree tree = build_encoding_tree(f);

    /* Encode message and compress alphabet tree relative to message. */
    struct huffman_encoding encoding = {
        .text_bits = build_encoding_bitq(f, &tree),
        .blueprint = compress_tree(&tree, arena),
    };

    /* Create header and write this compressed data to disk. */
    write_to_file(to_compress, &encoding);

    /* Free in memory resources. */
    free_encode_tree(&tree);
    bitq_clear_and_free(&encoding.text_bits);
    bitq_clear_and_free(&encoding.blueprint.tree_paths);
    (void)fclose(f);
    return encoding;
}

void
write_to_file(str_view const original_filepath,
              struct huffman_encoding *const encoding)
{
    size_t dir_delim
        = sv_rfind(original_filepath, sv_len(original_filepath), SV("/"));
    str_view const raw_file = dir_delim == sv_npos(original_filepath)
                                ? original_filepath
                                : sv_substr(original_filepath, dir_delim + 1,
                                            sv_len(original_filepath));
    prog_assert(sv_size(relative_output_dir) + sv_size(raw_file)
                    + sv_size(compression_file_suffix)
                < FILESYS_MAX_PATH);
    char full_path[FILESYS_MAX_PATH];
    size_t path_bytes
        = sv_fill(sv_size(relative_output_dir), full_path, relative_output_dir);
    size_t file_bytes
        = sv_fill(sv_size(raw_file), full_path + (path_bytes - 1), raw_file);
    (void)sv_fill(sv_size(compression_file_suffix),
                  full_path + (path_bytes - 1) + (file_bytes - 1),
                  compression_file_suffix);
    FILE *const cccz = fopen(full_path, "w");
    prog_assert(cccz, printf("%s", strerror(errno)););
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
    struct leaf_string leaves = {.start = str_arena_alloc(arena, 0)};
    prog_assert(bitq_reserve(&ret.tree_paths, tree->num_nodes)
                == CCC_RESULT_OK);
    size_t cur = tree->root;
    /* To properly emulate a recursive Pre-Order traversal with iteration we
       use the parent field for backtracking and an iterator for caching and
       progression. */
    while (cur)
    {
        struct huffman_node *const node = node_at(tree, cur);
        if (!node->link[1])
        {
            /* A leaf is always pushed because it is only seen once. */
            bitq_push_back(&ret.tree_paths, CCC_FALSE);
            str_arena_push_back(arena, leaves.start, leaves.len, node->ch);
            ++leaves.len;
            cur = node->parent;
        }
        else if (node->iter < ITER_END)
        {
            /* We only push internal 1 nodes the first time on the way down. We
               still need to access the second child so don't push a bit when we
               are simply progressing to the next child subtree. */
            if (node->iter == 0)
            {
                bitq_push_back(&ret.tree_paths, CCC_TRUE);
            }
            cur = node->link[node->iter++];
        }
        else
        {
            /* Both child subtrees have been explored, so cleanup/backtrack. */
            node->iter = 0;
            cur = node->parent;
        }
    }
    ret.leaf_string = str_arena_at(arena, leaves.start);
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
    prog_assert(reserve(&memo, tree->num_leaves, std_alloc) == CCC_RESULT_OK);
    foreach_filechar(f, c, {
        struct path_memo const *path = get_key_val(&memo, c);
        if (path)
        {
            for (size_t i = path->path_start_index,
                        end = path->path_start_index + path->path_len;
                 i < end; ++i)
            {
                ccc_tribool const bit = bitq_test(&ret, i);
                prog_assert(bit != CCC_TRIBOOL_ERROR);
                bitq_push_back(&ret, bit);
            }
        }
        else
        {
            memoize_path(tree, &memo, &ret, *c);
        }
    });
    prog_assert(clear_and_free_reserve(&memo, NULL, std_alloc)
                == CCC_RESULT_OK);
    return ret;
}

/** Finds the path to the current character in the encoding tree and adds it
as an entry in the path memo map. This function modifies the tree nodes by
altering their iterator field during the DFS, but it restores all nodes to their
original state before returning. */
static void
memoize_path(struct huffman_tree *const tree, flat_hash_map *const fh,
             struct bitq *const bq, char const c)
{
    struct path_memo *const path = insert_entry(
        entry_r(fh, &c),
        &(struct path_memo){.ch = c, .path_start_index = bitq_size(bq)});
    size_t cur = tree->root;
    /* An iterative depth first search is convenient because the bit path in
       the queue can represent the exact path we are currently on. Just be
       sure to backtrack up the path to cleanup iterators. */
    while (cur)
    {
        struct huffman_node *const node = node_at(tree, cur);
        /* This is the leaf we want. */
        if (!node->link[1] && node->ch == c)
        {
            break;
        }
        /* Wrong leaf or we have explored both subtrees of an internal node. */
        if (!node->link[1] || node->iter >= ITER_END)
        {
            node->iter = 0;
            cur = node->parent;
            bitq_pop_back(bq);
            continue;
        }
        /* Depth progression of depth first search. */
        prog_assert(node->iter <= CCC_TRUE);
        bitq_push_back(bq, node->iter);
        /* During backtracking this helps us know which child subtree needs to
           be explored or if we are done and can continue backtracking. */
        cur = node->link[node->iter++];
    }
    /* Cleanup because we now have the correct path. */
    for (; cur; cur = parent_i(tree, cur))
    {
        node_at(tree, cur)->iter = 0;
    }
    path->path_len = bitq_size(bq) - path->path_start_index;
}

static struct huffman_tree
build_encoding_tree(FILE *const f)
{
    struct huffman_tree ret = {
        .nodes = buf_init((struct huffman_node *)NULL, std_alloc, NULL, 0),
        .root = 0,
    };
    flat_priority_queue pq = build_encoding_pq(f, &ret);
    ret.num_leaves = ret.num_nodes = size(&pq).count;
    while (size(&pq).count >= 2)
    {
        /* Small elements and we need the pair so we can't hold references. */
        struct fpq_elem zero = *(struct fpq_elem *)front(&pq);
        prog_assert(pop(&pq) == CCC_RESULT_OK);
        struct fpq_elem one = *(struct fpq_elem *)front(&pq);
        prog_assert(pop(&pq) == CCC_RESULT_OK);
        struct huffman_node *const internal_one
            = push_back(&ret.nodes, &(struct huffman_node){
                                        .parent = 0,
                                        .link = {zero.node, one.node},
                                        .ch = '\0',
                                        .iter = 0,
                                    });
        size_t const new_root = buf_i(&ret.nodes, internal_one).count;
        prog_assert(internal_one);
        node_at(&ret, zero.node)->parent = new_root;
        node_at(&ret, one.node)->parent = new_root;
        ++ret.num_nodes;
        struct fpq_elem const *const pushed
            = push(&pq, &(struct fpq_elem){.freq = zero.freq + one.freq,
                                           .node = new_root});
        prog_assert(pushed);
        ret.root = new_root;
    }
    print_tree(&ret, ret.root);
    /* The flat pq was given no allocation permission because the memory it
       needs was already allocated by the buffer it stole from. The priority
       queue only gets smaller as the algorithms progresses so we didn't need
       to worry about growth. Provide same function used to reserve buffer. */
    prog_assert(clear_and_free_reserve(&pq, NULL, std_alloc) == CCC_RESULT_OK);
    return ret;
}

/** Returns a min priority queue sorted by frequency meaning the least frequent
character will be the root. The priority queue is built in O(N) time. The
priority queue has no allocation permission, knowing the space it needs to
reserve ahead of time and assuming the tree building algorithm will strictly
decrease the size of the priority queue. */
static flat_priority_queue
build_encoding_pq(FILE *const f, struct huffman_tree *const tree)
{
    /* For a buffer based tree 0 is the NULL node so we can't have actual data
       we want at that index in the tree. */
    prog_assert(push_back(&tree->nodes, &(struct huffman_node){}));
    flat_hash_map fh = fhm_init(NULL, struct ch_freq, ch, hash_ch, ch_eq,
                                std_alloc, NULL, 0);
    foreach_filechar(f, c, {
        struct ch_freq *const cf
            = or_insert(entry_r(&fh, c), &(struct ch_freq){.ch = *c});
        prog_assert(cf);
        ++cf->freq;
    });
    prog_assert(size(&fh).count >= 2);
    /* Use a buffer to simply push back elements we will heapify at the end. */
    buffer buf = buf_init((struct fpq_elem *)NULL, NULL, NULL, 0);
    /* Add one to reservation for the flat priority queue swap slot. */
    prog_assert(buf_alloc(&buf, size(&fh).count + 1, std_alloc)
                == CCC_RESULT_OK);
    for (struct ch_freq const *i = begin(&fh); i != end(&fh); i = next(&fh, i))
    {
        struct huffman_node *const node
            = buf_push_back(&tree->nodes, &(struct huffman_node){.ch = i->ch});
        struct fpq_elem const *const fe = push_back(
            &buf, &(struct fpq_elem){.freq = i->freq,
                                     .node = buf_i(&tree->nodes, node).count});
        prog_assert(fe);
    }
    /* Free map but not the buffer because the priority queue took buffer. */
    prog_assert(clear_and_free(&fh, NULL) == CCC_RESULT_OK);
    /* The buffer had no allocation permission and set up all the elements we
       needed to be in the flat priority queue. Now we take its memory and
       heapify the data in O(N) time rather than pushing each element. */
    return fpq_heapify_init((struct fpq_elem *)begin(&buf), CCC_LES, cmp_freqs,
                            NULL, NULL, capacity(&buf).count, size(&buf).count);
}

/*=========================     Huffman Decoding    =========================*/

void
decompress_file(str_view to_decompress, struct huffman_encoding *const he)
{
    prog_assert(bitq_size(&he->blueprint.tree_paths)
                && bitq_front(&he->blueprint.tree_paths) == CCC_TRUE);
    struct huffman_tree tree = reconstruct_tree(&he->blueprint);
    reconstruct_text(to_decompress, &tree, &he->text_bits);
    free_encode_tree(&tree);
}

/** Reconstructs a Huffman encoding tree based on the blueprint provided. The
tree is constructed in linear time and constant space additional to the nodes
being allocated. */
static struct huffman_tree
reconstruct_tree(struct compressed_huffman_tree *const blueprint)
{
    struct huffman_tree ret = {
        .nodes = ccc_buf_init((struct huffman_node *)NULL, std_alloc, NULL, 0),
        .num_nodes = bitq_size(&blueprint->tree_paths),
    };
    prog_assert(buf_alloc(&ret.nodes, ret.num_nodes + 1, std_alloc)
                == CCC_RESULT_OK);
    /* 0 index is NULL so real data can't be there. */
    prog_assert(push_back(&ret.nodes, &(struct huffman_node){}));
    struct huffman_node const *const first
        = push_back(&ret.nodes, &(struct huffman_node){});
    ret.root = buf_i(&ret.nodes, first).count;
    size_t prev = ret.root;
    size_t cur = 0;
    (void)bitq_pop_front(&blueprint->tree_paths);
    size_t ch_i = 0;
    while (bitq_size(&blueprint->tree_paths))
    {
        ccc_tribool bit = CCC_TRUE;
        if (!cur)
        {
            struct huffman_node *const prev_node = node_at(&ret, prev);
            bit = bitq_pop_front(&blueprint->tree_paths);
            struct huffman_node *const pushed
                = push_back(&ret.nodes, &(struct huffman_node){.parent = prev});
            cur = ccc_buf_i(&ret.nodes, pushed).count;
            prev_node->link[prev_node->iter++] = cur;
            if (!bit)
            {
                pushed->ch = blueprint->leaf_string[ch_i++];
            }
        }
        struct huffman_node *const cur_node = node_at(&ret, cur);
        /* An internal node has further child subtrees to build. */
        if (bit && cur_node->iter < ITER_END)
        {
            prev = cur;
            cur = cur_node->link[cur_node->iter];
            continue;
        }
        /* Backtrack. A leaf or internal node with both children built. */
        cur = prev;
        prev = parent_i(&ret, prev);
    }
    print_tree(&ret, ret.root);
    return ret;
}

void
reconstruct_text(str_view const to_decompress,
                 struct huffman_tree const *const tree, struct bitq *const bq)
{
    prog_assert(sv_ends_with(to_decompress, SV(".ccc")));
    FILE *const f = fopen(sv_begin(to_decompress), "w");
    prog_assert(f, printf("%s", strerror(errno)););
    size_t cur = tree->root;
    while (bitq_size(bq))
    {
        struct huffman_node const *const node = node_at(tree, cur);
        if (!node->link[1])
        {
            (void)fprintf(f, "%c", node->ch);
            cur = tree->root;
        }
        ccc_tribool const bit = bitq_pop_front(bq);
        prog_assert(bit != CCC_TRIBOOL_ERROR);
        cur = node->link[bit];
    }
    if (!branch_i(tree, cur, 1))
    {
        (void)fprintf(f, "%c", char_i(tree, cur));
    }
    (void)fclose(f);
}

/*=========================      Huffman Helpers    =========================*/

struct huffman_node *
node_at(struct huffman_tree const *const t, size_t const node)
{
    return ((struct huffman_node *)buf_at(&t->nodes, node));
}

size_t
branch_i(struct huffman_tree const *const t, size_t const node,
         uint8_t const dir)
{
    return ((struct huffman_node *)buf_at(&t->nodes, node))->link[dir];
}

size_t
parent_i(struct huffman_tree const *const t, size_t node)
{
    return ((struct huffman_node *)buf_at(&t->nodes, node))->parent;
}

char
char_i(struct huffman_tree const *const t, size_t const node)
{
    return ((struct huffman_node *)buf_at(&t->nodes, node))->ch;
}

/** Frees all encoding nodes from the tree provided. */
static void
free_encode_tree(struct huffman_tree *tree)
{
    prog_assert(buf_alloc(&tree->nodes, 0, std_alloc) == CCC_RESULT_OK);
    tree->num_leaves = 0;
    tree->num_nodes = 0;
    tree->root = 0;
}

/* NOLINTBEGIN(*misc-no-recursion) */

[[maybe_unused]] static void
print_tree(struct huffman_tree const *const tree, size_t const node)
{
    if (!tree)
    {
        return;
    }
    print_node(tree, node);
    print_inner_tree(tree, branch_i(tree, node, 1), BRANCH, "");
    print_inner_tree(tree, branch_i(tree, node, 0), LEAF, "");
}

[[maybe_unused]] static void
print_inner_tree(struct huffman_tree const *const tree, size_t const node,
                 enum print_branch const branch_type, char const *const prefix)
{
    if (!node)
    {
        return;
    }
    printf("%s", prefix);
    printf("%s", branch_type == LEAF ? " └──" : " ├──");

    print_node(tree, node);

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
    struct huffman_node const *const root = node_at(tree, node);
    if (!root->link[1])
    {
        print_inner_tree(tree, root->link[0], LEAF, str);
    }
    else if (!root->link[0])
    {
        print_inner_tree(tree, root->link[1], LEAF, str);
    }
    else
    {
        print_inner_tree(tree, root->link[1], BRANCH, str);
        print_inner_tree(tree, root->link[0], LEAF, str);
    }
    free(str);
}

[[maybe_unused]] static void
print_node(struct huffman_tree const *const tree, size_t const node)
{
    if (is_leaf(tree, node))
    {
        switch (char_i(tree, node))
        {
            case '\n':
                printf("(\\n)\n");
                break;
            case '\r':
                printf("(\\r)\n");
                break;
            case '\t':
                printf("(\\t)\n");
                break;
            case '\v':
                printf("(\\v)\n");
                break;
            case '\f':
                printf("(\\f)\n");
                break;
            case '\b':
                printf("(\\b)\n");
                break;
            default:
                printf("(%c)\n", char_i(tree, node));
        }
    }
    else
    {
        printf("1┐\n");
    }
}

[[maybe_unused]] static bool
is_leaf(struct huffman_tree const *const tree, size_t const node)
{
    struct huffman_node const *const root = node_at(tree, node);
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
        ccc_result const r = push_back(&bq->bs, bit);
        prog_assert(r == CCC_RESULT_OK);
    }
    else
    {
        ccc_tribool const was = bs_set(
            &bq->bs, ((bq->front + bq->size) % capacity(&bq->bs).count), bit);
        prog_assert(was != CCC_TRIBOOL_ERROR);
    }
    ++bq->size;
}

static ccc_tribool
bitq_pop_back(struct bitq *const bq)
{
    size_t const i = (bq->front + bq->size - 1) % capacity(&bq->bs).count;
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
    bq->front = (bq->front + 1) % size(&bq->bs).count;
    --bq->size;
    return bit;
}

static ccc_tribool
bitq_front(struct bitq const *const bq)
{
    ccc_tribool const bit = bs_test(&bq->bs, bq->front);
    prog_assert(bit != CCC_TRIBOOL_ERROR);
    return bit;
}

static ccc_tribool
bitq_test(struct bitq const *const bq, size_t const i)
{
    return bs_test(&bq->bs, (bq->front + i) % capacity(&bq->bs).count);
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
    ccc_result const r = clear_and_free(&bq->bs);
    prog_assert(r == CCC_RESULT_OK);
}

static ccc_result
bitq_reserve(struct bitq *const bq, size_t const to_add)
{
    return reserve(&bq->bs, to_add, std_alloc);
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

/*=========================     Help Message      ===========================*/

static void
print_help(void)
{
    static char const *const msg
        = "Compress and Decompress Files:\n-c=/file/name - [c]ompress the "
          "specified file to create a samples/output/name.ccc file\n"
          "-d=/samples/output/name.ccc - [d]ecompress the specified file to "
          "create a samples/output/name file\nSample "
          "Command:\n./build/bin/compress -c=README.md "
          "-d=samples/output/README.md.ccc\n";
    printf("%s", msg);
}
