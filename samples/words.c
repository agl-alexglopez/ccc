#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "cli.h"
#include "flat_realtime_ordered_map.h"
#include "str_view/str_view.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

enum action_type
{
    COUNT,
    FIND,
};

struct pool_string
{
    size_t next_string_offset;
    char *string;
};

struct string_pool
{
    char *pool;
};

struct word
{
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

static int const all_frequencies = 0;

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

#define CLI_ASSERT(cond)                                                       \
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

/*=======================   Prototypes     ==================================*/

static void print_found(FILE *file, str_view w);
static void print_top_n(FILE *file, int n);
static void print_top_n_rev(FILE *file, int n);
static void print_last_n(FILE *file, int n);
static struct int_conversion parse_n_ranks(str_view arg);

ccc_flat_realtime_ordered_map create_frequency_map(void);
void destroy_frequency_map(ccc_flat_realtime_ordered_map *);

FILE *open_file(str_view file);

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
            exe.freq_fn = print_top_n_rev;
            exe.n = all_frequencies;
        }
        else if (sv_starts_with(sv_arg, SV("-top=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_top_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            CLI_ASSERT(c.status != CONV_ER);
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-last=")))
        {
            exe.type = COUNT;
            exe.freq_fn = print_last_n;
            struct int_conversion c = parse_n_ranks(sv_arg);
            CLI_ASSERT(c.status != CONV_ER);
            exe.n = c.conversion;
        }
        else if (sv_starts_with(sv_arg, SV("-find=")))
        {
            str_view const raw_word = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            CLI_ASSERT(!sv_empty(raw_word));
            exe.type = FIND;
            exe.find_fn = print_found;
            exe.w = raw_word;
        }
        else if (sv_starts_with(sv_arg, SV("-f=")))
        {
            str_view const raw_file = sv_substr(
                sv_arg, sv_find(sv_arg, 0, SV("=")) + 1, sv_len(sv_arg));
            CLI_ASSERT(!sv_empty(raw_file));
            exe.file = raw_file;
        }
        else
        {
            QUIT_MSG(stderr, "unrecognized argument: %s", sv_begin(sv_arg));
        }
    }
    FILE *f = open_file(exe.file);
    CLI_ASSERT(f != NULL);
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
    (void)f;
    (void)w;
}

static void
print_top_n(FILE *const f, int n)
{

    (void)f;
    (void)n;
}

static void
print_top_n_rev(FILE *const f, int n)
{

    (void)f;
    (void)n;
}

static void
print_last_n(FILE *const f, int n)
{

    (void)f;
    (void)n;
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

FILE *
open_file(str_view file)
{
    if (sv_empty(file))
    {
        return NULL;
    }
    FILE *const f = fopen(sv_begin(file), "r");
    if (!f)
    {
        (void)fprintf(stderr, "File opening failed with errno set to: %d\n",
                      errno);
    }
    return f;
}
