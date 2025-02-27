#include <stddef.h>

#define HANDLE_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "alloc.h"
#include "checkers.h"
#include "handle_hash_map.h"
#include "hhmap_util.h"
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
static handle_hash_map static_fh
    = hhm_init(s_vals, e, key, hhmap_int_to_u64, hhmap_id_eq, NULL, NULL,
               sizeof(s_vals) / sizeof(s_vals[0]));

CHECK_BEGIN_STATIC_FN(hhmap_test_static_init)
{
    CHECK(hhm_capacity(&static_fh), sizeof(s_vals) / sizeof(s_vals[0]));
    CHECK(hhm_size(&static_fh), 0);
    CHECK(hhm_validate(&static_fh), true);
    CHECK(hhm_is_empty(&static_fh), true);
    struct val def = {.key = 137, .val = 0};

    /* Returning a vacant hhm_handle is possible when modification is attempted.
     */
    hhmap_handle *ent = hhm_and_modify(hhm_handle_r(&static_fh, &def.key), mod);
    CHECK(hhm_occupied(ent), false);
    CHECK((hhm_unwrap(ent) == 0), true);

    /* Inserting default value before an in place modification is possible. */
    ccc_handle_i h = hhm_or_insert(hhm_handle_r(&static_fh, &def.key), &def.e);
    CHECK(h != 0, true);
    struct val *v = hhm_at(&static_fh, h);
    CHECK(v != NULL, true);
    v->val++;
    h = hhm_get_key_val(&static_fh, &def.key);
    struct val const *const inserted = hhm_at(&static_fh, h);
    CHECK((inserted != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(inserted->val, 1);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    h = hhm_or_insert(hhm_and_modify(hhm_handle_r(&static_fh, &def.key), mod),
                      &def.e);
    struct val *const v2 = hhm_at(&static_fh, h);
    CHECK((v2 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v2->val, 6);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    h = hhm_or_insert(
        hhm_and_modify_aux(hhm_handle_r(&static_fh, &def.key), modw, &def.key),
        &def.e);
    struct val *const v3 = hhm_at(&static_fh, h);
    CHECK((v3 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v3->val, 137);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_copy_no_alloc)
{
    handle_hash_map src = hhm_init((struct val[11]){}, e, key, hhmap_int_zero,
                                   hhmap_id_eq, NULL, NULL, 11);
    handle_hash_map dst = hhm_init((struct val[13]){}, e, key, hhmap_int_zero,
                                   hhmap_id_eq, NULL, NULL, 13);
    (void)hhm_insert(&src, &(struct val){.key = 0}.e);
    CHECK(hhm_contains(&src, &(int){0}), true);
    (void)hhm_insert(&src, &(struct val){.key = 1, .val = 1}.e);
    CHECK(hhm_contains(&src, &(int){0}), true);
    CHECK(hhm_contains(&src, &(int){1}), true);
    (void)hhm_insert(&src, &(struct val){.key = 2, .val = 2}.e);
    CHECK(hhm_contains(&src, &(int){0}), true);
    CHECK(hhm_contains(&src, &(int){1}), true);
    CHECK(hhm_contains(&src, &(int){2}), true);
    CHECK(hhm_size(&src), 3);
    CHECK(hhm_is_empty(&dst), true);
    ccc_result res = hhm_copy(&dst, &src, NULL);
    CHECK(res, CCC_OK);
    CHECK(hhm_size(&dst), hhm_size(&src));
    for (int i = 0; i < 3; ++i)
    {
        ccc_handle src_e = hhm_remove(&src, &(struct val){.key = i}.e);
        ccc_handle dst_e = hhm_remove(&dst, &(struct val){.key = i}.e);
        CHECK(occupied(&src_e), occupied(&dst_e));
    }
    CHECK(hhm_is_empty(&src), hhm_is_empty(&dst));
    CHECK(hhm_is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_copy_no_alloc_fail)
{
    handle_hash_map src = hhm_init((struct val[11]){}, e, key, hhmap_int_zero,
                                   hhmap_id_eq, NULL, NULL, 11);
    handle_hash_map dst = hhm_init((struct val[7]){}, e, key, hhmap_int_zero,
                                   hhmap_id_eq, NULL, NULL, 7);
    (void)hhm_insert(&src, &(struct val){.key = 0}.e);
    (void)hhm_insert(&src, &(struct val){.key = 1, .val = 1}.e);
    (void)hhm_insert(&src, &(struct val){.key = 2, .val = 2}.e);
    CHECK(hhm_size(&src), 3);
    CHECK(hhm_is_empty(&dst), true);
    ccc_result res = hhm_copy(&dst, &src, NULL);
    CHECK(res != CCC_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_copy_alloc)
{
    handle_hash_map src = hhm_init((struct val *)NULL, e, key, hhmap_int_zero,
                                   hhmap_id_eq, std_alloc, NULL, 0);
    handle_hash_map dst = hhm_init((struct val *)NULL, e, key, hhmap_int_zero,
                                   hhmap_id_eq, std_alloc, NULL, 0);
    (void)hhm_insert(&src, &(struct val){.key = 0}.e);
    CHECK(hhm_contains(&src, &(int){0}), true);
    (void)hhm_insert(&src, &(struct val){.key = 1, .val = 1}.e);
    CHECK(hhm_contains(&src, &(int){0}), true);
    CHECK(hhm_contains(&src, &(int){1}), true);
    (void)hhm_insert(&src, &(struct val){.key = 2, .val = 2}.e);
    CHECK(hhm_contains(&src, &(int){0}), true);
    CHECK(hhm_contains(&src, &(int){1}), true);
    CHECK(hhm_contains(&src, &(int){2}), true);
    CHECK(hhm_size(&src), 3);
    CHECK(hhm_is_empty(&dst), true);
    ccc_result res = hhm_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_OK);
    CHECK(hhm_size(&dst), hhm_size(&src));
    for (int i = 0; i < 3; ++i)
    {
        ccc_handle src_e = hhm_remove(&src, &(struct val){.key = i}.e);
        ccc_handle dst_e = hhm_remove(&dst, &(struct val){.key = i}.e);
        CHECK(occupied(&src_e), occupied(&dst_e));
    }
    CHECK(hhm_is_empty(&src), hhm_is_empty(&dst));
    CHECK(hhm_is_empty(&dst), true);
    CHECK_END_FN({
        (void)hhm_clear_and_free(&src, NULL);
        (void)hhm_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(hhmap_test_copy_alloc_fail)
{
    handle_hash_map src = hhm_init((struct val *)NULL, e, key, hhmap_int_zero,
                                   hhmap_id_eq, std_alloc, NULL, 0);
    handle_hash_map dst = hhm_init((struct val *)NULL, e, key, hhmap_int_zero,
                                   hhmap_id_eq, std_alloc, NULL, 0);
    (void)hhm_insert(&src, &(struct val){.key = 0}.e);
    (void)hhm_insert(&src, &(struct val){.key = 1, .val = 1}.e);
    (void)hhm_insert(&src, &(struct val){.key = 2, .val = 2}.e);
    CHECK(hhm_size(&src), 3);
    CHECK(hhm_is_empty(&dst), true);
    ccc_result res = hhm_copy(&dst, &src, NULL);
    CHECK(res != CCC_OK, true);
    CHECK_END_FN({ (void)hhm_clear_and_free(&src, NULL); });
}

CHECK_BEGIN_STATIC_FN(hhmap_test_empty)
{
    struct val vals[5] = {};
    handle_hash_map fh = hhm_init(vals, key, e, hhmap_int_zero, hhmap_id_eq,
                                  NULL, NULL, sizeof(vals) / sizeof(vals[0]));
    CHECK(hhm_is_empty(&fh), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_hhm_handle_functional)
{
    handle_hash_map fh = hhm_init((struct val[5]){}, e, key, hhmap_int_zero,
                                  hhmap_id_eq, NULL, NULL, 5);
    CHECK(hhm_is_empty(&fh), true);
    struct val def = {.key = 137, .val = 0};
    hhmap_handle ent = hhm_handle(&fh, &def.key);
    CHECK(hhm_unwrap(&ent), false);
    ccc_handle_i h = hhm_or_insert(hhm_handle_r(&fh, &def.key), &def.e);
    struct val *v = hhm_at(&fh, h);
    CHECK(v != NULL, true);
    v->val += 1;
    h = hhm_get_key_val(&fh, &def.key);
    struct val const *const inserted = hhm_at(&fh, h);
    CHECK((inserted != NULL), true);
    CHECK(inserted->val, 1);
    h = hhm_or_insert(hhm_handle_r(&fh, &def.key), &def.e);
    v = hhm_at(&fh, h);
    CHECK(v != NULL, true);
    v->val += 1;
    CHECK(inserted->val, 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_hhm_handle_macros)
{
    handle_hash_map fh = hhm_init((struct val[5]){}, e, key, hhmap_int_zero,
                                  hhmap_id_eq, NULL, NULL, 5);
    CHECK(hhm_is_empty(&fh), true);
    CHECK(hhm_get_key_val(&fh, &(int){137}), false);
    int const key = 137;
    int mut = 99;
    /* The function with a side effect should execute. */
    ccc_handle_i h = hhm_or_insert_w(
        hhm_handle_r(&fh, &key), (struct val){.key = key, .val = def(&mut)});
    struct val *inserted = hhm_at(&fh, h);
    CHECK(inserted != NULL, true);
    CHECK(mut, 100);
    CHECK(inserted->val, 0);
    /* The function with a side effect should NOT execute. */
    h = hhm_or_insert_w(hhm_handle_r(&fh, &key),
                        (struct val){.key = key, .val = def(&mut)});
    struct val *v = hhm_at(&fh, h);
    CHECK(v != NULL, true);
    v->val++;
    CHECK(mut, 100);
    CHECK(inserted->val, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_hhm_handle_hhm_and_modify_functional)
{
    handle_hash_map fh = hhm_init((struct val[5]){}, e, key, hhmap_int_zero,
                                  hhmap_id_eq, NULL, NULL, 5);
    CHECK(hhm_is_empty(&fh), true);
    struct val def = {.key = 137, .val = 0};

    /* Returning a vacant hhm_handle is possible when modification is attempted.
     */
    hhmap_handle *ent = hhm_and_modify(hhm_handle_r(&fh, &def.key), mod);
    CHECK(hhm_occupied(ent), false);
    CHECK(hhm_unwrap(ent), false);

    /* Inserting default value before an in place modification is possible. */
    ccc_handle_i h = hhm_or_insert(hhm_handle_r(&fh, &def.key), &def.e);
    struct val *v = hhm_at(&fh, h);
    CHECK(v != NULL, true);
    v->val++;
    h = hhm_get_key_val(&fh, &def.key);
    struct val const *const inserted = hhm_at(&fh, h);
    CHECK((inserted != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(inserted->val, 1);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    h = hhm_or_insert(hhm_and_modify(hhm_handle_r(&fh, &def.key), mod), &def.e);
    struct val *v2 = hhm_at(&fh, h);
    CHECK((v2 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v2->val, 6);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    h = hhm_or_insert(
        hhm_and_modify_aux(hhm_handle_r(&fh, &def.key), modw, &def.key),
        &def.e);
    struct val *v3 = hhm_at(&fh, h);
    CHECK((v3 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v3->val, 137);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_hhm_handle_hhm_and_modify_macros)
{
    handle_hash_map fh = hhm_init((struct val[5]){}, e, key, hhmap_int_zero,
                                  hhmap_id_eq, NULL, NULL, 5);
    CHECK(hhm_is_empty(&fh), true);

    /* Returning a vacant hhm_handle is possible when modification is attempted.
     */
    hhmap_handle *ent = hhm_and_modify(hhm_handle_r(&fh, &(int){137}), mod);
    CHECK(hhm_occupied(ent), false);
    CHECK(hhm_unwrap(ent), false);

    int mut = 99;

    /* Inserting default value before an in place modification is possible. */
    ccc_handle_i h
        = hhm_or_insert_w(hhm_and_modify_w(hhm_handle_r(&fh, &(int){137}),
                                           struct val, { T->val = gen(&mut); }),
                          (struct val){.key = 137, .val = def(&mut)});
    struct val *v = hhm_at(&fh, h);
    CHECK((v != NULL), true);
    CHECK(v->key, 137);
    CHECK(v->val, 0);
    CHECK(mut, 100);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    h = hhm_or_insert_w(hhm_and_modify(hhm_handle_r(&fh, &(int){137}), mod),
                        (struct val){.key = 137, .val = def(&mut)});
    struct val *v2 = hhm_at(&fh, h);
    CHECK((v2 != NULL), true);
    CHECK(v2->key, 137);
    CHECK(v2->val, 5);
    CHECK(mut, 100);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. Generate val also has
       lazy evaluation. The function gen executes with its side effect,
       but the function def does not execute and therefore does not modify
       mut. */
    h = hhm_or_insert_w(hhm_and_modify_w(hhm_handle_r(&fh, &(int){137}),
                                         struct val, { T->val = gen(&mut); }),
                        (struct val){.key = 137, .val = def(&mut)});
    struct val *v3 = hhm_at(&fh, h);
    CHECK((v3 != NULL), true);
    CHECK(v3->key, 137);
    CHECK(v3->val, 42);
    CHECK(mut, 0);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(hhmap_test_static_init(), hhmap_test_copy_no_alloc(),
                     hhmap_test_copy_no_alloc_fail(), hhmap_test_copy_alloc(),
                     hhmap_test_copy_alloc_fail(), hhmap_test_empty(),
                     hhmap_test_hhm_handle_macros(),
                     hhmap_test_hhm_handle_functional(),
                     hhmap_test_hhm_handle_hhm_and_modify_functional(),
                     hhmap_test_hhm_handle_hhm_and_modify_macros());
}
