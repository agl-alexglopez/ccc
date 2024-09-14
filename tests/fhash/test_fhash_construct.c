#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "fhash_util.h"
#include "flat_hash_map.h"
#include "test.h"
#include "traits.h"

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
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(fhm_empty(&fh), true, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_functional(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(fhm_empty(&fh), true, "%d");
    struct val def = {.id = 137, .val = 0};
    ccc_fh_map_entry ent = entry(&fh, &def.id);
    CHECK(fhm_unwrap(&ent) == NULL, true, "%d");
    struct val *v = or_insert(entry_vr(&fh, &def.id), &def.e);
    CHECK(v != NULL, true, "%d");
    v->val += 1;
    struct val const *const inserted = get_key_val(&fh, &def.id);
    CHECK((inserted != NULL), true, "%d");
    CHECK(inserted->val, 1, "%d");
    v = or_insert(entry_vr(&fh, &def.id), &def.e);
    CHECK(v != NULL, true, "%d");
    v->val += 1;
    CHECK(inserted->val, 2, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_macros(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(fhm_empty(&fh), true, "%d");
    CHECK(FHM_GET_KEY_VAL(&fh, 137) == NULL, true, "%d");
    int const key = 137;
    int mut = 99;
    /* The function with a side effect should execute. */
    struct val *inserted = FHM_OR_INSERT(
        FHM_ENTRY(&fh, key), (struct val){.id = key, .val = def(&mut)});
    CHECK(inserted != NULL, true, "%d");
    CHECK(mut, 100, "%d");
    CHECK(inserted->val, 0, "%d");
    /* The function with a side effect should NOT execute. */
    struct val *v = FHM_OR_INSERT(FHM_ENTRY(&fh, key),
                                  (struct val){.id = key, .val = def(&mut)});
    CHECK(v != NULL, true, "%d");
    v->val++;
    CHECK(mut, 100, "%d");
    CHECK(inserted->val, 1, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_and_modify_functional(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(fhm_empty(&fh), true, "%d");
    struct val def = {.id = 137, .val = 0};

    /* Returning a vacant entry is possible when modification is attemtped. */
    ccc_fh_map_entry *ent = and_modify(entry_vr(&fh, &def.id), mod);
    CHECK(fhm_occupied(ent), false, "%d");
    CHECK((fhm_unwrap(ent) == NULL), true, "%d");

    /* Inserting default value before an in place modification is possible. */
    struct val *v = or_insert(entry_vr(&fh, &def.id), &def.e);
    CHECK(v != NULL, true, "%d");
    v->val++;
    struct val const *const inserted = get_key_val(&fh, &def.id);
    CHECK((inserted != NULL), true, "%d");
    CHECK(inserted->id, 137, "%d");
    CHECK(inserted->val, 1, "%d");

    /* Modifying an existing value or inserting default is possible when no
       auxilliary input is needed. */
    struct val *v2 = or_insert(and_modify(entry_vr(&fh, &def.id), mod), &def.e);
    CHECK((v2 != NULL), true, "%d");
    CHECK(inserted->id, 137, "%d");
    CHECK(v2->val, 6, "%d");

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct val *v3 = or_insert(
        and_modify_with(entry_vr(&fh, &def.id), modw, &def.id), &def.e);
    CHECK((v3 != NULL), true, "%d");
    CHECK(inserted->id, 137, "%d");
    CHECK(v3->val, 137, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_and_modify_macros(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    CHECK(fhm_empty(&fh), true, "%d");

    /* Returning a vacant entry is possible when modification is attemtped. */
    ccc_fh_map_entry *ent = FHM_AND_MODIFY(FHM_ENTRY(&fh, 137), mod);
    CHECK(fhm_occupied(ent), false, "%d");
    CHECK((fhm_unwrap(ent) == NULL), true, "%d");

    int mut = 99;

    /* Inserting default value before an in place modification is possible. */
    struct val *v
        = FHM_OR_INSERT(FHM_AND_MODIFY_W(FHM_ENTRY(&fh, 137), modw, gen(&mut)),
                        (struct val){.id = 137, .val = def(&mut)});
    CHECK((v != NULL), true, "%d");
    CHECK(v->id, 137, "%d");
    CHECK(v->val, 0, "%d");
    CHECK(mut, 100, "%d");

    /* Modifying an existing value or inserting default is possible when no
       auxilliary input is needed. */
    struct val *v2 = FHM_OR_INSERT(FHM_AND_MODIFY(FHM_ENTRY(&fh, 137), mod),
                                   (struct val){.id = 137, .val = def(&mut)});
    CHECK((v2 != NULL), true, "%d");
    CHECK(v2->id, 137, "%d");
    CHECK(v2->val, 5, "%d");
    CHECK(mut, 100, "%d");

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. Generate val also has
       lazy evaluation. The function gen executes with its side effect,
       but the function def does not execute and therefore does not modify
       mut. */
    struct val *v3
        = FHM_OR_INSERT(FHM_AND_MODIFY_W(FHM_ENTRY(&fh, 137), modw, gen(&mut)),
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
