/** This file dedicated to testing the Entry Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the entry functions. */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define ADAPTIVE_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "adaptive_map.h"
#include "adaptive_map_utility.h"
#include "checkers.h"
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
check_static_begin(fill_n, Adaptive_map *const om, size_t const n,
                   int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        CCC_Entry ent = swap_entry(
            om, &(struct Val){.key = id_and_val, .val = id_and_val}.elem,
            &(struct Val){}.elem);
        check(insert_error(&ent), false);
        check(occupied(&ent), false);
        check(validate(om), true);
    }
    check_end();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
check_static_begin(adaptive_map_test_validate)
{
    struct Val_pool vals
        = {.vals = (struct Val[3]){}, .next_free = 0, .capacity = 3};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    CCC_Entry ent = swap_entry(&om, &(struct Val){.key = -1, .val = -1}.elem,
                               &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, 1);
    ent = swap_entry(&om, &(struct Val){.key = -1, .val = -1}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, 1);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    check_end();
}

check_static_begin(adaptive_map_test_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry ent = swap_entry(&om, &(struct Val){.key = -1, .val = -1}.elem,
                               &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, 1);
    ent = swap_entry(&om, &(struct Val){.key = -1, .val = -1}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, 1);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = swap_entry(&om, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, i + 2);
    ent = swap_entry(&om, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = swap_entry(&om, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, i + 2);
    ent = swap_entry(&om, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(adaptive_map_test_remove)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry ent = CCC_remove(&om, &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, 0);
    ent = swap_entry(&om, &(struct Val){.key = -1, .val = -1}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, 1);
    ent = CCC_remove(&om, &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, 0);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = CCC_remove(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(count(&om).count, i);
    ent = swap_entry(&om, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, i + 1);
    ent = CCC_remove(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, i);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = CCC_remove(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(count(&om).count, i);
    ent = swap_entry(&om, &(struct Val){.key = i, .val = i}.elem,
                     &(struct Val){}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, i + 1);
    ent = CCC_remove(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, i);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(adaptive_map_test_try_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry ent = try_insert(&om, &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&om).count, 1);
    ent = try_insert(&om, &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, 1);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = try_insert(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = try_insert(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = try_insert(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = try_insert(&om, &(struct Val){.key = i, .val = i}.elem);
    check(occupied(&ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(adaptive_map_test_try_insert_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry *ent = adaptive_map_try_insert_with(&om, -1, val(-1));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, 1);
    ent = adaptive_map_try_insert_with(&om, -1, val(-1));
    check(validate(&om), true);
    check(occupied(ent), true);
    check(count(&om).count, 1);
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = adaptive_map_try_insert_with(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = adaptive_map_try_insert_with(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = adaptive_map_try_insert_with(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = adaptive_map_try_insert_with(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(adaptive_map_test_insert_or_assign)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry ent
        = insert_or_assign(&om, &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&om).count, 1);
    ent = insert_or_assign(&om, &(struct Val){.key = -1, .val = -2}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, 1);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -2);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = insert_or_assign(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = insert_or_assign(&om, &(struct Val){.key = i, .val = i + 1}.elem);
    check(occupied(&ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = insert_or_assign(&om, &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = insert_or_assign(&om, &(struct Val){.key = i, .val = i + 1}.elem);
    check(validate(&om), true);
    check(occupied(&ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check_end();
}

check_static_begin(adaptive_map_test_insert_or_assign_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry *ent = adaptive_map_insert_or_assign_with(&om, -1, val(-1));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, 1);
    ent = adaptive_map_insert_or_assign_with(&om, -1, val(-2));
    check(validate(&om), true);
    check(occupied(ent), true);
    check(count(&om).count, 1);
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, -2);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = adaptive_map_insert_or_assign_with(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = adaptive_map_insert_or_assign_with(&om, i, val(i + 1));
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = adaptive_map_insert_or_assign_with(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = adaptive_map_insert_or_assign_with(&om, i, val(i + 1));
    check(validate(&om), true);
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check_end();
}

check_static_begin(adaptive_map_test_entry_and_modify)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Adaptive_map_entry *ent = entry_wrap(&om, &(int){-1});
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, 0);
    ent = and_modify(ent, plus);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, 0);
    (void)adaptive_map_insert_or_assign_with(&om, -1, val(-1));
    check(validate(&om), true);
    ent = entry_wrap(&om, &(int){-1});
    check(occupied(ent), true);
    check(count(&om).count, 1);
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, 0);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = entry_wrap(&om, &i);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)adaptive_map_insert_or_assign_with(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_wrap(&om, &i);
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = entry_wrap(&om, &i);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)adaptive_map_insert_or_assign_with(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_wrap(&om, &i);
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check_end();
}

check_static_begin(adaptive_map_test_entry_and_modify_context)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    int context = 1;
    CCC_Adaptive_map_entry *ent = entry_wrap(&om, &(int){-1});
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, 0);
    (void)adaptive_map_insert_or_assign_with(&om, -1, val(-1));
    check(validate(&om), true);
    ent = entry_wrap(&om, &(int){-1});
    check(occupied(ent), true);
    check(count(&om).count, 1);
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, 0);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = entry_wrap(&om, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)adaptive_map_insert_or_assign_with(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_wrap(&om, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = entry_wrap(&om, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)adaptive_map_insert_or_assign_with(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_wrap(&om, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(adaptive_map_test_entry_and_modify_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    CCC_Adaptive_map_entry *ent = entry_wrap(&om, &(int){-1});
    ent = adaptive_map_and_modify_with(ent, struct Val, { T->val++; });
    check(count(&om).count, 0);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, 0);
    (void)adaptive_map_insert_or_assign_with(&om, -1, val(-1));
    check(validate(&om), true);
    ent = entry_wrap(&om, &(int){-1});
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    ent = adaptive_map_and_modify_with(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, 0);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = entry_wrap(&om, &i);
    ent = adaptive_map_and_modify_with(ent, struct Val, { T->val++; });
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)adaptive_map_insert_or_assign_with(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_wrap(&om, &i);
    ent = adaptive_map_and_modify_with(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = entry_wrap(&om, &i);
    ent = adaptive_map_and_modify_with(ent, struct Val, { T->val++; });
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)adaptive_map_insert_or_assign_with(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_wrap(&om, &i);
    ent = adaptive_map_and_modify_with(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(adaptive_map_test_or_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = or_insert(entry_wrap(&om, &(int){-1}),
                              &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    v = or_insert(entry_wrap(&om, &(int){-1}),
                  &(struct Val){.key = -1, .val = -2}.elem);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = or_insert(entry_wrap(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = or_insert(entry_wrap(&om, &i),
                  &(struct Val){.key = i, .val = i + 1}.elem);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = or_insert(entry_wrap(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = or_insert(entry_wrap(&om, &i),
                  &(struct Val){.key = i, .val = i + 1}.elem);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(adaptive_map_test_or_insert_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = adaptive_map_or_insert_with(entry_wrap(&om, &(int){-1}),
                                                idval(-1, -1));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    v = adaptive_map_or_insert_with(entry_wrap(&om, &(int){-1}), idval(-1, -2));
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = adaptive_map_or_insert_with(entry_wrap(&om, &i), idval(i, i));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = adaptive_map_or_insert_with(entry_wrap(&om, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = adaptive_map_or_insert_with(entry_wrap(&om, &i), idval(i, i));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = adaptive_map_or_insert_with(entry_wrap(&om, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(adaptive_map_test_insert_entry)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = insert_entry(entry_wrap(&om, &(int){-1}),
                                 &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    v = insert_entry(entry_wrap(&om, &(int){-1}),
                     &(struct Val){.key = -1, .val = -2}.elem);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -2);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = insert_entry(entry_wrap(&om, &i),
                     &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = insert_entry(entry_wrap(&om, &i),
                     &(struct Val){.key = i, .val = i + 1}.elem);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = insert_entry(entry_wrap(&om, &i),
                     &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = insert_entry(entry_wrap(&om, &i),
                     &(struct Val){.key = i, .val = i + 1}.elem);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(adaptive_map_test_insert_entry_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = adaptive_map_insert_entry_with(entry_wrap(&om, &(int){-1}),
                                                   idval(-1, -1));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    v = adaptive_map_insert_entry_with(entry_wrap(&om, &(int){-1}),
                                       idval(-1, -2));
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -2);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = adaptive_map_insert_entry_with(entry_wrap(&om, &i), idval(i, i));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = adaptive_map_insert_entry_with(entry_wrap(&om, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = adaptive_map_insert_entry_with(entry_wrap(&om, &i), idval(i, i));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = adaptive_map_insert_entry_with(entry_wrap(&om, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(adaptive_map_test_remove_entry)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Adaptive_map om = adaptive_map_initialize(
        om, struct Val, elem, key, id_order, val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = or_insert(entry_wrap(&om, &(int){-1}),
                              &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    CCC_Entry *e = remove_entry_wrap(entry_wrap(&om, &(int){-1}));
    check(validate(&om), true);
    check(occupied(e), true);
    check(count(&om).count, 0);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = or_insert(entry_wrap(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 1);
    e = remove_entry_wrap(entry_wrap(&om, &i));
    check(validate(&om), true);
    check(occupied(e), true);
    check(count(&om).count, i);

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = or_insert(entry_wrap(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 1);
    e = remove_entry_wrap(entry_wrap(&om, &i));
    check(validate(&om), true);
    check(occupied(e), true);
    check(count(&om).count, i);
    check_end();
}

int
main(void)
{
    return check_run(
        adaptive_map_test_insert(), adaptive_map_test_remove(),
        adaptive_map_test_validate(), adaptive_map_test_try_insert(),
        adaptive_map_test_try_insert_with(),
        adaptive_map_test_insert_or_assign(),
        adaptive_map_test_insert_or_assign_with(),
        adaptive_map_test_entry_and_modify(),
        adaptive_map_test_entry_and_modify_context(),
        adaptive_map_test_entry_and_modify_with(),
        adaptive_map_test_or_insert(), adaptive_map_test_or_insert_with(),
        adaptive_map_test_insert_entry(), adaptive_map_test_insert_entry_with(),
        adaptive_map_test_remove_entry());
}
