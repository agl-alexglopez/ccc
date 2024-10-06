#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC

#include "flat_double_ended_queue.h"
#include "test.h"
#include "traits.h"

#include <stddef.h>

BEGIN_STATIC_TEST(fdeq_test_construct)
{
    int vals[2];
    flat_double_ended_queue q
        = ccc_fdeq_init(vals, sizeof(vals) / sizeof(int), int, NULL, NULL);
    CHECK(empty(&q), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(fdeq_test_construct());
}
