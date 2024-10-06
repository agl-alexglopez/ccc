#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_ORDERED_MAP_USING_NAMESPACE_CCC

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ccc/flat_ordered_map.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/traits.h"
#include "ccc/types.h"
#include "cli.h"
#include "str_view/str_view.h"

typedef ssize_t str_ofs;

enum action_type
{
    COUNT,
    FIND,
};

enum word_clean_status
{
    WC_CLEAN,
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
    str_ofs arena_pos;
    /* How many times the word is encountered. Key for priority queue. */
    int freq;
};

struct word
{
    struct frequency freq;
    /* Map element for logging frequencies by string key, freq value. */
    f_om_elem map_elem;
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
         "./build/[[bin/] or [debug/bin]]/words [flag] -f=[path/to/file.txt]\n"
         "[flag]:\n"
         "-c\n"
         "\treports the words by count in descending order.\n"
         "-rc\n"
         "\treports words by count in ascending order.\n"
         "-top=N\n"
         "\treporst the top N words by frequency.\n"
         "-last=N\n"
         "\treports the last N words by frequency\n"
         "find=[WORD]\n"
         "\treports the count of the specified word.\n");

/*=======================   Prototypes     ==================================*/

#define PROG_ASSERT(cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
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
static struct frequency *copy_frequencies(ccc_flat_ordered_map const *map);
static void print_n(ccc_flat_priority_queue *, struct str_arena const *, int n);
static struct int_conversion parse_n_ranks(str_view arg);

/* String Helper Functions */
static struct clean_word clean_word(struct str_arena *, str_view word);

/* String Arena Functions */
static struct str_arena str_arena_create(size_t);
static ssize_t str_arena_alloc(struct str_arena *, size_t bytes);
static ccc_result str_arena_maybe_resize(struct str_arena *,
                                         size_t new_request);
static void str_arena_free_to_pos(struct str_arena *, str_ofs last_pos);
static bool str_arena_push_back(struct str_arena *, str_ofs str, size_t len,
                                char);
static void str_arena_free(struct str_arena *);
static char *str_arena_at(struct str_arena const *, str_ofs);

/* Container Functions */
static ccc_flat_ordered_map create_frequency_map(struct str_arena *, FILE *);
static ccc_threeway_cmp cmp_string_keys(ccc_key_cmp);
static ccc_threeway_cmp cmp_freqs(ccc_cmp);
static void print_word(ccc_user_type);

/* Misc. Functions */
static FILE *open_file(str_view file);

/*=======================     Main         ==================================*/

int
main(int argc, char *argv[])
{
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
            PROG_ASSERT(c.status != CONV_ER);
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-last=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            PROG_ASSERT(c.status != CONV_ER);
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-find=")))
        {
            str_view const raw_word = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            PROG_ASSERT(!sv_empty(raw_word));
            exe.type = FIND;
            exe.find_fn = print_found;
            exe.w = raw_word;
        }
        else if (sv_starts_with(sv_arg, SV("-f=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            PROG_ASSERT(!sv_empty(raw_file));
            exe.file = raw_file;
        }
        else
        {
            QUIT_MSG(stderr, "unrecognized argument: %s", sv_begin(sv_arg));
        }
    }
    FILE *f = open_file(exe.file);
    PROG_ASSERT(f != NULL);
    switch (exe.type)
    {
    case COUNT:
        exe.freq_fn(f, exe.n);
        break;
    case FIND:
        exe.find_fn(f, exe.w);
        break;
    }
    (void)fclose(f);
    return 0;
}

/*=======================   Static Impl    ==================================*/

static void
print_found(FILE *const f, str_view w)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    assert(a.arena);
    ccc_flat_ordered_map map = create_frequency_map(&a, f);
    assert(!empty(&map));
    struct clean_word w_clean = clean_word(&a, w);
    assert(w_clean.stat != WC_ARENA_ERR);
    if (w_clean.stat != WC_CLEAN)
    {
        return;
    }
    struct word const *const word = get_key_val(&map, &w_clean.str);
    if (!word)
    {
        return;
    }
    printf("%s %d\n", str_arena_at(&a, word->freq.arena_pos), word->freq.freq);
}

static void
print_top_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    assert(a.arena);
    ccc_flat_ordered_map map = create_frequency_map(&a, f);
    assert(!empty(&map));
    /* O(n) copy */
    struct frequency *const freqs = copy_frequencies(&map);
    /* O(n) sort kind of. Pops will be O(nlgn). But we don't have stdlib qsort
       space overhead to emulate why someone in an embedded or OS
       environment might choose this approach for sorting. Granted any O(1)
       space approach to sorting may beat the slower pop operation. */
    ccc_flat_priority_queue fpq = ccc_fpq_heapify_init(
        freqs, size(&map) + 1, size(&map), CCC_GRT, realloc, cmp_freqs, &a);
    assert(size(&fpq) == size(&map));
    if (!n)
    {
        n = size(&fpq);
    }
    print_n(&fpq, &a, n);
    str_arena_free(&a);
    ccc_fpq_clear_and_free(&fpq, NULL);
    ccc_fom_clear_and_free(&map, NULL);
}

static void
print_last_n(FILE *const f, int n)
{
    struct str_arena a = str_arena_create(arena_start_cap);
    assert(a.arena);
    ccc_flat_ordered_map map = create_frequency_map(&a, f);
    assert(!empty(&map));
    struct frequency *const freqs = copy_frequencies(&map);
    ccc_flat_priority_queue fpq = ccc_fpq_heapify_init(
        freqs, size(&map) + 1, size(&map), CCC_LES, realloc, cmp_freqs, &a);
    assert(size(&fpq) == size(&map));
    if (!n)
    {
        n = size(&fpq);
    }
    print_n(&fpq, &a, n);
    str_arena_free(&a);
    ccc_fpq_clear_and_free(&fpq, NULL);
    ccc_fom_clear_and_free(&map, NULL);
}

