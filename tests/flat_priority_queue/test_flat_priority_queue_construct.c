#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC

#include "buffer.h"
#include "checkers.h"
#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

static CCC_Order
int_cmp(CCC_Type_comparator_context const cmp)
{
    int a = *((int const *const)cmp.any_type_lhs);
    int b = *((int const *const)cmp.any_type_rhs);
    return (a > b) - (a < b);
}

CHECK_BEGIN_STATIC_FN(fpq_test_empty)
{
    struct val vals[2] = {};
    flat_priority_queue pq
        = fpq_initialize(vals, struct val, CCC_ORDER_LESS, val_cmp, NULL, NULL,
                         (sizeof(vals) / sizeof(struct val)));
    CHECK(fpq_is_empty(&pq), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_macro)
{
    struct val vals[2] = {};
    flat_priority_queue pq
        = fpq_initialize(vals, struct val, CCC_ORDER_LESS, val_cmp, NULL, NULL,
                         (sizeof(vals) / sizeof(struct val)));
    struct val *res = fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res != NULL, true);
    CHECK(fpq_is_empty(&pq), false);
    struct val *res2 = fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res2 != NULL, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_macro_grow)
{
    flat_priority_queue pq = fpq_initialize(NULL, struct val, CCC_ORDER_LESS,
                                            val_cmp, std_alloc, NULL, 0);
    struct val *res = fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res != NULL, true);
    CHECK(fpq_is_empty(&pq), false);
    struct val *res2 = fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res2 != NULL, true);
    CHECK_END_FN((void)CCC_fpq_clear_and_free(&pq, NULL););
}

