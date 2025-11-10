/** Author: Alexander G. Lopez
This file implements data compression algorithms over simple files, primarily
text files for demonstration purposes. The compression algorithm used now is
Huffman Encoding and Decoding but more methods could be added later. Such
algorithms use a wide range of data structures. */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#    include <linux/limits.h>
#    define FILESYS_MAX_PATH PATH_MAX
#endif
#ifdef __APPLE__
#    include <sys/syslimits.h>
#    define FILESYS_MAX_PATH NAME_MAX
#endif

#define BITSET_USING_NAMESPACE_CCC
#define BUFFER_USING_NAMESPACE_CCC
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "ccc/bitset.h"
#include "ccc/buffer.h"
#include "ccc/flat_hash_map.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "util/alloc.h"
#include "util/str_arena.h"
#include "util/str_view.h"

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
struct char_freq
{
    char ch;
    size_t freq;
};

enum : uint8_t
{
    /** Caching for iterative traversals uses this position as end sentinel. */
    ITER_END = 2,
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
    size_t link[ITER_END];
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
    ccc_buffer bump_arena;
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

/** The compact representation of the tree structure we will encode to a
compressed file for later reconstruction. */
struct compressed_huffman_tree
{
    /** The Pre-Order traversal of internal nodes and leaves. Every internal
    node encountered on the way down is a 1 and every leaf is a 0. */
    struct bitq tree_paths;
    /** The arena that allows us to form the dynamic string of leaves. */
    struct str_arena arena;
    /** The handle to the leaf string in the string arena. */
    struct str_ofs leaf_string;
};

enum : uint32_t
{
    /** File format "cccz" in header. */
    CCCZ_MAGIC = 0x6363637A,
};

/** The header representing the compressed file structure of a compressed file
via Huffman Encoding. The fields of this struct should be handled in order as
we write to or read from a file. It is not strictly necessary to use this but
it helps keep logic consistent across compression and decompression. */
struct huffman_encoding
{
    /** Magic to recognize our cccz files "cccz" */
    uint32_t magic;
    /** The number of leaves in the tree minus 1. Subtract 1 in case we actually
        have all 256 characters used as leaves in which case they could only
        be recorded in the file as 255, the number that fits in uint8_t. */
    uint8_t leaves_minus_one;
    /** Number of bits in the file content bit queue we encoded. */
    size_t file_bits_count;
    /** The compact representation of the tree to be reconstructed later. */
    struct compressed_huffman_tree blueprint;
    /** The path to every character encountered in the file text in order. */
    struct bitq file_bits;
};

/** Files the user wants zipped or unzipped. */
struct ccczip_actions
{
    str_view zip;
    str_view unzip;
};

static str_view const output_dir = SV("samples/output/");
static str_view const cccz_suffix = SV(".cccz");

/*===========================      Prototypes      ==========================*/

static void zip_file(str_view to_compress);
static flat_priority_queue build_encoding_pq(FILE *f, struct huffman_tree *);
static void bitq_push_back(struct bitq *, ccc_tribool);
static ccc_tribool bitq_pop_back(struct bitq *bq);
static ccc_tribool bitq_pop_front(struct bitq *);
static ccc_tribool bitq_test(struct bitq const *, size_t i);
static size_t bitq_count(struct bitq const *bq);
static void bitq_clear_and_free(struct bitq *);
static ccc_result bitq_reserve(struct bitq *, size_t to_add);
static uint64_t hash_char(ccc_any_key to_hash);
static ccc_threeway_cmp char_eq(ccc_any_key_cmp);
static ccc_threeway_cmp cmp_freqs(ccc_any_type_cmp cmp);
static ccc_threeway_cmp path_memo_cmp(ccc_any_key_cmp cmp);
static void memoize_path(struct huffman_tree *tree, flat_hash_map *fh,
                         struct bitq *, char c);
static struct bitq build_encoding_bitq(FILE *f, struct huffman_tree *tree);
static struct huffman_tree build_encoding_tree(FILE *f);
static struct compressed_huffman_tree compress_tree(struct huffman_tree *tree);
static void free_encode_tree(struct huffman_tree *);
static void print_tree(struct huffman_tree const *tree, size_t node);
static void print_inner_tree(struct huffman_tree const *tree, size_t node,
                             enum print_branch, char const *prefix);
static void print_node(struct huffman_tree const *tree, size_t node);
static bool is_leaf(struct huffman_tree const *tree, size_t node);
static void print_bitq(struct bitq const *bq);
static void unzip_file(str_view unzip);
static struct huffman_tree
reconstruct_tree(struct compressed_huffman_tree *blueprint);
static void reconstruct_text(FILE *f, struct huffman_tree const *,
                             struct bitq *);
static void print_help(void);
static size_t branch_i(struct huffman_tree const *t, size_t node, uint8_t dir);
static size_t parent_i(struct huffman_tree const *t, size_t node);
static char char_i(struct huffman_tree const *t, size_t node);
static struct huffman_node *node_at(struct huffman_tree const *t, size_t node);
static void write_to_file(str_view original_filepath, size_t original_filesize,
                          struct huffman_encoding *);
static void write_bitq(FILE *cccz, struct bitq *bq);
static struct huffman_encoding read_from_file(str_view unzip);
static size_t readbytes(FILE *f, void *base, size_t to_read);
static size_t writebytes(FILE *f, void const *base, size_t to_write);
static void fill_bitq(FILE *f, struct bitq *bq, size_t expected_bits);
static size_t file_size(FILE *f);

/** Asserts even in release mode. Run code in the second argument if needed. */
#define check(cond, ...)                                                       \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            __VA_OPT__(__VA_ARGS__)                                            \
            (void)fprintf(stderr, "%s, %d, condition is false: %s\n",          \
                          __FILE__, __LINE__, #cond);                          \
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
        check(fseek(file_ptr, 0L, SEEK_SET) >= 0);                             \
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
    struct ccczip_actions todo = {};
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
            check(!sv_empty(raw_file));
            todo.zip = raw_file;
        }
        else if (sv_starts_with(sv_arg, SV("-d=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            check(!sv_empty(raw_file));
            todo.unzip = raw_file;
        }
    }
    if (!sv_empty(todo.zip))
    {
        zip_file(todo.zip);
    }
    if (!sv_empty(todo.unzip))
    {
        unzip_file(todo.unzip);
    }
    return 0;
}

/*=========================     Huffman Encoding    =========================*/

/** Zips the requested file via Huffman Encoding into the output directory. The
compressed file has a header that can be used to reconstruct the data. */
void
zip_file(str_view const to_compress)
{
    FILE *const f = fopen(sv_begin(to_compress), "r");
    check(f, printf("%s", strerror(errno)););
    size_t const fsize = file_size(f);
    printf("Zip %s (%zu bytes).\n", sv_begin(to_compress), fsize);
    struct huffman_tree tree = build_encoding_tree(f);
    struct huffman_encoding encoding = {
        .magic = CCCZ_MAGIC,
        .file_bits = build_encoding_bitq(f, &tree),
        .blueprint = compress_tree(&tree),
    };
    encoding.leaves_minus_one = encoding.blueprint.leaf_string.len - 1,
    encoding.file_bits_count = bitq_count(&encoding.file_bits),
    write_to_file(to_compress, fsize, &encoding);
    free_encode_tree(&tree);
    bitq_clear_and_free(&encoding.file_bits);
    bitq_clear_and_free(&encoding.blueprint.tree_paths);
    str_arena_free(&encoding.blueprint.arena);
    (void)fclose(f);
}

/** Builds the Huffman Encoding tree that will allow us to encode file bytes
and later compress the tree structure. The tree is formed by pairing the two
least frequently occurring characters at a new root that is the sum of their
frequencies. This process takes O(log(N)) iterations because we pair nodes
at each step.

Because the priority queue is a min queue this means that high frequency
elements will be paired later in the algorithm and thus closer to the root of
the encoding tree. */
static struct huffman_tree
build_encoding_tree(FILE *const f)
{
    struct huffman_tree ret = {
        .bump_arena = buf_init(NULL, struct huffman_node, std_alloc, NULL, 0),
        .root = 0,
    };
    flat_priority_queue pq = build_encoding_pq(f, &ret);
    while (count(&pq).count >= 2)
    {
        /* Small elements and we need the pair so we can't hold references. */
        struct fpq_elem zero = *(struct fpq_elem *)front(&pq);
        ccc_result r = pop(&pq, &(struct fpq_elem){});
        check(r == CCC_RESULT_OK);
        struct fpq_elem one = *(struct fpq_elem *)front(&pq);
        r = pop(&pq, &(struct fpq_elem){});
        check(r == CCC_RESULT_OK);
        struct huffman_node *const internal_one
            = push_back(&ret.bump_arena,
                        &(struct huffman_node){.link = {zero.node, one.node}});
        size_t const new_root = buf_i(&ret.bump_arena, internal_one).count;
        check(internal_one);
        node_at(&ret, zero.node)->parent = new_root;
        node_at(&ret, one.node)->parent = new_root;
        struct fpq_elem const *const pushed = push(
            &pq,
            &(struct fpq_elem){.freq = zero.freq + one.freq, .node = new_root},
            &(struct fpq_elem){});
        check(pushed);
        ret.root = new_root;
    }
    /* The flat pq was given no allocation permission because the memory it
       needs was already allocated by the buffer it stole from. The priority
       queue only gets smaller as the algorithms progresses so we didn't need
       to worry about growth. Provide same function used to reserve buffer. */
    ccc_result const r = clear_and_free_reserve(&pq, NULL, std_alloc);
    check(r == CCC_RESULT_OK);
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
    flat_hash_map frequencies = fhm_init(NULL, struct char_freq, ch, hash_char,
                                         char_eq, std_alloc, NULL, 0);
    foreach_filechar(f, c, {
        struct char_freq *const ins
            = or_insert(fhm_and_modify_w(entry_r(&frequencies, c),
                                         struct char_freq, ++T->freq;),
                        &(struct char_freq){.ch = *c, .freq = 1});
        check(ins);
    });
    size_t const leaves = count(&frequencies).count;
    check(leaves >= 2);
    tree->num_leaves = leaves;
    tree->num_nodes = (2 * leaves) - 1;
    ccc_result r = reserve(&tree->bump_arena, tree->num_nodes + 1, std_alloc);
    check(r == CCC_RESULT_OK);
    /* For a buffer based tree 0 is the NULL node so we can't have actual data
       we want at that index in the tree. */
    struct huffman_node const *const nil
        = push_back(&tree->bump_arena, &(struct huffman_node){});
    check(nil);
    /* Use a buffer to simply push back elements we will heapify at the end. */
    buffer fpq_storage = buf_init(NULL, struct fpq_elem, NULL, NULL, 0);
    /* Add one to reservation for the flat priority queue swap slot. */
    r = reserve(&fpq_storage, count(&frequencies).count, std_alloc);
    check(r == CCC_RESULT_OK);
    for (struct char_freq const *i = begin(&frequencies);
         i != end(&frequencies); i = next(&frequencies, i))
    {
        struct huffman_node const *const node
            = push_back(&tree->bump_arena, &(struct huffman_node){.ch = i->ch});
        check(node);
        size_t const node_i = buf_i(&tree->bump_arena, node).count;
        struct fpq_elem const *const pushed = push_back(
            &fpq_storage, &(struct fpq_elem){.freq = i->freq, .node = node_i});
        check(pushed);
    }
    /* Free map but not the buffer because the priority queue took buffer. */
    r = clear_and_free(&frequencies, NULL);
    check(r == CCC_RESULT_OK);
    /* The buffer had no allocation permission and set up all the elements we
       needed to be in the flat priority queue. Now we take its memory and
       heapify the data in O(N) time rather than pushing each element. */
    return fpq_heapify_init(begin(&fpq_storage), struct fpq_elem, CCC_LES,
                            cmp_freqs, NULL, NULL, capacity(&fpq_storage).count,
                            count(&fpq_storage).count);
}

/** Returns the bit queue representing the bit path to every character in the
file. This queue represents each byte of the file in order; the first path in
at the front of the queue represents the first character. */
static struct bitq
build_encoding_bitq(FILE *const f, struct huffman_tree *const tree)
{
    struct bitq ret = {.bs = bs_init(NULL, std_alloc, NULL, 0)};
    /* By memoizing known bit sequences we can save significant time by not
       performing a DFS over the tree. This is especially helpful for large
       alphabets aka trees with many leaves. An array of 0-256 elements as a map
       could achieve the same result faster but it would waste much more space.
       It is rare to have a file use all 256 possible character values. */
    flat_hash_map memo = ccc_fhm_init(NULL, struct path_memo, ch, hash_char,
                                      path_memo_cmp, NULL, NULL, 0);
    ccc_result r = reserve(&memo, tree->num_leaves, std_alloc);
    check(r == CCC_RESULT_OK);
    foreach_filechar(f, c, {
        struct path_memo const *path = get_key_val(&memo, c);
        if (path)
        {
            size_t const end = path->path_start_index + path->path_len;
            for (size_t i = path->path_start_index; i < end; ++i)
            {
                ccc_tribool const bit = bitq_test(&ret, i);
                check(bit != CCC_TRIBOOL_ERROR);
                bitq_push_back(&ret, bit);
            }
        }
        else
        {
            memoize_path(tree, &memo, &ret, *c);
        }
    });
    r = clear_and_free_reserve(&memo, NULL, std_alloc);
    check(r == CCC_RESULT_OK);
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
        &(struct path_memo){.ch = c, .path_start_index = bitq_count(bq)});
    check(path);
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
        check(node->iter <= CCC_TRUE);
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
    path->path_len = bitq_count(bq) - path->path_start_index;
}

