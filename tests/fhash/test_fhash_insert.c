#include "buf.h"
#include "fhash_util.h"
#include "test.h"

static enum test_result fhash_test_insert(void);
static enum test_result fhash_test_insert_overwrite(void);
static enum test_result fhash_test_insert_then_bad_ideas(void);

#define NUM_TESTS (size_t)3
test_fn const all_tests[NUM_TESTS] = {
    fhash_test_insert,
    fhash_test_insert_overwrite,
    fhash_test_insert_then_bad_ideas,
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
fhash_test_insert(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val query = {.id = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_fhash_entry ent = ccc_fh_insert(&fh, &query.id, &query.e);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK(ccc_fh_get(ent), NULL, "%p");
    return PASS;
}

static enum test_result
fhash_test_insert_overwrite(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val q = {.id = 137, .val = 99};
    ccc_fhash_entry ent = ccc_fh_insert(&fh, &q.id, &q.e);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK(ccc_fh_get(ent), NULL, "%p");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 99, "%d");

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_fhash_entry old_ent = ccc_fh_insert(&fh, &q.id, &q.e);
    CHECK(ccc_fh_occupied(old_ent), true, "%d");

    /* The old contents are now in q and the entry is in the table. */
    CHECK(((struct val *)ccc_fh_get(old_ent))->val, 100, "%d");
    CHECK(q.val, 99, "%d");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 100, "%d");
    return PASS;
}

static enum test_result
fhash_test_insert_then_bad_ideas(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val q = {.id = 137, .val = 99};
    ccc_fhash_entry ent = ccc_fh_insert(&fh, &q.id, &q.e);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK(ccc_fh_get(ent), NULL, "%p");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 99, "%d");

    /* This is a dummy entry that indicates the entry was vacant in the table.
       so or insert and erase will do nothing. */
    CHECK(ccc_fh_or_insert(ent, &q.e), NULL, "%p");
    CHECK(ccc_fh_and_erase(ent, &q.e), NULL, "%p");

    q = (struct val){.id = 137, .val = 100};

    ccc_fhash_entry new_ent = ccc_fh_insert(&fh, &q.id, &q.e);
    CHECK(ccc_fh_occupied(new_ent), true, "%d");
    CHECK(((struct val *)ccc_fh_get(new_ent))->val, 100, "%d");
    CHECK(q.val, 99, "%d");
    q.val += 5;

    /* Now the expected behavior of or insert shall occur and no insertion
       will happen because the value is already occupied in the table. */
    CHECK(((struct val *)ccc_fh_or_insert(ent, &q.e))->val, 100, "%d");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 100, "%d");
    CHECK(q.val, 105, "%d");
    return PASS;
}
