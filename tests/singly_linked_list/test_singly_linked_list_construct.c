#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_utility.h"
#include "traits.h"

check_static_begin(singly_linked_list_test_construct)
{
    Singly_linked_list singly_linked_list = singly_linked_list_initialize(
        singly_linked_list, struct Val, e, val_order, NULL, NULL);
    check(is_empty(&singly_linked_list), true);
    check_end();
}

int
main()
{
    return check_run(singly_linked_list_test_construct());
}