CHECK_BEGIN_STATIC_FN(fpq_test_push)
{
    struct val vals[3] = {};
    flat_priority_queue pq
        = fpq_initialize(vals, struct val, CCC_ORDER_LESS, val_cmp, NULL, NULL,
                         (sizeof(vals) / sizeof(struct val)));
    struct val *res = push(&pq, &vals[0], &(struct val){});
    CHECK(res != NULL, true);
    CHECK(fpq_is_empty(&pq), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_raw_type)
{
    int vals[4] = {};
    flat_priority_queue pq
        = fpq_initialize(vals, int, CCC_ORDER_LESS, int_cmp, NULL, NULL,
                         (sizeof(vals) / sizeof(int)));
    int val = 1;
    int *res = push(&pq, &val, &(int){0});
    CHECK(res != NULL, true);
    CHECK(fpq_is_empty(&pq), false);
    res = fpq_emplace(&pq, -1);
    CHECK(res != NULL, true);
    CHECK(fpq_count(&pq).count, 2);
    int *popped = front(&pq);
    CHECK(*popped, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_heapify_init)
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
    flat_priority_queue pq
        = fpq_heapify_initialize(heap, int, CCC_ORDER_LESS, int_cmp, NULL, NULL,
                                 HEAPIFY_CAP, HEAPIFY_CAP);
    int prev = *((int *)fpq_front(&pq));
    (void)pop(&pq, &(int){0});
    while (!fpq_is_empty(&pq))
    {
        int cur = *((int *)fpq_front(&pq));
        (void)pop(&pq, &(int){0});
        CHECK(cur >= prev, true);
        prev = cur;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_heapify_copy)
{
    srand(time(NULL)); /* NOLINT */
    enum : size_t
    {
        HEAPIFY_COPY_CAP = 100,
    };
    int heap[HEAPIFY_COPY_CAP] = {};
    flat_priority_queue pq = fpq_initialize(heap, int, CCC_ORDER_LESS, int_cmp,
                                            NULL, NULL, HEAPIFY_COPY_CAP);
    int input[HEAPIFY_COPY_CAP] = {};
    for (size_t i = 0; i < HEAPIFY_COPY_CAP; ++i)
    {
        input[i] = rand_range(-99, 99); /* NOLINT */
    }
    CHECK(fpq_heapify(&pq, &(int){0}, input, HEAPIFY_COPY_CAP, sizeof(int)),
          CCC_RESULT_OK);
    CHECK(fpq_count(&pq).count, HEAPIFY_COPY_CAP);
    int prev = *((int *)fpq_front(&pq));
    (void)pop(&pq, &(int){0});
    while (!fpq_is_empty(&pq))
    {
        int cur = *((int *)fpq_front(&pq));
        (void)pop(&pq, &(int){0});
        CHECK(cur >= prev, true);
        prev = cur;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_heapsort)
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
    flat_priority_queue pq = fpq_heapify_initialize(
        heap, int, CCC_ORDER_LESS, int_cmp, NULL, NULL, HPSORTCAP, HPSORTCAP);
    CCC_Buffer const b = CCC_fpq_heapsort(&pq, &(int){0});
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

CHECK_BEGIN_STATIC_FN(fpq_test_copy_no_alloc)
{
    flat_priority_queue src = fpq_initialize((int[4]){}, int, CCC_ORDER_LESS,
                                             int_cmp, NULL, NULL, 4);
    flat_priority_queue dst = fpq_initialize((int[5]){}, int, CCC_ORDER_LESS,
                                             int_cmp, NULL, NULL, 5);
    (void)push(&src, &(int){0}, &(int){0});
    (void)push(&src, &(int){1}, &(int){0});
    (void)push(&src, &(int){2}, &(int){0});
    CHECK(count(&src).count, 3);
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    CCC_Result res = fpq_copy(&dst, &src, NULL);
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

CHECK_BEGIN_STATIC_FN(fpq_test_copy_no_alloc_fail)
{
    flat_priority_queue src = fpq_initialize((int[4]){}, int, CCC_ORDER_LESS,
                                             int_cmp, NULL, NULL, 4);
    flat_priority_queue dst = fpq_initialize((int[2]){}, int, CCC_ORDER_LESS,
                                             int_cmp, NULL, NULL, 2);
    (void)push(&src, &(int){0}, &(int){0});
    (void)push(&src, &(int){1}, &(int){0});
    (void)push(&src, &(int){2}, &(int){0});
    CHECK(count(&src).count, 3);
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    CCC_Result res = fpq_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fpq_test_copy_alloc)
{
    flat_priority_queue src = fpq_initialize(NULL, int, CCC_ORDER_LESS, int_cmp,
                                             std_alloc, NULL, 0);
    flat_priority_queue dst = fpq_initialize(NULL, int, CCC_ORDER_LESS, int_cmp,
                                             std_alloc, NULL, 0);
    (void)push(&src, &(int){0}, &(int){0});
    (void)push(&src, &(int){1}, &(int){0});
    (void)push(&src, &(int){2}, &(int){0});
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    CCC_Result res = fpq_copy(&dst, &src, std_alloc);
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
        (void)fpq_clear_and_free(&src, NULL);
        (void)fpq_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(fpq_test_copy_alloc_fail)
{
    flat_priority_queue src = fpq_initialize(NULL, int, CCC_ORDER_LESS, int_cmp,
                                             std_alloc, NULL, 0);
    flat_priority_queue dst = fpq_initialize(NULL, int, CCC_ORDER_LESS, int_cmp,
                                             std_alloc, NULL, 0);
    (void)push(&src, &(int){0}, &(int){0});
    (void)push(&src, &(int){1}, &(int){0});
    (void)push(&src, &(int){2}, &(int){0});
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    CCC_Result res = fpq_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)fpq_clear_and_free(&src, NULL); });
}

int
main()
{
    return CHECK_RUN(fpq_test_empty(), fpq_test_macro(), fpq_test_macro_grow(),
                     fpq_test_push(), fpq_test_raw_type(),
                     fpq_test_heapify_initialize(), fpq_test_heapify_copy(),
                     fpq_test_copy_no_alloc(), fpq_test_copy_no_alloc_fail(),
                     fpq_test_copy_alloc(), fpq_test_copy_alloc_fail(),
                     fpq_test_heapsort());
}
