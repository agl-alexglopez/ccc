#include "buf.h"
#include "entry.h"
#include "fhash_util.h"
#include "flat_hash.h"
#include "test.h"

#include <stddef.h>

static enum test_result fhash_test_empty(void);
static enum test_result fhash_test_entry_macros(void);
static enum test_result fhash_test_entry_functional(void);
static enum test_result fhash_test_entry_and_modify_functional(void);
static enum test_result fhash_test_entry_and_modify_macros(void);

#define NUM_TESTS (size_t)5
test_fn const all_tests[NUM_TESTS] = {
    fhash_test_empty,
    fhash_test_entry_macros,
    fhash_test_entry_functional,
    fhash_test_entry_and_modify_functional,
    fhash_test_entry_and_modify_macros,
};

static int def_and_side_effect(int *);
static void modify(void *, void *);
static void modify_w(void *, void *);

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
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_functional(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");
    struct val def = {.id = 137, .val = 0};
    ccc_fhash_entry ent = ccc_fh_entry(&fh, &def.id);
    CHECK(ccc_fh_get(ent) == NULL, true, "%d");
    ((struct val *)ccc_fh_or_insert(ccc_fh_entry(&fh, &def.id), &def.e))->val
        += 1;
    struct val const *const inserted = ccc_fh_get(ccc_fh_entry(&fh, &def.id));
    CHECK((inserted != NULL), true, "%d");
    CHECK(inserted->val, 1, "%d");
    ((struct val *)ccc_fh_or_insert(ccc_fh_entry(&fh, &def.id), &def.e))->val
        += 1;
    CHECK(inserted->val, 2, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_macros(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");
    CHECK(ccc_fh_get(ENTRY(&fh, 137)) == NULL, true, "%d");
    int to_affect = 99;
    /* The function with a side effect should execute. */
    struct val *inserted = OR_INSERT_WITH(
        ENTRY(&fh, 137),
        (struct val){.id = 137, .val = def_and_side_effect(&to_affect)});
    CHECK(inserted != NULL, true, "%d");
    inserted->val += 1;
    CHECK(to_affect, 100, "%d");
    CHECK(inserted->val, 1, "%d");
    /* The function with a side effect should NOT execute. */
    ((struct val *)OR_INSERT_WITH(
         ENTRY(&fh, 137),
         (struct val){.id = 137, .val = def_and_side_effect(&to_affect)}))
        ->val++;
    CHECK(to_affect, 100, "%d");
    CHECK(inserted->val, 2, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_and_modify_functional(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");
    struct val def = {.id = 137, .val = 0};

    /* Returning a vacant entry is possible when modification is attemtped. */
    ccc_fhash_entry ent = ccc_fh_and_modify(ccc_fh_entry(&fh, &def.id), modify);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK((ccc_fh_get(ent) == NULL), true, "%d");

    /* Inserting default value before an in place modification is possible. */
    ((struct val *)ccc_fh_or_insert(ccc_fh_entry(&fh, &def.id), &def.e))->val
        += 1;
    struct val const *const inserted = ccc_fh_get(ccc_fh_entry(&fh, &def.id));
    CHECK((inserted != NULL), true, "%d");
    CHECK(inserted->id, 137, "%d");
    CHECK(inserted->val, 1, "%d");

    /* Modifying an existing value or inserting default is possible when no
       auxilliary input is needed. */
    struct val *v2 = ccc_fh_or_insert(
        ccc_fh_and_modify(ccc_fh_entry(&fh, &def.id), modify), &def.e);
    CHECK((v2 != NULL), true, "%d");
    CHECK(inserted->id, 137, "%d");
    CHECK(v2->val, 6, "%d");

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct val *v3 = ccc_fh_or_insert(
        ccc_fh_and_modify_with(ccc_fh_entry(&fh, &def.id), modify_w, &def.id),
        &def.e);
    CHECK((v3 != NULL), true, "%d");
    CHECK(inserted->id, 137, "%d");
    CHECK(v3->val, 137, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_and_modify_macros(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_buf buf = CCC_BUF_INIT(vals, struct val, 2, NULL);
    ccc_fhash fh;
    ccc_result const res = ccc_fh_init(&fh, &buf, offsetof(struct val, e),
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");

    /* Returning a vacant entry is possible when modification is attemtped. */
    ccc_fhash_entry ent = AND_MODIFY(ENTRY(&fh, 137), modify);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK((ccc_fh_get(ent) == NULL), true, "%d");

    int to_affect = 99;

    /* Inserting default value before an in place modification is possible. */
    struct val *v = OR_INSERT_WITH(
        ENTRY(&fh, 137),
        (struct val){.id = 137, .val = def_and_side_effect(&to_affect)});
    CHECK((v != NULL), true, "%d");
    CHECK(v->id, 137, "%d");
    CHECK(v->val, 0, "%d");
    CHECK(to_affect, 100, "%d");

    /* Modifying an existing value or inserting default is possible when no
       auxilliary input is needed. */
    struct val *v2 = OR_INSERT_WITH(
        AND_MODIFY(ENTRY(&fh, 137), modify),
        (struct val){.id = 137, .val = def_and_side_effect(&to_affect)});
    CHECK((v2 != NULL), true, "%d");
    CHECK(v2->id, 137, "%d");
    CHECK(v2->val, 5, "%d");
    CHECK(to_affect, 100, "%d");

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct val *v3 = OR_INSERT_WITH(
        AND_MODIFY_WITH(ENTRY(&fh, 137), modify_w, 137),
        (struct val){.id = 137, .val = def_and_side_effect(&to_affect)});
    CHECK((v3 != NULL), true, "%d");
    CHECK(v3->id, 137, "%d");
    CHECK(v3->val, 137, "%d");
    CHECK(to_affect, 100, "%d");
    return PASS;
}

static void
modify(void *struct_val, void *aux)
{
    (void)aux;
    struct val *v = struct_val;
    v->val += 5;
}

static void
modify_w(void *struct_val, void *aux)
{
    struct val *v = struct_val;
    v->val = *((int *)aux);
}

static int
def_and_side_effect(int *to_affect)
{
    *to_affect += 1;
    return 0;
}
