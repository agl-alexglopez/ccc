/** Course-grained performance testing for the two hash map implementations
in this collection. The handle hash map offers a promise that user data does
not move from an index in an array once inserted. This comes with some
additional space and time complexity. The goal is to see if this additional
complexity creates a non-justifiable performance decrease. Dynamic maps that
allow resizing will be where the handle hash map performs the worst. The resize
operation is the slowest for that map.

Multiple sized types are tested as well. The hypothesis is that handle hash
maps refusing to move user data will be a benefit to performance for larger
user types. The data suggests this is true but only at the two largest sizes
and by a very close margin. Larger than xlarge and the performance gap grows
in favor of the handle hash map but such struct sizes may be quite uncommon.

Dynamic resizing is hard for the handle hash map but data suggests that if the
tables are pre-allocated with the maximum size needed and not allowed to resize
performance is very close across both maps which is good because we want the
user to choose based on if they need handle stability for larger types or are
just using a simple key value struct that is below 256 bytes, in which case
simple flat map gets the job done.

Disappointingly, the limitations imposed on the Collection prevents some
optimizations that could make these faster. Because these maps must operate
with the possibility that the user forbids memory allocation Robin Hood Hashing
is used with backshift deletion to avoid the need for tombstones/rehashing.
This way the table can have as long a lifetime as the user wishes with a fixed
size. The only downside is possible primary clustering that comes with linear
probing, but this is a hash strength and load factor issue rather than the
more fundamental threat that tombstones would pose to a non-resizeable
data structure with no access to any supplementary memory. The user also
provides the buffer for their defined type with our intrusive element so a
metadata array within a single allocation presents a strict-aliasing challenge.
I am unsure how something like Google's Abseil Flat Hash Map could be
implemented under the restrictions imposed on the C Container Collection.

Overall, in some preliminary testing, not included here because CCC is not C++
compatible C code, we seem to mostly lose to C++'s std::unordered_map. This
should be examined further because most linear or quadratic probing tables are
expected to handily beat std::unordered_map across all metrics. Again though,
this collection has some limitations on memory use compared to other tables or
STL that may need more creative solutions than I could come up with while
remaining C compliant. */
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_HASH_MAP_USING_NAMESPACE_CCC
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "flat_hash_map.h"
#include "handle_hash_map.h"
#include "random.h"
#include "traits.h"
#include "types.h"

typedef struct
{
    hhmap_elem e;
    int key;
    char buf[4];
} small_hhmap_type;

typedef struct
{
    hhmap_elem e;
    int key;
    char buf[24];
} medium_hhmap_type;

typedef struct
{
    hhmap_elem e;
    int key;
    char buf[256];
} large_hhmap_type;

typedef struct
{
    hhmap_elem e;
    int key;
    char buf[1024];
} xlarge_hhmap_type;

typedef struct
{
    fhmap_elem e;
    int key;
    char buf[4];
} small_fhmap_type;

typedef struct
{
    fhmap_elem e;
    int key;
    char buf[24];
} medium_fhmap_type;

typedef struct
{
    fhmap_elem e;
    int key;
    char buf[256];
} large_fhmap_type;

typedef struct
{
    fhmap_elem e;
    int key;
    char buf[1024];
} xlarge_fhmap_type;

enum struct_size
{
    SMALL = 0,
    MEDIUM,
    LARGE,
    XLARGE,
    TYPES_END,
};

char const *const struct_size_strs[TYPES_END] = {
    [SMALL] = "SMALL",
    [MEDIUM] = "MEDIUM",
    [LARGE] = "LARGE",
    [XLARGE] = "XLARGE",
};

#define generate_eq_fn(type_name)                                              \
    static ccc_tribool type_name##_eq(ccc_key_cmp cmp)                         \
    {                                                                          \
        type_name const *const t = cmp.user_type_rhs;                          \
        return t->key == *((int *)cmp.key_lhs);                                \
    }
