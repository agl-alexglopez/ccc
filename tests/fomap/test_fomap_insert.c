#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "flat_ordered_map.h"
#include "fomap_util.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

BEGIN_STATIC_TEST(fomap_test_insert_one)
{
    flat_ordered_map s
        = fom_init((struct val[2]){}, 2, elem, id, NULL, val_cmp, NULL);
    CHECK(occupied(insert_r(&s, &(struct val){}.elem)), false);
    CHECK(is_empty(&s), false);
    struct val *v = fom_root(&s);
    CHECK(v == NULL, false);
    CHECK(v->val, 0);
    END_TEST();
}

BEGIN_STATIC_TEST(fomap_test_insert_macros)
{
    /* This is also a good test to see if the buffer can manage its own memory
       when provided with a std_alloc function starting from NULL. */
    flat_ordered_map s
        = fom_init((struct val *)NULL, 0, elem, id, std_alloc, val_cmp, NULL);
    struct val *v = fom_or_insert_w(entry_r(&s, &(int){0}), (struct val){});
    CHECK(v != NULL, true);
    v = fom_insert_entry_w(entry_r(&s, &(int){0}),
                           (struct val){.id = 0, .val = 99});
    CHECK(validate(&s), true);
    CHECK(v == NULL, false);
    CHECK(v->val, 99);
    v = fom_insert_entry_w(entry_r(&s, &(int){9}),
                           (struct val){.id = 9, .val = 100});
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    v = unwrap(fom_insert_or_assign_w(&s, 1, (struct val){.val = 100}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(size(&s), 3);
    v = unwrap(fom_insert_or_assign_w(&s, 1, (struct val){.val = 99}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(size(&s), 3);
    v = unwrap(fom_try_insert_w(&s, 1, (struct val){.val = 2}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(size(&s), 3);
    v = unwrap(fom_try_insert_w(&s, 2, (struct val){.val = 2}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->val, 2);
    CHECK(size(&s), 4);
    END_TEST(fom_clear_and_free(&s, NULL););
}

BEGIN_STATIC_TEST(fomap_test_insert_shuffle)
{
    size_t const size = 50;
    ccc_flat_ordered_map s
        = fom_init((struct val[51]){}, 51, elem, id, NULL, val_cmp, NULL);
    CHECK(size > 1, true);
    int const prime = 53;
    CHECK(insert_shuffled(&s, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 1; i < size; ++i)
    {
        CHECK(sorted_check[i - 1] <= sorted_check[i], true);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(fomap_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    ccc_flat_ordered_map s
        = fom_init((struct val[1001]){}, 1001, elem, id, NULL, val_cmp, NULL);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        ccc_entry const e = insert(
            &s, &(struct val){.id = rand() /* NOLINT */, .val = i}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&s), true);
    }
    CHECK(size(&s), (size_t)num_nodes);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(fomap_test_insert_one(), fomap_test_insert_macros(),
                     fomap_test_insert_shuffle(),
                     fomap_test_insert_weak_srand());
}
