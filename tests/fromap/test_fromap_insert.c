#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "flat_realtime_ordered_map.h"
#include "fromap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

CHECK_BEGIN_STATIC_FN(fromap_test_insert_one)
{
    flat_realtime_ordered_map s
        = frm_init((struct val[2]){}, 2, elem, id, NULL, val_cmp, NULL);
    CHECK(occupied(insert_r(&s, &(struct val){}.elem)), false);
    CHECK(is_empty(&s), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_macros)
{
    /* This is also a good test to see if the buffer can manage its own memory
       when provided with a std_alloc function starting from NULL. */
    flat_realtime_ordered_map s
        = frm_init((struct val *)NULL, 0, elem, id, std_alloc, val_cmp, NULL);
    struct val *v = frm_or_insert_w(entry_r(&s, &(int){0}), (struct val){});
    CHECK(v != NULL, true);
    v = frm_insert_entry_w(entry_r(&s, &(int){0}),
                           (struct val){.id = 0, .val = 99});
    CHECK(validate(&s), true);
    CHECK(v == NULL, false);
    CHECK(v->val, 99);
    v = frm_insert_entry_w(entry_r(&s, &(int){9}),
                           (struct val){.id = 9, .val = 100});
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    v = unwrap(frm_insert_or_assign_w(&s, 1, (struct val){.val = 100}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(size(&s), 3);
    v = unwrap(frm_insert_or_assign_w(&s, 1, (struct val){.val = 99}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(size(&s), 3);
    v = unwrap(frm_try_insert_w(&s, 1, (struct val){.val = 2}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(size(&s), 3);
    v = unwrap(frm_try_insert_w(&s, 2, (struct val){.val = 2}));
    CHECK(validate(&s), true);
    CHECK(v != NULL, true);
    CHECK(v->val, 2);
    CHECK(size(&s), 4);
    CHECK_END_FN(frm_clear_and_free(&s, NULL););
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_shuffle)
{
    size_t const size = 50;
    ccc_flat_realtime_ordered_map s
        = frm_init((struct val[51]){}, 51, elem, id, NULL, val_cmp, NULL);
    CHECK(size > 1, true);
    int const prime = 53;
    CHECK(insert_shuffled(&s, size, prime), PASS);

    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (size_t i = 1; i < size; ++i)
    {
        CHECK(sorted_check[i - 1] <= sorted_check[i], true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    ccc_flat_realtime_ordered_map s
        = frm_init((struct val[1001]){}, 1001, elem, id, NULL, val_cmp, NULL);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        ccc_entry const e
            = insert(&s, &(struct val){.id = rand() /*NOLINT*/, .val = i}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&s), true);
    }
    CHECK(size(&s), (size_t)num_nodes);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fromap_test_insert_one(), fromap_test_insert_macros(),
                     fromap_test_insert_shuffle(),
                     fromap_test_insert_weak_srand());
}
