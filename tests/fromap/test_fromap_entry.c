/** This file dedicated to testing the Entry Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the entry functions. */
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_realtime_ordered_map.h"
#include "fromap_util.h"
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
CHECK_BEGIN_STATIC_FN(fill_n, ccc_flat_realtime_ordered_map *const frm,
                      size_t const n, int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        ccc_entry ent = insert(
            frm, &(struct val){.id = id_and_val, .val = id_and_val}.elem);
        CHECK(insert_error(&ent), false);
        CHECK(occupied(&ent), false);
        CHECK(validate(frm), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(fromap_test_validate)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[3]){}, elem, id, id_cmp, NULL, NULL, 3);
    ccc_entry ent = insert(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), 1);
    ent = insert(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert)
{
    int size = 30;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    ccc_entry ent = insert(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), 1);
    ent = insert(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), i + 2);
    ent = insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), i + 2);
    ent = insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_remove)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry ent = remove(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), 0);
    ent = insert(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), 1);
    ent = remove(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), 0);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = remove(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&frm), i);
    ent = insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), i + 1);
    ent = remove(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), i);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = remove(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&frm), i);
    ent = insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), i + 1);
    ent = remove(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), i);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_try_insert)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry ent = try_insert(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&frm), 1);
    ent = try_insert(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = try_insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&frm), i + 2);
    ent = try_insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = try_insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&frm), i + 2);
    ent = try_insert(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_try_insert_with)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry *ent = frm_try_insert_w(&frm, -1, val(-1));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&frm), 1);
    ent = frm_try_insert_w(&frm, -1, val(-1));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&frm), 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = frm_try_insert_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&frm), i + 2);
    ent = frm_try_insert_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = frm_try_insert_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&frm), i + 2);
    ent = frm_try_insert_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_or_assign)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry ent
        = insert_or_assign(&frm, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&frm), 1);
    ent = insert_or_assign(&frm, &(struct val){.id = -1, .val = -2}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = insert_or_assign(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&frm), i + 2);
    ent = insert_or_assign(&frm, &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = insert_or_assign(&frm, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&frm), i + 2);
    ent = insert_or_assign(&frm, &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(validate(&frm), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_or_assign_with)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry *ent = frm_insert_or_assign_w(&frm, -1, val(-1));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&frm), 1);
    ent = frm_insert_or_assign_w(&frm, -1, val(-2));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&frm), 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = frm_insert_or_assign_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&frm), i + 2);
    ent = frm_insert_or_assign_w(&frm, i, val(i + 1));
    CHECK(occupied(ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = frm_insert_or_assign_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&frm), i + 2);
    ent = frm_insert_or_assign_w(&frm, i, val(i + 1));
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), true);
    CHECK(size(&frm), i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_entry_and_modify)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_fromap_entry *ent = entry_r(&frm, &(int){-1});
    CHECK(validate(&frm), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), 0);
    ent = and_modify(ent, plus);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), 0);
    (void)frm_insert_or_assign_w(&frm, -1, val(-1));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&frm), 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&frm, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), i + 1);
    (void)frm_insert_or_assign_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&frm), i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = entry_r(&frm, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), i + 1);
    (void)frm_insert_or_assign_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&frm), i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_entry_and_modify_aux)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    int aux = 1;
    ccc_fromap_entry *ent = entry_r(&frm, &(int){-1});
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), 0);
    (void)frm_insert_or_assign_w(&frm, -1, val(-1));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&frm), 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&frm, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), i + 1);
    (void)frm_insert_or_assign_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&frm), i + 2);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = entry_r(&frm, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), i + 1);
    (void)frm_insert_or_assign_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&frm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_entry_and_modify_with)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_fromap_entry *ent = entry_r(&frm, &(int){-1});
    ent = frm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(size(&frm), 0);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), 0);
    (void)frm_insert_or_assign_w(&frm, -1, val(-1));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &(int){-1});
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = frm_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    CHECK(size(&frm), 1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&frm, &i);
    ent = frm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), i + 1);
    (void)frm_insert_or_assign_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &i);
    ent = frm_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&frm), i + 2);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    ent = entry_r(&frm, &i);
    ent = frm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&frm), i + 1);
    (void)frm_insert_or_assign_w(&frm, i, val(i));
    CHECK(validate(&frm), true);
    ent = entry_r(&frm, &i);
    ent = frm_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&frm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_or_insert)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = or_insert(entry_r(&frm, &(int){-1}),
                              &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&frm), 1);
    v = or_insert(entry_r(&frm, &(int){-1}),
                  &(struct val){.id = -1, .val = -2}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&frm), 1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    v = or_insert(entry_r(&frm, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    v = or_insert(entry_r(&frm, &i), &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    v = or_insert(entry_r(&frm, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    v = or_insert(entry_r(&frm, &i), &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_or_insert_with)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = frm_or_insert_w(entry_r(&frm, &(int){-1}), idval(-1, -1));
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&frm), 1);
    v = frm_or_insert_w(entry_r(&frm, &(int){-1}), idval(-1, -2));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&frm), 1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    v = frm_or_insert_w(entry_r(&frm, &i), idval(i, i));
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    v = frm_or_insert_w(entry_r(&frm, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    v = frm_or_insert_w(entry_r(&frm, &i), idval(i, i));
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    v = frm_or_insert_w(entry_r(&frm, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_entry)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = insert_entry(entry_r(&frm, &(int){-1}),
                                 &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&frm), 1);
    v = insert_entry(entry_r(&frm, &(int){-1}),
                     &(struct val){.id = -1, .val = -2}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(size(&frm), 1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    v = insert_entry(entry_r(&frm, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    v = insert_entry(entry_r(&frm, &i),
                     &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&frm), i + 2);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    v = insert_entry(entry_r(&frm, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    v = insert_entry(entry_r(&frm, &i),
                     &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&frm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_entry_with)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v
        = frm_insert_entry_w(entry_r(&frm, &(int){-1}), idval(-1, -1));
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&frm), 1);
    v = frm_insert_entry_w(entry_r(&frm, &(int){-1}), idval(-1, -2));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(size(&frm), 1);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    v = frm_insert_entry_w(entry_r(&frm, &i), idval(i, i));
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    v = frm_insert_entry_w(entry_r(&frm, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&frm), i + 2);
    ++i;

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    v = frm_insert_entry_w(entry_r(&frm, &i), idval(i, i));
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 2);
    v = frm_insert_entry_w(entry_r(&frm, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&frm), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_remove_entry)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = or_insert(entry_r(&frm, &(int){-1}),
                              &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&frm), 1);
    ccc_entry *e = remove_entry_r(entry_r(&frm, &(int){-1}));
    CHECK(validate(&frm), true);
    CHECK(occupied(e), true);
    v = unwrap(e);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&frm), 0);
    int i = 0;

    CHECK(fill_n(&frm, size / 2, i), PASS);

    i += (size / 2);
    v = or_insert(entry_r(&frm, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 1);
    e = remove_entry_r(entry_r(&frm, &i));
    CHECK(validate(&frm), true);
    CHECK(occupied(e), true);
    v = unwrap(e);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i);

    CHECK(fill_n(&frm, size - i, i), PASS);

    i = size;
    v = or_insert(entry_r(&frm, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&frm), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i + 1);
    e = remove_entry_r(entry_r(&frm, &i));
    CHECK(validate(&frm), true);
    CHECK(occupied(e), true);
    v = unwrap(e);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&frm), i);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(
        fromap_test_insert(), fromap_test_remove(), fromap_test_validate(),
        fromap_test_try_insert(), fromap_test_try_insert_with(),
        fromap_test_insert_or_assign(), fromap_test_insert_or_assign_with(),
        fromap_test_entry_and_modify(), fromap_test_entry_and_modify_aux(),
        fromap_test_entry_and_modify_with(), fromap_test_or_insert(),
        fromap_test_or_insert_with(), fromap_test_insert_entry(),
        fromap_test_insert_entry_with(), fromap_test_remove_entry());
}
