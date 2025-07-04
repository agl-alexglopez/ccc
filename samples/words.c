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

#define HANDLE_ORDERED_MAP_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#include "alloc.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/handle_ordered_map.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "cli.h"
#include "str_arena.h"
#include "str_view.h"

enum action_type
{
    COUNT,
    FIND,
};

enum word_clean_status
{
    WC_CLEAN_WORD,
    WC_NOT_WORD,
    WC_ARENA_ERR,
};

struct clean_word
{
    str_ofs str;
    enum word_clean_status stat;
};

/* Type that will serve as key val for both map and priority queue. */
struct frequency
{
    /* If arena resizes, string is not lost. This is an offset. */
    str_ofs ofs;
    /* How many times the word is encountered. Key for priority queue. */
    int freq;
};

/* Map element for logging frequencies by string key, freq value. */
typedef struct
{
    str_ofs ofs;
    int cnt;
} word;

struct frequency_alloc
{
    struct frequency *arr;
    size_t cap;
};

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

static size_t arena_start_cap = 4096;
static int const all_frequencies = 0;
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

#define check(cond, ...)                                                       \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            __VA_OPT__((void)fprintf(stderr, __VA_ARGS__);)                    \
            sv_print(stderr, directions);                                      \
            exit(1);                                                           \
        }                                                                      \
    }                                                                          \
    while (0)

#define QUIT_MSG(file, msg...)                                                 \
    do                                                                         \
    {                                                                          \
        (void)fprintf(file, msg);                                              \
        exit(1);                                                               \
    }                                                                          \
    while (0)

static void print_found(FILE *file, str_view w);
static void print_top_n(FILE *file, int n);
static void print_last_n(FILE *file, int n);
static void print_alpha_n(FILE *file, int n);
static void print_ralpha_n(FILE *file, int n);
static struct frequency_alloc copy_frequencies(handle_ordered_map const *map);
static void print_n(flat_priority_queue *, struct str_arena const *, int n);
static struct int_conversion parse_n_ranks(str_view arg);

/* String Helper Functions */
static struct clean_word clean_word(struct str_arena *, str_view wv);

/* Container Functions */
static handle_ordered_map create_frequency_map(struct str_arena *, FILE *);
static threeway_cmp cmp_string_keys(any_key_cmp);
static threeway_cmp cmp_freqs(any_type_cmp);

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
            exe.n = all_frequencies;
        }
        else if (sv_starts_with(sv_arg, SV("-rc")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            exe.n = all_frequencies;
        }
        else if (sv_starts_with(sv_arg, SV("-top=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_top_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER, "cannot convert -top= flag to int");
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-last=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER, "cannot convert -last= flat to int");
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-alph=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_alpha_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER, "cannot convert -alph= flag to int");
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-ralph=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_ralpha_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER, "cannot convert -ralph= flat to int");
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-find=")))
        {
            str_view const raw_word = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            check(!sv_empty(raw_word), "-find= flag has invalid entry");
            exe.type = FIND;
            exe.find_fn = print_found;
            exe.w = raw_word;
        }
        else if (sv_starts_with(sv_arg, SV("-f=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            check(!sv_empty(raw_file), "file string is empty");
            exe.file = raw_file;
        }
        else if (sv_starts_with(sv_arg, SV("-h")))
        {
            sv_print(stdout, directions);
            return 0;
        }
        else
        {
            QUIT_MSG(stderr, "unrecognized argument: %s\n", sv_begin(sv_arg));
        }
    }
    FILE *const f = open_file(exe.file);
    if (!f)
    {
        (void)fprintf(stderr, "error opening: %s\n", exe.file.s);
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
        (void)fprintf(stderr, "invalid count or empty word searched\n");
    }
    (void)fclose(f);
    return 0;
}

/*=======================   Static Impl    ==================================*/

static void
print_found(FILE *const f, str_view w)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    struct clean_word wc = clean_word(&a, w);
    if (wc.stat == WC_CLEAN_WORD)
    {
        word const *const found_w = hom_at(&map, get_key_val(&map, &wc.str));
        if (found_w)
        {
            printf("%s %d\n", str_arena_at(&a, found_w->ofs), found_w->cnt);
        }
    }
    str_arena_free(&a);
    (void)hom_clear_and_free(&map, NULL);
}

static void
print_top_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    /* O(n) copy */
    struct frequency_alloc freqs = copy_frequencies(&map);
    check(freqs.cap);
    /* O(n) sort kind of. Pops will be O(nlgn). But we don't have stdlib qsort
       space overhead to emulate why someone in an embedded or OS
       environment might choose this approach for sorting. Granted any O(1)
       space approach to sorting may beat the slower pop operation but strict
       O(lgN) runtime for heap pop is pretty good. */
    flat_priority_queue fpq
        = fpq_heapify_init(freqs.arr, struct frequency, CCC_GRT, cmp_freqs,
                           std_alloc, &a, freqs.cap, size(&map).count);
    check(size(&fpq).count == size(&map).count);
    if (!n)
    {
        n = size(&fpq).count;
    }
    print_n(&fpq, &a, n);
    str_arena_free(&a);
    (void)clear_and_free(&fpq, NULL);
    (void)clear_and_free(&map, NULL);
}

static void
print_last_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    struct frequency_alloc freqs = copy_frequencies(&map);
    check(freqs.cap);
    flat_priority_queue fpq
        = fpq_heapify_init(freqs.arr, struct frequency, CCC_LES, cmp_freqs,
                           std_alloc, &a, freqs.cap, size(&map).count);
    check(size(&fpq).count == size(&map).count);
    if (!n)
    {
        n = size(&fpq).count;
    }
    print_n(&fpq, &a, n);
    str_arena_free(&a);
    (void)clear_and_free(&fpq, NULL);
    (void)clear_and_free(&map, NULL);
}

