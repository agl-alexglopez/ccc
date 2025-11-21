#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_utility.h"
#include "traits.h"

static CCC_Singly_linked_list
construct_empty(void)
{
    CCC_Singly_linked_list return_this = CCC_singly_linked_list_initialize(
        struct Val, e, val_order, NULL, NULL);
    return return_this;
}

check_static_begin(singly_linked_list_test_construct)
{
    Singly_linked_list singly_linked_list
        = singly_linked_list_initialize(struct Val, e, val_order, NULL, NULL);
    check(is_empty(&singly_linked_list), true);
    check_end();
}

/** C has no constructor or destructor mechanism. Struct is copied by default.
Therefore the singly linked list MUST not use any sentinel mechanisms in which
the list struct holds references to itself. If the user tries to tidy up their
code by creating a constructor like function, we would immediately break and
enter Undefined Behavior when the list constructed in the helper function is
copied to the calling code's stack frame. Therefore, we implement the singly
linked list in a way that is paranoid about, and protected from, such misuse.
This way we do not enforce any coding style on the user. */
check_static_begin(singly_linked_list_test_constructor_copy)
{
    CCC_Singly_linked_list copy = construct_empty();
    struct Val val1 = {};
    check(is_empty(&copy), true);
    check(singly_linked_list_push_front(&copy, &val1.e) != NULL, true);
    check(is_empty(&copy), false);
    check(count(&copy).count, 1);
    check(validate(&copy), true);
    check_end();
}

int
main()
{
    return check_run(singly_linked_list_test_construct(),
                     singly_linked_list_test_constructor_copy());
}
