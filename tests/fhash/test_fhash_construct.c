#include "buf.h"
#include "fhash_util.h"
#include "flat_hash.h"
#include "test.h"

#include <stddef.h>

static enum test_result fhash_test_empty(void);
static enum test_result fhash_test_entry_macros(void);
static enum test_result fhash_test_entry_functional(void);

#define NUM_TESTS (size_t)3
test_fn const all_tests[NUM_TESTS] = {
    fhash_test_empty,
    fhash_test_entry_macros,
    fhash_test_entry_functional,
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
    ccc_result const res = ccc_fhash_init(&fh, &buf, offsetof(struct val, e),
                                          fhash_val_zero, fhash_val_eq, NULL);
    CHECK(res, CCC_OK, ccc_result, "%d");
    CHECK(ccc_fhash_empty(&fh), true, bool, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_functional(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_flat_hash fh;
    ccc_result const res = ccc_fhash_init(&fh, &buf, offsetof(struct val, e),
                                          fhash_val_zero, fhash_val_eq, NULL);
    CHECK(res, CCC_OK, ccc_result, "%d");
    CHECK(ccc_fhash_empty(&fh), true, bool, "%d");
    struct val v = {.id = 99, .val = 137};
    ccc_flat_hash_entry ent = ccc_fhash_entry(&fh, &v.e);
    CHECK(ent.impl.entry.found, false, bool, "%d");
    void *inserted = ccc_fhash_or_insert(ccc_fhash_entry(&fh, &v.e), &v.e);
    CHECK(inserted != NULL, true, bool, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_macros(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_flat_hash fh;
    ccc_result const res = ccc_fhash_init(&fh, &buf, offsetof(struct val, e),
                                          fhash_val_zero, fhash_val_eq, NULL);
    CHECK(res, CCC_OK, ccc_result, "%d");
    CHECK(ccc_fhash_empty(&fh), true, bool, "%d");
    ccc_flat_hash_entry ent = CCC_FHASH_ENTRY(&fh, struct val, {.val = 137});
    CHECK(ent.impl.entry.found, false, bool, "%d");
    ccc_result inserted = CCC_FHASH_OR_INSERT(CCC_FHASH_ENTRY(&fh, struct val,
                                                              {
                                                                  .val = 137,
                                                              }),
                                              struct val,
                                              {
                                                  .id = 99,
                                                  .val = 137,
                                              });
    CHECK(inserted, CCC_OK, ccc_result, "%d");
    return PASS;
}
