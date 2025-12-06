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

check_static_begin(flat_hash_map_test_insert)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);

    /* Nothing was there before so nothing is in the entry. */
    CCC_Entry ent = swap_entry(&fh, &(struct Val){.key = 137, .val = 99});
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    check(count(&fh).count, 1);
    check_end();
}

check_static_begin(flat_hash_map_test_insert_macros)
{
    CCC_Flat_hash_map fh = CCC_flat_hash_map_initialize(
        &(Small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);

    struct Val const *ins = CCC_flat_hash_map_or_insert_with(
        entry_wrap(&fh, &(int){2}), (struct Val){.key = 2, .val = 0});
    check(ins != NULL, true);
    check(validate(&fh), true);
    check(count(&fh).count, 1);
    ins = flat_hash_map_insert_entry_with(entry_wrap(&fh, &(int){2}),
                                          (struct Val){.key = 2, .val = 0});
    check(validate(&fh), true);
    check(ins != NULL, true);
    ins = flat_hash_map_insert_entry_with(entry_wrap(&fh, &(int){9}),
                                          (struct Val){.key = 9, .val = 1});
    check(validate(&fh), true);
    check(ins != NULL, true);
    ins = CCC_entry_unwrap(
        flat_hash_map_insert_or_assign_with(&fh, 3, (struct Val){.val = 99}));
    check(validate(&fh), true);
    check(ins == NULL, false);
    check(validate(&fh), true);
    check(ins->val, 99);
    check(count(&fh).count, 3);
    ins = CCC_entry_unwrap(
        flat_hash_map_insert_or_assign_with(&fh, 3, (struct Val){.val = 98}));
    check(validate(&fh), true);
    check(ins == NULL, false);
    check(ins->val, 98);
    check(count(&fh).count, 3);
    ins = CCC_entry_unwrap(
        flat_hash_map_try_insert_with(&fh, 3, (struct Val){.val = 100}));
    check(ins == NULL, false);
    check(validate(&fh), true);
    check(ins->val, 98);
    check(count(&fh).count, 3);
    ins = CCC_entry_unwrap(
        flat_hash_map_try_insert_with(&fh, 4, (struct Val){.val = 100}));
    check(ins == NULL, false);
    check(validate(&fh), true);
    check(ins->val, 100);
    check(count(&fh).count, 4);
    check_end(clear_and_free(&fh, NULL););
}

check_static_begin(flat_hash_map_test_insert_overwrite)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);

    struct Val q = {.key = 137, .val = 99};
    CCC_Entry ent = swap_entry(&fh, &q);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);

    struct Val const *v = unwrap(entry_wrap(&fh, &q.key));
    check(v != NULL, true);
    check(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct Val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    CCC_Entry old_ent = swap_entry(&fh, &q);
    check(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    check(v != NULL, true);
    check(v->val, 99);
    check(q.val, 99);
    v = unwrap(entry_wrap(&fh, &q.key));
    check(v != NULL, true);
    check(v->val, 100);
    check_end();
}

check_static_begin(flat_hash_map_test_insert_then_bad_ideas)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val q = {.key = 137, .val = 99};
    CCC_Entry ent = swap_entry(&fh, &q);
    check(occupied(&ent), false);
    check(unwrap(&ent) != NULL, true);
    struct Val const *v = unwrap(entry_wrap(&fh, &q.key));
    check(v != NULL, true);
    check(v->val, 99);

    q = (struct Val){.key = 137, .val = 100};

    ent = swap_entry(&fh, &q);
    check(occupied(&ent), true);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, 99);
    check(q.val, 99);
    q.val -= 9;

    v = get_key_value(&fh, &q.key);
    check(v != NULL, true);
    check(v->val, 100);
    check(q.val, 90);
    check_end();
}

