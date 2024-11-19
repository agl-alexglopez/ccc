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
-find=[WORD] reports the count of the specified word. */
#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_ORDERED_MAP_USING_NAMESPACE_CCC

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "ccc/flat_ordered_map.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "cli.h"
#include "str_view.h"

typedef ptrdiff_t str_ofs;

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

struct str_arena
{
    char *arena;
    size_t next_free_pos;
    size_t cap;
};

/* Type that will serve as key val for both map and priority queue. */
struct frequency
{
    /* If arena resizes, string is not lost. This is an offset. */
    str_ofs ofs;
    /* How many times the word is encountered. Key for priority queue. */
    int freq;
};

typedef struct
{
    str_ofs ofs;
    int cnt;
    /* Map element for logging frequencies by string key, freq value. */
    fomap_elem e;
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
         "-find=[WORD]\n"
         "\treports the count of the specified word.\n");

/*=======================   Prototypes     ==================================*/

#define PROG_ASSERT(cond, ...)                                                 \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            __VA_OPT__((void)fprintf(stderr, __VA_ARGS__);)                    \
            sv_print(stderr, directions);                                      \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define QUIT_MSG(file, msg...)                                                 \
    do                                                                         \
    {                                                                          \
        (void)fprintf(file, msg);                                              \
        exit(1);                                                               \
    } while (0)

static void print_found(FILE *file, str_view w);
static void print_top_n(FILE *file, int n);
static void print_last_n(FILE *file, int n);
static struct frequency_alloc copy_frequencies(ccc_flat_ordered_map const *map);
static void print_n(ccc_flat_priority_queue *, struct str_arena const *, int n);
static struct int_conversion parse_n_ranks(str_view arg);

/* String Helper Functions */
static struct clean_word clean_word(struct str_arena *, str_view wv);

/* String Arena Functions */
static struct str_arena str_arena_create(size_t);
static ptrdiff_t str_arena_alloc(struct str_arena *, size_t bytes);
static ccc_result str_arena_maybe_resize(struct str_arena *,
                                         size_t byte_request);
static ccc_result str_arena_maybe_resize_pos(struct str_arena *,
                                             size_t furthest_pos);
static void str_arena_free_to_pos(struct str_arena *, str_ofs last_str,
                                  size_t str_len);
static bool str_arena_push_back(struct str_arena *, str_ofs str, size_t len,
                                char);
static void str_arena_free(struct str_arena *);
static char *str_arena_at(struct str_arena const *, str_ofs);

/* Container Functions */
static ccc_flat_ordered_map create_frequency_map(struct str_arena *, FILE *);
static ccc_threeway_cmp cmp_string_keys(ccc_key_cmp);
static ccc_threeway_cmp cmp_freqs(ccc_cmp);

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
            PROG_ASSERT(c.status != CONV_ER,
                        "cannot convert -top= flag to int");
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-last=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            PROG_ASSERT(c.status != CONV_ER,
                        "cannot convert -last= flat to int");
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-find=")))
        {
            str_view const raw_word = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            PROG_ASSERT(!sv_empty(raw_word), "-find= flag has invalid entry");
            exe.type = FIND;
            exe.find_fn = print_found;
            exe.w = raw_word;
        }
        else if (sv_starts_with(sv_arg, SV("-f=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            PROG_ASSERT(!sv_empty(raw_file), "file string is empty");
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
    PROG_ASSERT(a.arena);
    ccc_flat_ordered_map map = create_frequency_map(&a, f);
    PROG_ASSERT(!is_empty(&map));
    struct clean_word wc = clean_word(&a, w);
    if (wc.stat == WC_CLEAN_WORD)
    {
        word const *const found_w = get_key_val(&map, &wc.str);
        if (found_w)
        {
            printf("%s %d\n", str_arena_at(&a, found_w->ofs), found_w->cnt);
        }
    }
    str_arena_free(&a);
    (void)ccc_fom_clear_and_free(&map, NULL);
}

static void
print_top_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    PROG_ASSERT(a.arena);
    ccc_flat_ordered_map map = create_frequency_map(&a, f);
    PROG_ASSERT(!is_empty(&map));
    /* O(n) copy */
    struct frequency_alloc freqs = copy_frequencies(&map);
    PROG_ASSERT(freqs.cap);
    /* O(n) sort kind of. Pops will be O(nlgn). But we don't have stdlib qsort
       space overhead to emulate why someone in an embedded or OS
       environment might choose this approach for sorting. Granted any O(1)
       space approach to sorting may beat the slower pop operation but strict
       O(lgN) runtime for heap pop is pretty good. */
    ccc_flat_priority_queue fpq = ccc_fpq_heapify_init(
        freqs.arr, freqs.cap, size(&map), CCC_GRT, std_alloc, cmp_freqs, &a);
    PROG_ASSERT(size(&fpq) == size(&map));
    if (!n)
    {
        n = size(&fpq);
    }
    print_n(&fpq, &a, n);
    str_arena_free(&a);
    (void)ccc_fpq_clear_and_free(&fpq, NULL);
    (void)ccc_fom_clear_and_free(&map, NULL);
}

static void
print_last_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    PROG_ASSERT(a.arena);
    ccc_flat_ordered_map map = create_frequency_map(&a, f);
    PROG_ASSERT(!is_empty(&map));
    struct frequency_alloc freqs = copy_frequencies(&map);
    PROG_ASSERT(freqs.cap);
    ccc_flat_priority_queue fpq = ccc_fpq_heapify_init(
        freqs.arr, freqs.cap, size(&map), CCC_LES, std_alloc, cmp_freqs, &a);
    PROG_ASSERT(size(&fpq) == size(&map));
    if (!n)
    {
        n = size(&fpq);
    }
    print_n(&fpq, &a, n);
    str_arena_free(&a);
    (void)ccc_fpq_clear_and_free(&fpq, NULL);
    (void)ccc_fom_clear_and_free(&map, NULL);
}

