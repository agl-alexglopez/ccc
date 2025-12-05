#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC

#include "checkers.h"
#include "singly_linked_list.h"
#include "singly_linked_list_utility.h"
#include "traits.h"
#include "utility/stack_allocator.h"

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

check_static_begin(singly_linked_list_test_construct_from)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(struct Val, 3);
    CCC_Singly_linked_list list = CCC_singly_linked_list_from(
        e, val_order, stack_allocator_allocate, NULL, &allocator,
        (struct Val[]){
            {.val = 0},
            {.val = 1},
            {.val = 2},
        });
    check(CCC_singly_linked_list_validate(&list), true);
    check(CCC_singly_linked_list_count(&list).count, 3);
    struct Val const *const v = CCC_singly_linked_list_front(&list);
    check(v != NULL, true);
    check(v->val, 0);
    check_end((void)CCC_singly_linked_list_clear(&list, NULL););
}

check_static_begin(singly_linked_list_test_construct_from_fail)
{
    CCC_Singly_linked_list list
        = CCC_singly_linked_list_from(e, val_order, NULL, NULL, NULL,
                                      (struct Val[]){
                                          {.val = 0},
                                          {.val = 1},
                                          {.val = 2},
                                      });
    check(CCC_singly_linked_list_validate(&list), true);
    check(CCC_singly_linked_list_is_empty(&list), true);
    check_end((void)CCC_singly_linked_list_clear(&list, NULL););
}

int
main()
{
    return check_run(singly_linked_list_test_construct(),
                     singly_linked_list_test_constructor_copy(),
                     singly_linked_list_test_construct_from(),
                     singly_linked_list_test_construct_from_fail());
}