check_static_begin(flat_hash_map_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Standard_fixed_map){}, struct Val, key, flat_hash_map_int_last_digit,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);
    size_t const size = 200;

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct Val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d = or_insert(entry_wrap(&fh, &def.key), &def);
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&fh).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d = or_insert(
            and_modify(entry_wrap(&fh, &def.key), flat_hash_map_modplus), &def);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->key, i);
        if (i % 2)
        {
            check(d->val, i);
        }
        else
        {
            check(d->val, i + 1);
        }
        check(d->val % 2, true);
    }
    check(count(&fh).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val *const in = or_insert(entry_wrap(&fh, &def.key), &def);
        in->val++;
        /* All values in the array should be odd now */
        check((in->val % 2 == 0), true);
    }
    check(count(&fh).count, (size / 2));
    check_end();
}

check_static_begin(flat_hash_map_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Standard_fixed_map){}, struct Val, key, flat_hash_map_int_last_digit,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct Val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d
            = insert_entry(entry_wrap(&fh, &def.key), &def);
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&fh).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i + 1;
        struct Val const *const d
            = insert_entry(entry_wrap(&fh, &def.key), &def);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->val, i + 1);
        if (i % 2)
        {
            check(d->val % 2 == 0, true);
        }
        else
        {
            check(d->val % 2, true);
        }
    }
    check(count(&fh).count, (size / 2));
    check_end();
}

check_static_begin(flat_hash_map_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Standard_fixed_map){}, struct Val, key, flat_hash_map_int_last_digit,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct Val const *const d
            = insert_entry(entry_wrap(&fh, &i), &(struct Val){i, i});
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&fh).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct Val const *const d
            = insert_entry(entry_wrap(&fh, &i), &(struct Val){i, i + 1});
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->val, i + 1);
        if (i % 2)
        {
            check(d->val % 2 == 0, true);
        }
        else
        {
            check(d->val % 2, true);
        }
    }
    check(count(&fh).count, (size / 2));
    check_end();
}

check_static_begin(flat_hash_map_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Standard_fixed_map){}, struct Val, key, flat_hash_map_int_last_digit,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct Val const *const d = flat_hash_map_or_insert_with(
            entry_wrap(&fh, &i), flat_hash_map_create(i, i));
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&fh).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val const *const d = flat_hash_map_or_insert_with(
            and_modify(entry_wrap(&fh, &i), flat_hash_map_modplus),
            flat_hash_map_create(i, i));
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->key, i);
        if (i % 2)
        {
            check(d->val, i);
        }
        else
        {
            check(d->val, i + 1);
        }
        check(d->val % 2, true);
    }
    check(count(&fh).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val *v
            = flat_hash_map_or_insert_with(entry_wrap(&fh, &i), (struct Val){});
        check(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        check(v->val % 2 == 0, true);
    }
    check(count(&fh).count, (size / 2));
    check_end();
}

check_static_begin(flat_hash_map_test_two_sum)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (size_t)(sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct Val const *const other_addend
            = get_key_value(&fh, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        CCC_Entry const e
            = insert_or_assign(&fh, &(struct Val){.key = addends[i], .val = i});
        check(insert_error(&e), false);
    }
    check(solution_indices[0], 8);
    check(solution_indices[1], 2);
    check_end();
}

