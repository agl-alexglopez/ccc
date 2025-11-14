/** This file dedicated to testing the Handle Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the handle functions. */
#include <stddef.h>
#include <stdlib.h>

#define HANDLE_ADAPTIVE_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_adaptive_map.h"
#include "handle_adaptive_map_utility.h"
#include "traits.h"
#include "types.h"

static inline struct Val
val(int const val)
{
    return (struct Val){.val = val};
}

static inline struct Val
idval(int const id, int const val)
{
    return (struct Val){.id = id, .val = val};
}

static inline void
plus(CCC_Type_context const t)
{
    ((struct Val *)t.type)->val++;
}

static inline void
pluscontext(CCC_Type_context const t)
{
    ((struct Val *)t.type)->val += *(int *)t.context;
}

/* Every test should have three uses of each tested function: one when the
   container is empty, one when the container has a few elements and one when
   the container has many elements. If the function has different behavior
   given an element being present or absent, each possibility should be
   tested at each of those three stages. */

/* Fills the container with n elements with id and val starting at the provided
   value and incrementing by 1 until n is reached. Assumes id_and_val are
   not present by key in the table and all subsequent inserts are unique. */
check_static_begin(fill_n, CCC_Handle_adaptive_map *const handle_adaptive_map,
                   size_t const n, int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        CCC_Handle hndl
            = swap_handle(handle_adaptive_map,
                          &(struct Val){.id = id_and_val, .val = id_and_val});
        check(insert_error(&hndl), false);
        check(occupied(&hndl), false);
        check(validate(handle_adaptive_map), true);
    }
    check_end();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
check_static_begin(handle_adaptive_map_test_validate)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Handle hndl
        = swap_handle(&handle_adaptive_map, &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, 1);
    hndl
        = swap_handle(&handle_adaptive_map, &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, 1);
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, -1);
    check(v->id, -1);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl
        = swap_handle(&handle_adaptive_map, &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, 1);
    hndl
        = swap_handle(&handle_adaptive_map, &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, 1);
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, -1);
    check(v->id, -1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = swap_handle(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = swap_handle(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, i);
    check(v->id, i);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = swap_handle(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = swap_handle(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, i);
    check(v->id, i);
    check_end();
}

check_static_begin(handle_adaptive_map_test_remove)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl
        = CCC_remove(&handle_adaptive_map, &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, 0);
    hndl
        = swap_handle(&handle_adaptive_map, &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, 1);
    struct Val old = {.id = -1};
    hndl = CCC_remove(&handle_adaptive_map, &old);
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, 0);
    check(old.val, -1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = CCC_remove(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i);
    hndl = swap_handle(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i + 1);
    old = (struct Val){.id = i};
    hndl = CCC_remove(&handle_adaptive_map, &old);
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, i);
    check(old.val, i);
    check(old.id, i);

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = CCC_remove(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i);
    hndl = swap_handle(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i + 1);
    old = (struct Val){.id = i};
    hndl = CCC_remove(&handle_adaptive_map, &old);
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, i);
    check(old.val, i);
    check(old.id, i);
    check_end();
}

check_static_begin(handle_adaptive_map_test_try_insert)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl
        = try_insert(&handle_adaptive_map, &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, 1);
    hndl = try_insert(&handle_adaptive_map, &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, 1);
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, -1);
    check(v->id, -1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = try_insert(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = try_insert(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, i);
    check(v->id, i);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = try_insert(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = try_insert(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, i);
    check(v->id, i);
    check_end();
}

check_static_begin(handle_adaptive_map_test_try_insert_with)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle *hndl = handle_adaptive_map_try_insert_with(&handle_adaptive_map,
                                                           -1, val(-1));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, 1);
    hndl = handle_adaptive_map_try_insert_with(&handle_adaptive_map, -1,
                                               val(-1));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, 1);
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, -1);
    check(v->id, -1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = handle_adaptive_map_try_insert_with(&handle_adaptive_map, i, val(i));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = handle_adaptive_map_try_insert_with(&handle_adaptive_map, i, val(i));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i);
    check(v->id, i);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = handle_adaptive_map_try_insert_with(&handle_adaptive_map, i, val(i));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = handle_adaptive_map_try_insert_with(&handle_adaptive_map, i, val(i));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i);
    check(v->id, i);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_or_assign)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl = insert_or_assign(&handle_adaptive_map,
                                       &(struct Val){.id = -1, .val = -1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, 1);
    hndl = insert_or_assign(&handle_adaptive_map,
                            &(struct Val){.id = -1, .val = -2});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, 1);
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, -2);
    check(v->id, -1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = insert_or_assign(&handle_adaptive_map,
                            &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = insert_or_assign(&handle_adaptive_map,
                            &(struct Val){.id = i, .val = i + 1});
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = insert_or_assign(&handle_adaptive_map,
                            &(struct Val){.id = i, .val = i});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = insert_or_assign(&handle_adaptive_map,
                            &(struct Val){.id = i, .val = i + 1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(&hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_or_assign_with)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle *hndl = handle_adaptive_map_insert_or_assign_with(
        &handle_adaptive_map, -1, val(-1));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, 1);
    hndl = handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, -1,
                                                     val(-2));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, 1);
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, -2);
    check(v->id, -1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                     val(i));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                     val(i + 1));
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                     val(i));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                     val(i + 1));
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    check_end();
}

