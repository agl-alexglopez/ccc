#include <stddef.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "alloc.h"
#include "checkers.h"
#include "fhmap_util.h"
#include "flat_hash_map.h"
#include "traits.h"
#include "types.h"

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

static struct val s_vals[10];
static flat_hash_map static_fh
    = fhm_init(s_vals, e, key, fhmap_int_to_u64, fhmap_id_eq, NULL, NULL,
               sizeof(s_vals) / sizeof(s_vals[0]));

CHECK_BEGIN_STATIC_FN(fhmap_test_static_init)
{
    CHECK(fhm_capacity(&static_fh).count, sizeof(s_vals) / sizeof(s_vals[0]));
    CHECK(fhm_size(&static_fh).count, 0);
    CHECK(validate(&static_fh), true);
    CHECK(is_empty(&static_fh), true);
    struct val def = {.key = 137, .val = 0};

    /* Returning a vacant entry is possible when modification is attempted. */
    fhmap_entry *ent = and_modify(entry_r(&static_fh, &def.key), mod);
    CHECK(occupied(ent), false);
    CHECK((unwrap(ent) == NULL), true);

    /* Inserting default value before an in place modification is possible. */
    struct val *v = or_insert(entry_r(&static_fh, &def.key), &def.e);
    CHECK(v != NULL, true);
    v->val++;
    struct val const *const inserted = get_key_val(&static_fh, &def.key);
    CHECK((inserted != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(inserted->val, 1);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    struct val *v2
        = or_insert(and_modify(entry_r(&static_fh, &def.key), mod), &def.e);
    CHECK((v2 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v2->val, 6);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct val *v3 = or_insert(
        and_modify_aux(entry_r(&static_fh, &def.key), modw, &def.key), &def.e);
    CHECK((v3 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v3->val, 137);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_copy_no_alloc)
{
    flat_hash_map src = fhm_init((struct val[11]){}, e, key, fhmap_int_zero,
                                 fhmap_id_eq, NULL, NULL, 11);
    flat_hash_map dst = fhm_init((struct val[13]){}, e, key, fhmap_int_zero,
                                 fhmap_id_eq, NULL, NULL, 13);
    (void)swap_entry(&src, &(struct val){.key = 0}.e);
    (void)swap_entry(&src, &(struct val){.key = 1, .val = 1}.e);
    (void)swap_entry(&src, &(struct val){.key = 2, .val = 2}.e);
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fhm_copy(&dst, &src, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(size(&dst).count, size(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        ccc_entry src_e = ccc_remove(&src, &(struct val){.key = i}.e);
        ccc_entry dst_e = ccc_remove(&dst, &(struct val){.key = i}.e);
        CHECK(occupied(&src_e), occupied(&dst_e));
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_copy_no_alloc_fail)
{
    flat_hash_map src = fhm_init((struct val[11]){}, e, key, fhmap_int_zero,
                                 fhmap_id_eq, NULL, NULL, 11);
    flat_hash_map dst = fhm_init((struct val[7]){}, e, key, fhmap_int_zero,
                                 fhmap_id_eq, NULL, NULL, 7);
    (void)swap_entry(&src, &(struct val){.key = 0}.e);
    (void)swap_entry(&src, &(struct val){.key = 1, .val = 1}.e);
    (void)swap_entry(&src, &(struct val){.key = 2, .val = 2}.e);
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fhm_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_copy_alloc)
{
    flat_hash_map src = fhm_init((struct val *)NULL, e, key, fhmap_int_zero,
                                 fhmap_id_eq, std_alloc, NULL, 0);
    flat_hash_map dst = fhm_init((struct val *)NULL, e, key, fhmap_int_zero,
                                 fhmap_id_eq, std_alloc, NULL, 0);
    (void)swap_entry(&src, &(struct val){.key = 0}.e);
    (void)swap_entry(&src, &(struct val){.key = 1, .val = 1}.e);
    (void)swap_entry(&src, &(struct val){.key = 2, .val = 2}.e);
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fhm_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_RESULT_OK);
    CHECK(size(&dst).count, size(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        ccc_entry src_e = ccc_remove(&src, &(struct val){.key = i}.e);
        ccc_entry dst_e = ccc_remove(&dst, &(struct val){.key = i}.e);
        CHECK(occupied(&src_e), occupied(&dst_e));
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN({
        (void)fhm_clear_and_free(&src, NULL);
        (void)fhm_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(fhmap_test_copy_alloc_fail)
{
    flat_hash_map src = fhm_init((struct val *)NULL, e, key, fhmap_int_zero,
                                 fhmap_id_eq, std_alloc, NULL, 0);
    flat_hash_map dst = fhm_init((struct val *)NULL, e, key, fhmap_int_zero,
                                 fhmap_id_eq, std_alloc, NULL, 0);
    (void)swap_entry(&src, &(struct val){.key = 0}.e);
    (void)swap_entry(&src, &(struct val){.key = 1, .val = 1}.e);
    (void)swap_entry(&src, &(struct val){.key = 2, .val = 2}.e);
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fhm_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)fhm_clear_and_free(&src, NULL); });
}

CHECK_BEGIN_STATIC_FN(fhmap_test_empty)
{
    struct val vals[5] = {};
    flat_hash_map fh = fhm_init(vals, key, e, fhmap_int_zero, fhmap_id_eq, NULL,
                                NULL, sizeof(vals) / sizeof(vals[0]));
    CHECK(is_empty(&fh), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_functional)
{
    flat_hash_map fh = fhm_init((struct val[5]){}, e, key, fhmap_int_zero,
                                fhmap_id_eq, NULL, NULL, 5);
    CHECK(is_empty(&fh), true);
    struct val def = {.key = 137, .val = 0};
    fhmap_entry ent = entry(&fh, &def.key);
    CHECK(unwrap(&ent) == NULL, true);
    struct val *v = or_insert(entry_r(&fh, &def.key), &def.e);
    CHECK(v != NULL, true);
    v->val += 1;
    struct val const *const inserted = get_key_val(&fh, &def.key);
    CHECK((inserted != NULL), true);
    CHECK(inserted->val, 1);
    v = or_insert(entry_r(&fh, &def.key), &def.e);
    CHECK(v != NULL, true);
    v->val += 1;
    CHECK(inserted->val, 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_macros)
{
    flat_hash_map fh = fhm_init((struct val[5]){}, e, key, fhmap_int_zero,
                                fhmap_id_eq, NULL, NULL, 5);
    CHECK(is_empty(&fh), true);
    CHECK(get_key_val(&fh, &(int){137}) == NULL, true);
    int const key = 137;
    int mut = 99;
    /* The function with a side effect should execute. */
    struct val *inserted = fhm_or_insert_w(
        entry_r(&fh, &key), (struct val){.key = key, .val = def(&mut)});
    CHECK(inserted != NULL, true);
    CHECK(mut, 100);
    CHECK(inserted->val, 0);
    /* The function with a side effect should NOT execute. */
    struct val *v = fhm_or_insert_w(entry_r(&fh, &key),
                                    (struct val){.key = key, .val = def(&mut)});
    CHECK(v != NULL, true);
    v->val++;
    CHECK(mut, 100);
    CHECK(inserted->val, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_and_modify_functional)
{
    flat_hash_map fh = fhm_init((struct val[5]){}, e, key, fhmap_int_zero,
                                fhmap_id_eq, NULL, NULL, 5);
    CHECK(is_empty(&fh), true);
    struct val def = {.key = 137, .val = 0};

    /* Returning a vacant entry is possible when modification is attempted. */
    fhmap_entry *ent = and_modify(entry_r(&fh, &def.key), mod);
    CHECK(occupied(ent), false);
    CHECK((unwrap(ent) == NULL), true);

    /* Inserting default value before an in place modification is possible. */
    struct val *v = or_insert(entry_r(&fh, &def.key), &def.e);
    CHECK(v != NULL, true);
    v->val++;
    struct val const *const inserted = get_key_val(&fh, &def.key);
    CHECK((inserted != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(inserted->val, 1);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    struct val *v2 = or_insert(and_modify(entry_r(&fh, &def.key), mod), &def.e);
    CHECK((v2 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v2->val, 6);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct val *v3 = or_insert(
        and_modify_aux(entry_r(&fh, &def.key), modw, &def.key), &def.e);
    CHECK((v3 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v3->val, 137);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_entry_and_modify_macros)
{
    flat_hash_map fh = fhm_init((struct val[5]){}, e, key, fhmap_int_zero,
                                fhmap_id_eq, NULL, NULL, 5);
    CHECK(is_empty(&fh), true);

    /* Returning a vacant entry is possible when modification is attempted. */
    fhmap_entry *ent = and_modify(entry_r(&fh, &(int){137}), mod);
    CHECK(occupied(ent), false);
    CHECK((unwrap(ent) == NULL), true);

    int mut = 99;

    /* Inserting default value before an in place modification is possible. */
    struct val *v = fhm_or_insert_w(fhm_and_modify_w(entry_r(&fh, &(int){137}),
                                                     struct val,
                                                     {
                                                         T->val = gen(&mut);
                                                     }),
                                    (struct val){.key = 137, .val = def(&mut)});
    CHECK((v != NULL), true);
    CHECK(v->key, 137);
    CHECK(v->val, 0);
    CHECK(mut, 100);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    struct val *v2
        = fhm_or_insert_w(and_modify(entry_r(&fh, &(int){137}), mod),
                          (struct val){.key = 137, .val = def(&mut)});
    CHECK((v2 != NULL), true);
    CHECK(v2->key, 137);
    CHECK(v2->val, 5);
    CHECK(mut, 100);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. Generate val also has
       lazy evaluation. The function gen executes with its side effect,
       but the function def does not execute and therefore does not modify
       mut. */
    struct val *v3 = fhm_or_insert_w(
        fhm_and_modify_w(entry_r(&fh, &(int){137}), struct val,
                         {
                             T->val = gen(&mut);
                         }),
        (struct val){.key = 137, .val = def(&mut)});
    CHECK((v3 != NULL), true);
    CHECK(v3->key, 137);
    CHECK(v3->val, 42);
    CHECK(mut, 0);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fhmap_test_static_init(), fhmap_test_copy_no_alloc(),
                     fhmap_test_copy_no_alloc_fail(), fhmap_test_copy_alloc(),
                     fhmap_test_copy_alloc_fail(), fhmap_test_empty(),
                     fhmap_test_entry_macros(), fhmap_test_entry_functional(),
                     fhmap_test_entry_and_modify_functional(),
                     fhmap_test_entry_and_modify_macros());
}
