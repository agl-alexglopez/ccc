#include "buf.h"
#include "fhash_util.h"
#include "test.h"

static enum test_result fhash_test_insert(void);
static enum test_result fhash_test_insert_overwrite(void);
static enum test_result fhash_test_insert_then_bad_ideas(void);
static enum test_result fhash_test_entry_api_functional(void);
static enum test_result fhash_test_entry_api_macros(void);

#define NUM_TESTS (size_t)5
test_fn const all_tests[NUM_TESTS] = {
    fhash_test_insert,
    fhash_test_insert_overwrite,
    fhash_test_insert_then_bad_ideas,
    fhash_test_entry_api_functional,
    fhash_test_entry_api_macros,
};

static void mod(ccc_update);
static struct val create_val(int id, int val);

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
    q.val -= 9;

    /* Now the expected behavior of or insert shall occur and no insertion
       will happen because the value is already occupied in the table. */
    CHECK(((struct val *)ccc_fh_or_insert(new_ent, &q.e))->val, 100, "%d");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 100, "%d");
    CHECK(q.val, 90, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_api_functional(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, size, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = ccc_fh_or_insert(ccc_fh_entry(&fh, &def.id), &def.e);
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d = ccc_fh_or_insert(
            ccc_fh_and_modify(ccc_fh_entry(&fh, &def.id), mod), &def.e);
        /* All values in the array should be odd now */
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        if (i % 2)
        {
            CHECK(d->val, i, "%d");
        }
        else
        {
            CHECK(d->val, i + 1, "%d");
        }
        CHECK(d->val % 2, true, "%d");
    }
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val *const in
            = ccc_fh_or_insert(ccc_fh_entry(&fh, &def.id), &def.e);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true, "%d");
    }
    return PASS;
}

static enum test_result
fhash_test_entry_api_macros(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    struct val vals[size];
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, size, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d
            = OR_INSERT_WITH(ENTRY(&fh, i), create_val(i, i));
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = OR_INSERT_WITH(
            AND_MODIFY(ENTRY(&fh, i), mod), (struct val){.id = i, .val = i});
        /* All values in the array should be odd now */
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        if (i % 2)
        {
            CHECK(d->val, i, "%d");
        }
        else
        {
            CHECK(d->val, i + 1, "%d");
        }
        CHECK(d->val % 2, true, "%d");
    }
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        OR_INSERT_WITH(ENTRY(&fh, i), (struct val){0})->val++;
        /* All values in the array should be odd now */
        CHECK(((struct val *)ccc_fh_get(ENTRY(&fh, i)))->val % 2 == 0, true,
              "%d");
    }
    return PASS;
}

static void
mod(ccc_update const mod)
{
    ((struct val *)mod.container)->val++;
}

static struct val
create_val(int id, int val)
{
    return (struct val){.id = id, .val = val};
}