check_static_begin(flat_hash_map_test_longest_consecutive_sequence)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Standard_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);
    /* Longest sequence is 1,2,3,4,5,6,7,8,9,10 of length 10. */
    int const nums[] = {
        99, 54, 1, 4, 9,  2, 3,   4,  8,  271, 32, 45, 86, 44, 7,  777, 6,  20,
        19, 5,  9, 1, 10, 4, 101, 15, 16, 17,  18, 19, 20, 10, 21, 22,  23,
    };
    check(sizeof(nums) / sizeof(nums[0]) < STANDARD_FIXED_CAP / 2, CCC_TRUE);
    int const correct_max_run = 10;
    size_t const nums_size = sizeof(nums) / sizeof(nums[0]);
    int max_run = 0;
    for (size_t i = 0; i < nums_size; ++i)
    {
        int const n = nums[i];
        CCC_Entry const *const seen_n
            = try_insert_wrap(&fh, &(struct Val){.key = n, .val = 1});
        /* We have already connected this run as much as possible. */
        if (occupied(seen_n))
        {
            continue;
        }

        /* There may or may not be runs already existing to left and right. */
        struct Val const *const connect_left
            = get_key_value(&fh, &(int){n - 1});
        struct Val const *const connect_right
            = get_key_value(&fh, &(int){n + 1});
        int const left_run = connect_left ? connect_left->val : 0;
        int const right_run = connect_right ? connect_right->val : 0;
        int const full_run = left_run + 1 + right_run;

        /* Track solution to problem. */
        max_run = full_run > max_run ? full_run : max_run;

        /* Update the boundaries of the full run range. */
        ((struct Val *)unwrap(seen_n))->val = full_run;
        CCC_Entry const *const run_min = insert_or_assign_wrap(
            &fh, &(struct Val){.key = n - left_run, .val = full_run});
        CCC_Entry const *const run_max = insert_or_assign_wrap(
            &fh, &(struct Val){.key = n + right_run, .val = full_run});

        /* Validate for testing purposes. */
        check(occupied(run_min), CCC_TRUE);
        check(insert_error(run_min), CCC_FALSE);
        check(occupied(run_max), CCC_TRUE);
        check(insert_error(run_max), CCC_FALSE);
    }
    check(max_run, correct_max_run);
    check_end();
}