/** Compresses the Huffman tree by creating its representation as a Pre-Order
traversal of the tree. The traversal is entered into a bit queue. We also push
the leaves to a string as they are encountered during this traversal. By the
end of the operation, we have a bit queue of our traversal where every internal
node encountered on the way down is a 1 and every leaf is a 0. We also have a
string of leaf characters that we encountered in order. */
static struct compressed_huffman_tree
compress_tree(struct huffman_tree *const tree)
{
    struct compressed_huffman_tree ret = {
        .tree_paths = {.bs = bs_init(NULL, std_alloc, NULL, 0)},
        .arena = str_arena_create(START_STR_ARENA_CAP),
    };
    check(ret.arena.arena);
    ret.leaf_string = str_arena_alloc(&ret.arena, 0);
    ccc_result const r = bitq_reserve(&ret.tree_paths, tree->num_nodes);
    check(r == CCC_RESULT_OK);
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
            check(str_arena_push_back(&ret.arena, &ret.leaf_string, node->ch)
                  == STR_ARENA_OK);
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
    return ret;
}

/** Writes all encoded information to a file with the help of a header for
later file reconstruction. */
static void
write_to_file(str_view const original_filepath, size_t const original_filesize,
              struct huffman_encoding *const header)
{
    /* We write all new files to output directory so create the new path. */
    char path_to_cccz[FILESYS_MAX_PATH];
    size_t const dir_delim
        = sv_rfind(original_filepath, sv_len(original_filepath), SV("/"));
    str_view const raw_file = dir_delim == sv_npos(original_filepath)
                                ? original_filepath
                                : sv_substr(original_filepath, dir_delim + 1,
                                            sv_len(original_filepath));
    size_t const total_bytes
        = sv_size(output_dir) + sv_size(raw_file) + sv_size(cccz_suffix);
    check(total_bytes < FILESYS_MAX_PATH);
    size_t const path_bytes
        = sv_fill(sv_size(output_dir), path_to_cccz, output_dir);
    size_t const file_bytes
        = sv_fill(sv_size(raw_file), path_to_cccz + (path_bytes - 1), raw_file);
    (void)sv_fill(sv_size(cccz_suffix),
                  path_to_cccz + (path_bytes - 1) + (file_bytes - 1),
                  cccz_suffix);

