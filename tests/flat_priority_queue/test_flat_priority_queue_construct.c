#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC

#include "buffer.h"
#include "checkers.h"
#include "flat_priority_queue.h"
#include "flat_priority_queue_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"
#include "utility/stack_allocator.h"

static CCC_Order
int_order(CCC_Type_comparator_context const order)
{
    int a = *((int const *const)order.type_left);
    int b = *((int const *const)order.type_right);
    return (a > b) - (a < b);
}

check_static_begin(flat_priority_queue_test_empty)
{
    struct Val vals[2] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
        (sizeof(vals) / sizeof(struct Val)));
    check(flat_priority_queue_is_empty(&priority_queue), true);
    check_end();
}

check_static_begin(flat_priority_queue_test_macro)
{
    struct Val vals[2] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
        (sizeof(vals) / sizeof(struct Val)));
    struct Val *res = flat_priority_queue_emplace(
        &priority_queue, (struct Val){.val = 0, .id = 0});
    check(res != NULL, true);
    check(flat_priority_queue_is_empty(&priority_queue), false);
    struct Val *res2 = flat_priority_queue_emplace(
        &priority_queue, (struct Val){.val = 0, .id = 0});
    check(res2 != NULL, true);
    check_end();
}

check_static_begin(flat_priority_queue_test_macro_grow)
{
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        NULL, struct Val, CCC_ORDER_LESSER, val_order, std_allocate, NULL, 0);
    struct Val *res = flat_priority_queue_emplace(
        &priority_queue, (struct Val){.val = 0, .id = 0});
    check(res != NULL, true);
    check(flat_priority_queue_is_empty(&priority_queue), false);
    struct Val *res2 = flat_priority_queue_emplace(
        &priority_queue, (struct Val){.val = 0, .id = 0});
    check(res2 != NULL, true);
    check_end(
        (void)CCC_flat_priority_queue_clear_and_free(&priority_queue, NULL););
}

check_static_begin(flat_priority_queue_test_push)
{
    struct Val vals[3] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
        (sizeof(vals) / sizeof(struct Val)));
    struct Val *res = push(&priority_queue, &vals[0], &(struct Val){});
    check(res != NULL, true);
    check(flat_priority_queue_is_empty(&priority_queue), false);
    check_end();
}

check_static_begin(flat_priority_queue_test_raw_type)
{
    int vals[4] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        vals, int, CCC_ORDER_LESSER, int_order, NULL, NULL,
        (sizeof(vals) / sizeof(int)));
    int val = 1;
    int *res = push(&priority_queue, &val, &(int){0});
    check(res != NULL, true);
    check(flat_priority_queue_is_empty(&priority_queue), false);
    res = flat_priority_queue_emplace(&priority_queue, -1);
    check(res != NULL, true);
    check(flat_priority_queue_count(&priority_queue).count, 2);
    int *popped = front(&priority_queue);
    check(*popped, -1);
    check_end();
}

check_static_begin(flat_priority_queue_test_heapify_initialize)
{
    srand(time(NULL)); /* NOLINT */
    enum : size_t
    {
        HEAPIFY_CAP = 100,
    };
    int heap[HEAPIFY_CAP] = {};
    for (size_t i = 0; i < HEAPIFY_CAP; ++i)
    {
        heap[i] = rand_range(-99, (int)HEAPIFY_CAP); /* NOLINT */
    }
    Flat_priority_queue priority_queue = flat_priority_queue_heapify_initialize(
        heap, int, CCC_ORDER_LESSER, int_order, NULL, NULL, HEAPIFY_CAP,
        HEAPIFY_CAP);
    int prev = *((int *)flat_priority_queue_front(&priority_queue));
    (void)pop(&priority_queue, &(int){0});
    while (!flat_priority_queue_is_empty(&priority_queue))
    {
        int cur = *((int *)flat_priority_queue_front(&priority_queue));
        (void)pop(&priority_queue, &(int){0});
        check(cur >= prev, true);
        prev = cur;
    }
    check_end();
}

check_static_begin(flat_priority_queue_test_heapify_copy)
{
    srand(time(NULL)); /* NOLINT */
    enum : size_t
    {
        HEAPIFY_COPY_CAP = 100,
    };
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        (int[HEAPIFY_COPY_CAP]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL,
        HEAPIFY_COPY_CAP);
    int input[HEAPIFY_COPY_CAP] = {};
    for (size_t i = 0; i < HEAPIFY_COPY_CAP; ++i)
    {
        input[i] = rand_range(-99, 99); /* NOLINT */
    }
    check(flat_priority_queue_heapify(&priority_queue, &(int){0}, input,
                                      HEAPIFY_COPY_CAP, sizeof(int)),
          CCC_RESULT_OK);
    check(flat_priority_queue_count(&priority_queue).count, HEAPIFY_COPY_CAP);
    int prev = *((int *)flat_priority_queue_front(&priority_queue));
    (void)pop(&priority_queue, &(int){0});
    while (!flat_priority_queue_is_empty(&priority_queue))
    {
        int cur = *((int *)flat_priority_queue_front(&priority_queue));
        (void)pop(&priority_queue, &(int){0});
        check(cur >= prev, true);
        prev = cur;
    }
    check_end();
}