generate_eq_fn(small_fhmap_type);
generate_eq_fn(medium_fhmap_type);
generate_eq_fn(large_fhmap_type);
generate_eq_fn(xlarge_fhmap_type);
generate_eq_fn(small_hhmap_type);
generate_eq_fn(medium_hhmap_type);
generate_eq_fn(large_hhmap_type);
generate_eq_fn(xlarge_hhmap_type);

static int const step = 100000;
static int const end_size = 1100000;

enum : int
{
    SAMPLE_SIZE = 100,
};

typedef void (*perf_fn)(void);
static void test_dynamic_insert(void);
static void test_dynamic_insert_remove(void);
static void test_fixed_insert(void);
static void test_fixed_insert_remove(void);
static void test_successful_find_time(void);
static void test_unsuccessful_find_time(void);
static perf_fn const perf_tests[] = {
    test_unsuccessful_find_time, test_fixed_insert_remove,
    test_dynamic_insert_remove,  test_successful_find_time,
    test_fixed_insert,           test_dynamic_insert,
};
static ptrdiff_t const num_tests
    = (ptrdiff_t)(sizeof(perf_tests) / sizeof(perf_tests[0]));

static void iota_keys(int *keys, ptrdiff_t n_keys, int start_key);
static void *malloc_or_exit(size_t bytes);
static bool add_overflow(int a, int b);
static uint64_t hash_int_to_u64(int key);
static uint64_t hash_key(ccc_user_key k);
static ptrdiff_t n_with_load_factor(ptrdiff_t n);

int
main()
{
    random_seed(time(NULL));
    for (ptrdiff_t i = 0; i < num_tests; ++i)
    {
        perf_tests[i]();
    }
    return 0;
}

#define insert_or_assign_n_keys(map_ptr, type_name, keys_ptr, n_keys)          \
    do                                                                         \
    {                                                                          \
        for (int i = 0; i < (n_keys); ++i)                                     \
        {                                                                      \
            (void)insert_or_assign(map_ptr,                                    \
                                   &(type_name){.key = (keys_ptr)[i]}.e);      \
        }                                                                      \
    } while (0)

#define remove_n_keys(map_ptr, type_name, keys_ptr, n_keys)                    \
    do                                                                         \
    {                                                                          \
        for (int i = 0; i < (n_keys); ++i)                                     \
        {                                                                      \
            (void)ccc_remove(map_ptr, &(type_name){.key = (keys_ptr)[i]}.e);   \
        }                                                                      \
    } while (0)

/* A smart compiler could mess up timing here if it sees we throw away the
query so put the blindfold on. The other functions have side effects so ok. */
#define find_n_keys(map_ptr, keys_ptr, n_keys)                                 \
    do                                                                         \
    {                                                                          \
        for (int i = 0; i < (n_keys); ++i)                                     \
        {                                                                      \
            __auto_type volatile k = get_key_val((map_ptr), &((keys_ptr)[i])); \
            (void)k;                                                           \
        }                                                                      \
    } while (0)