static void
print_alpha_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    if (!n)
    {
        n = size(&map).count;
    }
    int i = 0;
    /* The ordered nature of the map comes in handy for alpha printing. */
    for (word *w = begin(&map); w != end(&map) && i < n; w = next(&map, w), ++i)
    {
        printf("%s %d\n", str_arena_at(&a, w->ofs), w->cnt);
    }
    str_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static void
print_ralpha_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    check(a.arena);
    handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    if (!n)
    {
        n = size(&map).count;
    }
    int i = 0;
    /* The ordered nature of the map comes in handy for reverse iteration. */
    for (word *w = rbegin(&map); w != rend(&map) && i < n;
         w = rnext(&map, w), ++i)
    {
        printf("%s %d\n", str_arena_at(&a, w->ofs), w->cnt);
    }
    str_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static struct frequency_alloc
copy_frequencies(handle_ordered_map const *const map)
{
    check(!is_empty(map));
    size_t const cap = sizeof(struct frequency) * (size(map).count + 1);
    struct frequency *const freqs = malloc(cap);
    check(freqs);
    size_t i = 0;
    for (word const *w = begin(map); w != end(map) && i < cap;
         w = next(map, w), ++i)
    {
        freqs[i].ofs = w->ofs;
        freqs[i].freq = w->cnt;
    }
    return (struct frequency_alloc){.arr = freqs, .cap = cap};
}

static void
print_n(flat_priority_queue *const fpq, struct str_arena const *const a,
        int const n)
{
    if (n <= 0)
    {
        return;
    }
    for (int w = 0; w < n && !is_empty(fpq); ++w)
    {
        struct frequency *const front_w = front(fpq);
        char const *const arena_str = str_arena_at(a, front_w->ofs);
        if (arena_str)
        {
            printf("%d. %s %d\n", w + 1, arena_str, front_w->freq);
        }
        (void)pop(fpq);
    }
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
            struct clean_word const cw = clean_word(a, word_view);
            check(cw.stat != WC_ARENA_ERR);
            if (cw.stat == WC_CLEAN_WORD)
            {
                /* There are a few idiomatic ways we could tackle this step.
                   However, this demonstrates the user of "closures" in the
                   and modify with macro. The and modify macro gives a closure
                   over the user type T if the entry is Occupied. If the entry
                   is Vacant the closure does not execute. You can uncomment
                   the below code if you prefer a different style. */
                /*
                word const *const w = hom_at(
                    &hom,
                    hom_or_insert_w(hom_and_modify_w(handle_r(&hom, &cw.str),
                                                     word, { T->cnt++; }),
                                    (word){.ofs = cw.str, .cnt = 1}));
                 */
                homap_handle *e = handle_r(&hom, &cw.str);
                e = hom_and_modify_w(e, word, { T->cnt++; });
                word const *const w = hom_at(
                    &hom, hom_or_insert_w(e, (word){.ofs = cw.str, .cnt = 1}));
                check(w);
            }
        }
    }
    free(lineptr);
    return hom;
}

static struct clean_word
clean_word(struct str_arena *const a, str_view wv)
{
    /* It is hard to know how many characters will make it to a cleaned word
       and one pass is ideal so arena api allows push back on last alloc. */
    str_ofs const str = str_arena_alloc(a, 0);
    if (str < 0)
    {
        return (struct clean_word){.stat = WC_ARENA_ERR};
    }
    size_t str_len = 0;
    for (char const *c = sv_begin(wv); c != sv_end(wv); c = sv_next(c))
    {
        if (!isalpha(*c) && *c != '-')
        {
            str_arena_free_to_pos(a, str, str_len);
            return (struct clean_word){.stat = WC_NOT_WORD};
        }
        [[maybe_unused]] bool const pushed_char
            = str_arena_push_back(a, str, str_len, (char)tolower(*c));
        check(pushed_char);
        ++str_len;
    }
    if (!str_len)
    {
        return (struct clean_word){.stat = WC_NOT_WORD};
    }
    char const *const w = str_arena_at(a, str);
    check(w);
    if (!isalpha(*w) || !isalpha(*(w + (str_len - 1))))
    {
        str_arena_free_to_pos(a, str, str_len);
        return (struct clean_word){.stat = WC_NOT_WORD};
    }
    return (struct clean_word){.str = str, .stat = WC_CLEAN_WORD};
}

/*=======================   Container Helpers    ============================*/

static threeway_cmp
cmp_string_keys(any_key_cmp const c)
{
    word const *const w = c.any_type_rhs;
    struct str_arena const *const a = c.aux;
    str_ofs const *const id = c.any_key_lhs;
    char const *const key_word = str_arena_at(a, *id);
    char const *const struct_word = str_arena_at(a, w->ofs);
    check(key_word && struct_word);
    int const res = strcmp(key_word, struct_word);
    return (res > 0) - (res < 0);
}

/* Sorts by frequency then alphabetic order if frequencies are tied. */
static threeway_cmp
cmp_freqs(any_type_cmp const c)
{
    struct frequency const *const lhs = c.any_type_lhs;
    struct frequency const *const rhs = c.any_type_rhs;
    threeway_cmp freq_cmp = (lhs->freq > rhs->freq) - (lhs->freq < rhs->freq);
    if (freq_cmp != CCC_EQL)
    {
        return freq_cmp;
    }
    struct str_arena const *const arena = c.aux;
    char const *const lhs_word = str_arena_at(arena, lhs->ofs);
    char const *const rhs_word = str_arena_at(arena, rhs->ofs);
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
