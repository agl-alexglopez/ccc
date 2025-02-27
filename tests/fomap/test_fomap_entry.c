/** This file dedicated to testing the Entry Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the entry functions. */
#define FLAT_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_ordered_map.h"
#include "fomap_util.h"
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
CHECK_BEGIN_STATIC_FN(fill_n, ccc_flat_ordered_map *const fom, size_t const n,
                      int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        ccc_entry ent = insert(
            fom, &(struct val){.id = id_and_val, .val = id_and_val}.elem);
        CHECK(insert_error(&ent), false);
        CHECK(occupied(&ent), false);
        CHECK(validate(fom), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(fomap_test_validate)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[3]){}, elem, id, id_cmp, NULL, NULL, 3);
    ccc_entry ent = insert(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fom), 1);
    ent = insert(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_insert)
{
    int size = 30;
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    ccc_entry ent = insert(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fom), 1);
    ent = insert(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fom), i + 2);
    ent = insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fom), i + 2);
    ent = insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_remove)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry ent = remove(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fom), 0);
    ent = insert(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fom), 1);
    ent = remove(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), 0);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = remove(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&fom), i);
    ent = insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fom), i + 1);
    ent = remove(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), i);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = remove(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&fom), i);
    ent = insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fom), i + 1);
    ent = remove(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), i);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_try_insert)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry ent = try_insert(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&fom), 1);
    ent = try_insert(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = try_insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&fom), i + 2);
    ent = try_insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = try_insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&fom), i + 2);
    ent = try_insert(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_try_insert_with)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry *ent = fom_try_insert_w(&fom, -1, val(-1));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&fom), 1);
    ent = fom_try_insert_w(&fom, -1, val(-1));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&fom), 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = fom_try_insert_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&fom), i + 2);
    ent = fom_try_insert_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = fom_try_insert_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&fom), i + 2);
    ent = fom_try_insert_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_insert_or_assign)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry ent
        = insert_or_assign(&fom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&fom), 1);
    ent = insert_or_assign(&fom, &(struct val){.id = -1, .val = -2}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = insert_or_assign(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&fom), i + 2);
    ent = insert_or_assign(&fom, &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = insert_or_assign(&fom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&fom), i + 2);
    ent = insert_or_assign(&fom, &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(validate(&fom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_insert_or_assign_with)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_entry *ent = fom_insert_or_assign_w(&fom, -1, val(-1));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&fom), 1);
    ent = fom_insert_or_assign_w(&fom, -1, val(-2));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&fom), 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = fom_insert_or_assign_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&fom), i + 2);
    ent = fom_insert_or_assign_w(&fom, i, val(i + 1));
    CHECK(occupied(ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = fom_insert_or_assign_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&fom), i + 2);
    ent = fom_insert_or_assign_w(&fom, i, val(i + 1));
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&fom), i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_entry_and_modify)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_fomap_entry *ent = entry_r(&fom, &(int){-1});
    CHECK(validate(&fom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), 0);
    ent = and_modify(ent, plus);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), 0);
    (void)fom_insert_or_assign_w(&fom, -1, val(-1));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&fom), 1);
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

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&fom, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), i + 1);
    (void)fom_insert_or_assign_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&fom), i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = entry_r(&fom, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), i + 1);
    (void)fom_insert_or_assign_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&fom), i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_entry_and_modify_aux)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    int aux = 1;
    ccc_fomap_entry *ent = entry_r(&fom, &(int){-1});
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), 0);
    (void)fom_insert_or_assign_w(&fom, -1, val(-1));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&fom), 1);
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

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&fom, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), i + 1);
    (void)fom_insert_or_assign_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&fom), i + 2);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = entry_r(&fom, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), i + 1);
    (void)fom_insert_or_assign_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&fom), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_entry_and_modify_with)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    ccc_fomap_entry *ent = entry_r(&fom, &(int){-1});
    ent = fom_and_modify_w(ent, struct val, { T->val++; });
    CHECK(size(&fom), 0);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), 0);
    (void)fom_insert_or_assign_w(&fom, -1, val(-1));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &(int){-1});
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = fom_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    CHECK(size(&fom), 1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&fom, &i);
    ent = fom_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), i + 1);
    (void)fom_insert_or_assign_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &i);
    ent = fom_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&fom), i + 2);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    ent = entry_r(&fom, &i);
    ent = fom_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&fom), i + 1);
    (void)fom_insert_or_assign_w(&fom, i, val(i));
    CHECK(validate(&fom), true);
    ent = entry_r(&fom, &i);
    ent = fom_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&fom), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_or_insert)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = or_insert(entry_r(&fom, &(int){-1}),
                              &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&fom), 1);
    v = or_insert(entry_r(&fom, &(int){-1}),
                  &(struct val){.id = -1, .val = -2}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&fom), 1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    v = or_insert(entry_r(&fom, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    v = or_insert(entry_r(&fom, &i), &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    v = or_insert(entry_r(&fom, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    v = or_insert(entry_r(&fom, &i), &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_or_insert_with)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = fom_or_insert_w(entry_r(&fom, &(int){-1}), idval(-1, -1));
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&fom), 1);
    v = fom_or_insert_w(entry_r(&fom, &(int){-1}), idval(-1, -2));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&fom), 1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    v = fom_or_insert_w(entry_r(&fom, &i), idval(i, i));
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    v = fom_or_insert_w(entry_r(&fom, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    v = fom_or_insert_w(entry_r(&fom, &i), idval(i, i));
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    v = fom_or_insert_w(entry_r(&fom, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_insert_entry)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = insert_entry(entry_r(&fom, &(int){-1}),
                                 &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&fom), 1);
    v = insert_entry(entry_r(&fom, &(int){-1}),
                     &(struct val){.id = -1, .val = -2}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(size(&fom), 1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    v = insert_entry(entry_r(&fom, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    v = insert_entry(entry_r(&fom, &i),
                     &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&fom), i + 2);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    v = insert_entry(entry_r(&fom, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    v = insert_entry(entry_r(&fom, &i),
                     &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&fom), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_insert_entry_with)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v
        = fom_insert_entry_w(entry_r(&fom, &(int){-1}), idval(-1, -1));
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&fom), 1);
    v = fom_insert_entry_w(entry_r(&fom, &(int){-1}), idval(-1, -2));
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(size(&fom), 1);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    v = fom_insert_entry_w(entry_r(&fom, &i), idval(i, i));
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    v = fom_insert_entry_w(entry_r(&fom, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&fom), i + 2);
    ++i;

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    v = fom_insert_entry_w(entry_r(&fom, &i), idval(i, i));
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 2);
    v = fom_insert_entry_w(entry_r(&fom, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&fom), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_remove_entry)
{
    ccc_flat_ordered_map fom
        = fom_init((struct val[33]){}, elem, id, id_cmp, NULL, NULL, 33);
    int size = 30;
    struct val *v = or_insert(entry_r(&fom, &(int){-1}),
                              &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&fom), 1);
    ccc_entry *e = remove_entry_r(entry_r(&fom, &(int){-1}));
    CHECK(validate(&fom), true);
    CHECK(occupied(e), true);
    v = unwrap(e);
    CHECK(v != NULL, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&fom), 0);
    int i = 0;

    CHECK(fill_n(&fom, size / 2, i), PASS);

    i += (size / 2);
    v = or_insert(entry_r(&fom, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 1);
    e = remove_entry_r(entry_r(&fom, &i));
    CHECK(validate(&fom), true);
    CHECK(occupied(e), true);
    v = unwrap(e);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i);

    CHECK(fill_n(&fom, size - i, i), PASS);

    i = size;
    v = or_insert(entry_r(&fom, &i), &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&fom), true);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i + 1);
    e = remove_entry_r(entry_r(&fom, &i));
    CHECK(validate(&fom), true);
    CHECK(occupied(e), true);
    v = unwrap(e);
    CHECK(v != NULL, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&fom), i);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(
        fomap_test_insert(), fomap_test_remove(), fomap_test_validate(),
        fomap_test_try_insert(), fomap_test_try_insert_with(),
        fomap_test_insert_or_assign(), fomap_test_insert_or_assign_with(),
        fomap_test_entry_and_modify(), fomap_test_entry_and_modify_aux(),
        fomap_test_entry_and_modify_with(), fomap_test_or_insert(),
        fomap_test_or_insert_with(), fomap_test_insert_entry(),
        fomap_test_insert_entry_with(), fomap_test_remove_entry());
}