check_static_begin(handle_adaptive_map_test_handle_and_modify)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle_adaptive_map_handle *hndl
        = handle_wrap(&handle_adaptive_map, &(int){-1});
    check(validate(&handle_adaptive_map), true);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, 0);
    hndl = and_modify(hndl, plus);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, 0);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, -1,
                                                    val(-1));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &(int){-1});
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, 1);
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, -1);
    check(v->id, -1);
    hndl = and_modify(hndl, plus);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, 0);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 1);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                    val(i));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = and_modify(hndl, plus);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = handle_wrap(&handle_adaptive_map, &i);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 1);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                    val(i));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, i + 2);
    hndl = and_modify(hndl, plus);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    check_end();
}

check_static_begin(handle_adaptive_map_test_handle_and_modify_context)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    int context = 1;
    CCC_Handle_adaptive_map_handle *hndl
        = handle_wrap(&handle_adaptive_map, &(int){-1});
    hndl = and_modify_context(hndl, pluscontext, &context);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, 0);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, -1,
                                                    val(-1));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &(int){-1});
    check(occupied(hndl), true);
    check(count(&handle_adaptive_map).count, 1);
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, -1);
    check(v->id, -1);
    hndl = and_modify_context(hndl, pluscontext, &context);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, 0);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    hndl = and_modify_context(hndl, pluscontext, &context);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 1);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                    val(i));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    hndl = and_modify_context(hndl, pluscontext, &context);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    check(count(&handle_adaptive_map).count, i + 2);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = handle_wrap(&handle_adaptive_map, &i);
    hndl = and_modify_context(hndl, pluscontext, &context);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 1);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                    val(i));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    hndl = and_modify_context(hndl, pluscontext, &context);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    check(count(&handle_adaptive_map).count, i + 2);
    check_end();
}

check_static_begin(handle_adaptive_map_test_handle_and_modify_with)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle_adaptive_map_handle *hndl
        = handle_wrap(&handle_adaptive_map, &(int){-1});
    hndl = handle_adaptive_map_and_modify_with(hndl, struct Val, { T->val++; });
    check(count(&handle_adaptive_map).count, 0);
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, 0);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, -1,
                                                    val(-1));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &(int){-1});
    struct Val *v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, -1);
    check(v->id, -1);
    hndl = handle_adaptive_map_and_modify_with(hndl, struct Val, { T->val++; });
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, 0);
    check(count(&handle_adaptive_map).count, 1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    hndl = handle_adaptive_map_and_modify_with(hndl, struct Val, { T->val++; });
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 1);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                    val(i));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    hndl = handle_adaptive_map_and_modify_with(hndl, struct Val, { T->val++; });
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    check(count(&handle_adaptive_map).count, i + 2);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    hndl = handle_wrap(&handle_adaptive_map, &i);
    hndl = handle_adaptive_map_and_modify_with(hndl, struct Val, { T->val++; });
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, i + 1);
    (void)handle_adaptive_map_insert_or_assign_with(&handle_adaptive_map, i,
                                                    val(i));
    check(validate(&handle_adaptive_map), true);
    hndl = handle_wrap(&handle_adaptive_map, &i);
    hndl = handle_adaptive_map_and_modify_with(hndl, struct Val, { T->val++; });
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(hndl));
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->id, i);
    check(count(&handle_adaptive_map).count, i + 2);
    check_end();
}

