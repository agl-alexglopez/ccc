#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "doubly_linked_list_util.h"
#include "traits.h"

CHECK_BEGIN_STATIC_FN(doubly_linked_list_test_construct)
{
    struct Val val = {};
    Doubly_linked_list doubly_linked_list = doubly_linked_list_initialize(
        doubly_linked_list, struct Val, e, val_order, NULL, NULL);
    CHECK(is_empty(&doubly_linked_list), true);
    CHECK(doubly_linked_list_push_front(&doubly_linked_list, &val.e) != NULL,
          true);
    CHECK(is_empty(&doubly_linked_list), false);
    CHECK(count(&doubly_linked_list).count, 1);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(doubly_linked_list_test_construct());
}
