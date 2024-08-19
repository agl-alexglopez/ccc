#include "buf.h"
#include "fhash_util.h"
#include "flat_hash.h"
#include "test.h"

#include <stddef.h>
#include <stdint.h>

static enum test_result fhash_test_empty(void);
static enum test_result fhash_test_insert_one(void);

#define NUM_TESTS (size_t)2
test_fn const all_tests[NUM_TESTS] = {
    fhash_test_empty,
    fhash_test_insert_one,
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
fhash_test_empty(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_flat_hash fh;
    ccc_fhash_result const res = ccc_fhash_init(
        &fh, &buf, offsetof(struct val, e), fhash_val_zero, fhash_val_eq, NULL);
    CHECK(res, CCC_FHASH_OK, ccc_fhash_result, "%d");
    CHECK(ccc_fhash_empty(&fh), true, bool, "%d");
    return PASS;
}

static enum test_result
fhash_test_insert_one(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_flat_hash fh;
    ccc_fhash_result const res = ccc_fhash_init(
        &fh, &buf, offsetof(struct val, e), fhash_val_zero, fhash_val_eq, NULL);
    CHECK(res, CCC_FHASH_OK, ccc_fhash_result, "%d");
    CHECK(ccc_fhash_empty(&fh), true, bool, "%d");
    struct val v = {.id = 99, .val = 137};
    ccc_fhash_result const push_res = ccc_fhash_insert(&fh, &v);
    CHECK(push_res, CCC_FHASH_OK, ccc_fhash_result, "%d");
    CHECK(ccc_fhash_size(&fh), 1, size_t, "%zu");
    return PASS;
}