#define time_remove_n_keys(map_ptr, map_abbrev, type_name, keys_ptr, n_keys)   \
    (__extension__({                                                           \
        clock_t map_abbrev##remove_begin = clock();                            \
        remove_n_keys(map_ptr, type_name, (keys_ptr), (n_keys));               \
        clock_t map_abbrev##remove_end = clock();                              \
        double const map_abbrev##remove_map_time                               \
            = (double)(map_abbrev##remove_end - map_abbrev##remove_begin)      \
              / CLOCKS_PER_SEC;                                                \
        map_abbrev##remove_map_time;                                           \
    }))

#define time_alloc_insert_n_keys(map_type, map_abbrev, type_name, keys_ptr,    \
                                 n_keys)                                       \
    (__extension__({                                                           \
        map_type map_abbrev##_map                                              \
            = map_abbrev##_init((type_name *)NULL, e, key, hash_key,           \
                                type_name##_eq, std_alloc, NULL, 0);           \
        clock_t map_abbrev##begin = clock();                                   \
        insert_or_assign_n_keys(&(map_abbrev##_map), type_name, (keys_ptr),    \
                                (n_keys));                                     \
        clock_t map_abbrev##end = clock();                                     \
        double const map_abbrev##_map_time                                     \
            = (double)(map_abbrev##end - map_abbrev##begin) / CLOCKS_PER_SEC;  \
        (void)map_abbrev##_clear_and_free(&(map_abbrev##_map), NULL);          \
        map_abbrev##_map_time;                                                 \
    }))

#define time_fixed_insert_n_keys(map_type, map_abbrev, type_name, keys_ptr,    \
                                 n_keys)                                       \
    (__extension__({                                                           \
        /*NOLINTNEXTLINE*/                                                     \
        type_name *const type_name##_buf                                       \
            = malloc_or_exit(sizeof(type_name) * n_with_load_factor(n_keys));  \
        map_type map_abbrev##_map = map_abbrev##_init(                         \
            type_name##_buf, e, key, hash_key, type_name##_eq, NULL, NULL,     \
            n_with_load_factor(n_keys));                                       \
        clock_t map_abbrev##begin = clock();                                   \
        insert_or_assign_n_keys(&(map_abbrev##_map), type_name, (keys_ptr),    \
                                (n_keys));                                     \
        clock_t map_abbrev##end = clock();                                     \
        double const map_abbrev##_map_time                                     \
            = (double)(map_abbrev##end - map_abbrev##begin) / CLOCKS_PER_SEC;  \
        free(type_name##_buf);                                                 \
        map_abbrev##_map_time;                                                 \
    }))

#define time_alloc_insert_remove_n_keys(map_type, map_abbrev, type_name,       \
                                        keys_ptr, n_keys)                      \
    (__extension__({                                                           \
        map_type map_abbrev##_map                                              \
            = map_abbrev##_init((type_name *)NULL, e, key, hash_key,           \
                                type_name##_eq, std_alloc, NULL, 0);           \
        clock_t map_abbrev##begin = clock();                                   \
        insert_or_assign_n_keys(&(map_abbrev##_map), type_name, (keys_ptr),    \
                                (n_keys));                                     \
        clock_t map_abbrev##end = clock();                                     \
        double map_abbrev##_map_time                                           \
            = (double)(map_abbrev##end - map_abbrev##begin) / CLOCKS_PER_SEC;  \
        map_abbrev##_map_time += time_remove_n_keys(                           \
            &(map_abbrev##_map), map_abbrev, type_name, keys_ptr, n_keys);     \
        (void)map_abbrev##_clear_and_free(&(map_abbrev##_map), NULL);          \
        map_abbrev##_map_time;                                                 \
    }))

#define time_fixed_insert_remove_n_keys(map_type, map_abbrev, type_name,       \
                                        keys_ptr, n_keys)                      \
    (__extension__({                                                           \
        /*NOLINTNEXTLINE*/                                                     \
        type_name *const type_name##_buf                                       \
            = malloc_or_exit(sizeof(type_name) * n_with_load_factor(n_keys));  \
        map_type map_abbrev##_map = map_abbrev##_init(                         \
            type_name##_buf, e, key, hash_key, type_name##_eq, NULL, NULL,     \
            n_with_load_factor(n_keys));                                       \
        clock_t map_abbrev##begin = clock();                                   \
        insert_or_assign_n_keys(&(map_abbrev##_map), type_name, (keys_ptr),    \
                                (n_keys));                                     \
        clock_t map_abbrev##end = clock();                                     \
        double map_abbrev##_map_time                                           \
            = (double)(map_abbrev##end - map_abbrev##begin) / CLOCKS_PER_SEC;  \
        map_abbrev##_map_time += time_remove_n_keys(                           \
            &(map_abbrev##_map), map_abbrev, type_name, keys_ptr, n_keys);     \
        free(type_name##_buf);                                                 \
        map_abbrev##_map_time;                                                 \
    }))

#define time_find_n_keys_success(map_type, map_abbrev, type_name, keys_ptr,    \
                                 n_keys)                                       \
    (__extension__({                                                           \
        /*NOLINTNEXTLINE*/                                                     \
        type_name *const type_name##_buf                                       \
            = malloc_or_exit(sizeof(type_name) * n_with_load_factor(n_keys));  \
        map_type map_abbrev##_map = map_abbrev##_init(                         \
            type_name##_buf, e, key, hash_key, type_name##_eq, NULL, NULL,     \
            n_with_load_factor(n_keys));                                       \
        insert_or_assign_n_keys(&(map_abbrev##_map), type_name, (keys_ptr),    \
                                (n_keys));                                     \
        clock_t map_abbrev##begin = clock();                                   \
        find_n_keys(&(map_abbrev##_map), (keys_ptr), (n_keys));                \
        clock_t map_abbrev##end = clock();                                     \
        double const map_abbrev##_map_time                                     \
            = (double)(map_abbrev##end - map_abbrev##begin) / CLOCKS_PER_SEC;  \
        free(type_name##_buf);                                                 \
        map_abbrev##_map_time;                                                 \
    }))

#define time_find_n_keys_failure(map_type, map_abbrev, type_name, keys_ptr,    \
                                 n_keys)                                       \
    (__extension__({                                                           \
        /*NOLINTNEXTLINE*/                                                     \
        type_name *const type_name##_buf                                       \
            = malloc_or_exit(sizeof(type_name) * n_with_load_factor(n_keys));  \
        map_type map_abbrev##_map = map_abbrev##_init(                         \
            type_name##_buf, e, key, hash_key, type_name##_eq, NULL, NULL,     \
            n_with_load_factor(n_keys));                                       \
        insert_or_assign_n_keys(&(map_abbrev##_map), type_name, (keys_ptr),    \
                                (n_keys));                                     \
        iota_keys(keys_ptr, n_keys, INT_MIN);                                  \
        clock_t map_abbrev##begin = clock();                                   \
        find_n_keys(&(map_abbrev##_map), (keys_ptr), (n_keys));                \
        clock_t map_abbrev##end = clock();                                     \
        double const map_abbrev##_map_time                                     \
            = (double)(map_abbrev##end - map_abbrev##begin) / CLOCKS_PER_SEC;  \
        free(type_name##_buf);                                                 \
        map_abbrev##_map_time;                                                 \
    }))

/** How long does it take to grow a map to N elements when it can re-size. */
static void
test_dynamic_insert(void)
{
    printf("insert N elements into dynamic maps, fhmap vs hhmap \n");
    for (enum struct_size s = SMALL; s < TYPES_END; ++s)
    {
        printf("STRUCT SIZE: %s\n", struct_size_strs[s]);
        for (int n = step; n < end_size; n += step)
        {
            printf("N: %d, ", n);
            int *const keys = malloc_or_exit(sizeof(int) * n);
            iota_keys(keys, n, -(n / 2));
            rand_shuffle(sizeof(int), keys, n, &(int){});
            switch (s)
            {
            case SMALL:
            {
                double const fhmap_time = time_alloc_insert_n_keys(
                    flat_hash_map, fhm, small_fhmap_type, keys, n);
                double const hhmap_time = time_alloc_insert_n_keys(
                    handle_hash_map, hhm, small_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(small_fhmap_type), sizeof(small_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case MEDIUM:
            {
                double const fhmap_time = time_alloc_insert_n_keys(
                    flat_hash_map, fhm, medium_fhmap_type, keys, n);
                double const hhmap_time = time_alloc_insert_n_keys(
                    handle_hash_map, hhm, medium_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(medium_fhmap_type),
                             sizeof(medium_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            case LARGE:
            {
                double const fhmap_time = time_alloc_insert_n_keys(
                    flat_hash_map, fhm, large_fhmap_type, keys, n);
                double const hhmap_time = time_alloc_insert_n_keys(
                    handle_hash_map, hhm, large_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(large_fhmap_type), sizeof(large_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case XLARGE:
            {
                double const fhmap_time = time_alloc_insert_n_keys(
                    flat_hash_map, fhm, xlarge_fhmap_type, keys, n);
                double const hhmap_time = time_alloc_insert_n_keys(
                    handle_hash_map, hhm, xlarge_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(xlarge_fhmap_type),
                             sizeof(xlarge_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            default:
                return;
            }
            free(keys);
        }
    }
}

/** Which map is better at insert when no resizing is needed and all memory has
been allocated upfront. */
static void
test_fixed_insert(void)
{
    printf("insert N elements into fixed size maps, fhmap vs hhmap \n");
    for (enum struct_size s = SMALL; s < TYPES_END; ++s)
    {
        printf("STRUCT SIZE: %s\n", struct_size_strs[s]);
        for (int n = step; n < end_size; n += step)
        {
            printf("N: %d, ", n);
            int *const keys = malloc_or_exit(sizeof(int) * n);
            iota_keys(keys, n, -(n / 2));
            rand_shuffle(sizeof(int), keys, n, &(int){});
            switch (s)
            {
            case SMALL:
            {
                double const fhmap_time = time_fixed_insert_n_keys(
                    flat_hash_map, fhm, small_fhmap_type, keys, n);
                double const hhmap_time = time_fixed_insert_n_keys(
                    handle_hash_map, hhm, small_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(small_fhmap_type), sizeof(small_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case MEDIUM:
            {
                double const fhmap_time = time_fixed_insert_n_keys(
                    flat_hash_map, fhm, medium_fhmap_type, keys, n);
                double const hhmap_time = time_fixed_insert_n_keys(
                    handle_hash_map, hhm, medium_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(medium_fhmap_type),
                             sizeof(medium_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            case LARGE:
            {
                double const fhmap_time = time_fixed_insert_n_keys(
                    flat_hash_map, fhm, large_fhmap_type, keys, n);
                double const hhmap_time = time_fixed_insert_n_keys(
                    handle_hash_map, hhm, large_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(large_fhmap_type), sizeof(large_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case XLARGE:
            {
                double const fhmap_time = time_fixed_insert_n_keys(
                    flat_hash_map, fhm, xlarge_fhmap_type, keys, n);
                double const hhmap_time = time_fixed_insert_n_keys(
                    handle_hash_map, hhm, xlarge_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(xlarge_fhmap_type),
                             sizeof(xlarge_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            default:
                return;
            }
            free(keys);
        }
    }
}

static void
test_dynamic_insert_remove(void)
{
    printf("insert N remove N elements into dynamic maps, fhmap vs hhmap \n");
    for (enum struct_size s = SMALL; s < TYPES_END; ++s)
    {
        printf("STRUCT SIZE: %s\n", struct_size_strs[s]);
        for (int n = step; n < end_size; n += step)
        {
            printf("N: %d, ", n);
            int *const keys = malloc_or_exit(sizeof(int) * n);
            iota_keys(keys, n, -(n / 2));
            rand_shuffle(sizeof(int), keys, n, &(int){});
            switch (s)
            {
            case SMALL:
            {
                double const fhmap_time = time_alloc_insert_remove_n_keys(
                    flat_hash_map, fhm, small_fhmap_type, keys, n);
                double const hhmap_time = time_alloc_insert_remove_n_keys(
                    handle_hash_map, hhm, small_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(small_fhmap_type), sizeof(small_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case MEDIUM:
            {
                double const fhmap_time = time_alloc_insert_remove_n_keys(
                    flat_hash_map, fhm, medium_fhmap_type, keys, n);
                double const hhmap_time = time_alloc_insert_remove_n_keys(
                    handle_hash_map, hhm, medium_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(medium_fhmap_type),
                             sizeof(medium_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            case LARGE:
            {
                double const fhmap_time = time_alloc_insert_remove_n_keys(
                    flat_hash_map, fhm, large_fhmap_type, keys, n);
                double const hhmap_time = time_alloc_insert_remove_n_keys(
                    handle_hash_map, hhm, large_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(large_fhmap_type), sizeof(large_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case XLARGE:
            {
                double const fhmap_time = time_alloc_insert_remove_n_keys(
                    flat_hash_map, fhm, xlarge_fhmap_type, keys, n);
                double const hhmap_time = time_alloc_insert_remove_n_keys(
                    handle_hash_map, hhm, xlarge_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(xlarge_fhmap_type),
                             sizeof(xlarge_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            default:
                return;
            }
            free(keys);
        }
    }
}

static void
test_fixed_insert_remove(void)
{
    printf("insert N remove N elements into fixed maps, fhmap vs hhmap \n");
    for (enum struct_size s = SMALL; s < TYPES_END; ++s)
    {
        printf("STRUCT SIZE: %s\n", struct_size_strs[s]);
        for (int n = step; n < end_size; n += step)
        {
            printf("N: %d, ", n);
            int *const keys = malloc_or_exit(sizeof(int) * n);
            iota_keys(keys, n, -(n / 2));
            rand_shuffle(sizeof(int), keys, n, &(int){});
            switch (s)
            {
            case SMALL:
            {
                double const fhmap_time = time_fixed_insert_remove_n_keys(
                    flat_hash_map, fhm, small_fhmap_type, keys, n);
                double const hhmap_time = time_fixed_insert_remove_n_keys(
                    handle_hash_map, hhm, small_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(small_fhmap_type), sizeof(small_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case MEDIUM:
            {
                double const fhmap_time = time_fixed_insert_remove_n_keys(
                    flat_hash_map, fhm, medium_fhmap_type, keys, n);
                double const hhmap_time = time_fixed_insert_remove_n_keys(
                    handle_hash_map, hhm, medium_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(medium_fhmap_type),
                             sizeof(medium_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            case LARGE:
            {
                double const fhmap_time = time_fixed_insert_remove_n_keys(
                    flat_hash_map, fhm, large_fhmap_type, keys, n);
                double const hhmap_time = time_fixed_insert_remove_n_keys(
                    handle_hash_map, hhm, large_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(large_fhmap_type), sizeof(large_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case XLARGE:
            {
                double const fhmap_time = time_fixed_insert_remove_n_keys(
                    flat_hash_map, fhm, xlarge_fhmap_type, keys, n);
                double const hhmap_time = time_fixed_insert_remove_n_keys(
                    handle_hash_map, hhm, xlarge_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(xlarge_fhmap_type),
                             sizeof(xlarge_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            default:
                return;
            }
            free(keys);
        }
    }
}

static void
test_successful_find_time(void)
{
    printf("successfully find N keys, fhmap vs hhmap \n");
    for (enum struct_size s = SMALL; s < TYPES_END; ++s)
    {
        printf("STRUCT SIZE: %s\n", struct_size_strs[s]);
        for (int n = step; n < end_size; n += step)
        {
            printf("N: %d, ", n);
            int *const keys = malloc_or_exit(sizeof(int) * n);
            iota_keys(keys, n, -(n / 2));
            rand_shuffle(sizeof(int), keys, n, &(int){});
            switch (s)
            {
            case SMALL:
            {
                double const fhmap_time = time_find_n_keys_success(
                    flat_hash_map, fhm, small_fhmap_type, keys, n);
                double const hhmap_time = time_find_n_keys_success(
                    handle_hash_map, hhm, small_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(small_fhmap_type), sizeof(small_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case MEDIUM:
            {
                double const fhmap_time = time_find_n_keys_success(
                    flat_hash_map, fhm, medium_fhmap_type, keys, n);
                double const hhmap_time = time_find_n_keys_success(
                    handle_hash_map, hhm, medium_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(medium_fhmap_type),
                             sizeof(medium_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            case LARGE:
            {
                double const fhmap_time = time_find_n_keys_success(
                    flat_hash_map, fhm, large_fhmap_type, keys, n);
                double const hhmap_time = time_find_n_keys_success(
                    handle_hash_map, hhm, large_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(large_fhmap_type), sizeof(large_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case XLARGE:
            {
                double const fhmap_time = time_find_n_keys_success(
                    flat_hash_map, fhm, xlarge_fhmap_type, keys, n);
                double const hhmap_time = time_find_n_keys_success(
                    handle_hash_map, hhm, xlarge_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(xlarge_fhmap_type),
                             sizeof(xlarge_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            default:
                return;
            }
            free(keys);
        }
    }
}

static void
test_unsuccessful_find_time(void)
{
    printf("fail to find N keys, fhmap vs hhmap \n");
    for (enum struct_size s = SMALL; s < TYPES_END; ++s)
    {
        printf("STRUCT SIZE: %s\n", struct_size_strs[s]);
        for (int n = step; n < end_size; n += step)
        {
            printf("N: %d, ", n);
            int *const keys = malloc_or_exit(sizeof(int) * n);
            iota_keys(keys, n, -(n / 2));
            rand_shuffle(sizeof(int), keys, n, &(int){});
            switch (s)
            {
            case SMALL:
            {
                double const fhmap_time = time_find_n_keys_failure(
                    flat_hash_map, fhm, small_fhmap_type, keys, n);
                double const hhmap_time = time_find_n_keys_failure(
                    handle_hash_map, hhm, small_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(small_fhmap_type), sizeof(small_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case MEDIUM:
            {
                double const fhmap_time = time_find_n_keys_failure(
                    flat_hash_map, fhm, medium_fhmap_type, keys, n);
                double const hhmap_time = time_find_n_keys_failure(
                    handle_hash_map, hhm, medium_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(medium_fhmap_type),
                             sizeof(medium_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            case LARGE:
            {
                double const fhmap_time = time_find_n_keys_failure(
                    flat_hash_map, fhm, large_fhmap_type, keys, n);
                double const hhmap_time = time_find_n_keys_failure(
                    handle_hash_map, hhm, large_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(large_fhmap_type), sizeof(large_hhmap_type),
                             fhmap_time, hhmap_time);
            }
            break;
            case XLARGE:
            {
                double const fhmap_time = time_find_n_keys_failure(
                    flat_hash_map, fhm, xlarge_fhmap_type, keys, n);
                double const hhmap_time = time_find_n_keys_failure(
                    handle_hash_map, hhm, xlarge_hhmap_type, keys, n);
                (void)printf("FHMAP_TYPE_SIZE: %zu, HHMAP_TYPE_SIZE: %zu, "
                             "FHMAP: %f, HHMAP: %f\n",
                             sizeof(xlarge_fhmap_type),
                             sizeof(xlarge_hhmap_type), fhmap_time, hhmap_time);
            }
            break;
            default:
                return;
            }
            free(keys);
        }
    }
}

/*=======================   Static Helpers   =================================*/

/** Generates a random sequence of int keys from [start, n) on the heap. It is
the user's responsibility to free the memory. */
static void
iota_keys(int *const keys, ptrdiff_t n_keys, int const start_key)
{
    if (n_keys < 0 || (start_key > 0 && add_overflow((int)n_keys, start_key)))
    {
        (void)fprintf(stderr, "iota range exceeds range of int.\n");
        exit(1);
    }
    for (int i = 0, key = start_key; i < n_keys; ++i, ++key)
    {
        keys[i] = key;
    }
}

static void *
malloc_or_exit(size_t bytes)
{
    void *mem = malloc(bytes);
    if (!mem)
    {
        (void)fprintf(stderr, "heap is exhausted in perf, exiting program.\n");
        exit(1);
    }
    return mem;
}

static inline bool
add_overflow(int const a, int const b)
{
    return INT_MAX - b < a;
}

static uint64_t
hash_key(ccc_user_key k)
{
    return hash_int_to_u64(*(int *)k.user_key);
}

static inline uint64_t
hash_int_to_u64(int const key)
{
    uint64_t x = key;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

/** If we pre-allocate the tables we want to test the algorithms for now under
favorable load factors so just make it roughly 50%. This can be tuned and
tested more later. */
static inline ptrdiff_t
n_with_load_factor(ptrdiff_t const n)
{
    return n * 3;
}
