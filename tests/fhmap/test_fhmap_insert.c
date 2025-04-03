#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "fhmap_util.h"
#include "flat_hash_map.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

CHECK_BEGIN_STATIC_FN(fhmap_test_insert)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_zero,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);

    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = swap_entry(&fh, &(struct val){.key = 137, .val = 99});
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&fh).count, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_macros)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_zero,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);

    struct val const *ins = ccc_fhm_or_insert_w(
        entry_r(&fh, &(int){2}), (struct val){.key = 2, .val = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&fh), true);
    CHECK(size(&fh).count, 1);
    ins = fhm_insert_entry_w(entry_r(&fh, &(int){2}),
                             (struct val){.key = 2, .val = 0});
    CHECK(validate(&fh), true);
    CHECK(ins != NULL, true);
    ins = fhm_insert_entry_w(entry_r(&fh, &(int){9}),
                             (struct val){.key = 9, .val = 1});
    CHECK(validate(&fh), true);
    CHECK(ins != NULL, true);
    ins = ccc_entry_unwrap(
        fhm_insert_or_assign_w(&fh, 3, (struct val){.val = 99}));
    CHECK(validate(&fh), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&fh), true);
    CHECK(ins->val, 99);
    CHECK(size(&fh).count, 3);
    ins = ccc_entry_unwrap(
        fhm_insert_or_assign_w(&fh, 3, (struct val){.val = 98}));
    CHECK(validate(&fh), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&fh).count, 3);
    ins = ccc_entry_unwrap(fhm_try_insert_w(&fh, 3, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&fh), true);
    CHECK(ins->val, 98);
    CHECK(size(&fh).count, 3);
    ins = ccc_entry_unwrap(fhm_try_insert_w(&fh, 4, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&fh), true);
    CHECK(ins->val, 100);
    CHECK(size(&fh).count, 4);
    CHECK_END_FN(ccc_fhm_clear_and_free(&fh, NULL););
}

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_overwrite)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_zero,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);

    struct val q = {.key = 137, .val = 99};
    ccc_entry ent = swap_entry(&fh, &q);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);

    struct val const *v = unwrap(entry_r(&fh, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_entry old_ent = swap_entry(&fh, &q);
    CHECK(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(q.val, 99);
    v = unwrap(entry_r(&fh, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_then_bad_ideas)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_zero,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);
    struct val q = {.key = 137, .val = 99};
    ccc_entry ent = swap_entry(&fh, &q);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    struct val const *v = unwrap(entry_r(&fh, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    q = (struct val){.key = 137, .val = 100};

    ent = swap_entry(&fh, &q);
    CHECK(occupied(&ent), true);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(q.val, 99);
    q.val -= 9;

    v = get_key_val(&fh, &q.key);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    standard_fixed_map s;
    ccc_flat_hash_map fh
        = fhm_init(s.data, s.tag, key, fhmap_int_last_digit, fhmap_id_eq, NULL,
                   NULL, STANDARD_FIXED_CAP);
    size_t const size = 200;

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d = or_insert(entry_r(&fh, &def.key), &def);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&fh).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d = or_insert(
            and_modify(entry_r(&fh, &def.key), fhmap_modplus), &def);
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        if (i % 2)
        {
            CHECK(d->val, i);
        }
        else
        {
            CHECK(d->val, i + 1);
        }
        CHECK(d->val % 2, true);
    }
    CHECK(size(&fh).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val *const in = or_insert(entry_r(&fh, &def.key), &def);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&fh).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    standard_fixed_map s;
    ccc_flat_hash_map fh
        = fhm_init(s.data, s.tag, key, fhmap_int_last_digit, fhmap_id_eq, NULL,
                   NULL, STANDARD_FIXED_CAP);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d = insert_entry(entry_r(&fh, &def.key), &def);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&fh).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i + 1;
        struct val const *const d = insert_entry(entry_r(&fh, &def.key), &def);
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->val, i + 1);
        if (i % 2)
        {
            CHECK(d->val % 2 == 0, true);
        }
        else
        {
            CHECK(d->val % 2, true);
        }
    }
    CHECK(size(&fh).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    standard_fixed_map s;
    ccc_flat_hash_map fh
        = fhm_init(s.data, s.tag, key, fhmap_int_last_digit, fhmap_id_eq, NULL,
                   NULL, STANDARD_FIXED_CAP);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = insert_entry(entry_r(&fh, &i), &(struct val){i, i});
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&fh).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = insert_entry(entry_r(&fh, &i), &(struct val){i, i + 1});
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->val, i + 1);
        if (i % 2)
        {
            CHECK(d->val % 2 == 0, true);
        }
        else
        {
            CHECK(d->val % 2, true);
        }
    }
    CHECK(size(&fh).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    standard_fixed_map s;
    ccc_flat_hash_map fh
        = fhm_init(s.data, s.tag, key, fhmap_int_last_digit, fhmap_id_eq, NULL,
                   NULL, STANDARD_FIXED_CAP);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d
            = fhm_or_insert_w(entry_r(&fh, &i), fhmap_create(i, i));
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&fh).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = fhm_or_insert_w(
            and_modify(entry_r(&fh, &i), fhmap_modplus), fhmap_create(i, i));
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        if (i % 2)
        {
            CHECK(d->val, i);
        }
        else
        {
            CHECK(d->val, i + 1);
        }
        CHECK(d->val % 2, true);
    }
    CHECK(size(&fh).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v = fhm_or_insert_w(entry_r(&fh, &i), (struct val){});
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(size(&fh).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_two_sum)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_to_u64,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (size_t)(sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct val const *const other_addend
            = get_key_val(&fh, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_entry const e
            = insert_or_assign(&fh, &(struct val){.key = addends[i], .val = i});
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_resize)
{
    ccc_flat_hash_map fh
        = fhm_init((struct val *)NULL, NULL, key, fhmap_int_to_u64, fhmap_id_eq,
                   std_alloc, NULL, 0);

    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&fh, &elem.key), &elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&fh), true);
    }
    CHECK(size(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index};
        struct val const *const in_table
            = insert_entry(entry_r(&fh, &swap_slot.key), &swap_slot);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(size(&fh).count, to_insert);
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_resize_macros)
{
    ccc_flat_hash_map fh
        = fhm_init((struct val *)NULL, NULL, key, fhmap_int_to_u64, fhmap_id_eq,
                   std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&fh, &shuffled_index),
                                     &(struct val){shuffled_index, i});
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = fhm_or_insert_w(
            fhm_and_modify_w(entry_r(&fh, &shuffled_index), struct val,
                             {
                                 T->val = shuffled_index;
                             }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = fhm_or_insert_w(entry_r(&fh, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&fh, &shuffled_index);
        CHECK(v != NULL, true);
        CHECK(v->val, i);
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_resize_from_null)
{
    ccc_flat_hash_map fh
        = fhm_init((struct val *)NULL, NULL, key, fhmap_int_to_u64, fhmap_id_eq,
                   std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&fh, &elem.key), &elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index};
        struct val const *const in_table
            = insert_entry(entry_r(&fh, &swap_slot.key), &swap_slot);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_resize_from_null_macros)
{
    ccc_flat_hash_map fh
        = fhm_init((struct val *)NULL, NULL, key, fhmap_int_to_u64, fhmap_id_eq,
                   std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&fh, &shuffled_index),
                                     &(struct val){shuffled_index, i});
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&fh).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = fhm_or_insert_w(
            fhm_and_modify_w(entry_r(&fh, &shuffled_index), struct val,
                             {
                                 T->val = shuffled_index;
                             }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = fhm_or_insert_w(entry_r(&fh, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&fh, &shuffled_index);
        CHECK(v == NULL, false);
        CHECK(v->val, i);
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_limit)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_to_u64,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);

    int const size = SMALL_FIXED_CAP;
    int const larger_prime = 1097;
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct val *v = insert_entry(entry_r(&fh, &shuffled_index),
                                     &(struct val){shuffled_index, i});
        if (!v)
        {
            break;
        }
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = size(&fh).count;
    /* The last successful entry is still in the table and is overwritten. */
    struct val v = {.key = last_index, .val = -1};
    ccc_entry ent = swap_entry(&fh, &v);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(insert_error(&ent), false);
    CHECK(size(&fh).count, final_size);

    v = (struct val){.key = last_index, .val = -2};
    struct val *in_table = insert_entry(entry_r(&fh, &v.key), &v);
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -2);
    CHECK(size(&fh).count, final_size);

    in_table = insert_entry(entry_r(&fh, &last_index),
                            &(struct val){.key = last_index, .val = -3});
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -3);
    CHECK(size(&fh).count, final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.key = shuffled_index, .val = -4};
    in_table = insert_entry(entry_r(&fh, &v.key), &v);
    CHECK(in_table == NULL, true);
    CHECK(size(&fh).count, final_size);

    in_table = insert_entry(entry_r(&fh, &shuffled_index),
                            &(struct val){.key = shuffled_index, .val = -4});
    CHECK(in_table == NULL, true);
    CHECK(size(&fh).count, final_size);

    ent = swap_entry(&fh, &v);
    CHECK(unwrap(&ent) == NULL, true);
    CHECK(insert_error(&ent), true);
    CHECK(size(&fh).count, final_size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_and_find)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_to_u64,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);

    int const size = SMALL_FIXED_CAP;

    for (int i = 0; i < size; i += 2)
    {
        ccc_entry e = try_insert(&fh, &(struct val){.key = i, .val = i});
        CHECK(occupied(&e), false);
        CHECK(validate(&fh), true);
        e = try_insert(&fh, &(struct val){.key = i, .val = i});
        CHECK(occupied(&e), true);
        CHECK(validate(&fh), true);
        struct val const *const v = unwrap(&e);
        CHECK(v == NULL, false);
        CHECK(v->key, i);
        CHECK(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&fh, &i), true);
        CHECK(occupied(entry_r(&fh, &i)), true);
        CHECK(validate(&fh), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&fh, &i), false);
        CHECK(occupied(entry_r(&fh, &i)), false);
        CHECK(validate(&fh), true);
    }
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        fhmap_test_insert(), fhmap_test_insert_macros(),
        fhmap_test_insert_and_find(), fhmap_test_insert_overwrite(),
        fhmap_test_insert_then_bad_ideas(), fhmap_test_insert_via_entry(),
        fhmap_test_insert_via_entry_macros(), fhmap_test_entry_api_functional(),
        fhmap_test_entry_api_macros(), fhmap_test_two_sum(),
        fhmap_test_resize(), fhmap_test_resize_macros(),
        fhmap_test_resize_from_null(), fhmap_test_resize_from_null_macros(),
        fhmap_test_insert_limit());
}
