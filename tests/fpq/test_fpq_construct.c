#define TRAITS_USING_NAMESPACE_CCC

#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

static ccc_threeway_cmp
int_cmp(ccc_cmp const *const cmp)
{
    int a = *((int const *const)cmp->container_a);
    int b = *((int const *const)cmp->container_b);
    return (a > b) - (a < b);
}

BEGIN_STATIC_TEST(pq_test_empty)
{
    struct val vals[2] = {};
    ccc_flat_priority_queue pq
        = ccc_fpq_init(vals, (sizeof(vals) / sizeof(struct val)), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    CHECK(ccc_fpq_empty(&pq), true);
    END_TEST();
}

BEGIN_STATIC_TEST(pq_test_macro)
{
    struct val vals[2] = {};
    ccc_flat_priority_queue pq
        = ccc_fpq_init(&vals, (sizeof(vals) / sizeof(struct val)), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    struct val *res = ccc_fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_empty(&pq), false);
    struct val *res2 = ccc_fpq_emplace(&pq, (struct val){.val = 0, .id = 0});
    CHECK(res2 == NULL, true);
    END_TEST();
}

BEGIN_STATIC_TEST(pq_test_push)
{
    struct val vals[3] = {};
    ccc_flat_priority_queue pq
        = ccc_fpq_init(&vals, (sizeof(vals) / sizeof(struct val)), struct val,
                       CCC_LES, NULL, val_cmp, NULL);
    struct val *res = push(&pq, &vals[0]);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_empty(&pq), false);
    END_TEST();
}

BEGIN_STATIC_TEST(pq_test_raw_type)
{
    int vals[4] = {};
    ccc_flat_priority_queue pq = ccc_fpq_init(
        &vals, (sizeof(vals) / sizeof(int)), int, CCC_LES, NULL, int_cmp, NULL);
    int val = 1;
    int *res = push(&pq, &val);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_empty(&pq), false);
    res = ccc_fpq_emplace(&pq, -1);
    CHECK(res != NULL, true);
    CHECK(ccc_fpq_size(&pq), 2);
    int *popped = front(&pq);
    CHECK(*popped, -1);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(pq_test_empty(), pq_test_macro(), pq_test_push(),
                     pq_test_raw_type());
}
