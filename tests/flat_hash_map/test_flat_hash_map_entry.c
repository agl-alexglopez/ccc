/** This file dedicated to testing the Entry Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the entry functions. */
#include <stddef.h>
#include <stdlib.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_hash_map.h"
#include "flat_hash_map_utility.h"
#include "traits.h"
#include "types.h"

static inline struct Val
val(int const val)
{
    return (struct Val){.val = val};
}

static inline struct Val
idval(int const key, int const val)
{
    return (struct Val){.key = key, .val = val};
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
check_static_begin(fill_n, CCC_Flat_hash_map *const fh, size_t const n,
                   int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        CCC_Entry ent = swap_entry(
            fh, &(struct Val){.key = id_and_val, .val = id_and_val});
        check(insert_error(&ent), false);
        check(occupied(&ent), false);
        check(validate(fh), true);
    }
    check_end();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
check_static_begin(flat_hash_map_test_validate)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);

    CCC_Entry ent = swap_entry(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, 1);
    ent = swap_entry(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, 1);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    check_end();
}

check_static_begin(flat_hash_map_test_insert)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Entry ent = swap_entry(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, 1);
    ent = swap_entry(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, 1);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = swap_entry(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = swap_entry(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = swap_entry(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = swap_entry(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(flat_hash_map_test_remove)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Entry ent = CCC_remove(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&fh).count, 0);
    ent = swap_entry(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, 1);
    ent = CCC_remove(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, 0);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = CCC_remove(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(count(&fh).count, i);
    ent = swap_entry(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, i + 1);
    ent = CCC_remove(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, i);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = CCC_remove(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(count(&fh).count, i);
    ent = swap_entry(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, i + 1);
    ent = CCC_remove(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, i);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(flat_hash_map_test_try_insert)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Entry ent = try_insert(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, 1);
    ent = try_insert(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, 1);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = try_insert(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = try_insert(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = try_insert(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = try_insert(&fh, &(struct Val){.key = i, .val = i});
    check(occupied(&ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(flat_hash_map_test_try_insert_with)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Entry *ent = flat_hash_map_try_insert_w(&fh, -1, val(-1));
    check(validate(&fh), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&fh).count, 1);
    ent = flat_hash_map_try_insert_w(&fh, -1, val(-1));
    check(validate(&fh), true);
    check(occupied(ent), true);
    check(count(&fh).count, 1);
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = flat_hash_map_try_insert_w(&fh, i, val(i));
    check(validate(&fh), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = flat_hash_map_try_insert_w(&fh, i, val(i));
    check(validate(&fh), true);
    check(occupied(ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = flat_hash_map_try_insert_w(&fh, i, val(i));
    check(validate(&fh), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = flat_hash_map_try_insert_w(&fh, i, val(i));
    check(validate(&fh), true);
    check(occupied(ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i);
    check(v->key, i);
    check_end();
}

check_static_begin(flat_hash_map_test_insert_or_assign)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Entry ent = insert_or_assign(&fh, &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, 1);
    ent = insert_or_assign(&fh, &(struct Val){.key = -1, .val = -2});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, 1);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, -2);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = insert_or_assign(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = insert_or_assign(&fh, &(struct Val){.key = i, .val = i + 1});
    check(occupied(&ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = insert_or_assign(&fh, &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = insert_or_assign(&fh, &(struct Val){.key = i, .val = i + 1});
    check(validate(&fh), true);
    check(occupied(&ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check_end();
}

check_static_begin(flat_hash_map_test_insert_or_assign_with)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Entry *ent = flat_hash_map_insert_or_assign_w(&fh, -1, val(-1));
    check(validate(&fh), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&fh).count, 1);
    ent = flat_hash_map_insert_or_assign_w(&fh, -1, val(0));
    check(validate(&fh), true);
    check(occupied(ent), true);
    check(count(&fh).count, 1);
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, 0);
    check(v->key, -1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = flat_hash_map_insert_or_assign_w(&fh, i, val(i));
    check(validate(&fh), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = flat_hash_map_insert_or_assign_w(&fh, i, val(i + 1));
    check(occupied(ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = flat_hash_map_insert_or_assign_w(&fh, i, val(i));
    check(validate(&fh), true);
    check(occupied(ent), false);
    check(unwrap(ent) != NULL, true);
    check(count(&fh).count, i + 2);
    ent = flat_hash_map_insert_or_assign_w(&fh, i, val(i + 1));
    check(validate(&fh), true);
    check(occupied(ent), true);
    check(count(&fh).count, i + 2);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check_end();
}

check_static_begin(flat_hash_map_test_entry_and_modify)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Flat_hash_map_entry *ent = entry_r(&fh, &(int){-1});
    check(validate(&fh), true);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, 0);
    ent = and_modify(ent, plus);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, 0);
    (void)flat_hash_map_insert_or_assign_w(&fh, -1, val(-1));
    check(validate(&fh), true);
    ent = entry_r(&fh, &(int){-1});
    check(occupied(ent), true);
    check(count(&fh).count, 1);
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

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = entry_r(&fh, &i);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, i + 1);
    (void)flat_hash_map_insert_or_assign_w(&fh, i, val(i));
    check(validate(&fh), true);
    ent = entry_r(&fh, &i);
    check(occupied(ent), true);
    check(count(&fh).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = entry_r(&fh, &i);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, i + 1);
    (void)flat_hash_map_insert_or_assign_w(&fh, i, val(i));
    check(validate(&fh), true);
    ent = entry_r(&fh, &i);
    check(occupied(ent), true);
    check(count(&fh).count, i + 2);
    ent = and_modify(ent, plus);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check_end();
}

check_static_begin(flat_hash_map_test_entry_and_modify_context)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    int context = 1;
    CCC_Flat_hash_map_entry *ent = entry_r(&fh, &(int){-1});
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, 0);
    (void)flat_hash_map_insert_or_assign_w(&fh, -1, val(-1));
    check(validate(&fh), true);
    ent = entry_r(&fh, &(int){-1});
    check(occupied(ent), true);
    check(count(&fh).count, 1);
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

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = entry_r(&fh, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, i + 1);
    (void)flat_hash_map_insert_or_assign_w(&fh, i, val(i));
    check(validate(&fh), true);
    ent = entry_r(&fh, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&fh).count, i + 2);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = entry_r(&fh, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, i + 1);
    (void)flat_hash_map_insert_or_assign_w(&fh, i, val(i));
    check(validate(&fh), true);
    ent = entry_r(&fh, &i);
    ent = and_modify_context(ent, pluscontext, &context);
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&fh).count, i + 2);
    check_end();
}

check_static_begin(flat_hash_map_test_entry_and_modify_with)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CCC_Flat_hash_map_entry *ent = entry_r(&fh, &(int){-1});
    ent = flat_hash_map_and_modify_w(ent, struct Val, { T->val++; });
    check(count(&fh).count, 0);
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, 0);
    (void)flat_hash_map_insert_or_assign_w(&fh, -1, val(-1));
    check(validate(&fh), true);
    ent = entry_r(&fh, &(int){-1});
    struct Val *v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, -1);
    check(v->key, -1);
    ent = flat_hash_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, 0);
    check(count(&fh).count, 1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    ent = entry_r(&fh, &i);
    ent = flat_hash_map_and_modify_w(ent, struct Val, { T->val++; });
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, i + 1);
    (void)flat_hash_map_insert_or_assign_w(&fh, i, val(i));
    check(validate(&fh), true);
    ent = entry_r(&fh, &i);
    ent = flat_hash_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&fh).count, i + 2);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    ent = entry_r(&fh, &i);
    ent = flat_hash_map_and_modify_w(ent, struct Val, { T->val++; });
    check(occupied(ent), false);
    check(unwrap(ent) == NULL, true);
    check(count(&fh).count, i + 1);
    (void)flat_hash_map_insert_or_assign_w(&fh, i, val(i));
    check(validate(&fh), true);
    ent = entry_r(&fh, &i);
    ent = flat_hash_map_and_modify_w(ent, struct Val, { T->val++; });
    v = unwrap(ent);
    check(v != NULL, true);
    check(v->val, i + 1);
    check(v->key, i);
    check(count(&fh).count, i + 2);
    check_end();
}

check_static_begin(flat_hash_map_test_or_insert)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val *v = or_insert(entry_r(&fh, &(int){-1}),
                              &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&fh).count, 1);
    v = or_insert(entry_r(&fh, &(int){-1}),
                  &(struct Val){.key = -1, .val = -2});
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&fh).count, 1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = or_insert(entry_r(&fh, &i), &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    v = or_insert(entry_r(&fh, &i), &(struct Val){.key = i, .val = i + 1});
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    v = or_insert(entry_r(&fh, &i), &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    v = or_insert(entry_r(&fh, &i), &(struct Val){.key = i, .val = i + 1});
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    check_end();
}

check_static_begin(flat_hash_map_test_or_insert_with)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val *v
        = flat_hash_map_or_insert_w(entry_r(&fh, &(int){-1}), idval(-1, -1));
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&fh).count, 1);
    v = flat_hash_map_or_insert_w(entry_r(&fh, &(int){-1}), idval(-1, -2));
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&fh).count, 1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = flat_hash_map_or_insert_w(entry_r(&fh, &i), idval(i, i));
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    v = flat_hash_map_or_insert_w(entry_r(&fh, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    v = flat_hash_map_or_insert_w(entry_r(&fh, &i), idval(i, i));
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    v = flat_hash_map_or_insert_w(entry_r(&fh, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    check_end();
}

check_static_begin(flat_hash_map_test_insert_entry)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val *v = insert_entry(entry_r(&fh, &(int){-1}),
                                 &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&fh).count, 1);
    v = insert_entry(entry_r(&fh, &(int){-1}),
                     &(struct Val){.key = -1, .val = -2});
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -2);
    check(count(&fh).count, 1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = insert_entry(entry_r(&fh, &i), &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    v = insert_entry(entry_r(&fh, &i), &(struct Val){.key = i, .val = i + 1});
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&fh).count, i + 2);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    v = insert_entry(entry_r(&fh, &i), &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    v = insert_entry(entry_r(&fh, &i), &(struct Val){.key = i, .val = i + 1});
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&fh).count, i + 2);
    check_end();
}

