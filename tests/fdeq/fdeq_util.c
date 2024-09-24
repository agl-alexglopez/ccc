#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "fdeq_util.h"
#include "flat_double_ended_queue.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdio.h>

BEGIN_TEST(create_queue, flat_double_ended_queue *const q, size_t const n,
           int const vals[])
{
    if (n)
    {
        ccc_result const res = fdeq_push_back_range(q, n, vals);
        CHECK(res, CCC_OK);
        CHECK(validate(q), true);
    }
    END_TEST();
}

BEGIN_TEST(check_order, flat_double_ended_queue const *const q, size_t const n,
           int const order[])
{
    size_t i = 0;
    int *v = begin(q);
    for (; v != end(q); v = next(q, v), ++i)
    {
        CHECK(v == NULL, false);
        CHECK(*v, order[i]);
    }
    i = n;
    v = rbegin(q);
    for (; v != rend(q) && i--; v = rnext(q, v))
    {
        CHECK(v == NULL, false);
        CHECK(*v, order[i]);
    }
    END_FAIL({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", order[j]);
        }
        (void)fprintf(stderr, "}\n%s", NONE);
        (void)fprintf(stderr, "%sERROR:%s (int[%zu]){", RED, GREEN, n);
        v = begin(q);
        for (size_t j = 0; j < n && v != end(q); ++j, v = next(q, v))
        {
            if (!v)
            {
                return TEST_STATUS;
            }
            if (order[j] == *v)
            {
                (void)fprintf(stderr, "%s%d, %s", GREEN, order[j], NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", RED, *v, NONE);
            }
        }
        for (; v != end(q); v = next(q, v))
        {
            (void)fprintf(stderr, "%s%d, %s", RED, *v, NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", GREEN, NONE);
    });
}