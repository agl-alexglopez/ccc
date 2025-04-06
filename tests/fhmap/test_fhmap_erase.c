#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "fhmap_util.h"
#include "flat_hash_map.h"
#include "random.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

CHECK_BEGIN_STATIC_FN(fhmap_test_erase)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_zero,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);
    struct val query = {.key = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = swap_entry(&fh, &query);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(size(&fh).count, 1);
    ent = ccc_remove(&fh, &query);
    CHECK(occupied(&ent), true);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->key, 137);
    CHECK(v->val, 99);
    CHECK(size(&fh).count, 0);
    query.key = 101;
    ent = ccc_remove(&fh, &query);
    CHECK(occupied(&ent), false);
    CHECK(size(&fh).count, 0);
    ccc_fhm_insert_entry_w(entry_r(&fh, &(int){137}),
                           (struct val){.key = 137, .val = 99});
    CHECK(size(&fh).count, 1);
    CHECK(occupied(remove_entry_r(entry_r(&fh, &(int){137}))), true);
    CHECK(size(&fh).count, 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_shuffle_insert_erase)
{
    ccc_flat_hash_map h
        = fhm_init((struct val *)NULL, NULL, key, fhmap_int_to_u64, fhmap_id_eq,
                   std_alloc, NULL, 0);

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
    CHECK(size(&h).count, to_insert);
    size_t cur_size = size(&h).count;
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
        CHECK(size(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(size(&h).count, 0);
    CHECK_END_FN(fhm_clear_and_free(&h, NULL););
}

CHECK_BEGIN_STATIC_FN(fhmap_test_shuffle_erase_fixed)
{
    standard_fixed_map mem;
    size_t const fix_cap = fhm_fixed_capacity(standard_fixed_map);
    ccc_flat_hash_map h = fhm_init(mem.data, mem.tag, key, fhmap_int_to_u64,
                                   fhmap_id_eq, NULL, NULL, fix_cap);
    int to_insert[fhm_fixed_capacity(standard_fixed_map)];
    iota(to_insert, fix_cap, 0);
    rand_shuffle(sizeof(int), to_insert, fix_cap, &(int){});
    int inserted = 0;
    for (; inserted < (int)fix_cap; ++inserted)
    {
        int const cur = to_insert[inserted];
        struct val const *const v = unwrap(
            fhm_insert_or_assign_w(&h, cur, (struct val){.val = inserted}));
        if (!v)
        {
            break;
        }
        CHECK(v->key, to_insert[inserted]);
        CHECK(v->val, inserted);
        CHECK(validate(&h), true);
    }
    size_t const full_size = size(&h).count;
    size_t cur_size = size(&h).count;
    int i = 0;
    for (; i < (int)(full_size / 3); ++i)
    {
        int const cur = to_insert[i];
        ccc_tribool const check = contains(&h, &cur);
        CHECK(check, true);
        ccc_entry removed = remove_entry(entry_r(&h, &cur));
        CHECK(occupied(&removed), true);
        CHECK(validate(&h), true);
    }
    i = 0;
    for (; i < (int)(full_size / 3); ++i)
    {
        int const cur = to_insert[i];
        struct val const *const v
            = unwrap(fhm_insert_or_assign_w(&h, cur, (struct val){.val = i}));
        CHECK(v != NULL, true);
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
        CHECK(size(&h).count, cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(size(&h).count, 0);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fhmap_test_erase(), fhmap_test_shuffle_insert_erase(),
                     fhmap_test_shuffle_erase_fixed());
}
