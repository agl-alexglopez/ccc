#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "doubly_linked_list_utility.h"
#include "traits.h"
#include "utility/stack_allocator.h"

static CCC_Doubly_linked_list
construct_empty(void)
{
    CCC_Doubly_linked_list return_this = CCC_doubly_linked_list_initialize(
        struct Val, e, val_order, NULL, NULL);
    return return_this;
}

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

/** C has no constructor or destructor mechanism. Struct is copied by default.
Therefore the doubly linked list MUST not use any sentinel mechanisms in which
the list struct holds references to itself. If the user tries to tidy up their
code by creating a constructor like function, we would immediately break and
enter Undefined Behavior when the list constructed in the helper function is
copied to the calling code's stack frame. Therefore, we implement the doubly
linked list in a way that is paranoid about, and protected from, such misuse.
This way we do not enforce any coding style on the user. */
check_static_begin(doubly_linked_list_test_constructor_copy)
{
    CCC_Doubly_linked_list copy = construct_empty();
    struct Val val1 = {};
    struct Val val2 = {};
    check(is_empty(&copy), true);
    check(doubly_linked_list_push_front(&copy, &val1.e) != NULL, true);
    check(is_empty(&copy), false);
    check(count(&copy).count, 1);
    check(validate(&copy), true);
    check(doubly_linked_list_push_back(&copy, &val2.e) != NULL, true);
    check(count(&copy).count, 2);
    check(validate(&copy), true);
    check_end();
}

check_static_begin(doubly_linked_list_test_construct_from)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Doubly_linked_list list = CCC_doubly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[]){
            {.val = 0},
            {.val = 1},
            {.val = 2},
        });
    check(CCC_doubly_linked_list_validate(&list), true);
    check(CCC_doubly_linked_list_count(&list).count, 3);
    struct Val const *const v = CCC_doubly_linked_list_front(&list);
    check(v != NULL, true);
    check(v->val, 0);
    check_end((void)CCC_doubly_linked_list_clear(&list, NULL););
}

check_static_begin(doubly_linked_list_test_construct_from_fail)
{
    CCC_Doubly_linked_list list
        = CCC_doubly_linked_list_from(e, val_order, NULL, NULL, NULL,
                                      (struct Val[]){
                                          {.val = 0},
                                          {.val = 1},
                                          {.val = 2},
                                      });
    check(CCC_doubly_linked_list_validate(&list), true);
    check(CCC_doubly_linked_list_is_empty(&list), true);
    check_end((void)CCC_doubly_linked_list_clear(&list, NULL););
}

int
main()
{
    return check_run(doubly_linked_list_test_construct(),
                     doubly_linked_list_test_constructor_copy(),
                     doubly_linked_list_test_construct_from(),
                     doubly_linked_list_test_construct_from_fail());
}