static struct frequency_alloc
copy_frequencies(ccc_flat_ordered_map const *const map)
{
    PROG_ASSERT(!is_empty(map));
    size_t const cap = sizeof(struct frequency) * (size(map) + 1);
    struct frequency *const freqs = malloc(cap);
    PROG_ASSERT(freqs);
    size_t i = 0;
    for (word const *w = begin(map); w != end(map) && i < cap;
         w = next(map, &w->e), ++i)
    {
        freqs[i].ofs = w->ofs;
        freqs[i].freq = w->cnt;
    }
    return (struct frequency_alloc){.arr = freqs, .cap = cap};
}

static void
print_n(ccc_flat_priority_queue *const fpq, struct str_arena const *const a,
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

static ccc_flat_ordered_map
create_frequency_map(struct str_arena *const a, FILE *const f)
{
    char *lineptr = NULL;
    size_t len = 0;
    ptrdiff_t read = 0;
    ccc_flat_ordered_map fom
        = ccc_fom_init((word *)NULL, 0, e, ofs, std_alloc, cmp_string_keys, a);
    while ((read = getline(&lineptr, &len, f)) > 0)
    {
        str_view const line = {.s = lineptr, .len = read - 1};
        for (str_view word_view = sv_begin_tok(line, space);
             !sv_end_tok(line, word_view);
             word_view = sv_next_tok(line, word_view, space))
        {
            struct clean_word const cw = clean_word(a, word_view);
            PROG_ASSERT(cw.stat != WC_ARENA_ERR);
            if (cw.stat == WC_CLEAN_WORD)
            {
                /* There are a few idiomatic ways we could tackle this step.
                   However, this demonstrates the user of "closures" in the
                   and modify with macro. The and modify macro gives a closure
                   over the user type T if the entry is Occupied. If the entry
                   is Vacant the closure does not execute. You can uncomment
                   the below code if you prefer a more sequential style. */
                /*
                ccc_fomap_entry *e = entry_r(&fom, &cw.str);
                e = fom_and_modify_w(e, word, { T->cnt++; });
                word *w = fom_or_insert_w(e, (word){.ofs = cw.str, .cnt = 1});
                */
                word *w
                    = fom_or_insert_w(fom_and_modify_w(entry_r(&fom, &cw.str),
                                                       word, { T->cnt++; }),
                                      (word){.ofs = cw.str, .cnt = 1});
                PROG_ASSERT(w);
            }
        }
    }
    free(lineptr);
    return fom;
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
        PROG_ASSERT(pushed_char);
        ++str_len;
    }
    if (!str_len)
    {
        return (struct clean_word){.stat = WC_NOT_WORD};
    }
    char const *const w = str_arena_at(a, str);
    PROG_ASSERT(w);
    if (!isalpha(*w) || !isalpha(*(w + (str_len - 1))))
    {
        str_arena_free_to_pos(a, str, str_len);
        return (struct clean_word){.stat = WC_NOT_WORD};
    }
    return (struct clean_word){.str = str, .stat = WC_CLEAN_WORD};
}

/*=======================   Str Arena Allocator   ===========================*/

static struct str_arena
str_arena_create(size_t const cap)
{
    struct str_arena a;
    a.arena = calloc(cap, sizeof(char));
    if (!a.arena)
    {
        return (struct str_arena){};
    }
    a.cap = arena_start_cap;
    a.next_free_pos = 0;
    return a;
}

/* Allocates exactly bytes bytes from the arena. Do not forget the null
   terminator in requests if a string is requested. */
