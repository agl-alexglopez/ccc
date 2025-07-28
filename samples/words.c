/** The words program is a simple word counter that aims to demonstrate
the benefits of flexible memory layouts and auxiliary data.

Please specify a command as follows:
./build/[debug/]bin/words [flag] -f=[path/to/file]
[flag]:
-f=[/path/to/file]
-c reports the words by count in descending order.
-rc reports words by count in ascending order.
-top=N reports the top N words by frequency.
-last=N reports the last N words by frequency
-alph=N reports the first N words alphabetically with counts.
-ralph=N reports the last N words alphabetically with counts.
-find=[WORD] reports the count of the specified word. */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_USING_NAMESPACE_CCC
#define HANDLE_ORDERED_MAP_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC

#include "ccc/buffer.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/handle_ordered_map.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "util/alloc.h"
#include "util/cli.h"
#include "util/str_arena.h"
#include "util/str_view.h"

enum action_type
{
    COUNT,
    FIND,
};

/* A word will be counted with a map and then sorted with a buffer and flat
priority queue. One type works well for both because they don't need intrusive
elements to operate those containers. */
typedef struct
{
    struct str_ofs ofs;
    int freq;
} word;

struct action_pack
{
    str_view file;
    enum action_type type;
    union
    {
        int n;
        str_view w;
    };
    union
    {
        void (*freq_fn)(FILE *file, int n);
        void (*find_fn)(FILE *file, str_view w);
    };
};

/*=======================     Constants    ==================================*/

enum : size_t
{
    ARENA_START_CAP = 4096,
};

enum : int
{
    ALL_FREQUENCIES = 0,
};

static str_view const space = SV(" ");
static str_view const directions
    = SV("\nPlease specify a command as follows:\n"
         "./build/[debug/]bin/words [flag] -f=[path/to/file]\n"
         "[flag]:\n"
         "-f=[/path/to/file]\n"
         "-c\n"
         "\treports the words by count in descending order.\n"
         "-rc\n"
         "\treports words by count in ascending order.\n"
         "-top=N\n"
         "\treports the top N words by frequency.\n"
         "-last=N\n"
         "\treports the last N words by frequency\n"
         "-alph=N\n"
         "\treports the first N words alphabetically with counts.\n"
         "-ralph=N\n"
         "\treports the last N words alphabetically with counts.\n"
         "-find=[WORD]\n"
         "\treports the count of the specified word.\n");

/*=======================   Prototypes     ==================================*/

#define logerr(string...) (void)fprintf(stderr, string)

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

static void print_found(FILE *file, str_view w);
static void print_top_n(FILE *file, int n);
static void print_last_n(FILE *file, int n);
static void print_alpha_n(FILE *file, int n);
static void print_ralpha_n(FILE *file, int n);
static buffer copy_frequencies(handle_ordered_map const *map);
static void print_n(handle_ordered_map *, ccc_threeway_cmp, struct str_arena *,
                    int n);
static struct int_conversion parse_n_ranks(str_view arg);

/* String Helper Functions */
static struct str_ofs clean_word(struct str_arena *, str_view wv);

/* Container Functions */
static handle_ordered_map create_frequency_map(struct str_arena *, FILE *);
static threeway_cmp cmp_string_keys(any_key_cmp);
static threeway_cmp cmp_words(any_type_cmp);

/* Misc. Functions */
static FILE *open_file(str_view file);

/*=======================     Main         ==================================*/

