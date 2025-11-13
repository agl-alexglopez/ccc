#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_priority_queue.h"
#include "flat_priority_queue_util.h"
#include "traits.h"
#include "types.h"
#include "util/allocate.h"

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_one)
{
    struct Val single[2] = {};
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(
            single, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
            (sizeof(single) / sizeof(single[0])));
    single[0].val = 0;
    (void)push(&flat_priority_queue, &single[0], &(struct Val){});
    CHECK(CCC_flat_priority_queue_is_empty(&flat_priority_queue), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_three)
{
    size_t size = 3;
    struct Val three_vals[4] = {};
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(
            three_vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
            (sizeof(three_vals) / sizeof(three_vals[0])));
    for (size_t i = 0; i < size; ++i)
    {
        three_vals[i].val = (int)i;
        (void)push(&flat_priority_queue, &three_vals[i], &(struct Val){});
        CHECK(validate(&flat_priority_queue), true);
        CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count, i + 1);
    }
    CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count, size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_struct_getter)
{
    size_t const size = 10;
    struct Val vals[10 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    struct Val tester_clone[10 + 1];
    CCC_Flat_priority_queue flat_priority_queue_clone
        = CCC_flat_priority_queue_initialize(
            tester_clone, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
            (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < size; ++i)
    {
        struct Val const *res1 = CCC_flat_priority_queue_emplace(
            &flat_priority_queue, (struct Val){.id = (int)i, .val = (int)i});
        CHECK(res1 != NULL, true);
        struct Val const *res2 = CCC_flat_priority_queue_emplace(
            &flat_priority_queue_clone,
            (struct Val){.id = (int)i, .val = (int)i});
        CHECK(res2 != NULL, true);
        CHECK(validate(&flat_priority_queue), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct Val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val);
    }
    CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count,
          (size_t)10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_three_dups)
{
    struct Val three_vals[3 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(
            three_vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
            (sizeof(three_vals) / sizeof(three_vals[0])));
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        (void)push(&flat_priority_queue, &three_vals[i], &(struct Val){});
        CHECK(validate(&flat_priority_queue), true);
        CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count,
              (size_t)i + 1);
    }
    CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count, (size_t)3);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_shuffle)
{
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    CHECK(insert_shuffled(&flat_priority_queue, vals, size, prime), PASS);

    struct Val const *min = front(&flat_priority_queue);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &flat_priority_queue), PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_shuffle_grow)
{
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(NULL, struct Val, CCC_ORDER_LESSER,
                                             val_order, std_allocate, NULL, 0);
    CHECK(insert_shuffled(&flat_priority_queue, vals, size, prime), PASS);

    struct Val const *min = front(&flat_priority_queue);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &flat_priority_queue), PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    CHECK_END_FN((void)CCC_flat_priority_queue_clear_and_free(
                     &flat_priority_queue, NULL););
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_insert_shuffle_reserve)
{
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(NULL, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL, 0);
    CCC_Result const r = CCC_flat_priority_queue_reserve(&flat_priority_queue,
                                                         50, std_allocate);
    CHECK(r, CCC_RESULT_OK);
    CHECK(insert_shuffled(&flat_priority_queue, vals, size, prime), PASS);
    struct Val const *min = front(&flat_priority_queue);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &flat_priority_queue), PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    CHECK_END_FN(
        clear_and_free_reserve(&flat_priority_queue, NULL, std_allocate););
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_read_max_min)
{
    size_t const size = 10;
    struct Val vals[10 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)size - (int)i;
        (void)push(&flat_priority_queue, &vals[i], &(struct Val){});
        CHECK(validate(&flat_priority_queue), true);
        CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count, i + 1);
    }
    CHECK(CCC_flat_priority_queue_count(&flat_priority_queue).count,
          (size_t)10);
    struct Val const *min = front(&flat_priority_queue);
    CHECK(min->val, size - (size - 1));
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(flat_priority_queue_test_insert_one(),
                     flat_priority_queue_test_insert_three(),
                     flat_priority_queue_test_struct_getter(),
                     flat_priority_queue_test_insert_three_dups(),
                     flat_priority_queue_test_insert_shuffle(),
                     flat_priority_queue_test_insert_shuffle_grow(),
                     flat_priority_queue_test_insert_shuffle_reserve(),
                     flat_priority_queue_test_read_max_min());
}
