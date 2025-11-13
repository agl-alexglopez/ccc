#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_util.h"
#include "traits.h"

CHECK_BEGIN_STATIC_FN(singly_linked_list_test_construct)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct Val, e, val_order, NULL, NULL);
    CHECK(is_empty(&singly_linked_list), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(singly_linked_list_test_construct());
}
