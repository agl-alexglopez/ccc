/** This file dedicated to testing the Handle Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the handle functions. */
#include <stddef.h>
#include <stdlib.h>

#define HANDLE_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_ordered_map.h"
#include "handle_ordered_map_util.h"
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
CHECK_BEGIN_STATIC_FN(fill_n, CCC_Handle_ordered_map *const handle_ordered_map,
                      size_t const n, int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        CCC_Handle hndl
            = swap_handle(handle_ordered_map,
                          &(struct Val){.id = id_and_val, .val = id_and_val});
        CHECK(insert_error(&hndl), false);
        CHECK(occupied(&hndl), false);
        CHECK(validate(handle_ordered_map), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_validate)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Handle hndl
        = swap_handle(&handle_ordered_map, &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, 1);
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_insert)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl
        = swap_handle(&handle_ordered_map, &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, 1);
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_remove)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl
        = CCC_remove(&handle_ordered_map, &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, 0);
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val old = {.id = -1};
    hndl = CCC_remove(&handle_ordered_map, &old);
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, 0);
    CHECK(old.val, -1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl = CCC_remove(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i);
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 1);
    old = (struct Val){.id = i};
    hndl = CCC_remove(&handle_ordered_map, &old);
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, i);
    CHECK(old.val, i);
    CHECK(old.id, i);

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl = CCC_remove(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i);
    hndl = swap_handle(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 1);
    old = (struct Val){.id = i};
    hndl = CCC_remove(&handle_ordered_map, &old);
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, i);
    CHECK(old.val, i);
    CHECK(old.id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_try_insert)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl
        = try_insert(&handle_ordered_map, &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, 1);
    hndl = try_insert(&handle_ordered_map, &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl = try_insert(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = try_insert(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl = try_insert(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = try_insert(&handle_ordered_map, &(struct Val){.id = i, .val = i});
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_try_insert_with)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle *hndl
        = handle_ordered_map_try_insert_w(&handle_ordered_map, -1, val(-1));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, 1);
    hndl = handle_ordered_map_try_insert_w(&handle_ordered_map, -1, val(-1));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl = handle_ordered_map_try_insert_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = handle_ordered_map_try_insert_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl = handle_ordered_map_try_insert_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = handle_ordered_map_try_insert_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_insert_or_assign)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl = insert_or_assign(&handle_ordered_map,
                                       &(struct Val){.id = -1, .val = -1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, 1);
    hndl = insert_or_assign(&handle_ordered_map,
                            &(struct Val){.id = -1, .val = -2});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl = insert_or_assign(&handle_ordered_map,
                            &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = insert_or_assign(&handle_ordered_map,
                            &(struct Val){.id = i, .val = i + 1});
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl = insert_or_assign(&handle_ordered_map,
                            &(struct Val){.id = i, .val = i});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = insert_or_assign(&handle_ordered_map,
                            &(struct Val){.id = i, .val = i + 1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_insert_or_assign_with)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle *hndl = handle_ordered_map_insert_or_assign_w(
        &handle_ordered_map, -1, val(-1));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, 1);
    hndl = handle_ordered_map_insert_or_assign_w(&handle_ordered_map, -1,
                                                 val(-2));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl
        = handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i,
                                                 val(i + 1));
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl
        = handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i,
                                                 val(i + 1));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_handle_and_modify)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle_ordered_map_handle *hndl
        = handle_r(&handle_ordered_map, &(int){-1});
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, 0);
    hndl = and_modify(hndl, plus);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, 0);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, -1,
                                                val(-1));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &(int){-1});
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    hndl = and_modify(hndl, plus);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl = handle_r(&handle_ordered_map, &i);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 1);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &i);
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = and_modify(hndl, plus);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl = handle_r(&handle_ordered_map, &i);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 1);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &i);
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, i + 2);
    hndl = and_modify(hndl, plus);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_handle_and_modify_context)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    int context = 1;
    CCC_Handle_ordered_map_handle *hndl
        = handle_r(&handle_ordered_map, &(int){-1});
    hndl = and_modify_context(hndl, pluscontext, &context);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, 0);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, -1,
                                                val(-1));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &(int){-1});
    CHECK(occupied(hndl), true);
    CHECK(count(&handle_ordered_map).count, 1);
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    hndl = and_modify_context(hndl, pluscontext, &context);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl = handle_r(&handle_ordered_map, &i);
    hndl = and_modify_context(hndl, pluscontext, &context);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 1);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &i);
    hndl = and_modify_context(hndl, pluscontext, &context);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl = handle_r(&handle_ordered_map, &i);
    hndl = and_modify_context(hndl, pluscontext, &context);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 1);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &i);
    hndl = and_modify_context(hndl, pluscontext, &context);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_handle_and_modify_with)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle_ordered_map_handle *hndl
        = handle_r(&handle_ordered_map, &(int){-1});
    hndl = handle_ordered_map_and_modify_w(hndl, struct Val, { T->val++; });
    CHECK(count(&handle_ordered_map).count, 0);
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, 0);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, -1,
                                                val(-1));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &(int){-1});
    struct Val *v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    hndl = handle_ordered_map_and_modify_w(hndl, struct Val, { T->val++; });
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    CHECK(count(&handle_ordered_map).count, 1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    hndl = handle_r(&handle_ordered_map, &i);
    hndl = handle_ordered_map_and_modify_w(hndl, struct Val, { T->val++; });
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 1);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &i);
    hndl = handle_ordered_map_and_modify_w(hndl, struct Val, { T->val++; });
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    hndl = handle_r(&handle_ordered_map, &i);
    hndl = handle_ordered_map_and_modify_w(hndl, struct Val, { T->val++; });
    CHECK(occupied(hndl), false);
    CHECK(count(&handle_ordered_map).count, i + 1);
    (void)handle_ordered_map_insert_or_assign_w(&handle_ordered_map, i, val(i));
    CHECK(validate(&handle_ordered_map), true);
    hndl = handle_r(&handle_ordered_map, &i);
    hndl = handle_ordered_map_and_modify_w(hndl, struct Val, { T->val++; });
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_or_insert)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_ordered_map_at(
        &handle_ordered_map,
        or_insert(handle_r(&handle_ordered_map, &(int){-1}),
                  &(struct Val){.id = -1, .val = -1}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&handle_ordered_map).count, 1);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        or_insert(handle_r(&handle_ordered_map, &(int){-1}),
                  &(struct Val){.id = -1, .val = -2}));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&handle_ordered_map).count, 1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    v = handle_ordered_map_at(&handle_ordered_map,
                              or_insert(handle_r(&handle_ordered_map, &i),
                                        &(struct Val){.id = i, .val = i}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map,
                              or_insert(handle_r(&handle_ordered_map, &i),
                                        &(struct Val){.id = i, .val = i + 1}));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    v = handle_ordered_map_at(&handle_ordered_map,
                              or_insert(handle_r(&handle_ordered_map, &i),
                                        &(struct Val){.id = i, .val = i}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(&handle_ordered_map,
                              or_insert(handle_r(&handle_ordered_map, &i),
                                        &(struct Val){.id = i, .val = i + 1}));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_or_insert_with)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_or_insert_w(
            handle_r(&handle_ordered_map, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&handle_ordered_map).count, 1);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_or_insert_w(
            handle_r(&handle_ordered_map, &(int){-1}), idval(-1, -2)));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&handle_ordered_map).count, 1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_or_insert_w(handle_r(&handle_ordered_map, &i),
                                       idval(i, i)));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_or_insert_w(handle_r(&handle_ordered_map, &i),
                                       idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_or_insert_w(handle_r(&handle_ordered_map, &i),
                                       idval(i, i)));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_or_insert_w(handle_r(&handle_ordered_map, &i),
                                       idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_insert_handle)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_ordered_map_at(
        &handle_ordered_map,
        insert_handle(handle_r(&handle_ordered_map, &(int){-1}),
                      &(struct Val){.id = -1, .val = -1}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&handle_ordered_map).count, 1);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        insert_handle(handle_r(&handle_ordered_map, &(int){-1}),
                      &(struct Val){.id = -1, .val = -2}));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(count(&handle_ordered_map).count, 1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    v = handle_ordered_map_at(&handle_ordered_map,
                              insert_handle(handle_r(&handle_ordered_map, &i),
                                            &(struct Val){.id = i, .val = i}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        insert_handle(handle_r(&handle_ordered_map, &i),
                      &(struct Val){.id = i, .val = i + 1}));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(count(&handle_ordered_map).count, i + 2);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    v = handle_ordered_map_at(&handle_ordered_map,
                              insert_handle(handle_r(&handle_ordered_map, &i),
                                            &(struct Val){.id = i, .val = i}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        insert_handle(handle_r(&handle_ordered_map, &i),
                      &(struct Val){.id = i, .val = i + 1}));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(count(&handle_ordered_map).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_insert_handle_with)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_insert_handle_w(
            handle_r(&handle_ordered_map, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&handle_ordered_map).count, 1);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_insert_handle_w(
            handle_r(&handle_ordered_map, &(int){-1}), idval(-1, -2)));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(count(&handle_ordered_map).count, 1);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_insert_handle_w(handle_r(&handle_ordered_map, &i),
                                           idval(i, i)));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_insert_handle_w(handle_r(&handle_ordered_map, &i),
                                           idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(count(&handle_ordered_map).count, i + 2);
    ++i;

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_insert_handle_w(handle_r(&handle_ordered_map, &i),
                                           idval(i, i)));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 2);
    v = handle_ordered_map_at(
        &handle_ordered_map,
        handle_ordered_map_insert_handle_w(handle_r(&handle_ordered_map, &i),
                                           idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(count(&handle_ordered_map).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(handle_ordered_map_test_remove_handle)
{
    CCC_Handle_ordered_map handle_ordered_map
        = handle_ordered_map_initialize(&(small_fixed_map){}, struct Val, id,
                                        id_order, NULL, NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct Val *v = handle_ordered_map_at(
        &handle_ordered_map,
        or_insert(handle_r(&handle_ordered_map, &(int){-1}),
                  &(struct Val){.id = -1, .val = -1}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&handle_ordered_map).count, 1);
    CCC_Handle *e = remove_handle_r(handle_r(&handle_ordered_map, &(int){-1}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(e), true);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&handle_ordered_map).count, 0);
    int i = 0;

    CHECK(fill_n(&handle_ordered_map, size / 2, i), PASS);

    i += (size / 2);
    v = handle_ordered_map_at(&handle_ordered_map,
                              or_insert(handle_r(&handle_ordered_map, &i),
                                        &(struct Val){.id = i, .val = i}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 1);
    e = remove_handle_r(handle_r(&handle_ordered_map, &i));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(e), true);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i);

    CHECK(fill_n(&handle_ordered_map, size - i, i), PASS);

    i = size;
    v = handle_ordered_map_at(&handle_ordered_map,
                              or_insert(handle_r(&handle_ordered_map, &i),
                                        &(struct Val){.id = i, .val = i}));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i + 1);
    e = remove_handle_r(handle_r(&handle_ordered_map, &i));
    CHECK(validate(&handle_ordered_map), true);
    CHECK(occupied(e), true);
    v = handle_ordered_map_at(&handle_ordered_map, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&handle_ordered_map).count, i);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(handle_ordered_map_test_insert(),
                     handle_ordered_map_test_remove(),
                     handle_ordered_map_test_validate(),
                     handle_ordered_map_test_try_insert(),
                     handle_ordered_map_test_try_insert_with(),
                     handle_ordered_map_test_insert_or_assign(),
                     handle_ordered_map_test_insert_or_assign_with(),
                     handle_ordered_map_test_handle_and_modify(),
                     handle_ordered_map_test_handle_and_modify_context(),
                     handle_ordered_map_test_handle_and_modify_with(),
                     handle_ordered_map_test_or_insert(),
                     handle_ordered_map_test_or_insert_with(),
                     handle_ordered_map_test_insert_handle(),
                     handle_ordered_map_test_insert_handle_with(),
                     handle_ordered_map_test_remove_handle());
}