    /* Path is now correct and verified try to create file. */
    FILE *const cccz = fopen(path_to_cccz, "w");
    check(cccz, (void)fprintf(stderr, "%s", strerror(errno)););

    /* When writing bytes we need every bit to make it so use many checks. */
    size_t writ = writebytes(cccz, &header->magic, sizeof(header->magic));
    check(writ == sizeof(header->magic));
    writ = writebytes(cccz, &header->leaves_minus_one,
                      sizeof(header->leaves_minus_one));
    check(writ == sizeof(header->leaves_minus_one));
    char const *leaf_string = str_arena_at(&header->blueprint.arena,
                                           &header->blueprint.leaf_string);
    writ = writebytes(cccz, leaf_string, header->leaves_minus_one + 1);
    check(writ == (size_t)(header->leaves_minus_one + 1));
    writ = writebytes(cccz, &header->file_bits_count,
                      sizeof(header->file_bits_count));
    check(writ == sizeof(header->file_bits_count));
    write_bitq(cccz, &header->blueprint.tree_paths);
    write_bitq(cccz, &header->file_bits);

    size_t const cccz_size = file_size(cccz);
    printf("Zipped file %s has compression ratio of %.2lf%% (%zu bytes).\n",
           path_to_cccz,
           (100.0 * (double)cccz_size) / (double)(original_filesize),
           cccz_size);
    /* This file now lives in the output/ as long as user desires. */
    (void)fclose(cccz);
}

