/** This file dedicated to testing the Handle Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the handle functions. */
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "hromap_util.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

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
plus(ccc_user_type const t)
{
    ((struct val *)t.user_type)->val++;
}

static inline void
plusaux(ccc_user_type const t)
{
    ((struct val *)t.user_type)->val += *(int *)t.aux;
}

/* Every test should have three uses of each tested function: one when the
   container is empty, one when the container has a few elements and one when
   the container has many elements. If the function has different behavior
   given an element being present or absent, each possibility should be
   tested at each of those three stages. */

/* Fills the container with n elements with id and val starting at the provided
   value and incrementing by 1 until n is reached. Assumes id_and_val are
   not present by key in the table and all subsequent inserts are unique. */
CHECK_BEGIN_STATIC_FN(fill_n, ccc_handle_realtime_ordered_map *const hrm,
                      size_t const n, int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        ccc_handle ent = insert(
            hrm, &(struct val){.id = id_and_val, .val = id_and_val}.elem);
        CHECK(insert_error(&ent), false);
        CHECK(occupied(&ent), false);
        CHECK(validate(hrm), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(hromap_test_validate)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[3]){}, elem, id, id_cmp, NULL, NULL, 3);
    ccc_handle ent = insert(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), 0);
    CHECK(size(&hrm), 1);
    ent = insert(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), 1);
    struct val *v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert)
{
    int size = 30;
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    ccc_handle ent = insert(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), 0);
    CHECK(size(&hrm), 1);
    ent = insert(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), 1);
    struct val *v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), 0);
    CHECK(size(&hrm), i + 2);
    ent = insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), 0);
    CHECK(size(&hrm), i + 2);
    ent = insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_remove)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_handle ent = remove(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), 0);
    CHECK(size(&hrm), 0);
    ent = insert(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), 0);
    CHECK(size(&hrm), 1);
    ent = remove(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), 0);
    struct val *v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = remove(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hrm), i);
    ent = insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), 0);
    CHECK(size(&hrm), i + 1);
    ent = remove(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), i);
    v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = remove(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hrm), i);
    ent = insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), 0);
    CHECK(size(&hrm), i + 1);
    ent = remove(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), i);
    v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_try_insert)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_handle ent = try_insert(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(size(&hrm), 1);
    ent = try_insert(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), 1);
    struct val *v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = try_insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(size(&hrm), i + 2);
    ent = try_insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = try_insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(size(&hrm), i + 2);
    ent = try_insert(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_try_insert_with)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_handle *ent = hrm_try_insert_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != 0, true);
    CHECK(size(&hrm), 1);
    ent = hrm_try_insert_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), 1);
    struct val *v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = hrm_try_insert_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != 0, true);
    CHECK(size(&hrm), i + 2);
    ent = hrm_try_insert_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = hrm_try_insert_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != 0, true);
    CHECK(size(&hrm), i + 2);
    ent = hrm_try_insert_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_or_assign)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_handle ent
        = insert_or_assign(&hrm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(size(&hrm), 1);
    ent = insert_or_assign(&hrm, &(struct val){.id = -1, .val = -2}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), 1);
    struct val *v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = insert_or_assign(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(size(&hrm), i + 2);
    ent = insert_or_assign(&hrm, &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = insert_or_assign(&hrm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(size(&hrm), i + 2);
    ent = insert_or_assign(&hrm, &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(validate(&hrm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_or_assign_with)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_handle *ent = hrm_insert_or_assign_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != 0, true);
    CHECK(size(&hrm), 1);
    ent = hrm_insert_or_assign_w(&hrm, -1, val(-2));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), 1);
    struct val *v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != 0, true);
    CHECK(size(&hrm), i + 2);
    ent = hrm_insert_or_assign_w(&hrm, i, val(i + 1));
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != 0, true);
    CHECK(size(&hrm), i + 2);
    ent = hrm_insert_or_assign_w(&hrm, i, val(i + 1));
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_and_modify)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_hromap_handle *ent = handle_r(&hrm, &(int){-1});
    CHECK(validate(&hrm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), 0);
    ent = and_modify(ent, plus);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), 0);
    (void)hrm_insert_or_assign_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), 1);
    struct val *v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = and_modify(ent, plus);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hrm, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), i + 2);
    ent = and_modify(ent, plus);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = handle_r(&hrm, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), i + 2);
    ent = and_modify(ent, plus);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_and_modify_aux)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    int aux = 1;
    ccc_hromap_handle *ent = handle_r(&hrm, &(int){-1});
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), 0);
    (void)hrm_insert_or_assign_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&hrm), 1);
    struct val *v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hrm, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&hrm), i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = handle_r(&hrm, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&hrm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_and_modify_with)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_hromap_handle *ent = handle_r(&hrm, &(int){-1});
    ent = hrm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(size(&hrm), 0);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), 0);
    (void)hrm_insert_or_assign_w(&hrm, -1, val(-1));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &(int){-1});
    struct val *v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = hrm_and_modify_w(ent, struct val, { T->val++; });
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    CHECK(size(&hrm), 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hrm, &i);
    ent = hrm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &i);
    ent = hrm_and_modify_w(ent, struct val, { T->val++; });
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&hrm), i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    ent = handle_r(&hrm, &i);
    ent = hrm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == 0, true);
    CHECK(size(&hrm), i + 1);
    (void)hrm_insert_or_assign_w(&hrm, i, val(i));
    CHECK(validate(&hrm), true);
    ent = handle_r(&hrm, &i);
    ent = hrm_and_modify_w(ent, struct val, { T->val++; });
    v = hrm_at(&hrm, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&hrm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_or_insert)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v
        = hrm_at(&hrm, or_insert(handle_r(&hrm, &(int){-1}),
                                 &(struct val){.id = -1, .val = -1}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hrm), 1);
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &(int){-1}),
                               &(struct val){.id = -1, .val = -2}.elem));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hrm), 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &i),
                               &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &i),
                               &(struct val){.id = i, .val = i + 1}.elem));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &i),
                               &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &i),
                               &(struct val){.id = i, .val = i + 1}.elem));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_or_insert_with)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = hrm_at(
        &hrm, hrm_or_insert_w(handle_r(&hrm, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hrm), 1);
    v = hrm_at(&hrm,
               hrm_or_insert_w(handle_r(&hrm, &(int){-1}), idval(-1, -2)));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hrm), 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), idval(i, i)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), idval(i, i)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_handle)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v
        = hrm_at(&hrm, insert_handle(handle_r(&hrm, &(int){-1}),
                                     &(struct val){.id = -1, .val = -1}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hrm), 1);
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &(int){-1}),
                                   &(struct val){.id = -1, .val = -2}.elem));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(size(&hrm), 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                   &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                   &(struct val){.id = i, .val = i + 1}.elem));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hrm), i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                   &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                   &(struct val){.id = i, .val = i + 1}.elem));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hrm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_handle_with)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = hrm_at(
        &hrm, hrm_insert_handle_w(handle_r(&hrm, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hrm), 1);
    v = hrm_at(&hrm,
               hrm_insert_handle_w(handle_r(&hrm, &(int){-1}), idval(-1, -2)));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(size(&hrm), 1);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &i), idval(i, i)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hrm), i + 2);
    ++i;

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &i), idval(i, i)));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 2);
    v = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hrm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_remove_handle)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v
        = hrm_at(&hrm, or_insert(handle_r(&hrm, &(int){-1}),
                                 &(struct val){.id = -1, .val = -1}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hrm), 1);
    ccc_handle *e = remove_handle_r(handle_r(&hrm, &(int){-1}));
    CHECK(validate(&hrm), true);
    CHECK(occupied(e), true);
    v = hrm_at(&hrm, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hrm), 0);
    int i = 0;

    CHECK(fill_n(&hrm, size / 2, i), PASS);

    i += (size / 2);
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &i),
                               &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 1);
    e = remove_handle_r(handle_r(&hrm, &i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(e), true);
    v = hrm_at(&hrm, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i);

    CHECK(fill_n(&hrm, size - i, i), PASS);

    i = size;
    v = hrm_at(&hrm, or_insert(handle_r(&hrm, &i),
                               &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hrm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i + 1);
    e = remove_handle_r(handle_r(&hrm, &i));
    CHECK(validate(&hrm), true);
    CHECK(occupied(e), true);
    v = hrm_at(&hrm, unwrap(e));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hrm), i);
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
