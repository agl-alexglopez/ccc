#define FLAT_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_ordered_map.h"
#include "fomap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

CHECK_BEGIN_STATIC_FN(fomap_test_insert_erase_shuffled)
{
    struct val vals[51];
    ccc_flat_ordered_map s = fom_init(vals, elem, id, id_cmp, NULL, NULL, 51);
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
        struct val *v = unwrap(remove_r(&s, &(struct val){.id = i}.elem));
        CHECK(v != NULL, true);
        CHECK(v->id, i);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_prime_shuffle)
{
    struct val vals[51];
    ccc_flat_ordered_map s = fom_init(vals, elem, id, id_cmp, NULL, NULL, 51);
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
        if (occupied(try_insert_r(&s, &(struct val){.id = (int)shuffled_index,
                                                    .val = (int)shuffled_index}
                                           .elem)))
        {
            repeats[i] = true;
        }
        CHECK(validate(&s), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    CHECK(fom_size(&s) < size, true);
    for (size_t i = 0; i < size; ++i)
    {
        ccc_entry const *const e = remove_entry_r(entry_r(&s, &i));
        CHECK(occupied(e) || repeats[i], true);
        CHECK(validate(&s), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_weak_srand)
{
    struct val vals[1001];
    ccc_flat_ordered_map s
        = fom_init(vals, elem, id, id_cmp, NULL, NULL, 10001);
    /* NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    int id_keys[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        /* NOLINTNEXTLINE */
        int const rand_i = rand();
        (void)swap_entry(&s, &(struct val){.id = rand_i, .val = i}.elem);
        id_keys[i] = rand_i;
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        struct val *v
            = unwrap(remove_r(&s, &(struct val){.id = id_keys[i]}.elem));
        CHECK(v == NULL, false);
        CHECK(validate(&s), true);
    }
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fomap_test_insert_erase_shuffled(),
                     fomap_test_prime_shuffle(), fomap_test_weak_srand());
}
