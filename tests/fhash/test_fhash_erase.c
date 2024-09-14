#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "fhash_util.h"
#include "flat_hash_map.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

static enum test_result fhash_test_erase(void);
static enum test_result fhash_test_shuffle_insert_erase(void);

#define NUM_TESTS (size_t)2
test_fn const all_tests[NUM_TESTS] = {
    fhash_test_erase,
    fhash_test_shuffle_insert_erase,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        bool const fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
fhash_test_erase(void)
{
    struct val vals[2] = {{0}, {0}};
    flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val query = {.id = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = insert(&fh, &query.e);
    CHECK(occupied(&ent), false, "%d");
    CHECK(unwrap(&ent), NULL, "%p");
    CHECK(size(&fh), 1, "%zu");
    ent = remove(&fh, &query.e);
    CHECK(occupied(&ent), true, "%d");
    struct val *v = unwrap(&ent);
    CHECK(v != NULL, true, "%d");
    CHECK(v->id, 137, "%d");
    CHECK(v->val, 99, "%d");
    CHECK(size(&fh), 0, "%zu");
    query.id = 101;
    ent = remove(&fh, &query.e);
    CHECK(occupied(&ent), false, "%d");
    CHECK(size(&fh), 0, "%zu");
    FHM_INSERT_ENTRY(FHM_ENTRY(&fh, 137), (struct val){.id = 137, .val = 99});
    CHECK(size(&fh), 1, "%zu");
    CHECK(occupied(remove_entry_vr(FHM_ENTRY(&fh, 137))), true, "%d");
    CHECK(size(&fh), 0, "%zu");
    return PASS;
}

static enum test_result
fhash_test_shuffle_insert_erase(void)
{
    flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, NULL, 0, struct val, id, e, realloc,
                                    fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const to_insert = 100;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v = insert_entry(fhm_entry_vr(&fh, &elem.id), &elem.e);
        CHECK(v != NULL, true, "%d");
        CHECK(v->id, shuffled_index, "%d");
        CHECK(v->val, i, "%d");
        CHECK(fhm_validate(&fh), true, "%d");
    }
    CHECK(size(&fh), to_insert, "%zu");
    size_t cur_size = size(&fh);
    int i = 0;
    while (!empty(&fh) && cur_size)
    {
        CHECK(fhm_contains(&fh, &i), true, "%d");
        if (i % 2)
        {
            struct val swap_slot = {.id = i};
            struct val const *const old_val
                = unwrap(remove_vr(&fh, &swap_slot.e));
            CHECK(old_val != NULL, true, "%d");
            CHECK(old_val->id, i, "%d");
        }
        else
        {
            ccc_entry removed = remove_entry(FHM_ENTRY(&fh, i));
            CHECK(occupied(&removed), true, "%d");
        }
        --cur_size;
        ++i;
        CHECK(size(&fh), cur_size, "%zu");
        CHECK(fhm_validate(&fh), true, "%d");
    }
    CHECK(size(&fh), 0, "%zu");
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK, "%d");
    return PASS;
}