/** Writes a queue of bits to a file. Some bits may be unused in the last byte
written to the file but this should not be a problem because the header can
tell us the number of tree bits and file text bits we write to the file. The
information written to file is on a per byte basis where every bit of the byte
represents a bit in the bit queue. */
static void
write_bitq(FILE *const cccz, struct bitq *const bq)
{
    static_assert(sizeof(uint8_t) == sizeof(ccc_tribool));
    uint8_t buf = 0;
    uint8_t i = 0;
    while (bitq_count(bq))
    {
        buf |= (bitq_pop_front(bq) << i);
        ++i;
        if (i >= CHAR_BIT)
        {
            size_t const byte = writebytes(cccz, &buf, sizeof(buf));
            check(byte);
            buf = 0;
            i = 0;
        }
    }
    if (i)
    {
        size_t const byte = writebytes(cccz, &buf, sizeof(buf));
        check(byte);
    }
}

/** Write the specified bytes to the file or exit the program if an error
occurs. Returns the number of bytes written. */
static size_t
writebytes(FILE *const f, void const *const base, size_t const to_write)
{
    size_t count = 0;
    unsigned char const *p = base;
    while (count < to_write)
    {
        int const writ = fputc(*p, f);
        check(!ferror(f));
        if (writ == EOF)
        {
            return count;
        }
        ++p;
        ++count;
    }
    return count;
}

