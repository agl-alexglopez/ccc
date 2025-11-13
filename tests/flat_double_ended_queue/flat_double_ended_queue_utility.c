#include <stddef.h>
#include <stdio.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_double_ended_queue.h"
#include "flat_double_ended_queue_utility.h"
#include "traits.h"
#include "types.h"

check_begin(create_queue, Flat_double_ended_queue *const q, size_t const n,
            int const vals[])
{
    if (n)
    {
        CCC_Result const res
            = flat_double_ended_queue_push_back_range(q, n, vals);
        check(res, CCC_RESULT_OK);
        check(validate(q), true);
    }
    check_end();
}

check_begin(check_order, Flat_double_ended_queue const *const q, size_t const n,
            int const order[])
{
    size_t i = 0;
    int *v = begin(q);
    for (; v != end(q) && i < n; v = next(q, v), ++i)
    {
        check(v == NULL, false);
        check(*v, order[i]);
    }
    i = n;
    v = reverse_begin(q);
    for (; v != reverse_end(q) && i--; v = reverse_next(q, v))
    {
        check(v == NULL, false);
        check(*v, order[i]);
    }
    check_fail_end({
        (void)fprintf(stderr, "%sCHECK: (int[%zu]){", CHECK_GREEN, n);
        for (size_t j = 0; j < n; ++j)
        {
            (void)fprintf(stderr, "%d, ", order[j]);
        }
        (void)fprintf(stderr, "}\n%s", CHECK_NONE);
        (void)fprintf(stderr, "%sCHECK_ERROR:%s (int[%zu]){", CHECK_RED,
                      CHECK_GREEN, n);
        v = begin(q);
        for (size_t j = 0; j < n && v != end(q); ++j, v = next(q, v))
        {
            if (!v)
            {
                return CHECK_STATUS;
            }
            if (order[j] == *v)
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_GREEN, order[j],
                              CHECK_NONE);
            }
            else
            {
                (void)fprintf(stderr, "%s%d, %s", CHECK_RED, *v, CHECK_NONE);
            }
        }
        for (; v != end(q); v = next(q, v))
        {
            (void)fprintf(stderr, "%s%d, %s", CHECK_RED, *v, CHECK_NONE);
        }
        (void)fprintf(stderr, "%s}\n%s", CHECK_GREEN, CHECK_NONE);
    });
}
