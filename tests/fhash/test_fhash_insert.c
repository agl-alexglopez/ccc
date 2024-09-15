#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "fhash_util.h"
#include "flat_hash_map.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

static enum test_result fhash_test_insert(void);
static enum test_result fhash_test_insert_overwrite(void);
static enum test_result fhash_test_insert_via_entry(void);
static enum test_result fhash_test_insert_via_entry_macros(void);
static enum test_result fhash_test_insert_then_bad_ideas(void);
static enum test_result fhash_test_entry_api_functional(void);
static enum test_result fhash_test_entry_api_macros(void);
static enum test_result fhash_test_two_sum(void);
static enum test_result fhash_test_resize(void);
static enum test_result fhash_test_resize_macros(void);
static enum test_result fhash_test_resize_from_null(void);
static enum test_result fhash_test_resize_from_null_macros(void);
static enum test_result fhash_test_insert_limit(void);

#define NUM_TESTS (size_t)13
test_fn const all_tests[NUM_TESTS] = {
    fhash_test_insert,
    fhash_test_insert_overwrite,
    fhash_test_insert_then_bad_ideas,
    fhash_test_insert_via_entry,
    fhash_test_insert_via_entry_macros,
    fhash_test_entry_api_functional,
    fhash_test_entry_api_macros,
    fhash_test_two_sum,
    fhash_test_resize,
    fhash_test_resize_macros,
    fhash_test_resize_from_null,
    fhash_test_resize_from_null_macros,
    fhash_test_insert_limit,
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
fhash_test_insert(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent
        = insert(&fh, &(struct val){.id = 137, .val = 99}.e, &(struct val){});
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&fh), 1);
    return PASS;
}

static enum test_result
fhash_test_insert_overwrite(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    struct val q = {.id = 137, .val = 99};
    ccc_entry ent = insert(&fh, &q.e, &(struct val){});
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);

    struct val const *v = unwrap(entry_vr(&fh, &q.id));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_entry old_ent = insert(&fh, &q.e, &(struct val){});
    CHECK(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(q.val, 99);
    v = unwrap(entry_vr(&fh, &q.id));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    return PASS;
}

static enum test_result
fhash_test_insert_then_bad_ideas(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    struct val q = {.id = 137, .val = 99};
    ccc_entry ent = insert(&fh, &q.e, &(struct val){});
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    struct val const *v = unwrap(entry_vr(&fh, &q.id));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    q = (struct val){.id = 137, .val = 100};

    ent = insert(&fh, &q.e, &(struct val){});
    CHECK(occupied(&ent), true);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(q.val, 99);
    q.val -= 9;

    v = get_key_val(&fh, &q.id);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 90);
    return PASS;
}

static enum test_result
fhash_test_entry_api_functional(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[200];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d = or_insert(entry_vr(&fh, &def.id), &def.e);
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&fh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d = or_insert(
            and_modify(entry_vr(&fh, &def.id), fhash_modplus), &def.e);
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        if (i % 2)
        {
            CHECK(d->val, i);
        }
        else
        {
            CHECK(d->val, i + 1);
        }
        CHECK(d->val % 2, true);
    }
    CHECK(size(&fh), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val *const in = or_insert(entry_vr(&fh, &def.id), &def.e);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&fh), (size / 2));
    return PASS;
}

static enum test_result
fhash_test_insert_via_entry(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[200];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = insert_entry(entry_vr(&fh, &def.id), &def.e);
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&fh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = insert_entry(entry_vr(&fh, &def.id), &def.e);
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->val, i + 1);
        if (i % 2)
        {
            CHECK(d->val % 2 == 0, true);
        }
        else
        {
            CHECK(d->val % 2, true);
        }
    }
    CHECK(size(&fh), (size / 2));
    return PASS;
}

