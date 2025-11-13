/** This file dedicated to testing the Entry Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the entry functions. */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "realtime_ordered_map.h"
#include "realtime_ordered_map_utility.h"
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
    return (struct Val){.key = id, .val = val};
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
CHECK_BEGIN_STATIC_FN(fill_n, CCC_Realtime_ordered_map *const rom,
                      size_t const n, int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        CCC_Entry ent = swap_entry(
            rom, &(struct Val){.key = id_and_val, .val = id_and_val}.elem,
            &(struct Val){}.elem);
        CHECK(insert_error(&ent), false);
        CHECK(occupied(&ent), false);
        CHECK(validate(rom), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(romap_test_validate)
{
    struct Val_pool vals
        = {.vals = (struct Val[3]){}, .next_free = 0, .capacity = 3};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    CCC_Entry ent = swap_entry(&rom, &(struct Val){.key = -1, .val = -1}.elem,
                               &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&rom).count, 1);
    ent = swap_entry(&rom, &(struct Val){.key = -1, .val = -1}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, 1);
    struct Val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry ent = swap_entry(&rom, &(struct Val){.key = -1, .val = -1}.elem,
                               &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&rom).count, 1);
    ent = swap_entry(&rom, &(struct Val){.key = -1, .val = -1}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, 1);
    struct Val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = swap_entry(&rom, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&rom).count, i + 2);
    ent = swap_entry(&rom, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = swap_entry(&rom, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&rom).count, i + 2);
    ent = swap_entry(&rom, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_remove)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry ent = CCC_remove(&rom, &(struct Val){.key = -1, .val = -1}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&rom).count, 0);
    ent = swap_entry(&rom, &(struct Val){.key = -1, .val = -1}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&rom).count, 1);
    ent = CCC_remove(&rom, &(struct Val){.key = -1, .val = -1}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, 0);
    struct Val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = CCC_remove(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(count(&rom).count, i);
    ent = swap_entry(&rom, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&rom).count, i + 1);
    ent = CCC_remove(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, i);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = CCC_remove(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(count(&rom).count, i);
    ent = swap_entry(&rom, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&rom).count, i + 1);
    ent = CCC_remove(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, i);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_try_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry ent = try_insert(&rom, &(struct Val){.key = -1, .val = -1}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(count(&rom).count, 1);
    ent = try_insert(&rom, &(struct Val){.key = -1, .val = -1}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, 1);
    struct Val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = try_insert(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(count(&rom).count, i + 2);
    ent = try_insert(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = try_insert(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(count(&rom).count, i + 2);
    ent = try_insert(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_try_insert_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry *ent = realtime_ordered_map_try_insert_w(&rom, -1, val(-1));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(count(&rom).count, 1);
    ent = realtime_ordered_map_try_insert_w(&rom, -1, val(-1));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, 1);
    struct Val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = realtime_ordered_map_try_insert_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(count(&rom).count, i + 2);
    ent = realtime_ordered_map_try_insert_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = realtime_ordered_map_try_insert_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(count(&rom).count, i + 2);
    ent = realtime_ordered_map_try_insert_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_or_assign)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry ent
        = insert_or_assign(&rom, &(struct Val){.key = -1, .val = -1}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(count(&rom).count, 1);
    ent = insert_or_assign(&rom, &(struct Val){.key = -1, .val = -2}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, 1);
    struct Val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = insert_or_assign(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(count(&rom).count, i + 2);
    ent = insert_or_assign(&rom, &(struct Val){.key = i, .val = i + 1}.elem);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = insert_or_assign(&rom, &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(count(&rom).count, i + 2);
    ent = insert_or_assign(&rom, &(struct Val){.key = i, .val = i + 1}.elem);
    CHECK(validate(&rom), true);
    CHECK(occupied(&ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_or_assign_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry *ent = realtime_ordered_map_insert_or_assign_w(&rom, -1, val(-1));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(count(&rom).count, 1);
    ent = realtime_ordered_map_insert_or_assign_w(&rom, -1, val(-2));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, 1);
    struct Val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = realtime_ordered_map_insert_or_assign_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(count(&rom).count, i + 2);
    ent = realtime_ordered_map_insert_or_assign_w(&rom, i, val(i + 1));
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = realtime_ordered_map_insert_or_assign_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) != NULL, true);
    CHECK(count(&rom).count, i + 2);
    ent = realtime_ordered_map_insert_or_assign_w(&rom, i, val(i + 1));
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, i + 2);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_entry_and_modify)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Realtime_ordered_map_entry *ent = entry_r(&rom, &(int){-1});
    CHECK(validate(&rom), true);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, 0);
    ent = and_modify(ent, plus);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, 0);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, -1, val(-1));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, 1);
    struct Val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&rom, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, i + 1);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &i);
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = entry_r(&rom, &i);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, i + 1);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &i);
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_entry_and_modify_context)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    int context = 1;
    CCC_Realtime_ordered_map_entry *ent = entry_r(&rom, &(int){-1});
    ent = and_modify_context(ent, pluscontext, &context);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, 0);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, -1, val(-1));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(count(&rom).count, 1);
    struct Val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&rom, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, i + 1);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(count(&rom).count, i + 2);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = entry_r(&rom, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, i + 1);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(count(&rom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_entry_and_modify_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Realtime_ordered_map_entry *ent = entry_r(&rom, &(int){-1});
    ent = realtime_ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    CHECK(count(&rom).count, 0);
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, 0);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, -1, val(-1));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &(int){-1});
    struct Val *v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = realtime_ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    CHECK(count(&rom).count, 1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    ent = entry_r(&rom, &i);
    ent = realtime_ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, i + 1);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &i);
    ent = realtime_ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(count(&rom).count, i + 2);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    ent = entry_r(&rom, &i);
    ent = realtime_ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(unwrap(ent) == NULL, true);
    CHECK(count(&rom).count, i + 1);
    (void)realtime_ordered_map_insert_or_assign_w(&rom, i, val(i));
    CHECK(validate(&rom), true);
    ent = entry_r(&rom, &i);
    ent = realtime_ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(count(&rom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_or_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = or_insert(entry_r(&rom, &(int){-1}),
                              &(struct Val){.key = -1, .val = -1}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(count(&rom).count, 1);
    v = or_insert(entry_r(&rom, &(int){-1}),
                  &(struct Val){.key = -1, .val = -2}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(count(&rom).count, 1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    v = or_insert(entry_r(&rom, &i), &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    v = or_insert(entry_r(&rom, &i),
                  &(struct Val){.key = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    v = or_insert(entry_r(&rom, &i), &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    v = or_insert(entry_r(&rom, &i),
                  &(struct Val){.key = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_or_insert_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = realtime_ordered_map_or_insert_w(entry_r(&rom, &(int){-1}),
                                                     idval(-1, -1));
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(count(&rom).count, 1);
    v = realtime_ordered_map_or_insert_w(entry_r(&rom, &(int){-1}),
                                         idval(-1, -2));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(count(&rom).count, 1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    v = realtime_ordered_map_or_insert_w(entry_r(&rom, &i), idval(i, i));
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    v = realtime_ordered_map_or_insert_w(entry_r(&rom, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    v = realtime_ordered_map_or_insert_w(entry_r(&rom, &i), idval(i, i));
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    v = realtime_ordered_map_or_insert_w(entry_r(&rom, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_entry)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = insert_entry(entry_r(&rom, &(int){-1}),
                                 &(struct Val){.key = -1, .val = -1}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(count(&rom).count, 1);
    v = insert_entry(entry_r(&rom, &(int){-1}),
                     &(struct Val){.key = -1, .val = -2}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -2);
    CHECK(count(&rom).count, 1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    v = insert_entry(entry_r(&rom, &i), &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    v = insert_entry(entry_r(&rom, &i),
                     &(struct Val){.key = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(count(&rom).count, i + 2);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    v = insert_entry(entry_r(&rom, &i), &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    v = insert_entry(entry_r(&rom, &i),
                     &(struct Val){.key = i, .val = i + 1}.elem);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(count(&rom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_entry_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = realtime_ordered_map_insert_entry_w(
        entry_r(&rom, &(int){-1}), idval(-1, -1));
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(count(&rom).count, 1);
    v = realtime_ordered_map_insert_entry_w(entry_r(&rom, &(int){-1}),
                                            idval(-1, -2));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -2);
    CHECK(count(&rom).count, 1);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    v = realtime_ordered_map_insert_entry_w(entry_r(&rom, &i), idval(i, i));
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    v = realtime_ordered_map_insert_entry_w(entry_r(&rom, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(count(&rom).count, i + 2);
    ++i;

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    v = realtime_ordered_map_insert_entry_w(entry_r(&rom, &i), idval(i, i));
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 2);
    v = realtime_ordered_map_insert_entry_w(entry_r(&rom, &i), idval(i, i + 1));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(count(&rom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_remove_entry)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    CCC_Realtime_ordered_map rom = realtime_ordered_map_initialize(
        rom, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = or_insert(entry_r(&rom, &(int){-1}),
                              &(struct Val){.key = -1, .val = -1}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(count(&rom).count, 1);
    CCC_Entry *e = remove_entry_r(entry_r(&rom, &(int){-1}));
    CHECK(validate(&rom), true);
    CHECK(occupied(e), true);
    CHECK(count(&rom).count, 0);
    int i = 0;

    CHECK(fill_n(&rom, size / 2, i), PASS);

    i += (size / 2);
    v = or_insert(entry_r(&rom, &i), &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 1);
    e = remove_entry_r(entry_r(&rom, &i));
    CHECK(validate(&rom), true);
    CHECK(occupied(e), true);
    CHECK(count(&rom).count, i);

    CHECK(fill_n(&rom, size - i, i), PASS);

    i = size;
    v = or_insert(entry_r(&rom, &i), &(struct Val){.key = i, .val = i}.elem);
    CHECK(validate(&rom), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(count(&rom).count, i + 1);
    e = remove_entry_r(entry_r(&rom, &i));
    CHECK(validate(&rom), true);
    CHECK(occupied(e), true);
    CHECK(count(&rom).count, i);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(
        romap_test_insert(), romap_test_remove(), romap_test_validate(),
        romap_test_try_insert(), romap_test_try_insert_with(),
        romap_test_insert_or_assign(), romap_test_insert_or_assign_with(),
        romap_test_entry_and_modify(), romap_test_entry_and_modify_context(),
        romap_test_entry_and_modify_with(), romap_test_or_insert(),
        romap_test_or_insert_with(), romap_test_insert_entry(),
        romap_test_insert_entry_with(), romap_test_remove_entry());
}
