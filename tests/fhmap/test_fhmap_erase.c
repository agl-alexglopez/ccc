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

CHECK_BEGIN_STATIC_FN(fhmap_test_erase)
{
    small_fixed_map s;
    ccc_flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_zero,
                                    fhmap_id_eq, NULL, NULL, SMALL_FIXED_CAP);
    struct val query = {.key = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = swap_entry(&fh, &query);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
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

int
main()
{
    return CHECK_RUN(fhmap_test_erase(), fhmap_test_shuffle_insert_erase());
}
