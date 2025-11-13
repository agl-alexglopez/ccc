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

static CCC_Order
int_order(CCC_Type_comparator_context const order)
{
    int a = *((int const *const)order.type_lhs);
    int b = *((int const *const)order.type_rhs);
    return (a > b) - (a < b);
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_empty)
{
    struct Val vals[2] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
        (sizeof(vals) / sizeof(struct Val)));
    CHECK(flat_priority_queue_is_empty(&priority_queue), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_macro)
{
    struct Val vals[2] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
        (sizeof(vals) / sizeof(struct Val)));
    struct Val *res = flat_priority_queue_emplace(
        &priority_queue, (struct Val){.val = 0, .id = 0});
    CHECK(res != NULL, true);
    CHECK(flat_priority_queue_is_empty(&priority_queue), false);
    struct Val *res2 = flat_priority_queue_emplace(
        &priority_queue, (struct Val){.val = 0, .id = 0});
    CHECK(res2 != NULL, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_macro_grow)
{
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        NULL, struct Val, CCC_ORDER_LESSER, val_order, std_allocate, NULL, 0);
    struct Val *res = flat_priority_queue_emplace(
        &priority_queue, (struct Val){.val = 0, .id = 0});
    CHECK(res != NULL, true);
    CHECK(flat_priority_queue_is_empty(&priority_queue), false);
    struct Val *res2 = flat_priority_queue_emplace(
        &priority_queue, (struct Val){.val = 0, .id = 0});
    CHECK(res2 != NULL, true);
    CHECK_END_FN(
        (void)CCC_flat_priority_queue_clear_and_free(&priority_queue, NULL););
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_push)
{
    struct Val vals[3] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        vals, struct Val, CCC_ORDER_LESSER, val_order, NULL, NULL,
        (sizeof(vals) / sizeof(struct Val)));
    struct Val *res = push(&priority_queue, &vals[0], &(struct Val){});
    CHECK(res != NULL, true);
    CHECK(flat_priority_queue_is_empty(&priority_queue), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_raw_type)
{
    int vals[4] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        vals, int, CCC_ORDER_LESSER, int_order, NULL, NULL,
        (sizeof(vals) / sizeof(int)));
    int val = 1;
    int *res = push(&priority_queue, &val, &(int){0});
    CHECK(res != NULL, true);
    CHECK(flat_priority_queue_is_empty(&priority_queue), false);
    res = flat_priority_queue_emplace(&priority_queue, -1);
    CHECK(res != NULL, true);
    CHECK(flat_priority_queue_count(&priority_queue).count, 2);
    int *popped = front(&priority_queue);
    CHECK(*popped, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_heapify_initialize)
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
        CHECK(cur >= prev, true);
        prev = cur;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_heapify_copy)
{
    srand(time(NULL)); /* NOLINT */
    enum : size_t
    {
        HEAPIFY_COPY_CAP = 100,
    };
    int heap[HEAPIFY_COPY_CAP] = {};
    Flat_priority_queue priority_queue = flat_priority_queue_initialize(
        heap, int, CCC_ORDER_LESSER, int_order, NULL, NULL, HEAPIFY_COPY_CAP);
    int input[HEAPIFY_COPY_CAP] = {};
    for (size_t i = 0; i < HEAPIFY_COPY_CAP; ++i)
    {
        input[i] = rand_range(-99, 99); /* NOLINT */
    }
    CHECK(flat_priority_queue_heapify(&priority_queue, &(int){0}, input,
                                      HEAPIFY_COPY_CAP, sizeof(int)),
          CCC_RESULT_OK);
    CHECK(flat_priority_queue_count(&priority_queue).count, HEAPIFY_COPY_CAP);
    int prev = *((int *)flat_priority_queue_front(&priority_queue));
    (void)pop(&priority_queue, &(int){0});
    while (!flat_priority_queue_is_empty(&priority_queue))
    {
        int cur = *((int *)flat_priority_queue_front(&priority_queue));
        (void)pop(&priority_queue, &(int){0});
        CHECK(cur >= prev, true);
        prev = cur;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_heapsort)
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
    CHECK(prev != NULL, true);
    CHECK(CCC_buffer_count(&b).count, HPSORTCAP);
    size_t count = 1;
    for (int const *cur = next(&b, prev); cur != end(&b); cur = next(&b, cur))
    {
        CHECK(*prev >= *cur, true);
        prev = cur;
        ++count;
    }
    CHECK(count, HPSORTCAP);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_copy_no_allocate)
{
    Flat_priority_queue src = flat_priority_queue_initialize(
        (int[4]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL, 4);
    Flat_priority_queue dst = flat_priority_queue_initialize(
        (int[5]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL, 5);
    (void)push(&src, &(int){0}, &(int){0});
    (void)push(&src, &(int){1}, &(int){0});
    (void)push(&src, &(int){2}, &(int){0});
    CHECK(count(&src).count, 3);
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    CCC_Result res = flat_priority_queue_copy(&dst, &src, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, 3);
    while (!is_empty(&src) && !is_empty(&dst))
    {
        int f1 = *(int *)front(&src);
        int f2 = *(int *)front(&dst);
        (void)pop(&src, &(int){0});
        (void)pop(&dst, &(int){0});
        CHECK(f1, f2);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_copy_no_allocate_fail)
{
    Flat_priority_queue src = flat_priority_queue_initialize(
        (int[4]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL, 4);
    Flat_priority_queue dst = flat_priority_queue_initialize(
        (int[2]){}, int, CCC_ORDER_LESSER, int_order, NULL, NULL, 2);
    (void)push(&src, &(int){0}, &(int){0});
    (void)push(&src, &(int){1}, &(int){0});
    (void)push(&src, &(int){2}, &(int){0});
    CHECK(count(&src).count, 3);
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    CCC_Result res = flat_priority_queue_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_copy_allocate)
{
    Flat_priority_queue src = flat_priority_queue_initialize(
        NULL, int, CCC_ORDER_LESSER, int_order, std_allocate, NULL, 0);
    Flat_priority_queue dst = flat_priority_queue_initialize(
        NULL, int, CCC_ORDER_LESSER, int_order, std_allocate, NULL, 0);
    (void)push(&src, &(int){0}, &(int){0});
    (void)push(&src, &(int){1}, &(int){0});
    (void)push(&src, &(int){2}, &(int){0});
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    CCC_Result res = flat_priority_queue_copy(&dst, &src, std_allocate);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, 3);
    while (!is_empty(&src) && !is_empty(&dst))
    {
        int f1 = *(int *)front(&src);
        int f2 = *(int *)front(&dst);
        (void)pop(&src, &(int){0});
        (void)pop(&dst, &(int){0});
        CHECK(f1, f2);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK_END_FN({
        (void)flat_priority_queue_clear_and_free(&src, NULL);
        (void)flat_priority_queue_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(flat_priority_queue_test_copy_allocate_fail)
{
    Flat_priority_queue src = flat_priority_queue_initialize(
        NULL, int, CCC_ORDER_LESSER, int_order, std_allocate, NULL, 0);
    Flat_priority_queue dst = flat_priority_queue_initialize(
        NULL, int, CCC_ORDER_LESSER, int_order, std_allocate, NULL, 0);
    (void)push(&src, &(int){0}, &(int){0});
    (void)push(&src, &(int){1}, &(int){0});
    (void)push(&src, &(int){2}, &(int){0});
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    CCC_Result res = flat_priority_queue_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)flat_priority_queue_clear_and_free(&src, NULL); });
}

int
main()
{
    return CHECK_RUN(
        flat_priority_queue_test_empty(), flat_priority_queue_test_macro(),
        flat_priority_queue_test_macro_grow(), flat_priority_queue_test_push(),
        flat_priority_queue_test_raw_type(),
        flat_priority_queue_test_heapify_initialize(),
        flat_priority_queue_test_heapify_copy(),
        flat_priority_queue_test_copy_no_allocate(),
        flat_priority_queue_test_copy_no_allocate_fail(),
        flat_priority_queue_test_copy_allocate(),
        flat_priority_queue_test_copy_allocate_fail(),
        flat_priority_queue_test_heapsort());
}