static enum test_result
fhash_test_insert_via_entry_macros(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[200];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, i), (struct val){i, i, {}});
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&fh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, i), (struct val){i, i + 1, {}});
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->val, i + 1);
        if (i % 2)
        {
            CHECK(d->val % 2 == 0, true);
        }
        else
        {
            CHECK(d->val % 2, true);
        }
    }
    CHECK(size(&fh), (size / 2));
    return PASS;
}

static enum test_result
fhash_test_entry_api_macros(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    struct val vals[200];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d
            = FHM_OR_INSERT(FHM_ENTRY(&fh, i), fhash_create(i, i));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&fh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = FHM_OR_INSERT(FHM_AND_MODIFY(FHM_ENTRY(&fh, i), fhash_modplus),
                            fhash_create(i, i));
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        if (i % 2)
        {
            CHECK(d->val, i);
        }
        else
        {
            CHECK(d->val, i + 1);
        }
        CHECK(d->val % 2, true);
    }
    CHECK(size(&fh), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v = FHM_OR_INSERT(FHM_ENTRY(&fh, i), (struct val){0});
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(size(&fh), (size / 2));
    return PASS;
}

static enum test_result
fhash_test_two_sum(void)
{
    struct val vals[20];
    ccc_flat_hash_map fh;
    ccc_result const res
        = FHM_INIT(&fh, vals, sizeof(vals) / sizeof(vals[0]), struct val, id, e,
                   NULL, fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct val const *const other_addend
            = get_key_val(&fh, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_entry const e
            = try_insert(&fh, &(struct val){.id = addends[i], .val = i}.e);
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    return PASS;
}

static enum test_result
fhash_test_resize(void)
{
    size_t const prime_start = 5;
    struct val *vals = malloc(sizeof(struct val) * prime_start);
    CHECK(vals == NULL, false);
    ccc_flat_hash_map fh;
    ccc_result const res
        = FHM_INIT(&fh, vals, prime_start, struct val, id, e, realloc,
                   fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, ccc_impl_fhm_base(&fh.impl_));
    int const to_insert = 1000;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_vr(&fh, &elem.id), &elem.e);
        CHECK(v != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->id, shuffled_index, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->val, i, ccc_impl_fhm_base(&fh.impl_));
        CHECK(fhm_validate(&fh), true, ccc_impl_fhm_base(&fh.impl_));
    }
    CHECK(size(&fh), to_insert, ccc_impl_fhm_base(&fh.impl_));
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_vr(&fh, &swap_slot.id), &swap_slot.e);
        CHECK(in_table != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(in_table->val, shuffled_index, ccc_impl_fhm_base(&fh.impl_));
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK, ccc_impl_fhm_base(&fh.impl_));
    return PASS;
}

static enum test_result
fhash_test_resize_macros(void)
{
    size_t const prime_start = 5;
    struct val *vals = malloc(sizeof(struct val) * prime_start);
    CHECK(vals == NULL, false);
    ccc_flat_hash_map fh;
    ccc_result const res
        = FHM_INIT(&fh, vals, prime_start, struct val, id, e, realloc,
                   fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, ccc_impl_fhm_base(&fh.impl_));
    int const to_insert = 1000;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, shuffled_index),
                                         fhash_create(shuffled_index, i));
        CHECK(v != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->id, shuffled_index, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->val, i, ccc_impl_fhm_base(&fh.impl_));
    }
    CHECK(size(&fh), to_insert, ccc_impl_fhm_base(&fh.impl_));
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table
            = FHM_OR_INSERT(FHM_AND_MODIFY_W(FHM_ENTRY(&fh, shuffled_index),
                                             fhash_swap_val, shuffled_index),
                            (struct val){0});
        CHECK(in_table != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(in_table->val, shuffled_index, ccc_impl_fhm_base(&fh.impl_));
        struct val *v
            = FHM_OR_INSERT(FHM_ENTRY(&fh, shuffled_index), (struct val){0});
        CHECK(v == NULL, false);
        v->val = i;
        v = FHM_GET_KEY_VAL(&fh, shuffled_index);
        CHECK(v != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->val, i, ccc_impl_fhm_base(&fh.impl_));
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK, ccc_impl_fhm_base(&fh.impl_));
    return PASS;
}

