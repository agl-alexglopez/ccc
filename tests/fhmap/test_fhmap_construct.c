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
mod(ccc_any_type const u)
{
    struct val *v = u.any_type;
    v->val += 5;
}

static void
modw(ccc_any_type const u)
{
    struct val *v = u.any_type;
    v->val = *((int *)u.aux);
}

static small_fixed_map static_fh_mem;
static ccc_flat_hash_map static_fh
    = fhm_init(static_fh_mem.data, static_fh_mem.tag, key, fhmap_int_to_u64,
               fhmap_id_eq, nullptr, nullptr, SMALL_FIXED_CAP);

CHECK_BEGIN_STATIC_FN(fhmap_test_static_init)
{
    CHECK(fhm_capacity(&static_fh).count, SMALL_FIXED_CAP);
    CHECK(fhm_size(&static_fh).count, 0);
    CHECK(validate(&static_fh), true);
    CHECK(is_empty(&static_fh), true);
    struct val def = {.key = 137, .val = 0};

    /* Returning a vacant entry is possible when modification is attempted. */
    fhmap_entry *ent = and_modify(entry_r(&static_fh, &def.key), mod);
    CHECK(occupied(ent), false);
    CHECK((unwrap(ent) == nullptr), true);

    /* Inserting default value before an in place modification is possible. */
    struct val *v = or_insert(entry_r(&static_fh, &def.key), &def);
    CHECK(v != nullptr, true);
    v->val++;
    struct val const *const inserted = get_key_val(&static_fh, &def.key);
    CHECK((inserted != nullptr), true);
    CHECK(inserted->key, 137);
    CHECK(inserted->val, 1);

    /* Modifying an existing value or inserting default is possible when no
       auxiliary input is needed. */
    struct val *v2
        = or_insert(and_modify(entry_r(&static_fh, &def.key), mod), &def);
    CHECK((v2 != nullptr), true);
    CHECK(inserted->key, 137);
    CHECK(v2->val, 6);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct val *v3 = or_insert(
        and_modify_aux(entry_r(&static_fh, &def.key), modw, &def.key), &def);
    CHECK((v3 != nullptr), true);
    CHECK(inserted->key, 137);
    CHECK(v3->val, 137);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_copy_no_alloc)
{
    small_fixed_map src_mem;
    standard_fixed_map dst_mem;
    flat_hash_map src
        = fhm_init(src_mem.data, src_mem.tag, key, fhmap_int_zero, fhmap_id_eq,
                   nullptr, nullptr, SMALL_FIXED_CAP);
    flat_hash_map dst
        = fhm_init(dst_mem.data, dst_mem.tag, key, fhmap_int_zero, fhmap_id_eq,
                   nullptr, nullptr, STANDARD_FIXED_CAP);
    (void)swap_entry(&src, &(struct val){.key = 0});
    (void)swap_entry(&src, &(struct val){.key = 1, .val = 1});
    (void)swap_entry(&src, &(struct val){.key = 2, .val = 2});
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fhm_copy(&dst, &src, nullptr);
    CHECK(res, CCC_RESULT_OK);
    CHECK(size(&dst).count, size(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        ccc_entry src_e = ccc_remove(&src, &(struct val){.key = i});
        ccc_entry dst_e = ccc_remove(&dst, &(struct val){.key = i});
        CHECK(occupied(&src_e), occupied(&dst_e));
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_copy_no_alloc_fail)
{
    standard_fixed_map src_mem;
    small_fixed_map dst_mem;
    flat_hash_map src
        = fhm_init(src_mem.data, src_mem.tag, key, fhmap_int_zero, fhmap_id_eq,
                   nullptr, nullptr, STANDARD_FIXED_CAP);
    flat_hash_map dst
        = fhm_init(dst_mem.data, dst_mem.tag, key, fhmap_int_zero, fhmap_id_eq,
                   nullptr, nullptr, SMALL_FIXED_CAP);
    (void)swap_entry(&src, &(struct val){.key = 0});
    (void)swap_entry(&src, &(struct val){.key = 1, .val = 1});
    (void)swap_entry(&src, &(struct val){.key = 2, .val = 2});
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fhm_copy(&dst, &src, nullptr);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fhmap_test_copy_alloc)
{
    flat_hash_map src
        = fhm_init((struct val *)nullptr, nullptr, key, fhmap_int_zero,
                   fhmap_id_eq, std_alloc, nullptr, 0);
    flat_hash_map dst
        = fhm_init((struct val *)nullptr, nullptr, key, fhmap_int_zero,
                   fhmap_id_eq, std_alloc, nullptr, 0);
    (void)swap_entry(&src, &(struct val){.key = 0});
    (void)swap_entry(&src, &(struct val){.key = 1, .val = 1});
    (void)swap_entry(&src, &(struct val){.key = 2, .val = 2});
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fhm_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_RESULT_OK);
    CHECK(size(&dst).count, size(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        ccc_entry src_e = ccc_remove(&src, &(struct val){.key = i});
        ccc_entry dst_e = ccc_remove(&dst, &(struct val){.key = i});
        CHECK(occupied(&src_e), occupied(&dst_e));
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN({
        (void)fhm_clear_and_free(&src, nullptr);
        (void)fhm_clear_and_free(&dst, nullptr);
    });
}

CHECK_BEGIN_STATIC_FN(fhmap_test_copy_alloc_fail)
{
    flat_hash_map src
        = fhm_init((struct val *)nullptr, nullptr, key, fhmap_int_zero,
                   fhmap_id_eq, std_alloc, nullptr, 0);
    flat_hash_map dst
        = fhm_init((struct val *)nullptr, nullptr, key, fhmap_int_zero,
                   fhmap_id_eq, std_alloc, nullptr, 0);
    (void)swap_entry(&src, &(struct val){.key = 0});
    (void)swap_entry(&src, &(struct val){.key = 1, .val = 1});
    (void)swap_entry(&src, &(struct val){.key = 2, .val = 2});
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fhm_copy(&dst, &src, nullptr);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)fhm_clear_and_free(&src, nullptr); });
}

CHECK_BEGIN_STATIC_FN(fhmap_test_empty)
{
    small_fixed_map s;
    flat_hash_map fh = fhm_init(s.data, s.tag, key, fhmap_int_zero, fhmap_id_eq,
                                nullptr, nullptr, SMALL_FIXED_CAP);
    CHECK(is_empty(&fh), true);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(fhmap_test_static_init(), fhmap_test_copy_no_alloc(),
                     fhmap_test_copy_no_alloc_fail(), fhmap_test_copy_alloc(),
                     fhmap_test_copy_alloc_fail(), fhmap_test_empty());
}