int
main(int argc, char *argv[])
{
    if (argc == 2 && sv_starts_with(sv(argv[1]), SV("-h")))
    {
        sv_print(stdout, directions);
        return 0;
    }
    if (argc < 3)
    {
        return 0;
    }
    struct action_pack exe = {};
    for (int arg = 1; arg < argc; ++arg)
    {
        str_view const sv_arg = sv(argv[arg]);
        if (sv_starts_with(sv_arg, SV("-c")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_top_n;
            exe.n = ALL_FREQUENCIES;
        }
        else if (sv_starts_with(sv_arg, SV("-rc")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            exe.n = ALL_FREQUENCIES;
        }
        else if (sv_starts_with(sv_arg, SV("-top=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_top_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER,
                  logerr("cannot convert -top= flag to int"););
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-last=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER,
                  logerr("cannot convert -last= flat to int"););
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-alph=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_alpha_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER,
                  logerr("cannot convert -alph= flag to int"););
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-ralph=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_ralpha_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER,
                  logerr("cannot convert -ralph= flat to int"););
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-find=")))
        {
            str_view const raw_word = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            check(!sv_empty(raw_word),
                  logerr("-find= flag has invalid entry"););
            exe.type = FIND;
            exe.find_fn = print_found;
            exe.w = raw_word;
        }
        else if (sv_starts_with(sv_arg, SV("-f=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            check(!sv_empty(raw_file), logerr("file string is empty"););
            exe.file = raw_file;
        }
        else if (sv_starts_with(sv_arg, SV("-h")))
        {
            sv_print(stdout, directions);
            return 0;
        }
        else
        {
            logerr("unrecognized argument: %s\n", sv_begin(sv_arg));
            return 1;
        }
    }
    FILE *const f = open_file(exe.file);
    if (!f)
    {
        logerr("error opening: %s\n", exe.file.s);
        return 1;
    }
    if (exe.type == COUNT)
    {
        exe.freq_fn(f, exe.n);
    }
    else if (exe.type == FIND && exe.w.s)
    {
        exe.find_fn(f, exe.w);
    }
    else
    {
        logerr("invalid count or empty word searched\n");
        return 1;
    }
    (void)fclose(f);
    return 0;
}

/*=======================   Static Impl    ==================================*/

static void
print_found(FILE *const f, str_view w)
{
    struct str_arena a = str_arena_create(ARENA_START_CAP);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    struct str_ofs wc = clean_word(&a, w);
    if (!wc.error)
    {
        word const *const found_w = hom_at(&map, get_key_val(&map, &wc));
        if (found_w)
        {
            printf("%s %d\n", str_arena_at(&a, &found_w->ofs), found_w->freq);
        }
    }
    str_arena_free(&a);
    (void)hom_clear_and_free(&map, NULL);
}

static void
print_top_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(ARENA_START_CAP);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    print_n(&map, CCC_GRT, &a, n);
    str_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static void
print_last_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(ARENA_START_CAP);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    print_n(&map, CCC_LES, &a, n);
    str_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static void
print_alpha_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(ARENA_START_CAP);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    if (!n)
    {
        n = count(&map).count;
    }
    int i = 0;
    /* The ordered nature of the map comes in handy for alpha printing. */
    for (word *w = begin(&map); w != end(&map) && i < n; w = next(&map, w), ++i)
    {
        printf("%s %d\n", str_arena_at(&a, &w->ofs), w->freq);
    }
    str_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static void
print_ralpha_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(ARENA_START_CAP);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    if (!n)
    {
        n = count(&map).count;
    }
    int i = 0;
    /* The ordered nature of the map comes in handy for reverse iteration. */
    for (word *w = rbegin(&map); w != rend(&map) && i < n;
         w = rnext(&map, w), ++i)
    {
        printf("%s %d\n", str_arena_at(&a, &w->ofs), w->freq);
    }
    str_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static buffer
copy_frequencies(handle_ordered_map const *const map)
{
    check(!is_empty(map));
    buffer freqs = buf_init(NULL, word, std_alloc, NULL, 0);
    ccc_result const r = buf_reserve(&freqs, count(map).count, std_alloc);
    check(r == CCC_RESULT_OK);
    size_t const cap = capacity(&freqs).count;
    size_t i = 0;
    for (word const *w = begin(map); w != end(map) && i < cap;
         w = next(map, w), ++i)
    {
        word const *const pushed = buf_push_back(&freqs, w);
        check(pushed);
    }
    return freqs;
}

static void
print_n(ccc_handle_ordered_map *const map, ccc_threeway_cmp const ord,
        struct str_arena *const a, int n)
{
    buffer freqs = copy_frequencies(map);
    check(!buf_is_empty(&freqs));
    flat_priority_queue fpq
        = fpq_heapify_init(begin(&freqs), word, ord, cmp_words, NULL, a,
                           capacity(&freqs).count, count(&freqs).count);
    check(count(&fpq).count == count(&freqs).count);
    if (!n)
    {
        n = count(&fpq).count;
    }
    /* Because all CCC containers are complete they can be treated as copyable
       types like this. There is no opaque container in CCC. */
    freqs = ccc_fpq_heapsort(&fpq, &(word){});
    check(!fpq.buf.mem);
    int w = 0;
    /* Heap sort puts the root most nodes at the back of the buffer. */
    for (word const *i = rbegin(&freqs); i != rend(&freqs) && w < n;
         i = rnext(&freqs, i), ++w)
    {
        char const *const arena_str = str_arena_at(a, &i->ofs);
        if (arena_str)
        {
            printf("%d. %s %d\n", w + 1, arena_str, i->freq);
        }
    }
    (void)clear_and_free(&freqs, NULL);
}

/*=====================    Container Construction     =======================*/

static handle_ordered_map
create_frequency_map(struct str_arena *const a, FILE *const f)
{
    char *lineptr = NULL;
    size_t len = 0;
    ptrdiff_t read = 0;
    handle_ordered_map hom
        = hom_init(NULL, word, ofs, cmp_string_keys, std_alloc, a, 0);
    while ((read = getline(&lineptr, &len, f)) > 0)
    {
        str_view const line = {.s = lineptr, .len = read - 1};
        for (str_view word_view = sv_begin_tok(line, space);
             !sv_end_tok(line, word_view);
             word_view = sv_next_tok(line, word_view, space))
        {
            struct str_ofs const cw = clean_word(a, word_view);
            if (!cw.error)
            {
                homap_handle const *e = handle_r(&hom, &cw);
                e = hom_and_modify_w(e, word, { T->freq++; });
                word const *const w = hom_at(
                    &hom, hom_or_insert_w(e, (word){.ofs = cw, .freq = 1}));
                check(w);
            }
        }
    }
    free(lineptr);
    return hom;
}

static struct str_ofs
clean_word(struct str_arena *const a, str_view wv)
{
    /* It is hard to know how many characters will make it to a cleaned word
       and one pass is ideal so arena api allows push back on last alloc. */
    struct str_ofs str = str_arena_alloc(a, 0);
    if (str.error)
    {
        return str;
    }
    for (char const *c = sv_begin(wv); c != sv_end(wv); c = sv_next(c))
    {
        if (!isalpha(*c) && *c != '-')
        {
            str_arena_pop_str(a, &str);
            return (struct str_ofs){.error = STR_ARENA_INVALID};
        }
        enum str_arena_result const pushed_char
            = str_arena_push_back(a, &str, (char)tolower(*c));
        check(pushed_char == STR_ARENA_OK);
    }
    if (!str.len)
    {
        return (struct str_ofs){.error = STR_ARENA_INVALID};
    }
    char const *const w = str_arena_at(a, &str);
    check(w);
    if (!isalpha(*w) || !isalpha(*(w + (str.len - 1))))
    {
        enum str_arena_result const pop = str_arena_pop_str(a, &str);
        check(pop == STR_ARENA_OK);
        return (struct str_ofs){.error = STR_ARENA_INVALID};
    }
    return str;
}

/*=======================   Container Helpers    ============================*/

static threeway_cmp
cmp_string_keys(any_key_cmp const c)
{
    word const *const w = c.any_type_rhs;
    struct str_arena const *const a = c.aux;
    struct str_ofs const *const id = c.any_key_lhs;
    char const *const key_word = str_arena_at(a, id);
    char const *const struct_word = str_arena_at(a, &w->ofs);
    check(key_word && struct_word);
    int const res = strcmp(key_word, struct_word);
    return (res > 0) - (res < 0);
}

/* Sorts by frequency then alphabetic order if frequencies are tied. */
static threeway_cmp
cmp_words(any_type_cmp const c)
{
    word const *const lhs = c.any_type_lhs;
    word const *const rhs = c.any_type_rhs;
    threeway_cmp freq_cmp = (lhs->freq > rhs->freq) - (lhs->freq < rhs->freq);
    if (freq_cmp != CCC_EQL)
    {
        return freq_cmp;
    }
    struct str_arena const *const arena = c.aux;
    char const *const lhs_word = str_arena_at(arena, &lhs->ofs);
    char const *const rhs_word = str_arena_at(arena, &rhs->ofs);
    check(lhs_word && rhs_word);
    int const res = strcmp(lhs_word, rhs_word);
    /* Looks like we have chosen wrong order to return but not so: greater
       lexicographic order is sorted first in a min priority queue or CCC_LES
       in this case. */
    return (res < 0) - (res > 0);
}

/*=======================   CLI Helpers    ==================================*/

static struct int_conversion
parse_n_ranks(str_view arg)
{
    size_t const eql = sv_rfind(arg, sv_npos(arg), SV("="));
    if (eql == sv_npos(arg))
    {
        return (struct int_conversion){.status = CONV_ER};
    }
    arg = sv_substr(arg, eql + 1, ULLONG_MAX);
    if (sv_empty(arg))
    {
        (void)fprintf(stderr, "please specify word frequency range.\n");
        return (struct int_conversion){.status = CONV_ER};
    }
    return convert_to_int(sv_begin(arg));
}

static FILE *
open_file(str_view file)
{
    if (sv_empty(file))
    {
        return NULL;
    }
    FILE *const f = fopen(sv_begin(file), "r");
    if (!f)
    {
        (void)fprintf(stderr,
                      "Opening file [%s] failed with errno set to: %d\n",
                      sv_begin(file), errno);
    }
    return f;
}
