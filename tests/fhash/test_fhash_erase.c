#include "fhash_util.h"
#include "flat_hash.h"
#include "test.h"
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
    ccc_flat_hash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val query = {.id = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_fhash_entry ent = ccc_fh_insert(&fh, &query.e);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK(ccc_fh_unwrap(ent), NULL, "%p");
    CHECK(ccc_fh_size(&fh), 1, "%zu");
    struct val *v = ccc_fh_remove(&fh, &query.e);
    CHECK(v != NULL, true, "%d");
    CHECK(v->id, 137, "%d");
    CHECK(v->val, 99, "%d");
    CHECK(ccc_fh_size(&fh), 0, "%zu");
    query.id = 101;
    v = ccc_fh_remove(&fh, &query.e);
    CHECK(v == NULL, true, "%d");
    CHECK(ccc_fh_size(&fh), 0, "%zu");
    FH_INSERT_ENTRY(FH_ENTRY(&fh, 137), (struct val){.id = 137, .val = 99});
    CHECK(ccc_fh_size(&fh), 1, "%zu");
    CHECK(ccc_fh_remove_entry(FH_ENTRY(&fh, 137)), true, "%d");
    CHECK(ccc_fh_size(&fh), 0, "%zu");
    return PASS;
}

static enum test_result
fhash_test_shuffle_insert_erase(void)
{
    ccc_flat_hash fh;
    ccc_result const res = CCC_FH_INIT(&fh, NULL, 0, struct val, id, e, realloc,
                                       fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const to_insert = 100;
    int const larger_prime = (int)ccc_fh_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = ccc_fh_insert_entry(ccc_fh_entry(&fh, &elem.id), &elem.e);
        CHECK(v->id, shuffled_index, "%d");
        CHECK(v->val, i, "%d");
        CHECK(ccc_fh_validate(&fh), true, "%d");
    }
    CHECK(ccc_fh_size(&fh), to_insert, "%zu");
    size_t cur_size = ccc_fh_size(&fh);
    int i = 0;
    while (!ccc_fh_empty(&fh) && cur_size)
    {
        CHECK(ccc_fh_contains(&fh, &i), true, "%d");
        if (i % 2)
        {
            struct val swap_slot = {.id = i};
            struct val const *const old_val = ccc_fh_remove(&fh, &swap_slot.e);
            CHECK(old_val != NULL, true, "%d");
            CHECK(old_val->id, i, "%d");
        }
        else
        {
            bool const removed = ccc_fh_remove_entry(FH_ENTRY(&fh, i));
            CHECK(removed, true, "%d");
        }
        --cur_size;
        ++i;
        CHECK(ccc_fh_size(&fh), cur_size, "%zu");
        CHECK(ccc_fh_validate(&fh), true, "%d");
    }
    CHECK(ccc_fh_size(&fh), 0, "%zu");
    CHECK(ccc_fh_clear_and_free(&fh, NULL), CCC_OK, "%d");
    return PASS;
}
