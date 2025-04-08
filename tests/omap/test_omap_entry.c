/** This file dedicated to testing the Entry Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the entry functions. */
#define ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "omap_util.h"
#include "ordered_map.h"
#include "traits.h"
#include "types.h"

#include <assert.h>
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
    return (struct val){.key = id, .val = val};
}

static inline void
plus(ccc_any_type const t)
{
    ((struct val *)t.any_type)->val++;
}

static inline void
plusaux(ccc_any_type const t)
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
CHECK_BEGIN_STATIC_FN(fill_n, ordered_map *const om, size_t const n,
                      int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        ccc_entry ent = swap_entry(
            om, &(struct val){.key = id_and_val, .val = id_and_val}.elem,
            &(struct val){}.elem);
        CHECK(insert_error(&ent), false);
        CHECK(occupied(&ent), false);
        CHECK(validate(om), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(omap_test_validate)
{
    struct val_pool vals
        = {.vals = (struct val[3]){}, .next_free = 0, .capacity = 3};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    ccc_entry ent = swap_entry(&om, &(struct val){.key = -1, .val = -1}.elem,
                               &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, 1);
    ent = swap_entry(&om, &(struct val){.key = -1, .val = -1}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    ccc_entry ent = swap_entry(&om, &(struct val){.key = -1, .val = -1}.elem,
                               &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, 1);
    ent = swap_entry(&om, &(struct val){.key = -1, .val = -1}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = swap_entry(&om, &(struct val){.key = i, .val = i}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, i + 2);
    ent = swap_entry(&om, &(struct val){.key = i, .val = i}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = swap_entry(&om, &(struct val){.key = i, .val = i}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, i + 2);
    ent = swap_entry(&om, &(struct val){.key = i, .val = i}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_remove)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    ccc_entry ent = ccc_remove(&om, &(struct val){.key = -1, .val = -1}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, 0);
    ent = swap_entry(&om, &(struct val){.key = -1, .val = -1}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, 1);
    ent = ccc_remove(&om, &(struct val){.key = -1, .val = -1}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, 0);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = ccc_remove(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&om).count, i);
    ent = swap_entry(&om, &(struct val){.key = i, .val = i}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, i + 1);
    ent = ccc_remove(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, i);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = ccc_remove(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&om).count, i);
    ent = swap_entry(&om, &(struct val){.key = i, .val = i}.elem,
                     &(struct val){}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, i + 1);
    ent = ccc_remove(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, i);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_try_insert)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    ccc_entry ent = try_insert(&om, &(struct val){.key = -1, .val = -1}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&om).count, 1);
    ent = try_insert(&om, &(struct val){.key = -1, .val = -1}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = try_insert(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&om).count, i + 2);
    ent = try_insert(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = try_insert(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&om).count, i + 2);
    ent = try_insert(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_try_insert_with)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    ccc_entry *ent = om_try_insert_w(&om, -1, val(-1));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&om).count, 1);
    ent = om_try_insert_w(&om, -1, val(-1));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = om_try_insert_w(&om, i, val(i));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&om).count, i + 2);
    ent = om_try_insert_w(&om, i, val(i));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = om_try_insert_w(&om, i, val(i));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&om).count, i + 2);
    ent = om_try_insert_w(&om, i, val(i));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_or_assign)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    ccc_entry ent
        = insert_or_assign(&om, &(struct val){.key = -1, .val = -1}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&om).count, 1);
    ent = insert_or_assign(&om, &(struct val){.key = -1, .val = -2}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, 1);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = insert_or_assign(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&om).count, i + 2);
    ent = insert_or_assign(&om, &(struct val){.key = i, .val = i + 1}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = insert_or_assign(&om, &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&om).count, i + 2);
    ent = insert_or_assign(&om, &(struct val){.key = i, .val = i + 1}.elem);
    CHECK(validate(&om), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_or_assign_with)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    ccc_entry *ent = om_insert_or_assign_w(&om, -1, val(-1));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&om).count, 1);
    ent = om_insert_or_assign_w(&om, -1, val(-2));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = om_insert_or_assign_w(&om, i, val(i));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&om).count, i + 2);
    ent = om_insert_or_assign_w(&om, i, val(i + 1));
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = om_insert_or_assign_w(&om, i, val(i));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(size(&om).count, i + 2);
    ent = om_insert_or_assign_w(&om, i, val(i + 1));
    CHECK(validate(&om), true);
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_entry_and_modify)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    ccc_omap_entry *ent = entry_r(&om, &(int){-1});
    CHECK(validate(&om), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, 0);
    ent = and_modify(ent, plus);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, 0);
    (void)om_insert_or_assign_w(&om, -1, val(-1));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&om, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, i + 1);
    (void)om_insert_or_assign_w(&om, i, val(i));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = entry_r(&om, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, i + 1);
    (void)om_insert_or_assign_w(&om, i, val(i));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_entry_and_modify_aux)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    int aux = 1;
    ccc_omap_entry *ent = entry_r(&om, &(int){-1});
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, 0);
    (void)om_insert_or_assign_w(&om, -1, val(-1));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&om).count, 1);
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&om, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, i + 1);
    (void)om_insert_or_assign_w(&om, i, val(i));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(size(&om).count, i + 2);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = entry_r(&om, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, i + 1);
    (void)om_insert_or_assign_w(&om, i, val(i));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(size(&om).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_entry_and_modify_with)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    ccc_omap_entry *ent = entry_r(&om, &(int){-1});
    ent = om_and_modify_w(ent, struct val, { T->val++; });
    CHECK(size(&om).count, 0);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, 0);
    (void)om_insert_or_assign_w(&om, -1, val(-1));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &(int){-1});
    struct val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = om_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    CHECK(size(&om).count, 1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&om, &i);
    ent = om_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, i + 1);
    (void)om_insert_or_assign_w(&om, i, val(i));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &i);
    ent = om_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(size(&om).count, i + 2);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    ent = entry_r(&om, &i);
    ent = om_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(size(&om).count, i + 1);
    (void)om_insert_or_assign_w(&om, i, val(i));
    CHECK(validate(&om), true);
    ent = entry_r(&om, &i);
    ent = om_and_modify_w(ent, struct val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(size(&om).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_or_insert)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    struct val *v = or_insert(entry_r(&om, &(int){-1}),
                              &(struct val){.key = -1, .val = -1}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&om).count, 1);
    v = or_insert(entry_r(&om, &(int){-1}),
                  &(struct val){.key = -1, .val = -2}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&om).count, 1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    v = or_insert(entry_r(&om, &i), &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    v = or_insert(entry_r(&om, &i), &(struct val){.key = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    v = or_insert(entry_r(&om, &i), &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    v = or_insert(entry_r(&om, &i), &(struct val){.key = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_or_insert_with)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    struct val *v = om_or_insert_w(entry_r(&om, &(int){-1}), idval(-1, -1));
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&om).count, 1);
    v = om_or_insert_w(entry_r(&om, &(int){-1}), idval(-1, -2));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&om).count, 1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    v = om_or_insert_w(entry_r(&om, &i), idval(i, i));
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    v = om_or_insert_w(entry_r(&om, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    v = om_or_insert_w(entry_r(&om, &i), idval(i, i));
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    v = om_or_insert_w(entry_r(&om, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_entry)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    struct val *v = insert_entry(entry_r(&om, &(int){-1}),
                                 &(struct val){.key = -1, .val = -1}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&om).count, 1);
    v = insert_entry(entry_r(&om, &(int){-1}),
                     &(struct val){.key = -1, .val = -2}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -2);
    CHECK(size(&om).count, 1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    v = insert_entry(entry_r(&om, &i), &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    v = insert_entry(entry_r(&om, &i),
                     &(struct val){.key = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(size(&om).count, i + 2);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    v = insert_entry(entry_r(&om, &i), &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    v = insert_entry(entry_r(&om, &i),
                     &(struct val){.key = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(size(&om).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_entry_with)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    struct val *v = om_insert_entry_w(entry_r(&om, &(int){-1}), idval(-1, -1));
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&om).count, 1);
    v = om_insert_entry_w(entry_r(&om, &(int){-1}), idval(-1, -2));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -2);
    CHECK(size(&om).count, 1);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    v = om_insert_entry_w(entry_r(&om, &i), idval(i, i));
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    v = om_insert_entry_w(entry_r(&om, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(size(&om).count, i + 2);
    ++i;

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    v = om_insert_entry_w(entry_r(&om, &i), idval(i, i));
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 2);
    v = om_insert_entry_w(entry_r(&om, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(size(&om).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_remove_entry)
{
    struct val_pool vals
        = {.vals = (struct val[35]){}, .next_free = 0, .capacity = 35};
    ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, val_bump_alloc, &vals);
    int size = 30;
    struct val *v = or_insert(entry_r(&om, &(int){-1}),
                              &(struct val){.key = -1, .val = -1}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&om).count, 1);
    ccc_entry *e = remove_entry_r(entry_r(&om, &(int){-1}));
    CHECK(validate(&om), true);
    CHECK(occupied(e), true);
    CHECK(size(&om).count, 0);
    int i = 0;

    CHECK(fill_n(&om, size / 2, i), PASS);

    i += (size / 2);
    v = or_insert(entry_r(&om, &i), &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 1);
    e = remove_entry_r(entry_r(&om, &i));
    CHECK(validate(&om), true);
    CHECK(occupied(e), true);
    CHECK(size(&om).count, i);

    CHECK(fill_n(&om, size - i, i), PASS);

    i = size;
    v = or_insert(entry_r(&om, &i), &(struct val){.key = i, .val = i}.elem);
    CHECK(validate(&om), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&om).count, i + 1);
    e = remove_entry_r(entry_r(&om, &i));
    CHECK(validate(&om), true);
    CHECK(occupied(e), true);
    CHECK(size(&om).count, i);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(
        omap_test_insert(), omap_test_remove(), omap_test_validate(),
        omap_test_try_insert(), omap_test_try_insert_with(),
        omap_test_insert_or_assign(), omap_test_insert_or_assign_with(),
        omap_test_entry_and_modify(), omap_test_entry_and_modify_aux(),
        omap_test_entry_and_modify_with(), omap_test_or_insert(),
        omap_test_or_insert_with(), omap_test_insert_entry(),
        omap_test_insert_entry_with(), omap_test_remove_entry());
}