/*=========================     Huffman Decoding    =========================*/

/** Unzips the requested file. The file must end in the .cccz suffix. This file
is reconstructed and a copy of the original text is written to the output
directory as a new file with the same name. */
static void
unzip_file(str_view unzip)
{
    /* First we verify the compressed file is correct before creating new. */
    struct huffman_encoding he = read_from_file(unzip);
    struct huffman_tree tree = reconstruct_tree(&he.blueprint);

    /* This means the compressed file was valid so write a new one to output. */
    char path[FILESYS_MAX_PATH];
    ccc_tribool has_suf = sv_ends_with(unzip, cccz_suffix);
    check(has_suf);
    unzip = sv_remove_suffix(unzip, sv_len(cccz_suffix));
    size_t const dir_delim = sv_rfind(unzip, sv_len(unzip), SV("/"));
    str_view const raw_file
        = dir_delim == sv_npos(unzip)
            ? unzip
            : sv_substr(unzip, dir_delim + 1, sv_len(unzip));
    size_t const total_bytes = sv_size(output_dir) + sv_size(raw_file);
    check(total_bytes < FILESYS_MAX_PATH);
    size_t const prefix = sv_fill(sv_size(output_dir), path, output_dir);
    (void)sv_fill(sv_size(raw_file), path + (prefix - 1), raw_file);

    /* Checks are good and path is set this will be a fresh copy. */
    FILE *const copy_of_original = fopen(path, "w");
    check(copy_of_original, (void)fprintf(stderr, "%s", strerror(errno)););
    reconstruct_text(copy_of_original, &tree, &he.file_bits);

    /* All info is on disk we can free in memory resources. */
    free_encode_tree(&tree);
    bitq_clear_and_free(&he.file_bits);
    bitq_clear_and_free(&he.blueprint.tree_paths);
    str_arena_free(&he.blueprint.arena);

    printf("Unzipped %s (%zu bytes).\n", path, file_size(copy_of_original));
    /* Copy is now in output/ original file remains untouched. */
    (void)fclose(copy_of_original);
}

