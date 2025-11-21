#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "doubly_linked_list_utility.h"
#include "traits.h"

check_static_begin(doubly_linked_list_test_construct)
{
    struct Val val = {};
    Doubly_linked_list doubly_linked_list
        = doubly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    check(is_empty(&doubly_linked_list), true);
    check(doubly_linked_list_push_front(&doubly_linked_list, &val.e) != NULL,
          true);
    check(is_empty(&doubly_linked_list), false);
    check(count(&doubly_linked_list).count, 1);
    check_end();
}

int
main()
{
    return check_run(doubly_linked_list_test_construct());
}
