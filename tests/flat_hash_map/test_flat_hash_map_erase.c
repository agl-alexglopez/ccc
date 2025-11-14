#include <stddef.h>
#include <stdlib.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_hash_map.h"
#include "flat_hash_map_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"
#include "utility/random.h"

check_static_begin(flat_hash_map_test_erase)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val query = {.key = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    CCC_Entry ent = swap_entry(&fh, &query);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, 1);
    ent = CCC_remove(&fh, &query);
    check(occupied(&ent), true);
    struct Val *v = unwrap(&ent);
    check(v != NULL, true);
    check(v->key, 137);
    check(v->val, 99);
    check(count(&fh).count, 0);
    query.key = 101;
    ent = CCC_remove(&fh, &query);
    check(occupied(&ent), false);
    check(count(&fh).count, 0);
    CCC_flat_hash_map_insert_entry_with(entry_wrap(&fh, &(int){137}),
                                        (struct Val){.key = 137, .val = 99});
    check(count(&fh).count, 1);
    check(occupied(remove_entry_wrap(entry_wrap(&fh, &(int){137}))), true);
    check(count(&fh).count, 0);
    check_end();
}

check_static_begin(flat_hash_map_test_shuffle_insert_erase)
{
    CCC_Flat_hash_map h = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    int const to_insert = 100;
    int const larger_prime = 101;
    for (int i = 0, shuffle = larger_prime % to_insert; i < to_insert;
         ++i, shuffle = (shuffle + larger_prime) % to_insert)
    {
        struct Val *v = unwrap(flat_hash_map_insert_or_assign_with(
            &h, shuffle, (struct Val){.val = i}));
        check(v != NULL, true);
        check(v->key, shuffle);
        check(v->val, i);
        check(validate(&h), true);
    }
    check(count(&h).count, to_insert);
    size_t cur_size = count(&h).count;
    int i = 0;
    while (!is_empty(&h) && cur_size)
    {
        check(contains(&h, &i), true);
        if (i % 2)
        {
            struct Val const *const old_val
                = unwrap(remove_wrap(&h, &(struct Val){.key = i}));
            check(old_val != NULL, true);
            check(old_val->key, i);
        }
        else
        {
            CCC_Entry removed = remove_entry(entry_wrap(&h, &i));
            check(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        check(count(&h).count, cur_size);
        check(validate(&h), true);
    }
    check(count(&h).count, 0);
    check_end(flat_hash_map_clear_and_free(&h, NULL););
}

/* This test will force us to test our in place hashing algorithm. */
check_static_begin(flat_hash_map_test_shuffle_erase_fixed)
{
    CCC_Flat_hash_map h = flat_hash_map_initialize(
        &(standard_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);
    int to_insert[STANDARD_FIXED_CAP];
    iota(to_insert, STANDARD_FIXED_CAP, 0);
    rand_shuffle(sizeof(int), to_insert, STANDARD_FIXED_CAP, &(int){0});
    int i = 0;
    do
    {
        int const cur = to_insert[i];
        struct Val const *const v = unwrap(flat_hash_map_insert_or_assign_with(
            &h, cur, (struct Val){.val = i}));
        if (!v)
        {
            break;
        }
        check(v->key, to_insert[i]);
        check(v->val, i);
        check(validate(&h), true);
        ++i;
    }
    while (1);
    size_t const full_size = count(&h).count;
    size_t cur_size = count(&h).count;
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        check(check, true);
        CCC_Entry removed = remove_entry(entry_wrap(&h, &cur));
        check(occupied(&removed), true);
        check(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Entry const *const e = flat_hash_map_insert_or_assign_with(
            &h, cur, (struct Val){.val = i});
        check(occupied(e), false);
        check(validate(&h), true);
    }
    check(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        check(check, true);
        if (i % 2)
        {
            struct Val const *const old_val
                = unwrap(remove_wrap(&h, &(struct Val){.key = cur}));
            check(old_val != NULL, true);
            check(old_val->key, to_insert[i]);
        }
        else
        {
            CCC_Entry removed = remove_entry(entry_wrap(&h, &cur));
            check(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        check(count(&h).count, cur_size);
        check(validate(&h), true);
    }
    check(count(&h).count, 0);
    check_end();
}

check_static_begin(flat_hash_map_test_shuffle_erase_reserved)
{
    /* The map will be given dynamically reserved space but no ability to
       resize. All algorithms should function normally and in place rehashing
       should take effect. */
    CCC_Flat_hash_map h = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        NULL, NULL, 0);
    int const test_amount = 896;
    CCC_Result const res_check
        = CCC_flat_hash_map_reserve(&h, test_amount, std_allocate);
    check(res_check, CCC_RESULT_OK);

    /* Give ourselves plenty more to insert so we don't run out before cap. */
    int to_insert[1024];
    iota(to_insert, 1024, 0);
    rand_shuffle(sizeof(int), to_insert, 1024, &(int){0});
    int i = 0;
    do
    {
        int const cur = to_insert[i];
        struct Val const *const v = unwrap(flat_hash_map_insert_or_assign_with(
            &h, cur, (struct Val){.val = i}));
        if (!v)
        {
            break;
        }
        check(v->key, cur);
        check(v->val, i);
        check(validate(&h), true);
        ++i;
    }
    while (1);
    size_t const full_size = count(&h).count;
    size_t cur_size = count(&h).count;
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        check(check, true);
        CCC_Entry const removed = remove_entry(entry_wrap(&h, &cur));
        check(occupied(&removed), true);
        check(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Entry const *const e = flat_hash_map_insert_or_assign_with(
            &h, cur, (struct Val){.val = i});
        check(occupied(e), false);
        check(validate(&h), true);
    }
    check(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        check(check, true);
        if (i % 2)
        {
            struct Val const *const old_val
                = unwrap(remove_wrap(&h, &(struct Val){.key = cur}));
            check(old_val != NULL, true);
            check(old_val->key, to_insert[i]);
        }
        else
        {
            CCC_Entry removed = remove_entry(entry_wrap(&h, &cur));
            check(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        check(count(&h).count, cur_size);
        check(validate(&h), true);
    }
    check(count(&h).count, 0);
    check_end((void)CCC_flat_hash_map_clear_and_free_reserve(&h, NULL,
                                                             std_allocate););
}

check_static_begin(flat_hash_map_test_shuffle_erase_dynamic)
{
    CCC_Flat_hash_map h = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    int to_insert[1024];
    iota(to_insert, 1024, 0);
    rand_shuffle(sizeof(int), to_insert, 1024, &(int){0});
    int inserted = 0;
    for (; inserted < 1024; ++inserted)
    {
        int const cur = to_insert[inserted];
        struct Val const *const v = unwrap(flat_hash_map_insert_or_assign_with(
            &h, cur, (struct Val){.val = inserted}));
        check(v->key, to_insert[inserted]);
        check(v->val, inserted);
        check(validate(&h), true);
    }
    size_t const full_size = count(&h).count;
    size_t cur_size = count(&h).count;
    int i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        check(check, true);
        CCC_Entry removed = remove_entry(entry_wrap(&h, &cur));
        check(occupied(&removed), true);
        check(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Entry const *const e = flat_hash_map_insert_or_assign_with(
            &h, cur, (struct Val){.val = i});
        check(occupied(e), false);
        check(validate(&h), true);
    }
    check(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        check(check, true);
        if (i % 2)
        {
            struct Val const *const old_val
                = unwrap(remove_wrap(&h, &(struct Val){.key = cur}));
            check(old_val != NULL, true);
            check(old_val->key, to_insert[i]);
        }
        else
        {
            CCC_Entry removed = remove_entry(entry_wrap(&h, &cur));
            check(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        check(count(&h).count, cur_size);
        check(validate(&h), true);
    }
    check(count(&h).count, 0);
    check_end(flat_hash_map_clear_and_free(&h, NULL););
}

int
main()
{
    return check_run(flat_hash_map_test_erase(),
                     flat_hash_map_test_shuffle_insert_erase(),
                     flat_hash_map_test_shuffle_erase_fixed(),
                     flat_hash_map_test_shuffle_erase_reserved(),
                     flat_hash_map_test_shuffle_erase_dynamic());
}
