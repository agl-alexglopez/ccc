#include <stddef.h>
#include <stdlib.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "fhmap_util.h"
#include "flat_hash_map.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"
#include "util/random.h"

CHECK_BEGIN_STATIC_FN(fhmap_test_erase)
{
    ccc_flat_hash_map fh
        = fhm_init(&(small_fixed_map){}, struct val, key, fhmap_int_zero,
                   fhmap_id_cmp, NULL, NULL, SMALL_FIXED_CAP);
    struct val query = {.key = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = swap_entry(&fh, &query);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(count(&fh).count, 1);
    ent = ccc_remove(&fh, &query);
    CHECK(occupied(&ent), true);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->key, 137);
    CHECK(v->val, 99);
    CHECK(count(&fh).count, 0);
    query.key = 101;
    ent = ccc_remove(&fh, &query);
    CHECK(occupied(&ent), false);
    CHECK(count(&fh).count, 0);
    ccc_fhm_insert_entry_w(entry_r(&fh, &(int){137}),
                           (struct val){.key = 137, .val = 99});
    CHECK(count(&fh).count, 1);
    CHECK(occupied(remove_entry_r(entry_r(&fh, &(int){137}))), true);
    CHECK(count(&fh).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_shuffle_insert_erase)
{
    ccc_flat_hash_map h = fhm_init(NULL, struct val, key, fhmap_int_to_u64,
                                   fhmap_id_cmp, std_alloc, NULL, 0);
    int const to_insert = 100;
    int const larger_prime = 101;
    for (int i = 0, shuffle = larger_prime % to_insert; i < to_insert;
         ++i, shuffle = (shuffle + larger_prime) % to_insert)
    {
        struct val *v = unwrap(
            fhm_insert_or_assign_w(&h, shuffle, (struct val){.val = i}));
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
            struct val const *const old_val
                = unwrap(remove_r(&h, &(struct val){.key = i}));
            CHECK(old_val != NULL, true);
            CHECK(old_val->key, i);
        }
        else
        {
            ccc_entry removed = remove_entry(entry_r(&h, &i));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(count(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(count(&h).count, 0);
    CHECK_END_FN(fhm_clear_and_free(&h, NULL););
}

/* This test will force us to test our in place hashing algorithm. */
CHECK_BEGIN_STATIC_FN(fhmap_test_shuffle_erase_fixed)
{
    ccc_flat_hash_map h
        = fhm_init(&(standard_fixed_map){}, struct val, key, fhmap_int_to_u64,
                   fhmap_id_cmp, NULL, NULL, STANDARD_FIXED_CAP);
    int to_insert[STANDARD_FIXED_CAP];
    iota(to_insert, STANDARD_FIXED_CAP, 0);
    rand_shuffle(sizeof(int), to_insert, STANDARD_FIXED_CAP, &(int){0});
    int i = 0;
    do
    {
        int const cur = to_insert[i];
        struct val const *const v
            = unwrap(fhm_insert_or_assign_w(&h, cur, (struct val){.val = i}));
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
        ccc_tribool const check = contains(&h, &cur);
        CHECK(check, true);
        ccc_entry removed = remove_entry(entry_r(&h, &cur));
        CHECK(occupied(&removed), true);
        CHECK(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        ccc_entry const *const e
            = fhm_insert_or_assign_w(&h, cur, (struct val){.val = i});
        CHECK(occupied(e), false);
        CHECK(validate(&h), true);
    }
    CHECK(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        ccc_tribool const check = contains(&h, &cur);
        CHECK(check, true);
        if (i % 2)
        {
            struct val const *const old_val
                = unwrap(remove_r(&h, &(struct val){.key = cur}));
            CHECK(old_val != NULL, true);
            CHECK(old_val->key, to_insert[i]);
        }
        else
        {
            ccc_entry removed = remove_entry(entry_r(&h, &cur));
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

CHECK_BEGIN_STATIC_FN(fhmap_test_shuffle_erase_reserved)
{
    /* The map will be given dynamically reserved space but no ability to
       resize. All algorithms should function normally and in place rehashing
       should take effect. */
    ccc_flat_hash_map h = fhm_init(NULL, struct val, key, fhmap_int_to_u64,
                                   fhmap_id_cmp, NULL, NULL, 0);
    int const test_amount = 896;
    ccc_result const res_check = ccc_fhm_reserve(&h, test_amount, std_alloc);
    CHECK(res_check, CCC_RESULT_OK);

    /* Give ourselves plenty more to insert so we don't run out before cap. */
    int to_insert[1024];
    iota(to_insert, 1024, 0);
    rand_shuffle(sizeof(int), to_insert, 1024, &(int){0});
    int i = 0;
    do
    {
        int const cur = to_insert[i];
        struct val const *const v
            = unwrap(fhm_insert_or_assign_w(&h, cur, (struct val){.val = i}));
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
        ccc_tribool const check = contains(&h, &cur);
        CHECK(check, true);
        ccc_entry const removed = remove_entry(entry_r(&h, &cur));
        CHECK(occupied(&removed), true);
        CHECK(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        ccc_entry const *const e
            = fhm_insert_or_assign_w(&h, cur, (struct val){.val = i});
        CHECK(occupied(e), false);
        CHECK(validate(&h), true);
    }
    CHECK(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        ccc_tribool const check = contains(&h, &cur);
        CHECK(check, true);
        if (i % 2)
        {
            struct val const *const old_val
                = unwrap(remove_r(&h, &(struct val){.key = cur}));
            CHECK(old_val != NULL, true);
            CHECK(old_val->key, to_insert[i]);
        }
        else
        {
            ccc_entry removed = remove_entry(entry_r(&h, &cur));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(count(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(count(&h).count, 0);
    CHECK_END_FN((void)ccc_fhm_clear_and_free_reserve(&h, NULL, std_alloc););
}

CHECK_BEGIN_STATIC_FN(fhmap_test_shuffle_erase_dynamic)
{
    ccc_flat_hash_map h = fhm_init(NULL, struct val, key, fhmap_int_to_u64,
                                   fhmap_id_cmp, std_alloc, NULL, 0);
    int to_insert[1024];
    iota(to_insert, 1024, 0);
    rand_shuffle(sizeof(int), to_insert, 1024, &(int){0});
    int inserted = 0;
    for (; inserted < 1024; ++inserted)
    {
        int const cur = to_insert[inserted];
        struct val const *const v = unwrap(
            fhm_insert_or_assign_w(&h, cur, (struct val){.val = inserted}));
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
        ccc_tribool const check = contains(&h, &cur);
        CHECK(check, true);
        ccc_entry removed = remove_entry(entry_r(&h, &cur));
        CHECK(occupied(&removed), true);
        CHECK(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 2); ++i)
    {
        int const cur = to_insert[i];
        ccc_entry const *const e
            = fhm_insert_or_assign_w(&h, cur, (struct val){.val = i});
        CHECK(occupied(e), false);
        CHECK(validate(&h), true);
    }
    CHECK(full_size, cur_size);
    i = 0;
    while (!is_empty(&h) && cur_size)
    {
        int const cur = to_insert[i];
        ccc_tribool const check = contains(&h, &cur);
        CHECK(check, true);
        if (i % 2)
        {
            struct val const *const old_val
                = unwrap(remove_r(&h, &(struct val){.key = cur}));
            CHECK(old_val != NULL, true);
            CHECK(old_val->key, to_insert[i]);
        }
        else
        {
            ccc_entry removed = remove_entry(entry_r(&h, &cur));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(count(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(count(&h).count, 0);
    CHECK_END_FN(fhm_clear_and_free(&h, NULL););
}

int
main()
{
    return CHECK_RUN(fhmap_test_erase(), fhmap_test_shuffle_insert_erase(),
                     fhmap_test_shuffle_erase_fixed(),
                     fhmap_test_shuffle_erase_reserved(),
                     fhmap_test_shuffle_erase_dynamic());
}