check_static_begin(flat_priority_queue_test_heapsort)
{
    enum : size_t
    {
        HPSORTCAP = 100,
    };
    srand(time(NULL)); /* NOLINT */
    int heap[HPSORTCAP] = {};
    for (size_t i = 0; i < HPSORTCAP; ++i)
    {
        heap[i] = rand_range(-99, (int)(HPSORTCAP)); /* NOLINT */
    }
    Flat_priority_queue priority_queue = flat_priority_queue_heapify_initialize(
        heap, int, CCC_ORDER_LESSER, int_order, NULL, NULL, HPSORTCAP,
        HPSORTCAP);
    CCC_Buffer const b
        = CCC_flat_priority_queue_heapsort(&priority_queue, &(int){0});
    int const *prev = begin(&b);
    check(prev != NULL, true);
    check(CCC_buffer_count(&b).count, HPSORTCAP);
    size_t count = 1;
    for (int const *cur = next(&b, prev); cur != end(&b); cur = next(&b, cur))
    {
        check(*prev >= *cur, true);
        prev = cur;
        ++count;
    }
    check(count, HPSORTCAP);
    check_end();
}

check_static_begin(flat_priority_queue_test_copy_no_allocate)
{
    Flat_priority_queue source = flat_priority_queue_initialize(
        (int[4]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL, 4);
    Flat_priority_queue destination = flat_priority_queue_initialize(
        (int[5]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL, 5);
    (void)push(&source, &(int){0}, &(int){0});
    (void)push(&source, &(int){1}, &(int){0});
    (void)push(&source, &(int){2}, &(int){0});
    check(count(&source).count, 3);
    check(*(int *)front(&source), 0);
    check(is_empty(&destination), true);
    CCC_Result res = flat_priority_queue_copy(&destination, &source, NULL);
    check(res, CCC_RESULT_OK);
    check(count(&destination).count, 3);
    while (!is_empty(&source) && !is_empty(&destination))
    {
        int f1 = *(int *)front(&source);
        int f2 = *(int *)front(&destination);
        (void)pop(&source, &(int){0});
        (void)pop(&destination, &(int){0});
        check(f1, f2);
    }
    check(is_empty(&source), is_empty(&destination));
    check_end();
}

check_static_begin(flat_priority_queue_test_copy_no_allocate_fail)
{
    Flat_priority_queue source = flat_priority_queue_initialize(
        (int[4]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL, 4);
    Flat_priority_queue destination = flat_priority_queue_initialize(
        (int[2]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL, 2);
    (void)push(&source, &(int){0}, &(int){0});
    (void)push(&source, &(int){1}, &(int){0});
    (void)push(&source, &(int){2}, &(int){0});
    check(count(&source).count, 3);
    check(*(int *)front(&source), 0);
    check(is_empty(&destination), true);
    CCC_Result res = flat_priority_queue_copy(&destination, &source, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end();
}

check_static_begin(flat_priority_queue_test_copy_allocate)
{
    struct Stack_allocator allocator = stack_allocator_initialize(int, 16);
    Flat_priority_queue source = flat_priority_queue_with_capacity(
        int, CCC_ORDER_LESSER, int_order, stack_allocator_allocate, &allocator,
        8);
    Flat_priority_queue destination = flat_priority_queue_initialize(
        NULL, int, CCC_ORDER_LESSER, int_order, stack_allocator_allocate,
        &allocator, 0);
    (void)push(&source, &(int){0}, &(int){0});
    (void)push(&source, &(int){1}, &(int){0});
    (void)push(&source, &(int){2}, &(int){0});
    check(*(int *)front(&source), 0);
    check(is_empty(&destination), true);
    CCC_Result res = flat_priority_queue_copy(&destination, &source,
                                              stack_allocator_allocate);
    check(res, CCC_RESULT_OK);
    check(count(&destination).count, 3);
    while (!is_empty(&source) && !is_empty(&destination))
    {
        int f1 = *(int *)front(&source);
        int f2 = *(int *)front(&destination);
        (void)pop(&source, &(int){0});
        (void)pop(&destination, &(int){0});
        check(f1, f2);
    }
    check(is_empty(&source), is_empty(&destination));
    check_end({
        (void)flat_priority_queue_clear_and_free(&source, NULL);
        (void)flat_priority_queue_clear_and_free(&destination, NULL);
    });
}

check_static_begin(flat_priority_queue_test_copy_allocate_fail)
{
    struct Stack_allocator allocator = stack_allocator_initialize(int, 16);
    Flat_priority_queue source = flat_priority_queue_with_capacity(
        int, CCC_ORDER_LESSER, int_order, stack_allocator_allocate, &allocator,
        8);
    Flat_priority_queue destination = flat_priority_queue_initialize(
        NULL, int, CCC_ORDER_LESSER, int_order, stack_allocator_allocate,
        &allocator, 0);
    (void)push(&source, &(int){0}, &(int){0});
    (void)push(&source, &(int){1}, &(int){0});
    (void)push(&source, &(int){2}, &(int){0});
    check(*(int *)front(&source), 0);
    check(is_empty(&destination), true);
    CCC_Result res = flat_priority_queue_copy(&destination, &source, NULL);
    check(res != CCC_RESULT_OK, true);
    check_end({ (void)flat_priority_queue_clear_and_free(&source, NULL); });
}

check_static_begin(flat_priority_queue_test_init_from)
{
    struct Stack_allocator allocator = stack_allocator_initialize(int, 8);
    CCC_Flat_priority_queue queue = CCC_flat_priority_queue_from(
        CCC_ORDER_LESSER, int_order, stack_allocator_allocate, &allocator, 8,
        (int[7]){8, 6, 7, 5, 3, 0, 9});
    int count = 0;
    int prev = INT_MIN;
    check(CCC_flat_priority_queue_count(&queue).count, 7);
    while (!CCC_flat_priority_queue_is_empty(&queue))
    {
        int const front = *(int *)CCC_flat_priority_queue_front(&queue);
        check(front > prev, true);
        CCC_Result const pop = CCC_flat_priority_queue_pop(&queue, &(int){0});
        check(pop, CCC_RESULT_OK);
        ++count;
    }
    check(count, 7);
    check(CCC_flat_priority_queue_capacity(&queue).count >= 7, true);
    check_end((void)CCC_flat_priority_queue_clear_and_free(&queue, NULL););
}

check_static_begin(flat_priority_queue_test_init_from_fail)
{
    /* Whoops forgot allocation function. */
    CCC_Flat_priority_queue queue
        = CCC_flat_priority_queue_from(CCC_ORDER_LESSER, int_order, NULL, NULL,
                                       0, (int[]){8, 6, 7, 5, 3, 0, 9});
    int count = 0;
    int prev = INT_MIN;
    check(CCC_flat_priority_queue_count(&queue).count, 0);
    while (!CCC_flat_priority_queue_is_empty(&queue))
    {
        int const front = *(int *)CCC_flat_priority_queue_front(&queue);
        check(front > prev, true);
        ++count;
        CCC_Result const pop = CCC_flat_priority_queue_pop(&queue, &(int){0});
        check(pop, CCC_RESULT_OK);
    }
    check(count, 0);
    check(CCC_flat_priority_queue_capacity(&queue).count, 0);
    check(CCC_flat_priority_queue_push(&queue, &(int){12}, &(int){0}), NULL);
    check_end((void)CCC_flat_priority_queue_clear_and_free(&queue, NULL););
}

check_static_begin(flat_priority_queue_test_init_with_capacity)
{
    struct Stack_allocator allocator = stack_allocator_initialize(int, 8);
    CCC_Flat_priority_queue queue = CCC_flat_priority_queue_with_capacity(
        int, CCC_ORDER_LESSER, int_order, stack_allocator_allocate, &allocator,
        8);
    check(CCC_flat_priority_queue_capacity(&queue).count, 8);
    check(CCC_flat_priority_queue_push(&queue, &(int){9}, &(int){0}) != NULL,
          CCC_TRUE);
    check_end(CCC_flat_priority_queue_clear_and_free(&queue, NULL););
}

check_static_begin(flat_priority_queue_test_init_with_capacity_fail)
{
    /* Forgot allocation function. */
    CCC_Flat_priority_queue queue = CCC_flat_priority_queue_with_capacity(
        int, CCC_ORDER_LESSER, int_order, NULL, NULL, 8);
    check(CCC_flat_priority_queue_capacity(&queue).count, 0);
    check(CCC_flat_priority_queue_push(&queue, &(int){9}, &(int){0}), NULL);
    check_end(CCC_flat_priority_queue_clear_and_free(&queue, NULL););
}

int
main()
{
    return check_run(
        flat_priority_queue_test_empty(), flat_priority_queue_test_macro(),
        flat_priority_queue_test_macro_grow(), flat_priority_queue_test_push(),
        flat_priority_queue_test_raw_type(),
        flat_priority_queue_test_heapify_initialize(),
        flat_priority_queue_test_heapify_copy(),
        flat_priority_queue_test_copy_no_allocate(),
        flat_priority_queue_test_copy_no_allocate_fail(),
        flat_priority_queue_test_copy_allocate(),
        flat_priority_queue_test_copy_allocate_fail(),
        flat_priority_queue_test_heapsort(),
        flat_priority_queue_test_init_from(),
        flat_priority_queue_test_init_from_fail(),
        flat_priority_queue_test_init_with_capacity(),
        flat_priority_queue_test_init_with_capacity_fail());
}
