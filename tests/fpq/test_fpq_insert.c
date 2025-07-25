#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(fpq_test_insert_one)
{
    struct val single[2] = {};
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(single, struct val, CCC_LES, val_cmp, NULL, NULL,
                       (sizeof(single) / sizeof(single[0])));
    single[0].val = 0;
    (void)push(&fpq, &single[0], &(struct val){});
    CHECK(ccc_fpq_is_empty(&fpq), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_insert_three)
{
    size_t size = 3;
    struct val three_vals[4] = {};
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(three_vals, struct val, CCC_LES, val_cmp, NULL, NULL,
                       (sizeof(three_vals) / sizeof(three_vals[0])));
    for (size_t i = 0; i < size; ++i)
    {
        three_vals[i].val = (int)i;
        (void)push(&fpq, &three_vals[i], &(struct val){});
        CHECK(validate(&fpq), true);
        CHECK(ccc_fpq_count(&fpq).count, i + 1);
    }
    CHECK(ccc_fpq_count(&fpq).count, size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_struct_getter)
{
    size_t const size = 10;
    struct val vals[10 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(vals, struct val, CCC_LES, val_cmp, NULL, NULL,
                       (sizeof(vals) / sizeof(vals[0])));
    struct val tester_clone[10 + 1];
    ccc_flat_priority_queue fpq_clone
        = ccc_fpq_init(tester_clone, struct val, CCC_LES, val_cmp, NULL, NULL,
                       (sizeof(vals) / sizeof(vals[0])));
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
    CHECK(ccc_fpq_count(&fpq).count, (size_t)10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_insert_three_dups)
{
    struct val three_vals[3 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(three_vals, struct val, CCC_LES, val_cmp, NULL, NULL,
                       (sizeof(three_vals) / sizeof(three_vals[0])));
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        (void)push(&fpq, &three_vals[i], &(struct val){});
        CHECK(validate(&fpq), true);
        CHECK(ccc_fpq_count(&fpq).count, (size_t)i + 1);
    }
    CHECK(ccc_fpq_count(&fpq).count, (size_t)3);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_insert_shuffle)
{
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(vals, struct val, CCC_LES, val_cmp, NULL, NULL,
                       (sizeof(vals) / sizeof(vals[0])));
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS);

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
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_insert_shuffle_grow)
{
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(NULL, struct val, CCC_LES, val_cmp, std_alloc, NULL, 0);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS);

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
    CHECK_END_FN((void)ccc_fpq_clear_and_free(&fpq, NULL););
}

CHECK_BEGIN_STATIC_FN(fpq_test_insert_shuffle_reserve)
{
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(NULL, struct val, CCC_LES, val_cmp, NULL, NULL, 0);
    ccc_result const r = ccc_fpq_reserve(&fpq, 50, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    CHECK(insert_shuffled(&fpq, vals, size, prime), PASS);
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
    CHECK_END_FN(clear_and_free_reserve(&fpq, NULL, std_alloc););
}

CHECK_BEGIN_STATIC_FN(fpq_test_read_max_min)
{
    size_t const size = 10;
    struct val vals[10 + 1];
    ccc_flat_priority_queue fpq
        = ccc_fpq_init(vals, struct val, CCC_LES, val_cmp, NULL, NULL,
                       (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)size - (int)i;
        (void)push(&fpq, &vals[i], &(struct val){});
        CHECK(validate(&fpq), true);
        CHECK(ccc_fpq_count(&fpq).count, i + 1);
    }
    CHECK(ccc_fpq_count(&fpq).count, (size_t)10);
    struct val const *min = front(&fpq);
    CHECK(min->val, size - (size - 1));
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fpq_test_insert_one(), fpq_test_insert_three(),
                     fpq_test_struct_getter(), fpq_test_insert_three_dups(),
                     fpq_test_insert_shuffle(), fpq_test_insert_shuffle_grow(),
                     fpq_test_insert_shuffle_reserve(),
                     fpq_test_read_max_min());
}