/** Reconstructs the Huffman encoding queues from the header of the specified
file. Once complete this function returns all information needed to reconstruct
the tree and write out a copy of the original file to the output directory. */
static struct huffman_encoding
read_from_file(str_view const unzip)
{
    ccc_tribool has_suffix = sv_ends_with(unzip, SV(".cccz"));
    check(has_suffix);
    FILE *const cccz = fopen(sv_begin(unzip), "r");
    check(cccz, (void)fprintf(stderr, "%s", strerror(errno)););
    printf("Unzip %s (%zu bytes).\n", sv_begin(unzip), file_size(cccz));
    struct huffman_encoding ret = {
        .file_bits = {.bs = bs_init(NULL, std_alloc, NULL, 0)},
        .blueprint = {
            .arena = str_arena_create(START_STR_ARENA_CAP),
            .tree_paths = {.bs = bs_init(NULL, std_alloc, NULL, 0)},
        },
    };
    size_t read = readbytes(cccz, &ret.magic, sizeof(ret.magic));
    check(read == sizeof(ret.magic) && ret.magic == CCCZ_MAGIC);
    read = readbytes(cccz, &ret.leaves_minus_one, sizeof(ret.leaves_minus_one));
    check(read == sizeof(ret.leaves_minus_one));
    struct str_arena *const arena = &ret.blueprint.arena;
    struct str_ofs *const leaves = &ret.blueprint.leaf_string;
    /* Add 2: one for being minus 1 already and one for NULL terminator. */
    *leaves = str_arena_alloc(arena, ret.leaves_minus_one + 2);
    check(!leaves->error);
    read = readbytes(cccz, str_arena_at(arena, leaves), leaves->len);
    check(read == (size_t)(ret.leaves_minus_one + 1));
    read = readbytes(cccz, &ret.file_bits_count, sizeof(ret.file_bits_count));
    check(read == sizeof(ret.file_bits_count));
    /* The pairing method we used while building the tree makes this true. */
    size_t const tree_path_bits = (leaves->len * 2) - 1;
    fill_bitq(cccz, &ret.blueprint.tree_paths, tree_path_bits);
    fill_bitq(cccz, &ret.file_bits, ret.file_bits_count);

    (void)fclose(cccz);
    return ret;
}

/** Reconstructs a Huffman encoding tree based on the blueprint provided. The
tree is constructed in linear time and constant space additional to the nodes
being allocated. */
static struct huffman_tree
reconstruct_tree(struct compressed_huffman_tree *const blueprint)
{
    struct huffman_tree ret = {
        .bump_arena
        /* 0 index is NULL so real data can't be there. */
        = ccc_buf_from(std_alloc, NULL,
                       (struct huffman_node[]){
                           {}, // nil
                           {}, // root
                       }),
        /* By creating the root outside of the main loop we can be sure we
           always have valid prev node. Don't need to check on every loop
           iteration. */
        .root = 1,
        .num_nodes = bitq_count(&blueprint->tree_paths),
    };
    ccc_result r = reserve(&ret.bump_arena, ret.num_nodes + 1, std_alloc);
    check(r == CCC_RESULT_OK);
    (void)bitq_pop_front(&blueprint->tree_paths);
    size_t parent = ret.root;
    size_t node = 0;
    char const *leaves
        = str_arena_at(&blueprint->arena, &blueprint->leaf_string);
    while (bitq_count(&blueprint->tree_paths))
    {
        ccc_tribool bit = CCC_TRUE;
        if (!node)
        {
            struct huffman_node *const parent_r = node_at(&ret, parent);
            bit = bitq_pop_front(&blueprint->tree_paths);
            struct huffman_node *const pushed = push_back(
                &ret.bump_arena, &(struct huffman_node){.parent = parent});
            node = ccc_buf_i(&ret.bump_arena, pushed).count;
            parent_r->link[parent_r->iter++] = node;
            if (!bit)
            {
                pushed->ch = *leaves;
                ++leaves;
                ++ret.num_leaves;
            }
        }
        struct huffman_node *const node_r = node_at(&ret, node);
        /* An internal node has further child subtrees to build. */
        if (bit && node_r->iter < ITER_END)
        {
            parent = node;
            node = node_r->link[node_r->iter];
            continue;
        }
        /* Backtrack. A leaf or internal node with both children built. */
        node = parent;
        parent = parent_i(&ret, parent);
    }
    return ret;
}