static ptrdiff_t
str_arena_alloc(struct str_arena *const a, size_t const bytes)
{
    if (str_arena_maybe_resize(a, bytes) != CCC_OK)
    {
        return -1;
    }
    size_t const ret = a->next_free_pos;
    a->next_free_pos += bytes;
    return (ptrdiff_t)ret;
}

/* Push a character back to the last string allocation. This is possible
   and useful when a string may be edited depending on other factors
   before it is finally recorded for later use. One would overwrite other
   strings if this is not the last element. However all strings will remain
   null terminated. A null terminator is added after the new character.
   Assumes str_len + 1 bytes for str have been allocated already starting
   at str. */
static bool
str_arena_push_back(struct str_arena *const a, str_ofs const str,
                    size_t const str_len, char const c)
{
    size_t const new_pos = str + str_len + 1;
    if (str_arena_maybe_resize_pos(a, new_pos) != CCC_OK)
    {
        return false;
    }
    char *const string = str_arena_at(a, str);
    PROG_ASSERT(string);
    *(string + str_len) = c;
    *(string + str_len + 1) = '\0';
    if (new_pos >= a->next_free_pos)
    {
        a->next_free_pos += ((new_pos + 1) - a->next_free_pos);
    }
    return true;
}

static ccc_result
str_arena_maybe_resize(struct str_arena *const a, size_t const byte_request)
{
    if (!a)
    {
        return CCC_INPUT_ERR;
    }
    return str_arena_maybe_resize_pos(a, a->next_free_pos + byte_request);
}

static ccc_result
str_arena_maybe_resize_pos(struct str_arena *const a, size_t const furthest_pos)
{
    if (!a)
    {
        return CCC_INPUT_ERR;
    }
    if (furthest_pos >= a->cap)
    {
        size_t const new_cap = (furthest_pos) * 2;
        void *const moved_arena = std_alloc(a->arena, new_cap, NULL);
        if (!moved_arena)
        {
            return CCC_MEM_ERR;
        }
        memset((char *)moved_arena + a->cap, '\0', new_cap - a->cap);
        a->arena = moved_arena;
        a->cap = new_cap;
    }
    return CCC_OK;
}

/* Returns the last given out position to the arena. The only fine grained
   freeing made possible and requires the user knows the most recently
   allocated position or all strings between end and last_pos are
   invalidated. */
static void
str_arena_free_to_pos(struct str_arena *const a, str_ofs const last_str,
                      size_t const str_len)
{
    if (!a || !a->arena || !a->cap || !a->next_free_pos)
    {
        return;
    }
    if (str_len)
    {
        memset(a->arena + last_str, '\0', str_len);
    }
    a->next_free_pos = last_str;
}

static void
str_arena_free(struct str_arena *const a)
{
    if (!a)
    {
        return;
    }
    if (a->arena)
    {
        free(a->arena);
        a->arena = NULL;
    }
    a->next_free_pos = 0;
    a->cap = 0;
}

static char *
str_arena_at(struct str_arena const *const a, str_ofs const i)
{
    if (!a || (size_t)i >= a->cap)
    {
        return NULL;
    }
    return a->arena + i;
}

/*=======================   Container Helpers    ============================*/

static ccc_threeway_cmp
cmp_string_keys(ccc_key_cmp const c)
{
    word const *const w = c.user_type_rhs;
    struct str_arena const *const a = c.aux;
    str_ofs const *const id = c.key_lhs;
    char const *const key_word = str_arena_at(a, *id);
    char const *const struct_word = str_arena_at(a, w->ofs);
    PROG_ASSERT(key_word && struct_word);
    int const res = strcmp(key_word, struct_word);
    if (res > 0)
    {
        return CCC_GRT;
    }
    if (res < 0)
    {
        return CCC_LES;
    }
    return CCC_EQL;
}

/* Sorts by frequency then alphabetic order if frequencies are tied. */
static ccc_threeway_cmp
cmp_freqs(ccc_cmp const c)
{
    struct frequency const *const lhs = c.user_type_lhs;
    struct frequency const *const rhs = c.user_type_rhs;
    ccc_threeway_cmp cmp = (lhs->freq > rhs->freq) - (lhs->freq < rhs->freq);
    if (cmp != CCC_EQL)
    {
        return cmp;
    }
    struct str_arena const *const arena = c.aux;
    char const *const lhs_word = str_arena_at(arena, lhs->ofs);
    char const *const rhs_word = str_arena_at(arena, rhs->ofs);
    PROG_ASSERT(lhs_word && rhs_word);
    int const res = strcmp(lhs_word, rhs_word);
    /* Looks like we have chosen wrong order to return but not so: greater
       lexicographic order is sorted first in a min priority queue or CCC_LES
       in this case. */
    if (res > 0)
    {
        return CCC_LES;
    }
    if (res < 0)
    {
        return CCC_GRT;
    }
    return CCC_EQL;
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
