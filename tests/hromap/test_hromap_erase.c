#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "hromap_util.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(hromap_test_insert_erase_shuffled)
{
    CCC_handle_realtime_ordered_map s
        = hrm_init(&(small_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   SMALL_FIXED_CAP);
    size_t const size = 50;
    int const prime = 53;
    CHECK(insert_shuffled(&s, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 0; i + 1 < size; ++i)
    {
        CHECK(sorted_check[i] <= sorted_check[i + 1], true);
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        CCC_handle const *const h = remove_r(&s, &(struct val){.id = (int)i});
        CHECK(occupied(h), true);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_prime_shuffle)
{
    CCC_handle_realtime_ordered_map s
        = hrm_init(&(small_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   SMALL_FIXED_CAP);
    size_t const size = 50;
    size_t const prime = 53;
    size_t const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    size_t shuffled_index = prime % (size - less);
    bool repeats[50];
    memset(repeats, false, sizeof(bool) * size);
    for (size_t i = 0; i < size; ++i)
    {
        if (occupied(
                try_insert_r(&s, &(struct val){.id = (int)shuffled_index,
                                               .val = (int)shuffled_index})))
        {
            repeats[i] = true;
        }
        CHECK(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    CHECK(hrm_count(&s).count < size, true);
    for (size_t i = 0; i < size; ++i)
    {
        CCC_handle const *const e = remove_handle_r(handle_r(&s, &i));
        CHECK(occupied(e) || repeats[i], true);
        CHECK(validate(&s), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_weak_srand)
{
    CCC_handle_realtime_ordered_map s
        = hrm_init(&(standard_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   STANDARD_FIXED_CAP);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        (void)swap_handle(&s, &(struct val){.id = rand_i, .val = i});
        id_keys[i] = rand_i;
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_handle const h = CCC_remove(&s, &(struct val){.id = id_keys[i]});
        CHECK(occupied(&h), true);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_erase_cycles_no_alloc)
{
    CCC_handle_realtime_ordered_map s
        = hrm_init(&(standard_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   STANDARD_FIXED_CAP);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        (void)insert_or_assign(&s, &(struct val){.id = rand_i, .val = i});
        id_keys[i] = rand_i;
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_handle h = CCC_remove(&s, &(struct val){.id = id_keys[i]});
        CHECK(occupied(&h), true);
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_handle h = insert_or_assign(&s, &(struct val){.id = id_keys[i]});
        CHECK(occupied(&h), false);
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_handle h = CCC_remove(&s, &(struct val){.id = id_keys[i]});
        CHECK(occupied(&h), true);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_erase_cycles_alloc)
{
    CCC_handle_realtime_ordered_map s
        = hrm_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    srand(time(NULL)); /* NOLINT */
    int const num_nodes = 1000;
    int id_keys[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        int const rand_i = rand(); /* NOLINT */
        (void)insert_or_assign(&s, &(struct val){.id = rand_i, .val = i});
        id_keys[i] = rand_i;
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_handle h = CCC_remove(&s, &(struct val){.id = id_keys[i]});
        CHECK(occupied(&h), true);
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes / 2; ++i)
    {
        CCC_handle h = insert_or_assign(&s, &(struct val){.id = id_keys[i]});
        CHECK(occupied(&h), false);
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_handle h = CCC_remove(&s, &(struct val){.id = id_keys[i]});
        CHECK(occupied(&h), true);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN(hrm_clear_and_free(&s, NULL););
}

int
main()
{
    return CHECK_RUN(hromap_test_insert_erase_shuffled(),
                     hromap_test_prime_shuffle(), hromap_test_weak_srand(),
                     hromap_test_insert_erase_cycles_no_alloc(),
                     hromap_test_insert_erase_cycles_alloc());
}
