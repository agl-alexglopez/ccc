#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "sll_util.h"
#include "traits.h"

#include <stddef.h>

CHECK_BEGIN_STATIC_FN(sll_test_construct)
{
    singly_linked_list sll = sll_init(sll, struct val, e, val_cmp, NULL, NULL);
    CHECK(is_empty(&sll), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(sll_test_construct());
}
