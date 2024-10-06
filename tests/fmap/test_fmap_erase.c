#define FLAT_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "flat_ordered_map.h"
#include "fmap_util.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

BEGIN_STATIC_TEST(frtomap_test_insert_erase_shuffled)
{
    struct val vals[51];
    ccc_flat_ordered_map s = fom_init(vals, 51, elem, id, NULL, val_cmp, NULL);
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
        struct val *v = unwrap(remove_vr(&s, &(struct val){.id = i}.elem));
        CHECK(v != NULL, true);
        CHECK(v->id, i);
        CHECK(validate(&s), true);
    }
    CHECK(fom_empty(&s), true);
    END_TEST();
}

BEGIN_STATIC_TEST(frtomap_test_prime_shuffle)
{
    struct val vals[51];
    ccc_flat_ordered_map s = fom_init(vals, 51, elem, id, NULL, val_cmp, NULL);
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
        if (occupied(try_insert_vr(&s, &(struct val){.id = (int)shuffled_index,
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
        ccc_entry const *const e = remove_entry_vr(entry_vr(&s, &i));
        CHECK(occupied(e) || repeats[i], true);
        CHECK(validate(&s), true);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(frtomap_test_weak_srand)
{
    struct val vals[1001];
    ccc_flat_ordered_map s
        = fom_init(vals, 10001, elem, id, NULL, val_cmp, NULL);
    /* NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    int id_keys[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        /* NOLINTNEXTLINE */
        int const rand_i = rand();
        (void)insert(&s, &(struct val){.id = rand_i, .val = i}.elem,
                     &(struct val){});
        id_keys[i] = rand_i;
        CHECK(validate(&s), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        struct val *v
            = unwrap(remove_vr(&s, &(struct val){.id = id_keys[i]}.elem));
        CHECK(v == NULL, false);
        CHECK(validate(&s), true);
    }
    CHECK(fom_empty(&s), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(frtomap_test_insert_erase_shuffled(),
                     frtomap_test_prime_shuffle(), frtomap_test_weak_srand());
}