check_static_begin(flat_hash_map_test_insert_entry_with)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val *v
        = flat_hash_map_insert_entry_w(entry_r(&fh, &(int){-1}), idval(-1, -1));
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&fh).count, 1);
    v = flat_hash_map_insert_entry_w(entry_r(&fh, &(int){-1}), idval(-1, -2));
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -2);
    check(count(&fh).count, 1);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = flat_hash_map_insert_entry_w(entry_r(&fh, &i), idval(i, i));
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    v = flat_hash_map_insert_entry_w(entry_r(&fh, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&fh).count, i + 2);
    ++i;

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    v = flat_hash_map_insert_entry_w(entry_r(&fh, &i), idval(i, i));
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 2);
    v = flat_hash_map_insert_entry_w(entry_r(&fh, &i), idval(i, i + 1));
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i + 1);
    check(count(&fh).count, i + 2);
    check_end();
}

check_static_begin(flat_hash_map_test_remove_entry)
{
    int const size = 30;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val *v = or_insert(entry_r(&fh, &(int){-1}),
                              &(struct Val){.key = -1, .val = -1});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, -1);
    check(v->val, -1);
    check(count(&fh).count, 1);
    CCC_Entry *e = remove_entry_r(entry_r(&fh, &(int){-1}));
    check(validate(&fh), true);
    check(occupied(e), true);
    check(unwrap(e) == NULL, true);
    check(count(&fh).count, 0);
    int i = 0;

    check(fill_n(&fh, size / 2, i), CHECK_PASS);

    i += (size / 2);
    v = or_insert(entry_r(&fh, &i), &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 1);
    e = remove_entry_r(entry_r(&fh, &i));
    check(validate(&fh), true);
    check(occupied(e), true);
    check(unwrap(e) == NULL, true);
    check(count(&fh).count, i);

    check(fill_n(&fh, size - i, i), CHECK_PASS);

    i = size;
    v = or_insert(entry_r(&fh, &i), &(struct Val){.key = i, .val = i});
    check(validate(&fh), true);
    check(v != NULL, true);
    check(v->key, i);
    check(v->val, i);
    check(count(&fh).count, i + 1);
    e = remove_entry_r(entry_r(&fh, &i));
    check(validate(&fh), true);
    check(occupied(e), true);
    check(unwrap(e) == NULL, true);
    check(count(&fh).count, i);
    check_end();
}

int
main(void)
{
    return check_run(
        flat_hash_map_test_insert(), flat_hash_map_test_remove(),
        flat_hash_map_test_validate(), flat_hash_map_test_try_insert(),
        flat_hash_map_test_try_insert_with(),
        flat_hash_map_test_insert_or_assign(),
        flat_hash_map_test_insert_or_assign_with(),
        flat_hash_map_test_entry_and_modify(),
        flat_hash_map_test_entry_and_modify_context(),
        flat_hash_map_test_entry_and_modify_with(),
        flat_hash_map_test_or_insert(), flat_hash_map_test_or_insert_with(),
        flat_hash_map_test_insert_entry(),
        flat_hash_map_test_insert_entry_with(),
        flat_hash_map_test_remove_entry());
}
