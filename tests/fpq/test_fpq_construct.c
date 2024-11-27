#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#include "alloc.h"
#include "checkers.h"
#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "traits.h"
#include "types.h"

static ccc_threeway_cmp
int_cmp(ccc_cmp const cmp)
{
    int a = *((int const *const)cmp.user_type_lhs);
    int b = *((int const *const)cmp.user_type_rhs);
    return (a > b) - (a < b);
}

CHECK_BEGIN_STATIC_FN(pq_test_empty)
{
    struct val vals[2] = {};
    flat_priority_queue pq = fpq_init(vals, (sizeof(vals) / sizeof(struct val)),
                                      CCC_LES, NULL, val_cmp, NULL);
    CHECK(fpq_is_empty(&pq), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_macro)
{
    struct val vals[2] = {};
    flat_priority_queue pq = fpq_init(vals, (sizeof(vals) / sizeof(struct val)),
                                      CCC_LES, NULL, val_cmp, NULL);
    struct val *res = fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res != NULL, true);
    CHECK(fpq_is_empty(&pq), false);
    struct val *res2 = fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res2 == NULL, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_push)
{
    struct val vals[3] = {};
    flat_priority_queue pq = fpq_init(vals, (sizeof(vals) / sizeof(struct val)),
                                      CCC_LES, NULL, val_cmp, NULL);
    struct val *res = push(&pq, &vals[0]);
    CHECK(res != NULL, true);
    CHECK(fpq_is_empty(&pq), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_raw_type)
{
    int vals[4] = {};
    flat_priority_queue pq = fpq_init(vals, (sizeof(vals) / sizeof(int)),
                                      CCC_LES, NULL, int_cmp, NULL);
    int val = 1;
    int *res = push(&pq, &val);
    CHECK(res != NULL, true);
    CHECK(fpq_is_empty(&pq), false);
    res = fpq_emplace(&pq, -1);
    CHECK(res != NULL, true);
    CHECK(fpq_size(&pq), 2);
    int *popped = front(&pq);
    CHECK(*popped, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_heapify_init)
{
    srand(time(NULL)); /* NOLINT */
    int heap[100] = {};
    size_t const size = 99;
    for (size_t i = 0; i < size; ++i)
    {
        heap[i] = rand_range(-99, size); /* NOLINT */
    }
    flat_priority_queue pq = fpq_heapify_init(
        heap, (sizeof(heap) / sizeof(int)), size, CCC_LES, NULL, int_cmp, NULL);
    int prev = *((int *)fpq_front(&pq));
    (void)pop(&pq);
    while (!fpq_is_empty(&pq))
    {
        int cur = *((int *)fpq_front(&pq));
        (void)pop(&pq);
        CHECK(cur >= prev, true);
        prev = cur;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_heapify_copy)
{
    srand(time(NULL)); /* NOLINT */
    int heap[100] = {};
    flat_priority_queue pq = fpq_init(heap, (sizeof(heap) / sizeof(int)),
                                      CCC_LES, NULL, int_cmp, NULL);
    int input[99] = {};
    for (size_t i = 0; i < sizeof(input) / sizeof(int); ++i)
    {
        input[i] = rand_range(-99, 99); /* NOLINT */
    }
    CHECK(fpq_heapify(&pq, input, sizeof(input) / sizeof(int), sizeof(int)),
          CCC_OK);
    CHECK(fpq_size(&pq), sizeof(input) / sizeof(int));
    int prev = *((int *)fpq_front(&pq));
    (void)pop(&pq);
    while (!fpq_is_empty(&pq))
    {
        int cur = *((int *)fpq_front(&pq));
        (void)pop(&pq);
        CHECK(cur >= prev, true);
        prev = cur;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_copy_no_alloc)
{
    flat_priority_queue src
        = fpq_init((int[4]){}, 4, CCC_LES, NULL, int_cmp, NULL);
    flat_priority_queue dst
        = fpq_init((int[5]){}, 5, CCC_LES, NULL, int_cmp, NULL);
    (void)push(&src, &(int){0});
    (void)push(&src, &(int){1});
    (void)push(&src, &(int){2});
    CHECK(size(&src), 3);
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    ccc_result res = fpq_copy(&dst, &src, NULL);
    CHECK(res, CCC_OK);
    CHECK(size(&dst), 3);
    while (!is_empty(&src) && !is_empty(&dst))
    {
        int f1 = *(int *)front(&src);
        int f2 = *(int *)front(&dst);
        (void)pop(&src);
        (void)pop(&dst);
        CHECK(f1, f2);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_copy_no_alloc_fail)
{
    flat_priority_queue src
        = fpq_init((int[4]){}, 4, CCC_LES, NULL, int_cmp, NULL);
    flat_priority_queue dst
        = fpq_init((int[2]){}, 2, CCC_LES, NULL, int_cmp, NULL);
    (void)push(&src, &(int){0});
    (void)push(&src, &(int){1});
    (void)push(&src, &(int){2});
    CHECK(size(&src), 3);
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    ccc_result res = fpq_copy(&dst, &src, NULL);
    CHECK(res != CCC_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_copy_alloc)
{
    flat_priority_queue src
        = fpq_init((int *)NULL, 0, CCC_LES, std_alloc, int_cmp, NULL);
    flat_priority_queue dst
        = fpq_init((int *)NULL, 0, CCC_LES, std_alloc, int_cmp, NULL);
    (void)push(&src, &(int){0});
    (void)push(&src, &(int){1});
    (void)push(&src, &(int){2});
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    ccc_result res = fpq_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_OK);
    CHECK(size(&dst), 3);
    while (!is_empty(&src) && !is_empty(&dst))
    {
        int f1 = *(int *)front(&src);
        int f2 = *(int *)front(&dst);
        (void)pop(&src);
        (void)pop(&dst);
        CHECK(f1, f2);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK_END_FN({
        (void)fpq_clear_and_free(&src, NULL);
        (void)fpq_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(pq_test_copy_alloc_fail)
{
    flat_priority_queue src
        = fpq_init((int *)NULL, 0, CCC_LES, std_alloc, int_cmp, NULL);
    flat_priority_queue dst
        = fpq_init((int *)NULL, 0, CCC_LES, std_alloc, int_cmp, NULL);
    (void)push(&src, &(int){0});
    (void)push(&src, &(int){1});
    (void)push(&src, &(int){2});
    CHECK(*(int *)front(&src), 0);
    CHECK(is_empty(&dst), true);
    ccc_result res = fpq_copy(&dst, &src, NULL);
    CHECK(res != CCC_OK, true);
    CHECK_END_FN({ (void)fpq_clear_and_free(&src, NULL); });
}

int
main()
{
    return CHECK_RUN(pq_test_empty(), pq_test_macro(), pq_test_push(),
                     pq_test_raw_type(), pq_test_heapify_init(),
                     pq_test_heapify_copy(), pq_test_copy_no_alloc(),
                     pq_test_copy_no_alloc_fail(), pq_test_copy_alloc(),
                     pq_test_copy_alloc_fail());
}
