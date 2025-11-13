/** This file dedicated to testing the Handle Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the handle functions. */
#include <stddef.h>
#include <stdlib.h>

#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "hromap_util.h"
#include "traits.h"
#include "types.h"

static inline struct val
val(int const val)
{
    return (struct val){.val = val};
}

static inline struct val
idval(int const id, int const val)
{
    return (struct val){.id = id, .val = val};
}

static inline void
plus(CCC_Type_context const t)
{
    ((struct val *)t.any_type)->val++;
}

static inline void
plusaux(CCC_Type_context const t)
{
    ((struct val *)t.any_type)->val += *(int *)t.aux;
}

/* Every test should have three uses of each tested function: one when the
   container is empty, one when the container has a few elements and one when
   the container has many elements. If the function has different behavior
   given an element being present or absent, each possibility should be
   tested at each of those three stages. */

/* Fills the container with n elements with id and val starting at the provided
   value and incrementing by 1 until n is reached. Assumes id_and_val are
   not present by key in the table and all subsequent inserts are unique. */
CHECK_BEGIN_STATIC_FN(fill_n, CCC_handle_realtime_ordered_map *const hrm,
                      size_t const n, int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        CCC_Handle hndl = swap_handle(
            hrm, &(struct val){.id = id_and_val, .val = id_and_val});
        CHECK(insert_error(&hndl), false);
        CHECK(occupied(&hndl), false);
        CHECK(validate(hrm), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(hromap_test_validate)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    CCC_Handle hndl = swap_handle(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, 1);
    hndl = swap_handle(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, 1);
    struct val *v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl = swap_handle(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, 1);
    hndl = swap_handle(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, 1);
    struct val *v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = swap_handle(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = swap_handle(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = swap_handle(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = swap_handle(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_remove)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl = CCC_remove(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, 0);
    hndl = swap_handle(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, 1);
    struct val old = {.id = -1};
    hndl = CCC_remove(&hrm, &old);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, 0);
    CHECK(old.val, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = CCC_remove(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i);
    hndl = swap_handle(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i + 1);
    old = (struct val){.id = i};
    hndl = CCC_remove(&hrm, &old);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, i);
    CHECK(old.val, i);
    CHECK(old.id, i);

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = CCC_remove(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i);
    hndl = swap_handle(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i + 1);
    old = (struct val){.id = i};
    hndl = CCC_remove(&hrm, &old);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, i);
    CHECK(old.val, i);
    CHECK(old.id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_try_insert)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl = try_insert(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, 1);
    hndl = try_insert(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, 1);
    struct val *v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = try_insert(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = try_insert(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = try_insert(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = try_insert(&hrm, &(struct val){.id = i, .val = i});
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_try_insert_with)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle *hndl = hrm_try_insert_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, 1);
    hndl = hrm_try_insert_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, 1);
    struct val *v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = hrm_try_insert_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = hrm_try_insert_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = hrm_try_insert_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = hrm_try_insert_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_or_assign)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle hndl
        = insert_or_assign(&hrm, &(struct val){.id = -1, .val = -1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, 1);
    hndl = insert_or_assign(&hrm, &(struct val){.id = -1, .val = -2});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, 1);
    struct val *v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = insert_or_assign(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = insert_or_assign(&hrm, &(struct val){.id = i, .val = i + 1});
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = insert_or_assign(&hrm, &(struct val){.id = i, .val = i});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = insert_or_assign(&hrm, &(struct val){.id = i, .val = i + 1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(&hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_or_assign_with)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_Handle *hndl = hrm_insert_or_assign_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, 1);
    hndl = hrm_insert_or_assign_w(&hrm, -1, val(-2));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, 1);
    struct val *v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = hrm_insert_or_assign_w(&hrm, i, val(i + 1));
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 2);
    hndl = hrm_insert_or_assign_w(&hrm, i, val(i + 1));
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_and_modify)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_hromap_handle *hndl = handle_r(&hrm, &(int){-1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, 0);
    hndl = and_modify(hndl, plus);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, 0);
    (void)hrm_insert_or_assign_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &(int){-1});
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, 1);
    struct val *v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    hndl = and_modify(hndl, plus);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = handle_r(&hrm, &i);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &i);
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, i + 2);
    hndl = and_modify(hndl, plus);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = handle_r(&hrm, &i);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &i);
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, i + 2);
    hndl = and_modify(hndl, plus);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_and_modify_aux)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    int aux = 1;
    CCC_hromap_handle *hndl = handle_r(&hrm, &(int){-1});
    hndl = and_modify_aux(hndl, plusaux, &aux);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, 0);
    (void)hrm_insert_or_assign_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &(int){-1});
    CHECK(occupied(hndl), true);
    CHECK(count(&hrm).count, 1);
    struct val *v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    hndl = and_modify_aux(hndl, plusaux, &aux);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = handle_r(&hrm, &i);
    hndl = and_modify_aux(hndl, plusaux, &aux);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &i);
    hndl = and_modify_aux(hndl, plusaux, &aux);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(count(&hrm).count, i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = handle_r(&hrm, &i);
    hndl = and_modify_aux(hndl, plusaux, &aux);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &i);
    hndl = and_modify_aux(hndl, plusaux, &aux);
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(count(&hrm).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_and_modify_with)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    CCC_hromap_handle *hndl = handle_r(&hrm, &(int){-1});
    hndl = hrm_and_modify_w(hndl, struct val, { T->val++; });
    CHECK(count(&hrm).count, 0);
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, 0);
    (void)hrm_insert_or_assign_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &(int){-1});
    struct val *v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    hndl = hrm_and_modify_w(hndl, struct val, { T->val++; });
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    CHECK(count(&hrm).count, 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    hndl = handle_r(&hrm, &i);
    hndl = hrm_and_modify_w(hndl, struct val, { T->val++; });
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &i);
    hndl = hrm_and_modify_w(hndl, struct val, { T->val++; });
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(count(&hrm).count, i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    hndl = handle_r(&hrm, &i);
    hndl = hrm_and_modify_w(hndl, struct val, { T->val++; });
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    hndl = handle_r(&hrm, &i);
    hndl = hrm_and_modify_w(hndl, struct val, { T->val++; });
    v = hrm_at(&hrm, unwrap(hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(count(&hrm).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_or_insert)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct val *v = hrm_at(&hrm, or_insert(handle_r(&hrm, &(int){-1}),
                                           &(struct val){.id = -1, .val = -1}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&hrm).count, 1);
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &(int){-1}),
                               &(struct val){.id = -1, .val = -2}));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&hrm).count, 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm,
               or_insert(handle_r(&hrm, &i), &(struct val){.id = i, .val = i}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &i),
                               &(struct val){.id = i, .val = i + 1}));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm,
               or_insert(handle_r(&hrm, &i), &(struct val){.id = i, .val = i}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &i),
                               &(struct val){.id = i, .val = i + 1}));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_or_insert_with)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct val *v = hrm_at(
        &hrm, hrm_or_insert_w(handle_r(&hrm, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&hrm).count, 1);
    v = hrm_at(&hrm,
               hrm_or_insert_w(handle_r(&hrm, &(int){-1}), idval(-1, -2)));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&hrm).count, 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), idval(i, i)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), idval(i, i)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_handle)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct val *v
        = hrm_at(&hrm, insert_handle(handle_r(&hrm, &(int){-1}),
                                     &(struct val){.id = -1, .val = -1}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&hrm).count, 1);
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &(int){-1}),
                                   &(struct val){.id = -1, .val = -2}));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(count(&hrm).count, 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                   &(struct val){.id = i, .val = i}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                   &(struct val){.id = i, .val = i + 1}));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(count(&hrm).count, i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                   &(struct val){.id = i, .val = i}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                   &(struct val){.id = i, .val = i + 1}));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(count(&hrm).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_handle_with)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct val *v = hrm_at(
        &hrm, hrm_insert_handle_w(handle_r(&hrm, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&hrm).count, 1);
    v = hrm_at(&hrm,
               hrm_insert_handle_w(handle_r(&hrm, &(int){-1}), idval(-1, -2)));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(count(&hrm).count, 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &i), idval(i, i)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(count(&hrm).count, i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &i), idval(i, i)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 2);
    v = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(count(&hrm).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_remove_handle)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    int size = 30;
    struct val *v = hrm_at(&hrm, or_insert(handle_r(&hrm, &(int){-1}),
                                           &(struct val){.id = -1, .val = -1}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&hrm).count, 1);
    CCC_Handle *e = remove_handle_r(handle_r(&hrm, &(int){-1}));
    CHECK(validate(&hrm), true);
    CHECK(occupied(e), true);
    v = hrm_at(&hrm, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(count(&hrm).count, 0);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm,
               or_insert(handle_r(&hrm, &i), &(struct val){.id = i, .val = i}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 1);
    e = remove_handle_r(handle_r(&hrm, &i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(e), true);
    v = hrm_at(&hrm, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i);

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm,
               or_insert(handle_r(&hrm, &i), &(struct val){.id = i, .val = i}));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i + 1);
    e = remove_handle_r(handle_r(&hrm, &i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(e), true);
    v = hrm_at(&hrm, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(count(&hrm).count, i);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(
        hromap_test_insert(), hromap_test_remove(), hromap_test_validate(),
        hromap_test_try_insert(), hromap_test_try_insert_with(),
        hromap_test_insert_or_assign(), hromap_test_insert_or_assign_with(),
        hromap_test_handle_and_modify(), hromap_test_handle_and_modify_aux(),
        hromap_test_handle_and_modify_with(), hromap_test_or_insert(),
        hromap_test_or_insert_with(), hromap_test_insert_handle(),
        hromap_test_insert_handle_with(), hromap_test_remove_handle());
}
