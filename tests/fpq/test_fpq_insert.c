#define TRAITS_USING_NAMESPACE_CCC

#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

BEGIN_STATIC_TEST(fpq_test_insert_one)
{
    struct val single[2] = {};
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(single, (sizeof(single) / sizeof(single[0])), CCC_LES,
                       NULL, val_cmp, NULL);
    single[0].val = 0;
    (void)push(&fpq, &single[0]);
    CHECK(ccc_fpq_is_empty(&fpq), false);
    END_TEST();
}

BEGIN_STATIC_TEST(fpq_test_insert_three)
{
    size_t size = 3;
    struct val three_vals[4] = {};
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(three_vals, (sizeof(three_vals) / sizeof(three_vals[0])),
                       CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        three_vals[i].val = (int)i;
        (void)push(&fpq, &three_vals[i]);
        CHECK(validate(&fpq), true);
        CHECK(ccc_fpq_size(&fpq), i + 1);
    }
    CHECK(ccc_fpq_size(&fpq), size);
    END_TEST();
}

BEGIN_STATIC_TEST(fpq_test_struct_getter)
{
    size_t const size = 10;
    struct val vals[10 + 1];
    ccc_flat_priority_queue fpq = ccc_fpq_init(
        vals, (sizeof(vals) / sizeof(vals[0])), CCC_LES, NULL, val_cmp, NULL);
    struct val tester_clone[10 + 1];
    ccc_flat_priority_queue fpq_clone
        = ccc_fpq_init(tester_clone, (sizeof(vals) / sizeof(vals[0])), CCC_LES,
                       NULL, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *res1
            = ccc_fpq_emplace(&fpq, (struct val){.id = (int)i, .val = (int)i});
        CHECK(res1 != NULL, true);
        struct val const *res2 = ccc_fpq_emplace(
            &fpq_clone, (struct val){.id = (int)i, .val = (int)i});
        CHECK(res2 != NULL, true);
        CHECK(validate(&fpq), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)10);
    END_TEST();
}

BEGIN_STATIC_TEST(fpq_test_insert_three_dups)
{
    struct val three_vals[3 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(three_vals, (sizeof(three_vals) / sizeof(three_vals[0])),
                       CCC_LES, NULL, val_cmp, NULL);
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        (void)push(&fpq, &three_vals[i]);
        CHECK(validate(&fpq), true);
        CHECK(ccc_fpq_size(&fpq), (size_t)i + 1);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)3);
    END_TEST();
}

BEGIN_STATIC_TEST(fpq_test_insert_shuffle)
{
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50 + 1];
    ccc_flat_priority_queue fpq = ccc_fpq_init(
        vals, (sizeof(vals) / sizeof(vals[0])), CCC_LES, NULL, val_cmp, NULL);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS);

    /* Test the printing function at least once. */
    CHECK(ccc_fpq_print(&fpq, val_print), CCC_OK);

    struct val const *min = front(&fpq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &fpq), PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    END_TEST();
}

BEGIN_STATIC_TEST(fpq_test_read_max_min)
{
    size_t const size = 10;
    struct val vals[10 + 1];
    ccc_flat_priority_queue fpq = ccc_fpq_init(
        vals, (sizeof(vals) / sizeof(vals[0])), CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)i;
        (void)push(&fpq, &vals[i]);
        CHECK(validate(&fpq), true);
        CHECK(ccc_fpq_size(&fpq), i + 1);
    }
    CHECK(ccc_fpq_size(&fpq), (size_t)10);
    struct val const *min = front(&fpq);
    CHECK(min->val, 0);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(fpq_test_insert_one(), fpq_test_insert_three(),
                     fpq_test_struct_getter(), fpq_test_insert_three_dups(),
                     fpq_test_insert_shuffle(), fpq_test_read_max_min());
}
