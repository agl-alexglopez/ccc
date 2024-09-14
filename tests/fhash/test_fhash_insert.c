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
    CHECK(res, CCC_OK, "%d");
    struct val query = {.id = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = insert(&fh, &query.e);
    CHECK(occupied(&ent), false, "%d");
    CHECK(unwrap(&ent), NULL, "%p");
    CHECK(size(&fh), 1, "%zu");
    return PASS;
}

static enum test_result
fhash_test_insert_overwrite(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val q = {.id = 137, .val = 99};
    ccc_entry ent = insert(&fh, &q.e);
    CHECK(occupied(&ent), false, "%d");
    CHECK(unwrap(&ent), NULL, "%p");

    struct val const *v = unwrap(entry_vr(&fh, &q.id));
    CHECK(v != NULL, true, "%d");
    CHECK(v->val, 99, "%d");

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_entry old_ent = insert(&fh, &q.e);
    CHECK(occupied(&old_ent), true, "%d");

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    CHECK(v != NULL, true, "%d");
    CHECK(v->val, 99, "%d");
    CHECK(q.val, 99, "%d");
    v = unwrap(entry_vr(&fh, &q.id));
    CHECK(v != NULL, true, "%d");
    CHECK(v->val, 100, "%d");
    return PASS;
}

static enum test_result
fhash_test_insert_then_bad_ideas(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                    fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val q = {.id = 137, .val = 99};
    ccc_entry ent = insert(&fh, &q.e);
    CHECK(occupied(&ent), false, "%d");
    CHECK(unwrap(&ent), NULL, "%p");
    struct val const *v = unwrap(entry_vr(&fh, &q.id));
    CHECK(v != NULL, true, "%d");
    CHECK(v->val, 99, "%d");

    q = (struct val){.id = 137, .val = 100};

    ent = insert(&fh, &q.e);
    CHECK(occupied(&ent), true, "%d");
    v = unwrap(&ent);
    CHECK(v != NULL, true, "%d");
    CHECK(v->val, 99, "%d");
    CHECK(q.val, 99, "%d");
    q.val -= 9;

    v = get_key_val(&fh, &q.id);
    CHECK(v != NULL, true, "%d");
    CHECK(v->val, 100, "%d");
    CHECK(q.val, 90, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_api_functional(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[size];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d = or_insert(entry_vr(&fh, &def.id), &def.e);
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    CHECK(size(&fh), (size / 2) / 2, "%zu");
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d = or_insert(
            and_modify(entry_vr(&fh, &def.id), fhash_modplus), &def.e);
        /* All values in the array should be odd now */
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        if (i % 2)
        {
            CHECK(d->val, i, "%d");
        }
        else
        {
            CHECK(d->val, i + 1, "%d");
        }
        CHECK(d->val % 2, true, "%d");
    }
    CHECK(size(&fh), (size / 2), "%zu");
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val *const in = or_insert(entry_vr(&fh, &def.id), &def.e);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true, "%d");
    }
    CHECK(size(&fh), (size / 2), "%zu");
    return PASS;
}

static enum test_result
fhash_test_insert_via_entry(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[size];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
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
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    CHECK(size(&fh), (size / 2) / 2, "%zu");
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = insert_entry(entry_vr(&fh, &def.id), &def.e);
        /* All values in the array should be odd now */
        CHECK((d != NULL), true, "%d");
        CHECK(d->val, i + 1, "%d");
        if (i % 2)
        {
            CHECK(d->val % 2 == 0, true, "%d");
        }
        else
        {
            CHECK(d->val % 2, true, "%d");
        }
    }
    CHECK(size(&fh), (size / 2), "%zu");
    return PASS;
}

static enum test_result
fhash_test_insert_via_entry_macros(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[size];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, i), (struct val){i, i, {}});
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    CHECK(size(&fh), (size / 2) / 2, "%zu");
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, i), (struct val){i, i + 1, {}});
        /* All values in the array should be odd now */
        CHECK((d != NULL), true, "%d");
        CHECK(d->val, i + 1, "%d");
        if (i % 2)
        {
            CHECK(d->val % 2 == 0, true, "%d");
        }
        else
        {
            CHECK(d->val % 2, true, "%d");
        }
    }
    CHECK(size(&fh), (size / 2), "%zu");
    return PASS;
}

