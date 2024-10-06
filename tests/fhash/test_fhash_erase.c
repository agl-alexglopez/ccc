#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "fhash_util.h"
#include "flat_hash_map.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

BEGIN_STATIC_TEST(fhash_test_erase)
{
    struct val vals[10] = {};
    ccc_flat_hash_map fh;
    ccc_result const res
        = fhm_init(&fh, vals, sizeof(vals) / sizeof(vals[0]), id, e, NULL,
                   fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    struct val query = {.id = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = insert(&fh, &query.e, &(struct val){});
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fh), 1);
    ent = remove(&fh, &query.e);
    CHECK(occupied(&ent), true);
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->id, 137);
    CHECK(v->val, 99);
    CHECK(size(&fh), 0);
    query.id = 101;
    ent = remove(&fh, &query.e);
    CHECK(occupied(&ent), false);
    CHECK(size(&fh), 0);
    ccc_fhm_insert_entry_w(entry_vr(&fh, &(int){137}),
                           (struct val){.id = 137, .val = 99});
    CHECK(size(&fh), 1);
    CHECK(occupied(remove_entry_vr(entry_vr(&fh, &(int){137}))), true);
    CHECK(size(&fh), 0);
    END_TEST();
}

BEGIN_STATIC_TEST(fhash_test_shuffle_insert_erase)
{
    ccc_flat_hash_map h;
    ccc_result const res = fhm_init(&h, (struct val *)NULL, 0, id, e, realloc,
                                    fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    int const to_insert = 100;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffle = larger_prime % to_insert; i < to_insert;
         ++i, shuffle = (shuffle + larger_prime) % to_insert)
    {
        struct val *v = unwrap(
            fhm_insert_or_assign_w(&h, shuffle, (struct val){.val = i}));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffle);
        CHECK(v->val, i);
        CHECK(validate(&h), true);
    }
    CHECK(size(&h), to_insert);
    size_t cur_size = size(&h);
    int i = 0;
    while (!empty(&h) && cur_size)
    {
        CHECK(contains(&h, &i), true);
        if (i % 2)
        {
            struct val const *const old_val
                = unwrap(remove_vr(&h, &(struct val){.id = i}.e));
            CHECK(old_val != NULL, true);
            CHECK(old_val->id, i);
        }
        else
        {
            ccc_entry removed = remove_entry(entry_vr(&h, &i));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(size(&h), cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(size(&h), 0);
    END_TEST(fhm_clear_and_free(&h, NULL););
}

int
main()
{
    return RUN_TESTS(fhash_test_erase(), fhash_test_shuffle_insert_erase());
}