check_static_begin(handle_adaptive_map_test_or_insert)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_adaptive_map_at(
        &handle_adaptive_map,
        or_insert(handle_wrap(&handle_adaptive_map, &(int){-1}),
                  &(struct Val){.id = -1, .val = -1}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -1);
    check(count(&handle_adaptive_map).count, 1);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        or_insert(handle_wrap(&handle_adaptive_map, &(int){-1}),
                  &(struct Val){.id = -1, .val = -2}));
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -1);
    check(count(&handle_adaptive_map).count, 1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = handle_adaptive_map_at(&handle_adaptive_map,
                               or_insert(handle_wrap(&handle_adaptive_map, &i),
                                         &(struct Val){.id = i, .val = i}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map,
                               or_insert(handle_wrap(&handle_adaptive_map, &i),
                                         &(struct Val){.id = i, .val = i + 1}));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    v = handle_adaptive_map_at(&handle_adaptive_map,
                               or_insert(handle_wrap(&handle_adaptive_map, &i),
                                         &(struct Val){.id = i, .val = i}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(&handle_adaptive_map,
                               or_insert(handle_wrap(&handle_adaptive_map, &i),
                                         &(struct Val){.id = i, .val = i + 1}));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    check_end();
}

check_static_begin(handle_adaptive_map_test_or_insert_with)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_or_insert_with(
            handle_wrap(&handle_adaptive_map, &(int){-1}), idval(-1, -1)));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -1);
    check(count(&handle_adaptive_map).count, 1);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_or_insert_with(
            handle_wrap(&handle_adaptive_map, &(int){-1}), idval(-1, -2)));
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -1);
    check(count(&handle_adaptive_map).count, 1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_or_insert_with(
            handle_wrap(&handle_adaptive_map, &i), idval(i, i)));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_or_insert_with(
            handle_wrap(&handle_adaptive_map, &i), idval(i, i + 1)));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_or_insert_with(
            handle_wrap(&handle_adaptive_map, &i), idval(i, i)));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_or_insert_with(
            handle_wrap(&handle_adaptive_map, &i), idval(i, i + 1)));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_handle)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &(int){-1}),
                      &(struct Val){.id = -1, .val = -1}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -1);
    check(count(&handle_adaptive_map).count, 1);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &(int){-1}),
                      &(struct Val){.id = -1, .val = -2}));
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -2);
    check(count(&handle_adaptive_map).count, 1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &i),
                      &(struct Val){.id = i, .val = i}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &i),
                      &(struct Val){.id = i, .val = i + 1}));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i + 1);
    check(count(&handle_adaptive_map).count, i + 2);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &i),
                      &(struct Val){.id = i, .val = i}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &i),
                      &(struct Val){.id = i, .val = i + 1}));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i + 1);
    check(count(&handle_adaptive_map).count, i + 2);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_handle_with)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_insert_handle_with(
            handle_wrap(&handle_adaptive_map, &(int){-1}), idval(-1, -1)));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -1);
    check(count(&handle_adaptive_map).count, 1);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_insert_handle_with(
            handle_wrap(&handle_adaptive_map, &(int){-1}), idval(-1, -2)));
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -2);
    check(count(&handle_adaptive_map).count, 1);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_insert_handle_with(
            handle_wrap(&handle_adaptive_map, &i), idval(i, i)));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_insert_handle_with(
            handle_wrap(&handle_adaptive_map, &i), idval(i, i + 1)));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i + 1);
    check(count(&handle_adaptive_map).count, i + 2);
    ++i;

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_insert_handle_with(
            handle_wrap(&handle_adaptive_map, &i), idval(i, i)));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 2);
    v = handle_adaptive_map_at(
        &handle_adaptive_map,
        handle_adaptive_map_insert_handle_with(
            handle_wrap(&handle_adaptive_map, &i), idval(i, i + 1)));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i + 1);
    check(count(&handle_adaptive_map).count, i + 2);
    check_end();
}

check_static_begin(handle_adaptive_map_test_remove_handle)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_adaptive_map_at(
        &handle_adaptive_map,
        or_insert(handle_wrap(&handle_adaptive_map, &(int){-1}),
                  &(struct Val){.id = -1, .val = -1}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -1);
    check(count(&handle_adaptive_map).count, 1);
    CCC_Handle *e
        = remove_handle_wrap(handle_wrap(&handle_adaptive_map, &(int){-1}));
    check(validate(&handle_adaptive_map), true);
    check(occupied(e), true);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(e));
    check(v != NULL, true);
    check(v->id, -1);
    check(v->val, -1);
    check(count(&handle_adaptive_map).count, 0);
    int i = 0;

    check(fill_n(&handle_adaptive_map, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = handle_adaptive_map_at(&handle_adaptive_map,
                               or_insert(handle_wrap(&handle_adaptive_map, &i),
                                         &(struct Val){.id = i, .val = i}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 1);
    e = remove_handle_wrap(handle_wrap(&handle_adaptive_map, &i));
    check(validate(&handle_adaptive_map), true);
    check(occupied(e), true);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(e));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i);

    check(fill_n(&handle_adaptive_map, size - i, i), CHECK_PASS);

    i = size;
    v = handle_adaptive_map_at(&handle_adaptive_map,
                               or_insert(handle_wrap(&handle_adaptive_map, &i),
                                         &(struct Val){.id = i, .val = i}));
    check(validate(&handle_adaptive_map), true);
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i + 1);
    e = remove_handle_wrap(handle_wrap(&handle_adaptive_map, &i));
    check(validate(&handle_adaptive_map), true);
    check(occupied(e), true);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(e));
    check(v != NULL, true);
    check(v->id, i);
    check(v->val, i);
    check(count(&handle_adaptive_map).count, i);
    check_end();
}

int
main(void)
{
    return check_run(handle_adaptive_map_test_insert(),
                     handle_adaptive_map_test_remove(),
                     handle_adaptive_map_test_validate(),
                     handle_adaptive_map_test_try_insert(),
                     handle_adaptive_map_test_try_insert_with(),
                     handle_adaptive_map_test_insert_or_assign(),
                     handle_adaptive_map_test_insert_or_assign_with(),
                     handle_adaptive_map_test_handle_and_modify(),
                     handle_adaptive_map_test_handle_and_modify_context(),
                     handle_adaptive_map_test_handle_and_modify_with(),
                     handle_adaptive_map_test_or_insert(),
                     handle_adaptive_map_test_or_insert_with(),
                     handle_adaptive_map_test_insert_handle(),
                     handle_adaptive_map_test_insert_handle_with(),
                     handle_adaptive_map_test_remove_handle());
}
