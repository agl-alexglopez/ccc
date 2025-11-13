/** The words program is a simple word counter that aims to demonstrate
the benefits of flexible memory layouts and context data.

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
#include "utility/allocate.h"
#include "utility/cli.h"
#include "utility/string_arena.h"
#include "utility/string_view/string_view.h"

enum Action_type
{
    COUNT,
    FIND,
};

/* A word will be counted with a map and then sorted with a Buffer and flat
priority queue. One type works well for both because they don't need intrusive
elements to operate those containers. */
typedef struct
{
    struct String_offset ofs;
    int freq;
} Word;

struct Action_pack
{
    SV_String_view file;
    enum Action_type type;
    union
    {
        int n;
        SV_String_view w;
    };
    union
    {
        void (*freq_fn)(FILE *file, int n);
        void (*find_fn)(FILE *file, SV_String_view w);
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

static SV_String_view const space = SV(" ");
static SV_String_view const directions
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

static void print_found(FILE *file, SV_String_view w);
static void print_top_n(FILE *file, int n);
static void print_last_n(FILE *file, int n);
static void print_alpha_n(FILE *file, int n);
static void print_ralpha_n(FILE *file, int n);
static Buffer copy_frequencies(Handle_ordered_map const *map);
static void print_n(Handle_ordered_map *, CCC_Order, struct String_arena *,
                    int n);
static struct Int_conversion parse_n_ranks(SV_String_view arg);

/* String Helper Functions */
static struct String_offset clean_word(struct String_arena *,
                                       SV_String_view wv);

/* Container Functions */
static Handle_ordered_map create_frequency_map(struct String_arena *, FILE *);
static Order order_string_keys(Key_comparator_context);
static Order order_words(Type_comparator_context);

/* Misc. Functions */
static FILE *open_file(SV_String_view file);

/*=======================     Main         ==================================*/

int
main(int argc, char *argv[])
{
    if (argc == 2 && SV_starts_with(SV_sv(argv[1]), SV("-h")))
    {
        SV_print(stdout, directions);
        return 0;
    }
    if (argc < 3)
    {
        return 0;
    }
    struct Action_pack exe = {};
    for (int arg = 1; arg < argc; ++arg)
    {
        SV_String_view const sv_arg = SV_sv(argv[arg]);
        if (SV_starts_with(sv_arg, SV("-c")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_top_n;
            exe.n = ALL_FREQUENCIES;
        }
        else if (SV_starts_with(sv_arg, SV("-rc")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            exe.n = ALL_FREQUENCIES;
        }
        else if (SV_starts_with(sv_arg, SV("-top=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_top_n;
            struct Int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER,
                  logerr("cannot convert -top= flag to int"););
            exe.n = c.conversion;
        }
        else if (SV_starts_with(sv_arg, SV("-last=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            struct Int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER,
                  logerr("cannot convert -last= flat to int"););
            exe.n = c.conversion;
        }
        else if (SV_starts_with(sv_arg, SV("-alph=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_alpha_n;
            struct Int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER,
                  logerr("cannot convert -alph= flag to int"););
            exe.n = c.conversion;
        }
        else if (SV_starts_with(sv_arg, SV("-ralph=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_ralpha_n;
            struct Int_conversion c = parse_n_ranks(sv_arg);
            check(c.status != CONV_ER,
                  logerr("cannot convert -ralph= flat to int"););
            exe.n = c.conversion;
        }
        else if (SV_starts_with(sv_arg, SV("-find=")))
        {
            SV_String_view const raw_word = SV_substr(
                sv_arg, SV_find(sv_arg, 0, SV("=")) + 1, SV_len(sv_arg));
            check(!SV_empty(raw_word),
                  logerr("-find= flag has invalid entry"););
            exe.type = FIND;
            exe.find_fn = print_found;
            exe.w = raw_word;
        }
        else if (SV_starts_with(sv_arg, SV("-f=")))
        {
            SV_String_view const raw_file = SV_substr(
                sv_arg, SV_find(sv_arg, 0, SV("=")) + 1, SV_len(sv_arg));
            check(!SV_empty(raw_file), logerr("file string is empty"););
            exe.file = raw_file;
        }
        else if (SV_starts_with(sv_arg, SV("-h")))
        {
            SV_print(stdout, directions);
            return 0;
        }
        else
        {
            if (SV_begin(sv_arg))
            {
                logerr("unrecognized argument: %s\n", SV_begin(sv_arg));
            }
            return 1;
        }
    }
    FILE *const f = open_file(exe.file);
    if (!f)
    {
        if (exe.file.s)
        {
            logerr("error opening: %s\n", exe.file.s);
        }
        (void)fclose(f);
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
        (void)fclose(f);
        return 1;
    }
    (void)fclose(f);
    return 0;
}

/*=======================   Static Impl    ==================================*/

static void
print_found(FILE *const f, SV_String_view w)
{
    struct String_arena a = string_arena_create(ARENA_START_CAP);
    check(a.arena);
    Handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    struct String_offset wc = clean_word(&a, w);
    if (!wc.error)
    {
        Word const *const found_w
            = handle_ordered_map_at(&map, get_key_val(&map, &wc));
        if (found_w)
        {
            printf("%s %d\n", string_arena_at(&a, &found_w->ofs),
                   found_w->freq);
        }
    }
    string_arena_free(&a);
    (void)handle_ordered_map_clear_and_free(&map, NULL);
}

static void
print_top_n(FILE *const f, int n)
{
    struct String_arena a = string_arena_create(ARENA_START_CAP);
    check(a.arena);
    Handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    print_n(&map, CCC_ORDER_GREATER, &a, n);
    string_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static void
print_last_n(FILE *const f, int n)
{
    struct String_arena a = string_arena_create(ARENA_START_CAP);
    check(a.arena);
    Handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    print_n(&map, CCC_ORDER_LESSER, &a, n);
    string_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static void
print_alpha_n(FILE *const f, int n)
{
    struct String_arena a = string_arena_create(ARENA_START_CAP);
    check(a.arena);
    Handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    if (!n)
    {
        n = count(&map).count;
    }
    int i = 0;
    /* The ordered nature of the map comes in handy for alpha printing. */
    for (Word *w = begin(&map); w != end(&map) && i < n; w = next(&map, w), ++i)
    {
        printf("%s %d\n", string_arena_at(&a, &w->ofs), w->freq);
    }
    string_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static void
print_ralpha_n(FILE *const f, int n)
{
    struct String_arena a = string_arena_create(ARENA_START_CAP);
    check(a.arena);
    Handle_ordered_map map = create_frequency_map(&a, f);
    check(!is_empty(&map));
    if (!n)
    {
        n = count(&map).count;
    }
    int i = 0;
    /* The ordered nature of the map comes in handy for reverse iteration. */
    for (Word *w = rbegin(&map); w != rend(&map) && i < n;
         w = rnext(&map, w), ++i)
    {
        printf("%s %d\n", string_arena_at(&a, &w->ofs), w->freq);
    }
    string_arena_free(&a);
    (void)clear_and_free(&map, NULL);
}

static Buffer
copy_frequencies(Handle_ordered_map const *const map)
{
    check(!is_empty(map));
    Buffer freqs = buffer_initialize(NULL, Word, std_allocate, NULL, 0);
    CCC_Result const r = buffer_reserve(&freqs, count(map).count, std_allocate);
    check(r == CCC_RESULT_OK);
    size_t const cap = capacity(&freqs).count;
    size_t i = 0;
    for (Word const *w = begin(map); w != end(map) && i < cap;
         w = next(map, w), ++i)
    {
        Word const *const pushed = buffer_push_back(&freqs, w);
        check(pushed);
    }
    return freqs;
}

static void
print_n(CCC_Handle_ordered_map *const map, CCC_Order const ord,
        struct String_arena *const a, int n)
{
    Buffer freqs = copy_frequencies(map);
    check(!buffer_is_empty(&freqs));
    Flat_priority_queue flat_priority_queue
        = flat_priority_queue_heapify_initialize(
            begin(&freqs), Word, ord, order_words, NULL, a,
            capacity(&freqs).count, count(&freqs).count);
    check(count(&flat_priority_queue).count == count(&freqs).count);
    if (!n)
    {
        n = count(&flat_priority_queue).count;
    }
    /* Because all CCC containers are complete they can be treated as copyable
       types like this. There is no opaque container in CCC. */
    Buffer const sorted_freqs
        = CCC_flat_priority_queue_heapsort(&flat_priority_queue, &(Word){});
    check(!flat_priority_queue.buf.mem);
    int w = 0;
    /* Heap sort puts the root most nodes at the back of the buffer. */
    for (Word const *i = rbegin(&sorted_freqs);
         i != rend(&sorted_freqs) && w < n; i = rnext(&sorted_freqs, i), ++w)
    {
        char const *const arena_str = string_arena_at(a, &i->ofs);
        if (arena_str)
        {
            printf("%d. %s %d\n", w + 1, arena_str, i->freq);
        }
    }
    (void)clear_and_free(&freqs, NULL);
}

/*=====================    Container Construction     =======================*/

static Handle_ordered_map
create_frequency_map(struct String_arena *const a, FILE *const f)
{
    char *lineptr = NULL;
    size_t len = 0;
    ptrdiff_t read = 0;
    Handle_ordered_map handle_ordered_map = handle_ordered_map_initialize(
        NULL, Word, ofs, order_string_keys, std_allocate, a, 0);
    while ((read = getline(&lineptr, &len, f)) > 0)
    {
        SV_String_view const line = {.s = lineptr, .len = read - 1};
        for (SV_String_view word_view = SV_begin_tok(line, space);
             !SV_end_tok(line, word_view);
             word_view = SV_next_tok(line, word_view, space))
        {
            struct String_offset const cw = clean_word(a, word_view);
            if (!cw.error)
            {
                Handle_ordered_map_handle const *e
                    = handle_r(&handle_ordered_map, &cw);
                e = handle_ordered_map_and_modify_w(e, Word, { T->freq++; });
                Word const *const w = handle_ordered_map_at(
                    &handle_ordered_map, handle_ordered_map_or_insert_w(
                                             e, (Word){.ofs = cw, .freq = 1}));
                check(w);
            }
        }
    }
    free(lineptr);
    return handle_ordered_map;
}

static struct String_offset
clean_word(struct String_arena *const a, SV_String_view wv)
{
    /* It is hard to know how many characters will make it to a cleaned word
       and one pass is ideal so arena api allows push back on last alloc. */
    struct String_offset str = string_arena_allocate(a, 0);
    if (str.error)
    {
        return str;
    }
    for (char const *c = SV_begin(wv); c != SV_end(wv); c = SV_next(c))
    {
        if (!isalpha(*c) && *c != '-')
        {
            string_arena_pop_str(a, &str);
            return (struct String_offset){.error = STRING_ARENA_INVALID};
        }
        enum String_arena_result const pushed_char
            = string_arena_push_back(a, &str, (char)tolower(*c));
        check(pushed_char == STRING_ARENA_OK);
    }
    if (!str.len)
    {
        return (struct String_offset){.error = STRING_ARENA_INVALID};
    }
    char const *const w = string_arena_at(a, &str);
    check(w);
    if (!isalpha(*w) || !isalpha(*(w + (str.len - 1))))
    {
        enum String_arena_result const pop = string_arena_pop_str(a, &str);
        check(pop == STRING_ARENA_OK);
        return (struct String_offset){.error = STRING_ARENA_INVALID};
    }
    return str;
}

/*=======================   Container Helpers    ============================*/

static Order
order_string_keys(Key_comparator_context const c)
{
    Word const *const w = c.type_rhs;
    struct String_arena const *const a = c.context;
    struct String_offset const *const id = c.key_lhs;
    char const *const key_word = string_arena_at(a, id);
    char const *const struct_word = string_arena_at(a, &w->ofs);
    check(key_word && struct_word);
    int const res = strcmp(key_word, struct_word);
    return (res > 0) - (res < 0);
}

/* Sorts by frequency then alphabetic order if frequencies are tied. */
static Order
order_words(Type_comparator_context const c)
{
    Word const *const lhs = c.type_lhs;
    Word const *const rhs = c.type_rhs;
    Order freq_order = (lhs->freq > rhs->freq) - (lhs->freq < rhs->freq);
    if (freq_order != CCC_ORDER_EQUAL)
    {
        return freq_order;
    }
    struct String_arena const *const arena = c.context;
    char const *const lhs_word = string_arena_at(arena, &lhs->ofs);
    char const *const rhs_word = string_arena_at(arena, &rhs->ofs);
    check(lhs_word && rhs_word);
    int const res = strcmp(lhs_word, rhs_word);
    /* Looks like we have chosen wrong order to return but not so: greater
       lexicographic order is sorted first in a min priority queue or
       CCC_ORDER_LESSER in this case. */
    return (res < 0) - (res > 0);
}

/*=======================   CLI Helpers    ==================================*/

static struct Int_conversion
parse_n_ranks(SV_String_view arg)
{
    size_t const eql = SV_rfind(arg, SV_npos(arg), SV("="));
    if (eql == SV_npos(arg))
    {
        return (struct Int_conversion){.status = CONV_ER};
    }
    arg = SV_substr(arg, eql + 1, ULLONG_MAX);
    if (SV_empty(arg))
    {
        (void)fprintf(stderr, "please specify word frequency range.\n");
        return (struct Int_conversion){.status = CONV_ER};
    }
    return convert_to_int(SV_begin(arg));
}

static FILE *
open_file(SV_String_view file)
{
    if (SV_empty(file))
    {
        return NULL;
    }
    FILE *const f = fopen(SV_begin(file), "r");
    if (!f)
    {
        (void)fprintf(stderr,
                      "Opening file [%s] failed with errno set to: %d\n",
                      SV_begin(file), errno);
    }
    return f;
}
