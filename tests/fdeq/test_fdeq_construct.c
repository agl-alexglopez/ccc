#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_double_ended_queue.h"
#include "traits.h"

#include <stddef.h>

CHECK_BEGIN_STATIC_FN(fdeq_test_construct)
{
    int vals[2];
    flat_double_ended_queue q
        = ccc_fdeq_init(vals, NULL, NULL, sizeof(vals) / sizeof(int));
    CHECK(is_empty(&q), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fdeq_test_construct());
}
