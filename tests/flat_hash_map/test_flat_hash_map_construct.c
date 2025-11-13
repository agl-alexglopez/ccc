#include <stddef.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_hash_map.h"
#include "flat_hash_map_util.h"
#include "traits.h"
#include "types.h"
#include "util/allocate.h"

static void
mod(CCC_Type_context const u)
{
    struct Val *v = u.type;
    v->val += 5;
}

static void
modw(CCC_Type_context const u)
{
    struct Val *v = u.type;
    v->val = *((int *)u.context);
}

static CCC_Flat_hash_map static_fh = flat_hash_map_initialize(
    &(small_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
    flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_static_initialize)
{
    CHECK(flat_hash_map_capacity(&static_fh).count, SMALL_FIXED_CAP);
    CHECK(flat_hash_map_count(&static_fh).count, 0);
    CHECK(validate(&static_fh), true);
    CHECK(is_empty(&static_fh), true);
    struct Val def = {.key = 137, .val = 0};

    /* Returning a vacant entry is possible when modification is attempted. */
    Flat_hash_map_entry *ent = and_modify(entry_r(&static_fh, &def.key), mod);
    CHECK(occupied(ent), false);
    CHECK((unwrap(ent) == NULL), true);

    /* Inserting default value before an in place modification is possible. */
    struct Val *v = or_insert(entry_r(&static_fh, &def.key), &def);
    CHECK(v != NULL, true);
    v->val++;
    struct Val const *const inserted = get_key_val(&static_fh, &def.key);
    CHECK((inserted != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(inserted->val, 1);

    /* Modifying an existing value or inserting default is possible when no
       context input is needed. */
    struct Val *v2
        = or_insert(and_modify(entry_r(&static_fh, &def.key), mod), &def);
    CHECK((v2 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v2->val, 6);

    /* Modifying an existing value that requires external input is also
       possible with slightly different signature. */
    struct Val *v3 = or_insert(
        and_modify_context(entry_r(&static_fh, &def.key), modw, &def.key),
        &def);
    CHECK((v3 != NULL), true);
    CHECK(inserted->key, 137);
    CHECK(v3->val, 137);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_copy_no_allocate)
{
    Flat_hash_map src = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    Flat_hash_map dst = flat_hash_map_initialize(
        &(standard_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);
    (void)swap_entry(&src, &(struct Val){.key = 0});
    (void)swap_entry(&src, &(struct Val){.key = 1, .val = 1});
    (void)swap_entry(&src, &(struct Val){.key = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_Result res = flat_hash_map_copy(&dst, &src, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        CCC_Entry src_e = CCC_remove(&src, &(struct Val){.key = i});
        CCC_Entry dst_e = CCC_remove(&dst, &(struct Val){.key = i});
        CHECK(occupied(&src_e), occupied(&dst_e));
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_copy_no_allocate_fail)
{
    Flat_hash_map src = flat_hash_map_initialize(
        &(standard_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);
    Flat_hash_map dst = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_entry(&src, &(struct Val){.key = 0});
    (void)swap_entry(&src, &(struct Val){.key = 1, .val = 1});
    (void)swap_entry(&src, &(struct Val){.key = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_Result res = flat_hash_map_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_copy_allocate)
{
    Flat_hash_map dst = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_zero, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    Flat_hash_map src
        = flat_hash_map_from(key, flat_hash_map_int_zero,
                             flat_hash_map_id_order, std_allocate, NULL, 0,
                             (struct Val[]){
                                 {.key = 0},
                                 {.key = 1, .val = 1},
                                 {.key = 2, .val = 2},
                             });
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_Result res = flat_hash_map_copy(&dst, &src, std_allocate);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        CCC_Entry src_e = CCC_remove(&src, &(struct Val){.key = i});
        CCC_Entry dst_e = CCC_remove(&dst, &(struct Val){.key = i});
        CHECK(occupied(&src_e), occupied(&dst_e));
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN({
        (void)flat_hash_map_clear_and_free(&src, NULL);
        (void)flat_hash_map_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_copy_allocate_fail)
{
    Flat_hash_map src = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_zero, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    Flat_hash_map dst = flat_hash_map_initialize(
        NULL, struct Val, key, flat_hash_map_int_zero, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    (void)swap_entry(&src, &(struct Val){.key = 0});
    (void)swap_entry(&src, &(struct Val){.key = 1, .val = 1});
    (void)swap_entry(&src, &(struct Val){.key = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_Result res = flat_hash_map_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)flat_hash_map_clear_and_free(&src, NULL); });
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_empty)
{
    Flat_hash_map fh = flat_hash_map_initialize(
        &(small_fixed_map){}, struct Val, key, flat_hash_map_int_zero,
        flat_hash_map_id_order, NULL, NULL, SMALL_FIXED_CAP);
    CHECK(is_empty(&fh), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_init_from)
{
    Flat_hash_map map_from_list
        = flat_hash_map_from(key, flat_hash_map_int_to_u64,
                             flat_hash_map_id_order, std_allocate, NULL, 0,
                             (struct Val[]){
                                 {.key = 0, .val = 0},
                                 {.key = 1, .val = 1},
                                 {.key = 2, .val = 2},
                             });
    CHECK(validate(&map_from_list), true);
    CHECK(count(&map_from_list).count, 3);
    size_t seen = 0;
    for (struct Val const *i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        CHECK((i->key == 0 && i->val == 0) || (i->key == 1 && i->val == 1)
                  || (i->key == 2 && i->val == 2),
              true);
        ++seen;
    }
    CHECK(seen, 3);
    CHECK_END_FN(flat_hash_map_clear_and_free(&map_from_list, NULL););
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_init_from_overwrite)
{
    Flat_hash_map map_from_list
        = flat_hash_map_from(key, flat_hash_map_int_to_u64,
                             flat_hash_map_id_order, std_allocate, NULL, 0,
                             (struct Val[]){
                                 {.key = 0, .val = 0},
                                 {.key = 0, .val = 1},
                                 {.key = 0, .val = 2},
                             });
    CHECK(validate(&map_from_list), true);
    CHECK(count(&map_from_list).count, 1);
    size_t seen = 0;
    for (struct Val const *i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        CHECK(i->key, 0);
        CHECK(i->val, 2);
        ++seen;
    }
    CHECK(seen, 1);
    CHECK_END_FN(flat_hash_map_clear_and_free(&map_from_list, NULL););
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_init_from_fail)
{
    // Whoops forgot an allocation function.
    Flat_hash_map map_from_list = flat_hash_map_from(
        key, flat_hash_map_int_to_u64, flat_hash_map_id_order, NULL, NULL, 0,
        (struct Val[]){
            {.key = 0, .val = 0},
            {.key = 0, .val = 1},
            {.key = 0, .val = 2},
        });
    CHECK(validate(&map_from_list), true);
    CHECK(count(&map_from_list).count, 0);
    size_t seen = 0;
    for (struct Val const *i = begin(&map_from_list); i != end(&map_from_list);
         i = next(&map_from_list, i))
    {
        CHECK(i->key, 0);
        CHECK(i->val, 2);
        ++seen;
    }
    CHECK(seen, 0);
    CCC_Entry e = CCC_flat_hash_map_insert_or_assign(
        &map_from_list, &(struct Val){.key = 1, .val = 1});
    CHECK(CCC_entry_insert_error(&e), CCC_TRUE);
    CHECK_END_FN(flat_hash_map_clear_and_free(&map_from_list, NULL););
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_init_with_capacity)
{
    Flat_hash_map fh = flat_hash_map_with_capacity(
        struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 32);
    CHECK(validate(&fh), true);
    CHECK(flat_hash_map_capacity(&fh).count >= 32, true);
    for (int i = 0; i < 10; ++i)
    {
        CCC_Entry const e = CCC_flat_hash_map_insert_or_assign(
            &fh, &(struct Val){.key = i, .val = i});
        CHECK(CCC_entry_insert_error(&e), CCC_FALSE);
        CHECK(flat_hash_map_validate(&fh), CCC_TRUE);
    }
    CHECK(flat_hash_map_count(&fh).count, 10);
    size_t seen = 0;
    for (struct Val const *i = begin(&fh); i != end(&fh); i = next(&fh, i))
    {
        CHECK(i->key >= 0 && i->key < 10, true);
        CHECK(i->val >= 0 && i->val < 10, true);
        CHECK(i->val, i->key);
        ++seen;
    }
    CHECK(seen, 10);
    CHECK_END_FN(flat_hash_map_clear_and_free(&fh, NULL););
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_init_with_capacity_no_op)
{
    /* Initialize with 0 cap is OK just does nothing. */
    Flat_hash_map fh = flat_hash_map_with_capacity(
        struct Val, key, flat_hash_map_int_to_u64, flat_hash_map_id_order,
        std_allocate, NULL, 0);
    CHECK(validate(&fh), true);
    CHECK(flat_hash_map_capacity(&fh).count, 0);
    CHECK(flat_hash_map_count(&fh).count, 0);
    CCC_Entry const e = CCC_flat_hash_map_insert_or_assign(
        &fh, &(struct Val){.key = 1, .val = 1});
    CHECK(CCC_entry_insert_error(&e), CCC_FALSE);
    CHECK(flat_hash_map_validate(&fh), CCC_TRUE);
    CHECK(flat_hash_map_count(&fh).count, 1);
    size_t seen = 0;
    for (struct Val const *i = begin(&fh); i != end(&fh); i = next(&fh, i))
    {
        CHECK(i->key, i->val);
        ++seen;
    }
    CHECK(flat_hash_map_count(&fh).count, 1);
    CHECK(flat_hash_map_capacity(&fh).count > 0, true);
    CHECK(seen, 1);
    CHECK_END_FN(flat_hash_map_clear_and_free(&fh, NULL););
}

CHECK_BEGIN_STATIC_FN(flat_hash_map_test_init_with_capacity_fail)
{
    /* Forgot allocation function. */
    Flat_hash_map fh
        = flat_hash_map_with_capacity(struct Val, key, flat_hash_map_int_to_u64,
                                      flat_hash_map_id_order, NULL, NULL, 32);
    CHECK(validate(&fh), true);
    CHECK(flat_hash_map_capacity(&fh).count, 0);
    CCC_Entry const e = CCC_flat_hash_map_insert_or_assign(
        &fh, &(struct Val){.key = 1, .val = 1});
    CHECK(CCC_entry_insert_error(&e), CCC_TRUE);
    CHECK(flat_hash_map_validate(&fh), CCC_TRUE);
    CHECK(flat_hash_map_count(&fh).count, 0);
    size_t seen = 0;
    for (struct Val const *i = begin(&fh); i != end(&fh); i = next(&fh, i))
    {
        CHECK(i->key, i->val);
        ++seen;
    }
    CHECK(flat_hash_map_count(&fh).count, 0);
    CHECK(seen, 0);
    CHECK_END_FN(flat_hash_map_clear_and_free(&fh, NULL););
}

int
main()
{
    return CHECK_RUN(flat_hash_map_test_static_initialize(),
                     flat_hash_map_test_copy_no_allocate(),
                     flat_hash_map_test_copy_no_allocate_fail(),
                     flat_hash_map_test_copy_allocate(),
                     flat_hash_map_test_copy_allocate_fail(),
                     flat_hash_map_test_empty(), flat_hash_map_test_init_from(),
                     flat_hash_map_test_init_from_overwrite(),
                     flat_hash_map_test_init_from_fail(),
                     flat_hash_map_test_init_with_capacity(),
                     flat_hash_map_test_init_with_capacity_no_op(),
                     flat_hash_map_test_init_with_capacity_fail());
}
