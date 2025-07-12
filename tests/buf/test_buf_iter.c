#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buf_util.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"

CHECK_BEGIN_STATIC_FN(buf_test_iter_forward)
{
    buffer const b
        = buf_init(((int[6]){1, 2, 3, 4, 5, 6}), int, NULL, NULL, 6, 6);
    size_t count = 0;
    int prev = 0;
    for (int const *i = buf_begin(&b); i != buf_end(&b); i = buf_next(&b, i))
    {
        CHECK(i != NULL, CCC_TRUE);
        CHECK(*i > prev, CCC_TRUE);
        ++count;
    }
    CHECK(count, 6);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_iter_reverse)
{
    buffer const b
        = buf_init(((int[6]){1, 2, 3, 4, 5, 6}), int, NULL, NULL, 6, 6);
    size_t count = 0;
    int prev = 7;
    for (int const *i = buf_rbegin(&b); i != buf_rend(&b); i = buf_rnext(&b, i))
    {
        CHECK(i != NULL, CCC_TRUE);
        CHECK(*i < prev, CCC_TRUE);
        ++count;
    }
    CHECK(count, 6);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(buf_test_reverse_buf)
{
    buffer b = buf_init(((int[6]){1, 2, 3, 4, 5, 6}), int, NULL, NULL, 6, 6);
    int prev = 0;
    for (int const *i = buf_begin(&b); i != buf_end(&b); i = buf_next(&b, i))
    {
        CHECK(i != NULL, CCC_TRUE);
        CHECK(*i > prev, CCC_TRUE);
    }
    for (int *l = buf_begin(&b), *r = buf_rbegin(&b); l < r;
         l = buf_next(&b, l), r = buf_rnext(&b, r))
    {
        ccc_result const res
            = buf_swap(&b, &(int){}, buf_i(&b, l).count, buf_i(&b, r).count);
        CHECK(res, CCC_RESULT_OK);
    }
    prev = 7;
    for (int const *i = buf_begin(&b); i != buf_end(&b); i = buf_next(&b, i))
    {
        CHECK(i != NULL, CCC_TRUE);
        CHECK(*i < prev, CCC_TRUE);
    }
    CHECK_END_FN();
}

/** The concept of two pointers can be implemented quite cleanly with the buffer
iterator abstraction, especially because we don't force a foreach macro use
onto the user. They are able to set up a for loop freely. */
CHECK_BEGIN_STATIC_FN(buf_test_trap_rainwater_two_pointers)
{
    enum : size_t
    {
        HCAP = 12,
    };
    buffer const heights
        = buf_init(((int[HCAP]){0, 1, 0, 2, 1, 0, 1, 3, 2, 1, 2, 1}), int, NULL,
                   NULL, HCAP, HCAP);
    int const correct_trapped = 6;
    int trapped = 0;
    CHECK(buf_is_empty(&heights), CCC_FALSE);
    int lpeak = *buf_front_as(&heights, int);
    int rpeak = *buf_back_as(&heights, int);
    /* Easy way to have a "skip first" iterator because the iterator is
       returned from each iterator function. */
    int const *l = buf_next(&heights, buf_begin(&heights));
    int const *r = buf_rnext(&heights, buf_rbegin(&heights));
    while (l <= r)
    {
        if (lpeak < rpeak)
        {
            lpeak = maxint(lpeak, *l);
            trapped += (lpeak - *l);
            l = buf_next(&heights, l);
        }
        else
        {
            rpeak = maxint(rpeak, *r);
            trapped += (rpeak - *r);
            r = buf_rnext(&heights, r);
        }
    }
    CHECK(trapped, correct_trapped);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(buf_test_iter_forward(), buf_test_iter_reverse(),
                     buf_test_reverse_buf(),
                     buf_test_trap_rainwater_two_pointers());
}
