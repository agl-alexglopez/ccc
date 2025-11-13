#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_USING_NAMESPACE_CCC

#include "buffer_utility.h"
#include "ccc/buffer.h"
#include "ccc/types.h"
#include "checkers.h"

check_static_begin(buffer_test_iter_forward)
{
    Buffer const b = buffer_initialize(((int[6]){1, 2, 3, 4, 5, 6}), int, NULL,
                                       NULL, 6, 6);
    size_t count = 0;
    int prev = 0;
    for (int const *i = buffer_begin(&b); i != buffer_end(&b);
         i = buffer_next(&b, i))
    {
        check(i != NULL, CCC_TRUE);
        check(*i > prev, CCC_TRUE);
        ++count;
    }
    check(count, 6);
    check_end();
}

check_static_begin(buffer_test_iter_reverse)
{
    Buffer const b = buffer_initialize(((int[6]){1, 2, 3, 4, 5, 6}), int, NULL,
                                       NULL, 6, 6);
    size_t count = 0;
    int prev = 7;
    for (int const *i = buffer_rbegin(&b); i != buffer_rend(&b);
         i = buffer_rnext(&b, i))
    {
        check(i != NULL, CCC_TRUE);
        check(*i < prev, CCC_TRUE);
        ++count;
    }
    check(count, 6);
    check_end();
}

check_static_begin(buffer_test_reverse_buf)
{
    Buffer b = buffer_initialize(((int[6]){1, 2, 3, 4, 5, 6}), int, NULL, NULL,
                                 6, 6);
    int prev = 0;
    for (int const *i = buffer_begin(&b); i != buffer_end(&b);
         i = buffer_next(&b, i))
    {
        check(i != NULL, CCC_TRUE);
        check(*i > prev, CCC_TRUE);
    }
    for (int *l = buffer_begin(&b), *r = buffer_rbegin(&b); l < r;
         l = buffer_next(&b, l), r = buffer_rnext(&b, r))
    {
        CCC_Result const res = buffer_swap(&b, &(int){0}, buffer_i(&b, l).count,
                                           buffer_i(&b, r).count);
        check(res, CCC_RESULT_OK);
    }
    prev = 7;
    for (int const *i = buffer_begin(&b); i != buffer_end(&b);
         i = buffer_next(&b, i))
    {
        check(i != NULL, CCC_TRUE);
        check(*i < prev, CCC_TRUE);
    }
    check_end();
}

/** The concept of two pointers can be implemented quite cleanly with the buffer
iterator abstraction, especially because we don't force a foreach macro use
onto the user. They are able to set up a while/for loop freely. */
check_static_begin(buffer_test_trap_rainwater_two_pointers)
{
    enum : size_t
    {
        HCAP = 12,
    };
    Buffer const heights
        = buffer_initialize(((int[HCAP]){0, 1, 0, 2, 1, 0, 1, 3, 2, 1, 2, 1}),
                            int, NULL, NULL, HCAP, HCAP);
    int const correct_trapped = 6;
    int trapped = 0;
    check(buffer_is_empty(&heights), CCC_FALSE);
    int lpeak = *buffer_front_as(&heights, int);
    int rpeak = *buffer_back_as(&heights, int);
    /* Easy way to have a "skip first" iterator because the iterator is
       returned from each iterator function. */
    int const *l = buffer_next(&heights, buffer_begin(&heights));
    int const *r = buffer_rnext(&heights, buffer_rbegin(&heights));
    while (l <= r)
    {
        if (lpeak < rpeak)
        {
            lpeak = maxint(lpeak, *l);
            trapped += (lpeak - *l);
            l = buffer_next(&heights, l);
        }
        else
        {
            rpeak = maxint(rpeak, *r);
            trapped += (rpeak - *r);
            r = buffer_rnext(&heights, r);
        }
    }
    check(trapped, correct_trapped);
    check_end();
}

int
main(void)
{
    return check_run(buffer_test_iter_forward(), buffer_test_iter_reverse(),
                     buffer_test_reverse_buf(),
                     buffer_test_trap_rainwater_two_pointers());
}