static enum test_result
fhash_test_resize_from_null(void)
{
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, NULL, 0, struct val, id, e, realloc,
                                    fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, ccc_impl_fhm_base(&fh.impl_));
    int const to_insert = 1000;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_vr(&fh, &elem.id), &elem.e);
        CHECK(v != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->id, shuffled_index, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->val, i, ccc_impl_fhm_base(&fh.impl_));
    }
    CHECK(size(&fh), to_insert, ccc_impl_fhm_base(&fh.impl_));
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_vr(&fh, &swap_slot.id), &swap_slot.e);
        CHECK(in_table != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(in_table->val, shuffled_index, ccc_impl_fhm_base(&fh.impl_));
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK);
    return PASS;
}

static enum test_result
fhash_test_resize_from_null_macros(void)
{
    size_t const prime_start = 0;
    ccc_flat_hash_map fh;
    ccc_result const res
        = FHM_INIT(&fh, NULL, prime_start, struct val, id, e, realloc,
                   fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    int const to_insert = 1000;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, shuffled_index),
                                         fhash_create(shuffled_index, i));
        CHECK(v != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->id, shuffled_index, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->val, i, ccc_impl_fhm_base(&fh.impl_));
    }
    CHECK(size(&fh), to_insert, ccc_impl_fhm_base(&fh.impl_));
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table
            = FHM_OR_INSERT(FHM_AND_MODIFY_W(FHM_ENTRY(&fh, shuffled_index),
                                             fhash_swap_val, shuffled_index),
                            (struct val){0});
        CHECK(in_table != NULL, true, ccc_impl_fhm_base(&fh.impl_));
        CHECK(in_table->val, shuffled_index, ccc_impl_fhm_base(&fh.impl_));
        struct val *v
            = FHM_OR_INSERT(FHM_ENTRY(&fh, shuffled_index), (struct val){0});
        CHECK(v == NULL, false);
        v->val = i;
        v = FHM_GET_KEY_VAL(&fh, shuffled_index);
        CHECK(v == NULL, false, ccc_impl_fhm_base(&fh.impl_));
        CHECK(v->val, i, ccc_impl_fhm_base(&fh.impl_));
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK);
    return PASS;
}

static enum test_result
fhash_test_insert_limit(void)
{
    int const size = 101;
    struct val vals[101];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK);
    int const larger_prime = (int)fhm_next_prime(size);
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct val *v = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, shuffled_index),
                                         fhash_create(shuffled_index, i));
        if (!v)
        {
            break;
        }
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = size(&fh);
    /* The last successful entry is still in the table and is overwritten. */
    struct val v = {.id = last_index, .val = -1};
    ccc_entry ent = insert(&fh, &v.e, &(struct val){});
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(insert_error(&ent), false);
    CHECK(size(&fh), final_size);

    v = (struct val){.id = last_index, .val = -2};
    struct val *in_table = insert_entry(entry_vr(&fh, &v.id), &v.e);
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -2);
    CHECK(size(&fh), final_size);

    in_table = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, last_index),
                                (struct val){.id = last_index, .val = -3});
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -3);
    CHECK(size(&fh), final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.id = shuffled_index, .val = -4};
    in_table = insert_entry(entry_vr(&fh, &v.id), &v.e);
    CHECK(in_table == NULL, true);
    CHECK(size(&fh), final_size);

    in_table = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, shuffled_index),
                                (struct val){.id = shuffled_index, .val = -4});
    CHECK(in_table == NULL, true);
    CHECK(size(&fh), final_size);

    ent = insert(&fh, &v.e, &(struct val){});
    CHECK(unwrap(&ent) == NULL, true);
    CHECK(insert_error(&ent), true);
    CHECK(size(&fh), final_size);
    return PASS;
}
