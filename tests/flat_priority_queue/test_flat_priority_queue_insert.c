#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_priority_queue.h"
#include "flat_priority_queue_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

check_static_begin(flat_priority_queue_test_insert_one)
{
    struct Val single[2] = {};
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(
            single, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
            (sizeof(single) / sizeof(single[0])));
    single[0].val = 0;
    (void)push(&flat_priority_queue, &single[0], &(struct Val){});
    check(CCC_flat_priority_queue_is_empty(&flat_priority_queue), false);
    check_end();
}

check_static_begin(flat_priority_queue_test_insert_three)
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
        check(validate(&flat_priority_queue), true);
        check(CCC_flat_priority_queue_count(&flat_priority_queue).count, i + 1);
    }
    check(CCC_flat_priority_queue_count(&flat_priority_queue).count, size);
    check_end();
}

check_static_begin(flat_priority_queue_test_struct_getter)
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
        check(res1 != NULL, true);
        struct Val const *res2 = CCC_flat_priority_queue_emplace(
            &flat_priority_queue_clone,
            (struct Val){.id = (int)i, .val = (int)i});
        check(res2 != NULL, true);
        check(validate(&flat_priority_queue), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct Val const *get = &tester_clone[i];
        check(get->val, vals[i].val);
    }
    check(CCC_flat_priority_queue_count(&flat_priority_queue).count,
          (size_t)10);
    check_end();
}

check_static_begin(flat_priority_queue_test_insert_three_dups)
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
        check(validate(&flat_priority_queue), true);
        check(CCC_flat_priority_queue_count(&flat_priority_queue).count,
              (size_t)i + 1);
    }
    check(CCC_flat_priority_queue_count(&flat_priority_queue).count, (size_t)3);
    check_end();
}

check_static_begin(flat_priority_queue_test_insert_shuffle)
{
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(vals, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL,
                                             (sizeof(vals) / sizeof(vals[0])));
    check(insert_shuffled(&flat_priority_queue, vals, size, prime), CHECK_PASS);

    struct Val const *min = front(&flat_priority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &flat_priority_queue), CHECK_PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        check(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    check_end();
}

check_static_begin(flat_priority_queue_test_insert_shuffle_grow)
{
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50 + 1];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(NULL, struct Val, CCC_ORDER_LESSER,
                                             val_order, std_allocate, NULL, 0);
    check(insert_shuffled(&flat_priority_queue, vals, size, prime), CHECK_PASS);

    struct Val const *min = front(&flat_priority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &flat_priority_queue), CHECK_PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        check(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    check_end((void)CCC_flat_priority_queue_clear_and_free(&flat_priority_queue,
                                                           NULL););
}

check_static_begin(flat_priority_queue_test_insert_shuffle_reserve)
{
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50];
    CCC_Flat_priority_queue flat_priority_queue
        = CCC_flat_priority_queue_initialize(NULL, struct Val, CCC_ORDER_LESSER,
                                             val_order, NULL, NULL, 0);
    CCC_Result const r = CCC_flat_priority_queue_reserve(&flat_priority_queue,
                                                         50, std_allocate);
    check(r, CCC_RESULT_OK);
    check(insert_shuffled(&flat_priority_queue, vals, size, prime), CHECK_PASS);
    struct Val const *min = front(&flat_priority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &flat_priority_queue), CHECK_PASS);
    int prev = sorted_check[0];
    for (size_t i = 0; i < size; ++i)
    {
        check(prev <= sorted_check[i], true);
        prev = sorted_check[i];
    }
    check_end(
        clear_and_free_reserve(&flat_priority_queue, NULL, std_allocate););
}

check_static_begin(flat_priority_queue_test_read_max_min)
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
        check(validate(&flat_priority_queue), true);
        check(CCC_flat_priority_queue_count(&flat_priority_queue).count, i + 1);
    }
    check(CCC_flat_priority_queue_count(&flat_priority_queue).count,
          (size_t)10);
    struct Val const *min = front(&flat_priority_queue);
    check(min->val, size - (size - 1));
    check_end();
}

int
main()
{
    return check_run(flat_priority_queue_test_insert_one(),
                     flat_priority_queue_test_insert_three(),
                     flat_priority_queue_test_struct_getter(),
                     flat_priority_queue_test_insert_three_dups(),
                     flat_priority_queue_test_insert_shuffle(),
                     flat_priority_queue_test_insert_shuffle_grow(),
                     flat_priority_queue_test_insert_shuffle_reserve(),
                     flat_priority_queue_test_read_max_min());
}
