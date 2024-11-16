#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "fhmap_util.h"
#include "flat_hash_map.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>

static void
mod(ccc_user_type const u)
{
    struct val *v = u.user_type;
    v->val += 5;
}

static void
modw(ccc_user_type const u)
{
    struct val *v = u.user_type;
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

CHECK_BEGIN_STATIC_FN(fhmap_test_empty)
{
    struct val vals[5] = {};
    ccc_flat_hash_map fh;
    ccc_result const res
        = fhm_init(&fh, vals, sizeof(vals) / sizeof(vals[0]), id, e, NULL,
                   fhmap_int_zero, fhmap_id_eq, NULL);
    CHECK(res, CCC_OK);
    CHECK(is_empty(&fh), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_functional)
{
    struct val vals[5] = {};
    ccc_flat_hash_map fh;
    ccc_result const res
        = fhm_init(&fh, vals, sizeof(vals) / sizeof(vals[0]), id, e, NULL,
                   fhmap_int_zero, fhmap_id_eq, NULL);
    CHECK(res, CCC_OK);
    CHECK(is_empty(&fh), true);
    struct val def = {.id = 137, .val = 0};
    ccc_fhmap_entry ent = entry(&fh, &def.id);
    CHECK(unwrap(&ent) == NULL, true);
    struct val *v = or_insert(entry_r(&fh, &def.id), &def.e);
    CHECK(v != NULL, true);
    v->val += 1;
    struct val const *const inserted = get_key_val(&fh, &def.id);
    CHECK((inserted != NULL), true);
    CHECK(inserted->val, 1);
    v = or_insert(entry_r(&fh, &def.id), &def.e);
    CHECK(v != NULL, true);
    v->val += 1;
    CHECK(inserted->val, 2);
    return PASS;
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_macros)
{
    struct val vals[5] = {};
    ccc_flat_hash_map fh;
    ccc_result const res
        = fhm_init(&fh, vals, sizeof(vals) / sizeof(vals[0]), id, e, NULL,
                   fhmap_int_zero, fhmap_id_eq, NULL);
    CHECK(res, CCC_OK);
    CHECK(is_empty(&fh), true);
    CHECK(get_key_val(&fh, &(int){137}) == NULL, true);
    int const key = 137;
    int mut = 99;
    /* The function with a side effect should execute. */
    struct val *inserted = fhm_or_insert_w(
        entry_r(&fh, &key), (struct val){.id = key, .val = def(&mut)});
    CHECK(inserted != NULL, true);
    CHECK(mut, 100);
    CHECK(inserted->val, 0);
    /* The function with a side effect should NOT execute. */
    struct val *v = fhm_or_insert_w(entry_r(&fh, &key),
                                    (struct val){.id = key, .val = def(&mut)});
    CHECK(v != NULL, true);
    v->val++;
    CHECK(mut, 100);
    CHECK(inserted->val, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_and_modify_functional)
{
    struct val vals[5] = {};
    ccc_flat_hash_map fh;
    ccc_result const res
        = fhm_init(&fh, vals, sizeof(vals) / sizeof(vals[0]), id, e, NULL,
                   fhmap_int_zero, fhmap_id_eq, NULL);
    CHECK(res, CCC_OK);
    CHECK(is_empty(&fh), true);
    struct val def = {.id = 137, .val = 0};

    /* Returning a vacant entry is possible when modification is attempted. */
    ccc_fhmap_entry *ent = and_modify(entry_r(&fh, &def.id), mod);
    CHECK(occupied(ent), false);
    CHECK((unwrap(ent) == NULL), true);

    /* Inserting default value before an in place modification is possible. */
    struct val *v = or_insert(entry_r(&fh, &def.id), &def.e);
    CHECK(v != NULL, true);
    v->val++;
    struct val const *const inserted = get_key_val(&fh, &def.id);
    CHECK((inserted != NULL), true);
    CHECK(inserted->id, 137);
    CHECK(inserted->val, 1);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    struct val *v2 = or_insert(and_modify(entry_r(&fh, &def.id), mod), &def.e);
    CHECK((v2 != NULL), true);
    CHECK(inserted->id, 137);
    CHECK(v2->val, 6);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct val *v3 = or_insert(
        and_modify_aux(entry_r(&fh, &def.id), modw, &def.id), &def.e);
    CHECK((v3 != NULL), true);
    CHECK(inserted->id, 137);
    CHECK(v3->val, 137);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_and_modify_macros)
{
    struct val vals[5] = {};
    ccc_flat_hash_map fh;
    ccc_result const res
        = fhm_init(&fh, vals, sizeof(vals) / sizeof(vals[0]), id, e, NULL,
                   fhmap_int_zero, fhmap_id_eq, NULL);
    CHECK(res, CCC_OK);
    CHECK(is_empty(&fh), true);

    /* Returning a vacant entry is possible when modification is attempted. */
    ccc_fhmap_entry *ent = and_modify(entry_r(&fh, &(int){137}), mod);
    CHECK(occupied(ent), false);
    CHECK((unwrap(ent) == NULL), true);

    int mut = 99;

    /* Inserting default value before an in place modification is possible. */
    struct val *v
        = fhm_or_insert_w(fhm_and_modify_w(entry_r(&fh, &(int){137}),
                                           ((struct val *)T)->val = gen(&mut);),
                          (struct val){.id = 137, .val = def(&mut)});
    CHECK((v != NULL), true);
    CHECK(v->id, 137);
    CHECK(v->val, 0);
    CHECK(mut, 100);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    struct val *v2 = fhm_or_insert_w(and_modify(entry_r(&fh, &(int){137}), mod),
                                     (struct val){.id = 137, .val = def(&mut)});
    CHECK((v2 != NULL), true);
    CHECK(v2->id, 137);
    CHECK(v2->val, 5);
    CHECK(mut, 100);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. Generate val also has
       lazy evaluation. The function gen executes with its side effect,
       but the function def does not execute and therefore does not modify
       mut. */
    struct val *v3
        = fhm_or_insert_w(fhm_and_modify_w(entry_r(&fh, &(int){137}),
                                           ((struct val *)T)->val = gen(&mut);),
                          (struct val){.id = 137, .val = def(&mut)});
    CHECK((v3 != NULL), true);
    CHECK(v3->id, 137);
    CHECK(v3->val, 42);
    CHECK(mut, 0);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fhmap_test_empty(), fhmap_test_entry_macros(),
                     fhmap_test_entry_functional(),
                     fhmap_test_entry_and_modify_functional(),
                     fhmap_test_entry_and_modify_macros());
}