check_static_begin(flat_hash_map_test_resize)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.key = shuffled_index, .val = i};
        struct Val *v = insert_entry(entry_wrap(&fh, &elem.key), &elem);
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
        check(validate(&fh), true);
    }
    check(count(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index};
        struct Val const *const in_table
            = insert_entry(entry_wrap(&fh, &swap_slot.key), &swap_slot);
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
    }
    check(count(&fh).count, to_insert);
    check(flat_hash_map_clear_and_free(&fh, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(flat_hash_map_test_resize_macros)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val *v = insert_entry(entry_wrap(&fh, &shuffled_index),
                                     &(struct Val){shuffled_index, i});
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
    }
    check(count(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val const *const in_table = flat_hash_map_or_insert_with(
            flat_hash_map_and_modify_with(entry_wrap(&fh, &shuffled_index),
                                          struct Val,
                                          {
                                              T->val = shuffled_index;
                                          }),
            (struct Val){});
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
        struct Val *v = flat_hash_map_or_insert_with(
            entry_wrap(&fh, &shuffled_index), (struct Val){});
        check(v == NULL, false);
        v->val = i;
        v = get_key_value(&fh, &shuffled_index);
        check(v != NULL, true);
        check(v->val, i);
    }
    check(flat_hash_map_clear_and_free(&fh, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(flat_hash_map_test_resize_from_null)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.key = shuffled_index, .val = i};
        struct Val *v = insert_entry(entry_wrap(&fh, &elem.key), &elem);
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
    }
    check(count(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index};
        struct Val const *const in_table
            = insert_entry(entry_wrap(&fh, &swap_slot.key), &swap_slot);
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
    }
    check(flat_hash_map_clear_and_free(&fh, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(flat_hash_map_test_resize_from_null_macros)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val *v = insert_entry(entry_wrap(&fh, &shuffled_index),
                                     &(struct Val){shuffled_index, i});
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
    }
    check(count(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val const *const in_table = flat_hash_map_or_insert_with(
            flat_hash_map_and_modify_with(entry_wrap(&fh, &shuffled_index),
                                          struct Val,
                                          {
                                              T->val = shuffled_index;
                                          }),
            (struct Val){});
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
        struct Val *v = flat_hash_map_or_insert_with(
            entry_wrap(&fh, &shuffled_index), (struct Val){});
        check(v == NULL, false);
        v->val = i;
        v = get_key_value(&fh, &shuffled_index);
        check(v == NULL, false);
        check(v->val, i);
    }
    check(flat_hash_map_clear_and_free(&fh, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(flat_hash_map_test_insert_limit)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);

    int const size = SMALL_FIXED_CAP;
    int const larger_prime = 1097;
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct Val *v = insert_entry(entry_wrap(&fh, &shuffled_index),
                                     &(struct Val){shuffled_index, i});
        if (!v)
        {
            break;
        }
        check(v->key, shuffled_index);
        check(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = count(&fh).count;
    /* The last successful entry is still in the table and is overwritten. */
    struct Val v = {.key = last_index, .val = -1};
    CCC_Entry ent = swap_entry(&fh, &v);
    check(unwrap(&ent) != NULL, true);
    check(insert_error(&ent), false);
    check(count(&fh).count, final_size);

    v = (struct Val){.key = last_index, .val = -2};
    struct Val *in_table = insert_entry(entry_wrap(&fh, &v.key), &v);
    check(in_table != NULL, true);
    check(in_table->val, -2);
    check(count(&fh).count, final_size);

    in_table = insert_entry(entry_wrap(&fh, &last_index),
                            &(struct Val){.key = last_index, .val = -3});
    check(in_table != NULL, true);
    check(in_table->val, -3);
    check(count(&fh).count, final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct Val){.key = shuffled_index, .val = -4};
    in_table = insert_entry(entry_wrap(&fh, &v.key), &v);
    check(in_table == NULL, true);
    check(count(&fh).count, final_size);

    in_table = insert_entry(entry_wrap(&fh, &shuffled_index),
                            &(struct Val){.key = shuffled_index, .val = -4});
    check(in_table == NULL, true);
    check(count(&fh).count, final_size);

    ent = swap_entry(&fh, &v);
    check(unwrap(&ent) == NULL, true);
    check(insert_error(&ent), true);
    check(count(&fh).count, final_size);
    check_end();
}

check_static_begin(flat_hash_map_test_insert_and_find)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    int const size = SMALL_FIXED_CAP;

    for (int i = 0; i < size; i += 2)
    {
        CCC_Entry e = try_insert(&fh, &(struct Val){.key = i, .val = i});
        check(occupied(&e), false);
        check(validate(&fh), true);
        e = try_insert(&fh, &(struct Val){.key = i, .val = i});
        check(occupied(&e), true);
        check(validate(&fh), true);
        struct Val const *const v = unwrap(&e);
        check(v == NULL, false);
        check(v->key, i);
        check(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        check(contains(&fh, &i), true);
        check(occupied(entry_wrap(&fh, &i)), true);
        check(validate(&fh), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        check(contains(&fh, &i), false);
        check(occupied(entry_wrap(&fh, &i)), false);
        check(validate(&fh), true);
    }
    check_end();
}

check_static_begin(flat_hash_map_test_reserve_without_permissions)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        NULL, NULL, 0);
    /* The map must insert all of the requested elements but has no permission
       to resize. This ensures the reserve function works as expected. */
    int const to_insert = 1000;
    int const larger_prime = 1009;
    CCC_Result const res
        = CCC_flat_hash_map_reserve(&fh, to_insert, std_allocate);
    check(res, CCC_RESULT_OK);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.key = shuffled_index, .val = i};
        struct Val *v = insert_entry(entry_wrap(&fh, &elem.key), &elem);
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
    }
    check(count(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        CCC_Tribool const check = contains(&fh, &shuffled_index);
        check(check, true);
    }
    check(count(&fh).count, to_insert);
    check_end(flat_hash_map_clear_and_free_reserve(&fh, NULL, std_allocate););
}

int
main(void)
{
    return check_run(
        flat_hash_map_test_insert(), flat_hash_map_test_insert_macros(),
        flat_hash_map_test_insert_and_find(),
        flat_hash_map_test_insert_overwrite(),
        flat_hash_map_test_insert_then_bad_ideas(),
        flat_hash_map_test_insert_via_entry(),
        flat_hash_map_test_insert_via_entry_macros(),
        flat_hash_map_test_entry_api_functional(),
        flat_hash_map_test_entry_api_macros(), flat_hash_map_test_two_sum(),
        flat_hash_map_test_longest_consecutive_sequence(),
        flat_hash_map_test_resize(), flat_hash_map_test_resize_macros(),
        flat_hash_map_test_resize_from_null(),
        flat_hash_map_test_resize_from_null_macros(),
        flat_hash_map_test_insert_limit(),
        flat_hash_map_test_reserve_without_permissions());
}