/** Reconstructs the text by following the bit paths specified in the file text
bit queue. Bits are written on a per byte basis to the file and each bit of
the byte corresponds to a bit in the bit queue. */
void
reconstruct_text(FILE *const f, struct huffman_tree const *const tree,
                 struct bitq *const bq)
{
    size_t cur = tree->root;
    while (bitq_count(bq))
    {
        /* All paths started from the root during encoding and chose a direction
           first so popping is OK here. Root 1 node never was pushed to q. */
        ccc_tribool const bit = bitq_pop_front(bq);
        check(bit != CCC_TRIBOOL_ERROR);
        cur = branch_i(tree, cur, bit);
        if (!branch_i(tree, cur, 1))
        {
            char const c = char_i(tree, cur);
            size_t const byte = writebytes(f, &c, sizeof(c));
            check(byte);
            cur = tree->root;
        }
    }
}

/** Fills a bit queue from a file given an expected number of bits. The bits
are read in on a per byte basis where every bit of a file byte represents a bit
in the bit queue. Exits if not enough bits are found in the file. */
static void
fill_bitq(FILE *const f, struct bitq *const bq, size_t expected_bits)
{
    uint8_t buf = 0;
    uint8_t i = CHAR_BIT;
    while (expected_bits--)
    {
        if (i == CHAR_BIT)
        {
            size_t const byte = readbytes(f, &buf, sizeof(buf));
            check(byte);
            i = 0;
        }
        static_assert(sizeof(ccc_tribool) == sizeof(uint8_t));
        ccc_tribool const bit = (buf & ((uint8_t)1 << i)) != 0;
        bitq_push_back(bq, bit);
        ++i;
    }
}

/** Reads the specified number of bytes from the file or exits if an error
occurs. The bytes read are returned and may be less than requested if the file
ends. */
static size_t
readbytes(FILE *const f, void *const base, size_t const to_read)
{
    size_t count = 0;
    unsigned char *p = (unsigned char *)base;
    while (count < to_read)
    {
        int const read = fgetc(f);
        check(!ferror(f));
        if (read == EOF)
        {
            return count;
        }
        *p++ = (unsigned char)read;
        ++count;
    }
    return count;
}

/*=========================      Huffman Helpers    =========================*/

static struct huffman_node *
node_at(struct huffman_tree const *const t, size_t const node)
{
    return ((struct huffman_node *)buf_at(&t->bump_arena, node));
}

static size_t
branch_i(struct huffman_tree const *const t, size_t const node,
         uint8_t const dir)
{
    return ((struct huffman_node *)buf_at(&t->bump_arena, node))->link[dir];
}

static size_t
parent_i(struct huffman_tree const *const t, size_t node)
{
    return ((struct huffman_node *)buf_at(&t->bump_arena, node))->parent;
}

static char
char_i(struct huffman_tree const *const t, size_t const node)
{
    return ((struct huffman_node *)buf_at(&t->bump_arena, node))->ch;
}

/** Frees all encoding nodes from the tree provided. */
static void
free_encode_tree(struct huffman_tree *tree)
{
    ccc_result const r
        = clear_and_free_reserve(&tree->bump_arena, NULL, std_alloc);
    check(r == CCC_RESULT_OK);
    *tree = (struct huffman_tree){};
}

