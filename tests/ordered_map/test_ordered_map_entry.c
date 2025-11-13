/** This file dedicated to testing the Entry Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the entry functions. */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "ordered_map.h"
#include "ordered_map_utility.h"
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
check_static_begin(fill_n, Ordered_map *const om, size_t const n,
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
check_static_begin(ordered_map_test_validate)
{
    struct Val_pool vals
        = {.vals = (struct Val[3]){}, .next_free = 0, .capacity = 3};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
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

check_static_begin(ordered_map_test_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
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

check_static_begin(ordered_map_test_remove)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
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

check_static_begin(ordered_map_test_try_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
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

check_static_begin(ordered_map_test_try_insert_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry *ent = ordered_map_try_insert_w(&om, -1, val(-1));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, 1);
    ent = ordered_map_try_insert_w(&om, -1, val(-1));
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
    ent = ordered_map_try_insert_w(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = ordered_map_try_insert_w(&om, i, val(i));
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
    ent = ordered_map_try_insert_w(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = ordered_map_try_insert_w(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(ordered_map_test_insert_or_assign)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
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

check_static_begin(ordered_map_test_insert_or_assign_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    CCC_Entry *ent = ordered_map_insert_or_assign_w(&om, -1, val(-1));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, 1);
    ent = ordered_map_insert_or_assign_w(&om, -1, val(-2));
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
    ent = ordered_map_insert_or_assign_w(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = ordered_map_insert_or_assign_w(&om, i, val(i + 1));
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = ordered_map_insert_or_assign_w(&om, i, val(i));
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&om).count, i + 2);
    ent = ordered_map_insert_or_assign_w(&om, i, val(i + 1));
    check(validate(&om), true);
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check_end();
}

check_static_begin(ordered_map_test_entry_and_modify)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    CCC_Ordered_map_entry *ent = entry_r(&om, &(int){-1});
    check(validate(&om), true);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, 0);
    ent = and_modify(ent, plus);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, 0);
    (void)ordered_map_insert_or_assign_w(&om, -1, val(-1));
    check(validate(&om), true);
    ent = entry_r(&om, &(int){-1});
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
    ent = entry_r(&om, &i);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)ordered_map_insert_or_assign_w(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_r(&om, &i);
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
    ent = entry_r(&om, &i);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)ordered_map_insert_or_assign_w(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_r(&om, &i);
    check(occupied(ent), true);
    check(count(&om).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check_end();
}

check_static_begin(ordered_map_test_entry_and_modify_context)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    int context = 1;
    CCC_Ordered_map_entry *ent = entry_r(&om, &(int){-1});
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, 0);
    (void)ordered_map_insert_or_assign_w(&om, -1, val(-1));
    check(validate(&om), true);
    ent = entry_r(&om, &(int){-1});
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
    ent = entry_r(&om, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)ordered_map_insert_or_assign_w(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_r(&om, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = entry_r(&om, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)ordered_map_insert_or_assign_w(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_r(&om, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(ordered_map_test_entry_and_modify_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    CCC_Ordered_map_entry *ent = entry_r(&om, &(int){-1});
    ent = ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    check(count(&om).count, 0);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, 0);
    (void)ordered_map_insert_or_assign_w(&om, -1, val(-1));
    check(validate(&om), true);
    ent = entry_r(&om, &(int){-1});
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    ent = ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, 0);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = entry_r(&om, &i);
    ent = ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)ordered_map_insert_or_assign_w(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_r(&om, &i);
    ent = ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    ent = entry_r(&om, &i);
    ent = ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&om).count, i + 1);
    (void)ordered_map_insert_or_assign_w(&om, i, val(i));
    check(validate(&om), true);
    ent = entry_r(&om, &i);
    ent = ordered_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(ordered_map_test_or_insert)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = or_insert(entry_r(&om, &(int){-1}),
                              &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    v = or_insert(entry_r(&om, &(int){-1}),
                  &(struct Val){.key = -1, .val = -2}.elem);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = or_insert(entry_r(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = or_insert(entry_r(&om, &i), &(struct Val){.key = i, .val = i + 1}.elem);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = or_insert(entry_r(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = or_insert(entry_r(&om, &i), &(struct Val){.key = i, .val = i + 1}.elem);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(ordered_map_test_or_insert_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    struct Val *v
        = ordered_map_or_insert_w(entry_r(&om, &(int){-1}), idval(-1, -1));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    v = ordered_map_or_insert_w(entry_r(&om, &(int){-1}), idval(-1, -2));
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = ordered_map_or_insert_w(entry_r(&om, &i), idval(i, i));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = ordered_map_or_insert_w(entry_r(&om, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = ordered_map_or_insert_w(entry_r(&om, &i), idval(i, i));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = ordered_map_or_insert_w(entry_r(&om, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(ordered_map_test_insert_entry)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = insert_entry(entry_r(&om, &(int){-1}),
                                 &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    v = insert_entry(entry_r(&om, &(int){-1}),
                     &(struct Val){.key = -1, .val = -2}.elem);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -2);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = insert_entry(entry_r(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = insert_entry(entry_r(&om, &i),
                     &(struct Val){.key = i, .val = i + 1}.elem);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = insert_entry(entry_r(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = insert_entry(entry_r(&om, &i),
                     &(struct Val){.key = i, .val = i + 1}.elem);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(ordered_map_test_insert_entry_with)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    struct Val *v
        = ordered_map_insert_entry_w(entry_r(&om, &(int){-1}), idval(-1, -1));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    v = ordered_map_insert_entry_w(entry_r(&om, &(int){-1}), idval(-1, -2));
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -2);
    check(count(&om).count, 1);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = ordered_map_insert_entry_w(entry_r(&om, &i), idval(i, i));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = ordered_map_insert_entry_w(entry_r(&om, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&om).count, i + 2);
    ++i;

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = ordered_map_insert_entry_w(entry_r(&om, &i), idval(i, i));
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 2);
    v = ordered_map_insert_entry_w(entry_r(&om, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&om).count, i + 2);
    check_end();
}

check_static_begin(ordered_map_test_remove_entry)
{
    struct Val_pool vals
        = {.vals = (struct Val[35]){}, .next_free = 0, .capacity = 35};
    Ordered_map om = ordered_map_initialize(om, struct Val, elem, key, id_order,
                                            val_bump_allocate, &vals);
    int size = 30;
    struct Val *v = or_insert(entry_r(&om, &(int){-1}),
                              &(struct Val){.key = -1, .val = -1}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&om).count, 1);
    CCC_Entry *e = remove_entry_r(entry_r(&om, &(int){-1}));
    check(validate(&om), true);
    check(occupied(e), true);
    check(count(&om).count, 0);
    int i = 0;

    check(fill_n(&om, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = or_insert(entry_r(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 1);
    e = remove_entry_r(entry_r(&om, &i));
    check(validate(&om), true);
    check(occupied(e), true);
    check(count(&om).count, i);

    check(fill_n(&om, size - i, i), CHECK_PASS);

    i = size;
    v = or_insert(entry_r(&om, &i), &(struct Val){.key = i, .val = i}.elem);
    check(validate(&om), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&om).count, i + 1);
    e = remove_entry_r(entry_r(&om, &i));
    check(validate(&om), true);
    check(occupied(e), true);
    check(count(&om).count, i);
    check_end();
}

int
main(void)
{
    return check_run(
        ordered_map_test_insert(), ordered_map_test_remove(),
        ordered_map_test_validate(), ordered_map_test_try_insert(),
        ordered_map_test_try_insert_with(), ordered_map_test_insert_or_assign(),
        ordered_map_test_insert_or_assign_with(),
        ordered_map_test_entry_and_modify(),
        ordered_map_test_entry_and_modify_context(),
        ordered_map_test_entry_and_modify_with(), ordered_map_test_or_insert(),
        ordered_map_test_or_insert_with(), ordered_map_test_insert_entry(),
        ordered_map_test_insert_entry_with(), ordered_map_test_remove_entry());
}
