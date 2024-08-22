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

static int default_val_with_side_effect(int *to_alter);

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
                                          fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fhash_empty(&fh), true, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_functional(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_flat_hash fh;
    ccc_result const res = ccc_fhash_init(&fh, &buf, offsetof(struct val, e),
                                          fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fhash_empty(&fh), true, "%d");
    struct val def = {.id = 137, .val = 0};
    ccc_flat_hash_entry ent = ccc_fhash_entry(&fh, &def.id);
    CHECK(ccc_fhash_get(ent) == NULL, true, "%d");
    ((struct val *)ccc_fhash_or_insert(ccc_fhash_entry(&fh, &def.id), &def.e))
        ->val
        += 1;
    struct val const *const inserted
        = ccc_fhash_get(ccc_fhash_entry(&fh, &def.id));
    CHECK(inserted != NULL, true, "%d");
    CHECK(inserted->val, 1, "%d");
    ((struct val *)ccc_fhash_or_insert(ccc_fhash_entry(&fh, &def.id), &def.e))
        ->val
        += 1;
    CHECK(inserted->val, 2, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_macros(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_flat_hash fh;
    ccc_result const res = ccc_fhash_init(&fh, &buf, offsetof(struct val, e),
                                          fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fhash_empty(&fh), true, "%d");
    CHECK(ccc_fhash_get(CCC_FHASH_ENTRY(&fh, 137)) == NULL, true, "%d");
    int to_affect = 99;
    /* The function with a side effect should execute. */
    struct val *inserted = CCC_FHASH_OR_INSERT_WITH(
        CCC_FHASH_ENTRY(&fh, 137),
        (struct val){
            .id = 137,
            .val = default_val_with_side_effect(&to_affect),
        });
    CHECK(inserted != NULL, true, "%d");
    inserted->val += 1;
    CHECK(to_affect, 100, "%d");
    CHECK(inserted->val, 1, "%d");
    /* The function with a side effect should NOT execute. */
    ((struct val *)CCC_FHASH_OR_INSERT_WITH(
         CCC_FHASH_ENTRY(&fh, 137),
         (struct val){
             .id = 137,
             .val = default_val_with_side_effect(&to_affect),
         }))
        ->val
        += 1;
    CHECK(to_affect, 100, "%d");
    CHECK(inserted->val, 2, "%d");
    return PASS;
}

static int
default_val_with_side_effect(int *to_alter)
{
    *to_alter += 1;
    return 0;
}