static struct frequency *
copy_frequencies(ccc_flat_ordered_map const *const map)
{
    assert(!empty(map));
    size_t const cap = sizeof(struct frequency) * (size(map) + 1);
    struct frequency *const freqs = malloc(cap);
    assert(freqs);
    size_t i = 0;
    for (struct word *w = begin(map); w != end(map) && i < cap;
         w = next(map, &w->map_elem))
    {
        freqs[i++] = w->freq;
    }
    return freqs;
}

static void
print_n(ccc_flat_priority_queue *const fpq, struct str_arena const *const a,
        int const n)
{
    for (int w = 0; w < n && !empty(fpq); ++w)
    {
        struct frequency *const word = front(fpq);
        printf("%d. %s %d\n", w + 1, str_arena_at(a, word->arena_pos),
               word->freq);
        pop(fpq);
    }
}

/*=====================    Container Construction     =======================*/

static ccc_flat_ordered_map
create_frequency_map(struct str_arena *const a, FILE *const f)
{
    char *lineptr = NULL;
    size_t len = 0;
    ssize_t read = 0;
    ccc_flat_ordered_map fom
        = ccc_fom_init((struct word *)NULL, 0, map_elem, freq.arena_pos,
                       realloc, cmp_string_keys, a);
    while ((read = getline(&lineptr, &len, f)) > 0)
    {
        str_view const line = {.s = lineptr, .sz = read - 1};
        for (str_view tok = sv_begin_tok(line, space); !sv_end_tok(line, tok);
             tok = sv_next_tok(line, tok, space))
        {
            struct clean_word const cleaned = clean_word(a, tok);
            assert(cleaned.stat != WC_ARENA_ERR);
            if (cleaned.stat == WC_CLEAN)
            {
                fom_or_insert_w(
                    entry_vr(&fom, &cleaned.str),
                    (struct word){.freq = {.arena_pos = cleaned.str}})
                    ->freq.freq++;
            }
        }
    }
    free(lineptr);
    return fom;
}

static struct clean_word
clean_word(struct str_arena *const a, str_view word)
{
    /* It is hard to know how many characters will make it to a cleaned word
       and one pass is ideal so arena api allows push back on last alloc. */
    str_ofs const str = str_arena_alloc(a, 0);
    if (str < 0)
    {
        return (struct clean_word){.stat = WC_ARENA_ERR};
    }
    size_t str_len = 0;
    for (char const *c = sv_begin(word); c != sv_end(word); c = sv_next(c))
    {
        if (isalpha(*c) || *c == '-')
        {
            [[maybe_unused]] bool const pushed_char
                = str_arena_push_back(a, str, str_len, (char)tolower(*c));
            assert(pushed_char);
            ++str_len;
        }
    }
    if (!str_len)
    {
        return (struct clean_word){.stat = WC_NOT_WORD};
    }
    [[maybe_unused]] bool const pushed_char
        = str_arena_push_back(a, str, str_len, '\0');
    assert(pushed_char);
    if (sv_strcmp(SV("--"), str_arena_at(a, str)) == SV_EQL)
    {
        str_arena_free_to_pos(a, str);
        return (struct clean_word){.stat = WC_NOT_WORD};
    }
    return (struct clean_word){.str = str, .stat = WC_CLEAN};
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

static ssize_t
str_arena_alloc(struct str_arena *const a, size_t const bytes)
{
    if (str_arena_maybe_resize(a, bytes) != CCC_OK)
    {
        return -1;
    }
    size_t const ret = a->next_free_pos;
    a->next_free_pos += bytes;
    return (ssize_t)ret;
}

static bool
str_arena_push_back(struct str_arena *const a, str_ofs const str,
                    size_t const str_len, char const c)
{
    if (str_arena_maybe_resize(a, str + str_len + 1) != CCC_OK
        || *str_arena_at(a, str + (str_ofs)str_len + 1) != '\0')
    {
        return false;
    }
    *(str_arena_at(a, str) + str_len) = c;
    ++a->next_free_pos;
    return true;
}

static ccc_result
str_arena_maybe_resize(struct str_arena *const a, size_t const new_request)
{
    if (!a)
    {
        return CCC_INPUT_ERR;
    }
    if (a->next_free_pos + new_request >= a->cap)
    {
        size_t const new_cap = (a->next_free_pos + new_request) * 2;
        void *moved_arena = realloc(a->arena, new_cap);
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
   allocated position. */
static void
str_arena_free_to_pos(struct str_arena *const a, str_ofs const last_pos)
{
    if (!a || !a->arena || !a->cap || !a->next_free_pos)
    {
        return;
    }
    a->next_free_pos = last_pos;
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
    struct word const *const w = c.user_type;
    struct str_arena const *const a = c.aux;
    str_ofs const id = *((str_ofs *)c.key);
    int const res
        = strcmp(str_arena_at(a, id), str_arena_at(a, w->freq.arena_pos));
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
    struct frequency const *const a = c.user_type_a;
    struct frequency const *const b = c.user_type_b;
    struct str_arena const *const arena = c.aux;
    ccc_threeway_cmp cmp = (a->freq > b->freq) - (a->freq < b->freq);
    if (cmp != CCC_EQL)
    {
        return cmp;
    }
    int const res = strcmp(str_arena_at(arena, a->arena_pos),
                           str_arena_at(arena, b->arena_pos));
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

[[maybe_unused]] static void
print_word(ccc_user_type const u)
{
    struct word const *const w = u.user_type;
    struct str_arena const *const a = u.aux;
    printf("{%s, %d}", str_arena_at(a, w->freq.arena_pos), w->freq.freq);
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
