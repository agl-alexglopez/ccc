#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
    ccc_flat_priority_queue pq
        = ccc_fpq_init(vals, (sizeof(vals) / sizeof(struct val)), CCC_LES, NULL,
                       val_cmp, NULL);
    CHECK(ccc_fpq_is_empty(&pq), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_macro)
{
    struct val vals[2] = {};
    ccc_flat_priority_queue pq
        = ccc_fpq_init(vals, (sizeof(vals) / sizeof(struct val)), CCC_LES, NULL,
                       val_cmp, NULL);
    struct val *res = ccc_fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_is_empty(&pq), false);
    struct val *res2 = ccc_fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res2 == NULL, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_push)
{
    struct val vals[3] = {};
    ccc_flat_priority_queue pq
        = ccc_fpq_init(vals, (sizeof(vals) / sizeof(struct val)), CCC_LES, NULL,
                       val_cmp, NULL);
    struct val *res = push(&pq, &vals[0]);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_is_empty(&pq), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_raw_type)
{
    int vals[4] = {};
    ccc_flat_priority_queue pq = ccc_fpq_init(
        vals, (sizeof(vals) / sizeof(int)), CCC_LES, NULL, int_cmp, NULL);
    int val = 1;
    int *res = push(&pq, &val);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_is_empty(&pq), false);
    res = ccc_fpq_emplace(&pq, -1);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_size(&pq), 2);
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
    ccc_flat_priority_queue pq = ccc_fpq_heapify_init(
        heap, (sizeof(heap) / sizeof(int)), size, CCC_LES, NULL, int_cmp, NULL);
    int prev = *((int *)ccc_fpq_front(&pq));
    (void)pop(&pq);
    while (!ccc_fpq_is_empty(&pq))
    {
        int cur = *((int *)ccc_fpq_front(&pq));
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
    ccc_flat_priority_queue pq = ccc_fpq_init(
        heap, (sizeof(heap) / sizeof(int)), CCC_LES, NULL, int_cmp, NULL);
    int input[99] = {};
    for (size_t i = 0; i < sizeof(input) / sizeof(int); ++i)
    {
        input[i] = rand_range(-99, 99); /* NOLINT */
    }
    CHECK(ccc_fpq_heapify(&pq, input, sizeof(input) / sizeof(int), sizeof(int)),
          CCC_OK);
    CHECK(ccc_fpq_size(&pq), sizeof(input) / sizeof(int));
    int prev = *((int *)ccc_fpq_front(&pq));
    (void)pop(&pq);
    while (!ccc_fpq_is_empty(&pq))
    {
        int cur = *((int *)ccc_fpq_front(&pq));
        (void)pop(&pq);
        CHECK(cur >= prev, true);
        prev = cur;
    }
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(pq_test_empty(), pq_test_macro(), pq_test_push(),
                     pq_test_raw_type(), pq_test_heapify_init(),
                     pq_test_heapify_copy());
}
