#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "priority_queue.h"
#include "priority_queue_utility.h"
#include "traits.h"
#include "types.h"

check_static_begin(priority_queue_test_insert_one)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    struct Val single;
    single.val = 0;
    check(push(&priority_queue, &single.elem) != NULL, true);
    check(CCC_priority_queue_is_empty(&priority_queue), false);
    check_end();
}

check_static_begin(priority_queue_test_insert_three)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    struct Val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        check(push(&priority_queue, &three_vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
        check(CCC_priority_queue_count(&priority_queue).count, (size_t)i + 1);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)3);
    check_end();
}

check_static_begin(priority_queue_test_struct_getter)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    CCC_Priority_queue priority_queue_tester_clone
        = CCC_priority_queue_initialize(struct Val, elem, CCC_ORDER_LESSER,
                                        val_order, NULL, NULL);
    struct Val vals[10];
    struct Val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(push(&priority_queue_tester_clone, &tester_clone[i].elem) != NULL,
              true);
        check(validate(&priority_queue), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct Val const *get = &tester_clone[i];
        check(get->val, vals[i].val);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)10);
    check_end();
}

check_static_begin(priority_queue_test_insert_three_dups)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    struct Val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        check(push(&priority_queue, &three_vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
        check(CCC_priority_queue_count(&priority_queue).count, (size_t)i + 1);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)3);
    check_end();
}

check_static_begin(priority_queue_test_insert_shuffle)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct Val vals[50];
    check(insert_shuffled(&priority_queue, vals, size, prime), CHECK_PASS);
    struct Val const *min = front(&priority_queue);
    check(min->val, 0);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &priority_queue), CHECK_PASS);
    check_end();
}

check_static_begin(priority_queue_test_read_max_min)
{
    CCC_Priority_queue priority_queue = CCC_priority_queue_initialize(
        struct Val, elem, CCC_ORDER_LESSER, val_order, NULL, NULL);
    struct Val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        check(push(&priority_queue, &vals[i].elem) != NULL, true);
        check(validate(&priority_queue), true);
        check(CCC_priority_queue_count(&priority_queue).count, (size_t)i + 1);
    }
    check(CCC_priority_queue_count(&priority_queue).count, (size_t)10);
    struct Val const *min = front(&priority_queue);
    check(min->val, 0);
    check_end();
}

int
main()
{
    return check_run(priority_queue_test_insert_one(),
                     priority_queue_test_insert_three(),
                     priority_queue_test_struct_getter(),
                     priority_queue_test_insert_three_dups(),
                     priority_queue_test_insert_shuffle(),
                     priority_queue_test_read_max_min());
}