static enum test_result
fhash_test_entry_api_macros(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    struct val vals[size];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d
            = FHM_OR_INSERT(FHM_ENTRY(&fh, i), fhash_create(i, i));
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    CHECK(size(&fh), (size / 2) / 2, "%zu");
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = FHM_OR_INSERT(FHM_AND_MODIFY(FHM_ENTRY(&fh, i), fhash_modplus),
                            fhash_create(i, i));
        /* All values in the array should be odd now */
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        if (i % 2)
        {
            CHECK(d->val, i, "%d");
        }
        else
        {
            CHECK(d->val, i + 1, "%d");
        }
        CHECK(d->val % 2, true, "%d");
    }
    CHECK(size(&fh), (size / 2), "%zu");
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v = FHM_OR_INSERT(FHM_ENTRY(&fh, i), (struct val){0});
        CHECK(v != NULL, true, "%d");
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true, "%d");
    }
    CHECK(size(&fh), (size / 2), "%zu");
    return PASS;
}

static enum test_result
fhash_test_two_sum(void)
{
    size_t const size = 50;
    struct val vals[size];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const addends[10] = {1, 3, 5, 6, 7, 13, 44, 32, 10, -1};
    int const target = 15;
    int const correct[2] = {8, 2};
    int indices[2] = {-1, -1};
    for (int i = 0; i < 10; ++i)
    {
        struct val const *const v = FHM_GET_KEY_VAL(&fh, target - addends[i]);
        if (v)
        {
            indices[0] = i;
            indices[1] = v->val;
            break;
        }
        FHM_INSERT_ENTRY(FHM_ENTRY(&fh, addends[i]),
                         (struct val){.id = addends[i], .val = i});
    }
    CHECK(size(&fh), indices[0], "%zu");
    CHECK(indices[0], correct[0], "%d");
    CHECK(indices[1], correct[1], "%d");
    return PASS;
}

static enum test_result
fhash_test_resize(void)
{
    size_t const prime_start = 5;
    struct val *vals = malloc(sizeof(struct val) * prime_start);
    CHECK(vals == NULL, false, "%d");
    ccc_flat_hash_map fh;
    ccc_result const res
        = FHM_INIT(&fh, vals, prime_start, struct val, id, e, realloc,
                   fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d", vals);
    int const to_insert = 1000;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_vr(&fh, &elem.id), &elem.e);
        CHECK(v != NULL, true, "%d", vals);
        CHECK(v->id, shuffled_index, "%d", vals);
        CHECK(v->val, i, "%d", vals);
        CHECK(fhm_validate(&fh), true, "%d", vals);
    }
    CHECK(size(&fh), to_insert, "%zu", vals);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_vr(&fh, &swap_slot.id), &swap_slot.e);
        CHECK(in_table != NULL, true, "%d", vals);
        CHECK(in_table->val, shuffled_index, "%d", vals);
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK, "%d", vals);
    return PASS;
}

static enum test_result
fhash_test_resize_macros(void)
{
    size_t const prime_start = 5;
    struct val *vals = malloc(sizeof(struct val) * prime_start);
    CHECK(vals == NULL, false, "%d");
    ccc_flat_hash_map fh;
    ccc_result const res
        = FHM_INIT(&fh, vals, prime_start, struct val, id, e, realloc,
                   fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d", vals);
    int const to_insert = 1000;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, shuffled_index),
                                         fhash_create(shuffled_index, i));
        CHECK(v != NULL, true, "%d", vals);
        CHECK(v->id, shuffled_index, "%d", vals);
        CHECK(v->val, i, "%d", vals);
    }
    CHECK(size(&fh), to_insert, "%zu", vals);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table
            = FHM_OR_INSERT(FHM_AND_MODIFY_W(FHM_ENTRY(&fh, shuffled_index),
                                             fhash_swap_val, shuffled_index),
                            (struct val){0});
        CHECK(in_table != NULL, true, "%d", vals);
        CHECK(in_table->val, shuffled_index, "%d", vals);
        struct val *v
            = FHM_OR_INSERT(FHM_ENTRY(&fh, shuffled_index), (struct val){0});
        CHECK(v == NULL, false, "%d");
        v->val = i;
        v = FHM_GET_KEY_VAL(&fh, shuffled_index);
        CHECK(v != NULL, true, "%d", vals);
        CHECK(v->val, i, "%d", vals);
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK, "%d", vals);
    return PASS;
}