static size_t
file_size(FILE *f)
{
    check(fseek(f, 0L, SEEK_END) >= 0);
    size_t const ret = ftell(f);
    check(fseek(f, 0L, SEEK_SET) >= 0);
    return ret;
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
    check(str);
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
    for (size_t i = 0, col = 1, end = bitq_count(bq); i < end; ++i, ++col)
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

/*=====================       Bit Queue Helper Code     =====================*/

static void
bitq_push_back(struct bitq *const bq, ccc_tribool const bit)
{
    if (bq->size == bs_count(&bq->bs).count)
    {
        ccc_result const r = push_back(&bq->bs, bit);
        check(r == CCC_RESULT_OK);
    }
    else
    {
        ccc_tribool const was = bs_set(
            &bq->bs, (bq->front + bq->size) % capacity(&bq->bs).count, bit);
        check(was != CCC_TRIBOOL_ERROR);
    }
    ++bq->size;
}

static ccc_tribool
bitq_pop_back(struct bitq *const bq)
{
    if (!bq->size)
    {
        return CCC_TRIBOOL_ERROR;
    }
    size_t const i = (bq->front + bq->size - 1) % capacity(&bq->bs).count;
    ccc_tribool const bit = bs_test(&bq->bs, i);
    check(bit != CCC_TRIBOOL_ERROR);
    --bq->size;
    return bit;
}

static ccc_tribool
bitq_pop_front(struct bitq *const bq)
{
    if (!bq->size)
    {
        return CCC_TRIBOOL_ERROR;
    }
    ccc_tribool const bit = bs_test(&bq->bs, bq->front);
    check(bit != CCC_TRIBOOL_ERROR);
    bq->front = (bq->front + 1) % count(&bq->bs).count;
    --bq->size;
    return bit;
}

static ccc_tribool
bitq_test(struct bitq const *const bq, size_t const i)
{
    if (!bq->size)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return bs_test(&bq->bs, (bq->front + i) % capacity(&bq->bs).count);
}

static size_t
bitq_count(struct bitq const *const bq)
{
    return bq->size;
}

static void
bitq_clear_and_free(struct bitq *const bq)
{
    check(bq);
    ccc_result const r = clear_and_free(&bq->bs);
    check(r == CCC_RESULT_OK);
}

static ccc_result
bitq_reserve(struct bitq *const bq, size_t const to_add)
{
    return reserve(&bq->bs, to_add, std_alloc);
}

/*=====================       Container Helper Code     =====================*/

/** The flat hash map documentation mentions we should have good bit diversity
in the high and low byte of our hash but we are only hashing characters which
are their own unique value across all characters we will encounter. So the
hashed value will just be the character repeated at the high and low byte. */
static uint64_t
hash_char(ccc_any_key const to_hash)
{
    char const key = *(char *)to_hash.any_key;
    return ((uint64_t)key << 55) | key;
}

static ccc_threeway_cmp
char_eq(ccc_any_key_cmp const cmp)
{
    struct char_freq const *const rhs = (struct char_freq *)cmp.any_type_rhs;
    char const lhs = *(char *)cmp.any_key_lhs;
    return (lhs > rhs->ch) - (lhs < rhs->ch);
}

static ccc_threeway_cmp
cmp_freqs(ccc_any_type_cmp const cmp)
{
    struct fpq_elem const *const lhs = (struct fpq_elem *)cmp.any_type_lhs;
    struct fpq_elem const *const rhs = (struct fpq_elem *)cmp.any_type_rhs;
    return (lhs->freq > rhs->freq) - (lhs->freq < rhs->freq);
}

static ccc_threeway_cmp
path_memo_cmp(ccc_any_key_cmp const cmp)
{
    struct path_memo const *const rhs = (struct path_memo *)cmp.any_type_rhs;
    char const lhs = *(char *)cmp.any_key_lhs;
    return (lhs > rhs->ch) - (lhs < rhs->ch);
}

/*=========================     Help Message      ===========================*/

static void
print_help(void)
{
    static char const *const msg
        = "Compress and Decompress Files:\n\n\t-c=/file/name - [c]ompress the "
          "file to create a samples/output/name.ccc file\n\t"
          "-d=/samples/output/name.ccc - [d]ecompress the file to "
          "create a samples/output/name file\n\nNote: Compression comes before "
          "decompression.\nThe following command compresses a file and then "
          "decompresses it.\nThe final copy of the original file is in the "
          "output directory.\nSample Command:\n./build/bin/ccczip "
          "-c=README.md -d=samples/output/README.md.ccc\n";
    printf("%s", msg);
}
