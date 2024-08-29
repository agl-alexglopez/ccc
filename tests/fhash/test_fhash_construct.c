#include "fhash_util.h"
#include "flat_hash.h"
#include "test.h"

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

static int def(int *);
static void mod(ccc_update);
static void modw(ccc_update);
static int gen(int *);

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
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_functional(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
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
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");
    CHECK(ccc_fh_get(FH_ENT(&fh, 137)) == NULL, true, "%d");
    int const key = 137;
    int mut = 99;
    /* The function with a side effect should execute. */
    struct val *inserted = FH_OR_INS(FH_ENT(&fh, key),
                                     (struct val){.id = key, .val = def(&mut)});
    CHECK(inserted != NULL, true, "%d");
    CHECK(mut, 100, "%d");
    CHECK(inserted->val, 0, "%d");
    /* The function with a side effect should NOT execute. */
    FH_OR_INS(FH_ENT(&fh, key), (struct val){.id = key, .val = def(&mut)})
        ->val++;
    CHECK(mut, 100, "%d");
    CHECK(inserted->val, 1, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_and_modify_functional(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");
    struct val def = {.id = 137, .val = 0};

    /* Returning a vacant entry is possible when modification is attemtped. */
    ccc_fhash_entry ent = ccc_fh_and_modify(ccc_fh_entry(&fh, &def.id), mod);
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
        ccc_fh_and_modify(ccc_fh_entry(&fh, &def.id), mod), &def.e);
    CHECK((v2 != NULL), true, "%d");
    CHECK(inserted->id, 137, "%d");
    CHECK(v2->val, 6, "%d");

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct val *v3 = ccc_fh_or_insert(
        ccc_fh_and_modify_with(ccc_fh_entry(&fh, &def.id), modw, &def.id),
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
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(ccc_fh_empty(&fh), true, "%d");

    /* Returning a vacant entry is possible when modification is attemtped. */
    ccc_fhash_entry ent = FH_AND_MODIFY(FH_ENT(&fh, 137), mod);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK((ccc_fh_get(ent) == NULL), true, "%d");

    int mut = 99;

    /* Inserting default value before an in place modification is possible. */
    struct val *v
        = FH_OR_INS(FH_AND_MODIFY_W(FH_ENT(&fh, 137), modw, gen(&mut)),
                    (struct val){.id = 137, .val = def(&mut)});
    CHECK((v != NULL), true, "%d");
    CHECK(v->id, 137, "%d");
    CHECK(v->val, 0, "%d");
    CHECK(mut, 100, "%d");

    /* Modifying an existing value or inserting default is possible when no
       auxilliary input is needed. */
    struct val *v2 = FH_OR_INS(FH_AND_MODIFY(FH_ENT(&fh, 137), mod),
                               (struct val){.id = 137, .val = def(&mut)});
    CHECK((v2 != NULL), true, "%d");
    CHECK(v2->id, 137, "%d");
    CHECK(v2->val, 5, "%d");
    CHECK(mut, 100, "%d");

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. Generate val also has
       lazy evaluation. */
    struct val *v3
        = FH_OR_INS(FH_AND_MODIFY_W(FH_ENT(&fh, 137), modw, gen(&mut)),
                    (struct val){.id = 137, .val = def(&mut)});
    CHECK((v3 != NULL), true, "%d");
    CHECK(v3->id, 137, "%d");
    CHECK(v3->val, 42, "%d");
    CHECK(mut, 0, "%d");
    return PASS;
}

static void
mod(ccc_update const u)
{
    struct val *v = u.container;
    v->val += 5;
}

static void
modw(ccc_update const u)
{
    struct val *v = u.container;
    v->val = *((int *)u.aux);
}

static int
def(int *to_affect)
{
    *to_affect += 1;
    return 0;
}

static int
gen(int *to_affect)
{
    *to_affect = 0;
    return 42;
}