static enum test_result
fhash_test_resize_from_null(void)
{
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, NULL, 0, struct val, id, e, realloc,
                                    fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d", ccc_buf_base(&fh.impl_.buf_));
    int const to_insert = 1000;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_vr(&fh, &elem.id), &elem.e);
        CHECK(v != NULL, true, "%d", ccc_buf_base(&fh.impl_.buf_));
        CHECK(v->id, shuffled_index, "%d", ccc_buf_base(&fh.impl_.buf_));
        CHECK(v->val, i, "%d", ccc_buf_base(&fh.impl_.buf_));
    }
    CHECK(size(&fh), to_insert, "%zu", ccc_buf_base(&fh.impl_.buf_));
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_vr(&fh, &swap_slot.id), &swap_slot.e);
        CHECK(in_table != NULL, true, "%d", ccc_buf_base(&fh.impl_.buf_));
        CHECK(in_table->val, shuffled_index, "%d",
              ccc_buf_base(&fh.impl_.buf_));
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK, "%d");
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
    CHECK(res, CCC_OK, "%d");
    int const to_insert = 1000;
    int const larger_prime = (int)fhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, shuffled_index),
                                         fhash_create(shuffled_index, i));
        CHECK(v != NULL, true, "%d", ccc_buf_base(&fh.impl_.buf_));
        CHECK(v->id, shuffled_index, "%d", ccc_buf_base(&fh.impl_.buf_));
        CHECK(v->val, i, "%d", ccc_buf_base(&fh.impl_.buf_));
    }
    CHECK(size(&fh), to_insert, "%zu", ccc_buf_base(&fh.impl_.buf_));
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table
            = FHM_OR_INSERT(FHM_AND_MODIFY_W(FHM_ENTRY(&fh, shuffled_index),
                                             fhash_swap_val, shuffled_index),
                            (struct val){0});
        CHECK(in_table != NULL, true, "%d", ccc_buf_base(&fh.impl_.buf_));
        CHECK(in_table->val, shuffled_index, "%d",
              ccc_buf_base(&fh.impl_.buf_));
        struct val *v
            = FHM_OR_INSERT(FHM_ENTRY(&fh, shuffled_index), (struct val){0});
        CHECK(v == NULL, false, "%d");
        v->val = i;
        v = FHM_GET_KEY_VAL(&fh, shuffled_index);
        CHECK(v == NULL, false, "%d", ccc_buf_base(&fh.impl_.buf_));
        CHECK(v->val, i, "%d", ccc_buf_base(&fh.impl_.buf_));
    }
    CHECK(fhm_clear_and_free(&fh, NULL), CCC_OK, "%d");
    return PASS;
}

static enum test_result
fhash_test_insert_limit(void)
{
    int const size = 101;
    struct val vals[size];
    ccc_flat_hash_map fh;
    ccc_result const res = FHM_INIT(&fh, vals, size, struct val, id, e, NULL,
                                    fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
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
        CHECK(v->id, shuffled_index, "%d");
        CHECK(v->val, i, "%d");
        last_index = shuffled_index;
    }
    size_t const final_size = size(&fh);
    /* The last successful entry is still in the table and is overwritten. */
    struct val v = {.id = last_index, .val = -1};
    ccc_entry ent = insert(&fh, &v.e);
    CHECK(unwrap(&ent) != NULL, true, "%d");
    CHECK(insert_error(&ent), false, "%d");
    CHECK(size(&fh), final_size, "%zu");

    v = (struct val){.id = last_index, .val = -2};
    struct val *in_table = insert_entry(entry_vr(&fh, &v.id), &v.e);
    CHECK(in_table != NULL, true, "%d");
    CHECK(in_table->val, -2, "%d");
    CHECK(size(&fh), final_size, "%zu");

    in_table = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, last_index),
                                (struct val){.id = last_index, .val = -3});
    CHECK(in_table != NULL, true, "%d");
    CHECK(in_table->val, -3, "%d");
    CHECK(size(&fh), final_size, "%zu");

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.id = shuffled_index, .val = -4};
    in_table = insert_entry(entry_vr(&fh, &v.id), &v.e);
    CHECK(in_table == NULL, true, "%d");
    CHECK(size(&fh), final_size, "%zu");

    in_table = FHM_INSERT_ENTRY(FHM_ENTRY(&fh, shuffled_index),
                                (struct val){.id = shuffled_index, .val = -4});
    CHECK(in_table == NULL, true, "%d");
    CHECK(size(&fh), final_size, "%zu");

    ent = insert(&fh, &v.e);
    CHECK(unwrap(&ent) == NULL, true, "%d");
    CHECK(insert_error(&ent), true, "%d");
    CHECK(size(&fh), final_size, "%zu");
    return PASS;
}
