#include <stddef.h>
#include <stdlib.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_hash_map.h"
#include "flat_hash_map_util.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"
#include "util/random.h"

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_erase)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val query = {.key = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    CCC_Entry ent = swap_entry(&fh, &query);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(count(&fh).count, 1);
    ent = CCC_remove(&fh, &query);
    CHECK(occupied(&ent), true);
    struct Val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->key, 137);
    CHECK(v->val, 99);
    CHECK(count(&fh).count, 0);
    query.key = 101;
    ent = CCC_remove(&fh, &query);
    CHECK(occupied(&ent), false);
    CHECK(count(&fh).count, 0);
    CCC_flat_hash_map_insert_entry_w(entry_r(&fh, &(int){137}),
                                     (struct Val){.key = 137, .val = 99});
    CHECK(count(&fh).count, 1);
    CHECK(occupied(remove_entry_r(entry_r(&fh, &(int){137}))), true);
    CHECK(count(&fh).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_shuffle_insert_erase)
{
    CCC_Flat_hash_map h = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    int const to_insert = 100;
    int const larger_prime = 101;
    for (int i = 0, shuffle = larger_prime % to_insert; i < to_insert;
         ++i, shuffle = (shuffle + larger_prime) % to_insert)
    {
        struct Val *v = unwrap(flat_hash_map_insert_or_assign_w(
            &h, shuffle, (struct Val){.val = i}));
        CHECK(v != NULL, true);
        CHECK(v->key, shuffle);
        CHECK(v->val, i);
        CHECK(validate(&h), true);
    }
    CHECK(count(&h).count, to_insert);
    size_t cur_size = count(&h).count;
    int i = 0;
    while (!is_empty(&h) && cur_size)
    {
        CHECK(contains(&h, &i), true);
        if (i % 2)
        {
            struct Val const *const old_val
                = unwrap(remove_r(&h, &(struct Val){.key = i}));
            CHECK(old_val != NULL, true);
            CHECK(old_val->key, i);
        }
        else
        {
            CCC_Entry removed = remove_entry(entry_r(&h, &i));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(count(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(count(&h).count, 0);
    CHECK_END_FN(flat_hash_map_clear_and_free(&h, NULL););
}

/* This test will force us to test our in place hashing algorithm. */
CHECK_BEGIN_STATIC_FN(flat_hash_map_test_shuffle_erase_fixed)
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
        struct Val const *const v = unwrap(
            flat_hash_map_insert_or_assign_w(&h, cur, (struct Val){.val = i}));
        if (!v)
        {
            break;
        }
        CHECK(v->key, to_insert[i]);
        CHECK(v->val, i);
        CHECK(validate(&h), true);
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
        CHECK(check, true);
        CCC_Entry removed = remove_entry(entry_r(&h, &cur));
        CHECK(occupied(&removed), true);
        CHECK(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Entry const *const e
            = flat_hash_map_insert_or_assign_w(&h, cur, (struct Val){.val = i});
        CHECK(occupied(e), false);
        CHECK(validate(&h), true);
    }
    CHECK(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        CHECK(check, true);
        if (i % 2)
        {
            struct Val const *const old_val
                = unwrap(remove_r(&h, &(struct Val){.key = cur}));
            CHECK(old_val != NULL, true);
            CHECK(old_val->key, to_insert[i]);
        }
        else
        {
            CCC_Entry removed = remove_entry(entry_r(&h, &cur));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(count(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(count(&h).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_shuffle_erase_reserved)
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
    CHECK(res_check, CCC_RESULT_OK);

    /* Give ourselves plenty more to insert so we don't run out before cap. */
    int to_insert[1024];
    iota(to_insert, 1024, 0);
    rand_shuffle(sizeof(int), to_insert, 1024, &(int){0});
    int i = 0;
    do
    {
        int const cur = to_insert[i];
        struct Val const *const v = unwrap(
            flat_hash_map_insert_or_assign_w(&h, cur, (struct Val){.val = i}));
        if (!v)
        {
            break;
        }
        CHECK(v->key, cur);
        CHECK(v->val, i);
        CHECK(validate(&h), true);
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
        CHECK(check, true);
        CCC_Entry const removed = remove_entry(entry_r(&h, &cur));
        CHECK(occupied(&removed), true);
        CHECK(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Entry const *const e
            = flat_hash_map_insert_or_assign_w(&h, cur, (struct Val){.val = i});
        CHECK(occupied(e), false);
        CHECK(validate(&h), true);
    }
    CHECK(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        CHECK(check, true);
        if (i % 2)
        {
            struct Val const *const old_val
                = unwrap(remove_r(&h, &(struct Val){.key = cur}));
            CHECK(old_val != NULL, true);
            CHECK(old_val->key, to_insert[i]);
        }
        else
        {
            CCC_Entry removed = remove_entry(entry_r(&h, &cur));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(count(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(count(&h).count, 0);
    CHECK_END_FN((void)CCC_flat_hash_map_clear_and_free_reserve(&h, NULL,
                                                                std_allocate););
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_shuffle_erase_dynamic)
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
        struct Val const *const v = unwrap(flat_hash_map_insert_or_assign_w(
            &h, cur, (struct Val){.val = inserted}));
        CHECK(v->key, to_insert[inserted]);
        CHECK(v->val, inserted);
        CHECK(validate(&h), true);
    }
    size_t const full_size = count(&h).count;
    size_t cur_size = count(&h).count;
    int i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        CHECK(check, true);
        CCC_Entry removed = remove_entry(entry_r(&h, &cur));
        CHECK(occupied(&removed), true);
        CHECK(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        CCC_Entry const *const e
            = flat_hash_map_insert_or_assign_w(&h, cur, (struct Val){.val = i});
        CHECK(occupied(e), false);
        CHECK(validate(&h), true);
    }
    CHECK(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        CCC_Tribool const check = contains(&h, &cur);
        CHECK(check, true);
        if (i % 2)
        {
            struct Val const *const old_val
                = unwrap(remove_r(&h, &(struct Val){.key = cur}));
            CHECK(old_val != NULL, true);
            CHECK(old_val->key, to_insert[i]);
        }
        else
        {
            CCC_Entry removed = remove_entry(entry_r(&h, &cur));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(count(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(count(&h).count, 0);
    CHECK_END_FN(flat_hash_map_clear_and_free(&h, NULL););
}

int
main()
{
    return CHECK_RUN(flat_hash_map_test_erase(),
                     flat_hash_map_test_shuffle_insert_erase(),
                     flat_hash_map_test_shuffle_erase_fixed(),
                     flat_hash_map_test_shuffle_erase_reserved(),
                     flat_hash_map_test_shuffle_erase_dynamic());
}
