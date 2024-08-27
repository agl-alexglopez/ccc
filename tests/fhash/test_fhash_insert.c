#include "fhash_util.h"
#include "flat_hash.h"
#include "test.h"
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

#define NUM_TESTS (size_t)12
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
};

static void mod(ccc_update);
static struct val create(int id, int val);
static void swap_val(ccc_update u);

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
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val query = {.id = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_fhash_entry ent = ccc_fh_insert(&fh, &query.id, &query.e);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK(ccc_fh_get(ent), NULL, "%p");
    return PASS;
}

static enum test_result
fhash_test_insert_overwrite(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val q = {.id = 137, .val = 99};
    ccc_fhash_entry ent = ccc_fh_insert(&fh, &q.id, &q.e);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK(ccc_fh_get(ent), NULL, "%p");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 99, "%d");

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_fhash_entry old_ent = ccc_fh_insert(&fh, &q.id, &q.e);
    CHECK(ccc_fh_occupied(old_ent), true, "%d");

    /* The old contents are now in q and the entry is in the table. */
    CHECK(((struct val *)ccc_fh_get(old_ent))->val, 100, "%d");
    CHECK(q.val, 99, "%d");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 100, "%d");
    return PASS;
}

static enum test_result
fhash_test_insert_then_bad_ideas(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val q = {.id = 137, .val = 99};
    ccc_fhash_entry ent = ccc_fh_insert(&fh, &q.id, &q.e);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK(ccc_fh_get(ent), NULL, "%p");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 99, "%d");

    /* This is a dummy entry that indicates the entry was vacant in the table.
       so or insert and erase will do nothing. */
    CHECK(ccc_fh_or_insert(ent, &q.e), NULL, "%p");

    q = (struct val){.id = 137, .val = 100};

    ccc_fhash_entry new_ent = ccc_fh_insert(&fh, &q.id, &q.e);
    CHECK(ccc_fh_occupied(new_ent), true, "%d");
    CHECK(((struct val *)ccc_fh_get(new_ent))->val, 100, "%d");
    CHECK(q.val, 99, "%d");
    q.val -= 9;

    /* Now the expected behavior of or insert shall occur and no insertion
       will happen because the value is already occupied in the table. */
    CHECK(((struct val *)ccc_fh_or_insert(new_ent, &q.e))->val, 100, "%d");
    CHECK(((struct val *)ccc_fh_get(ccc_fh_entry(&fh, &q.id)))->val, 100, "%d");
    CHECK(q.val, 90, "%d");
    return PASS;
}

static enum test_result
fhash_test_entry_api_functional(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[size];
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, size, struct val, id, e, NULL,
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
            = ccc_fh_or_insert(ccc_fh_entry(&fh, &def.id), &def.e);
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    CHECK(ccc_fh_size(&fh), (size / 2) / 2, "%zu");
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d = ccc_fh_or_insert(
            ccc_fh_and_modify(ccc_fh_entry(&fh, &def.id), mod), &def.e);
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
    CHECK(ccc_fh_size(&fh), (size / 2), "%zu");
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val *const in
            = ccc_fh_or_insert(ccc_fh_entry(&fh, &def.id), &def.e);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true, "%d");
    }
    CHECK(ccc_fh_size(&fh), (size / 2), "%zu");
    return PASS;
}

static enum test_result
fhash_test_insert_via_entry(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[size];
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, size, struct val, id, e, NULL,
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
            = ccc_fh_insert_entry(ccc_fh_entry(&fh, &def.id), &def.e);
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    CHECK(ccc_fh_size(&fh), (size / 2) / 2, "%zu");
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = ccc_fh_insert_entry(ccc_fh_entry(&fh, &def.id), &def.e);
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
    CHECK(ccc_fh_size(&fh), (size / 2), "%zu");
    return PASS;
}

static enum test_result
fhash_test_insert_via_entry_macros(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    struct val vals[size];
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, size, struct val, id, e, NULL,
                                       fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = INSERT_ENTRY(ENTRY(&fh, i), (struct val){i, i, {}});
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    CHECK(ccc_fh_size(&fh), (size / 2) / 2, "%zu");
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = INSERT_ENTRY(ENTRY(&fh, i), (struct val){i, i + 1, {}});
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
    CHECK(ccc_fh_size(&fh), (size / 2), "%zu");
    return PASS;
}

static enum test_result
fhash_test_entry_api_macros(void)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    struct val vals[size];
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, size, struct val, id, e, NULL,
                                       fhash_int_last_digit, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d = OR_INSERT(ENTRY(&fh, i), create(i, i));
        CHECK((d != NULL), true, "%d");
        CHECK(d->id, i, "%d");
        CHECK(d->val, i, "%d");
    }
    CHECK(ccc_fh_size(&fh), (size / 2) / 2, "%zu");
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = OR_INSERT(AND_MODIFY(ENTRY(&fh, i), mod), create(i, i));
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
    CHECK(ccc_fh_size(&fh), (size / 2), "%zu");
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        OR_INSERT(ENTRY(&fh, i), (struct val){0})->val++;
        /* All values in the array should be odd now */
        CHECK(OR_INSERT(ENTRY(&fh, i), (struct val){0})->val % 2 == 0, true,
              "%d");
    }
    CHECK(ccc_fh_size(&fh), (size / 2), "%zu");
    return PASS;
}

static enum test_result
fhash_test_two_sum(void)
{
    size_t const size = 50;
    struct val vals[size];
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, size, struct val, id, e, NULL,
                                       fhash_id_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const addends[10] = {1, 3, 5, 6, 7, 13, 44, 32, 10, -1};
    int const target = 15;
    int const correct[2] = {8, 2};
    int indices[2];
    for (int i = 0; i < 10; ++i)
    {
        struct val const *const v = GET(ENTRY(&fh, target - addends[i]));
        if (v)
        {
            indices[0] = i;
            indices[1] = v->val;
            break;
        }
        INSERT_ENTRY(ENTRY(&fh, addends[i]),
                     (struct val){.id = addends[i], .val = i});
    }
    CHECK(ccc_fh_size(&fh), indices[0], "%zu");
    CHECK(indices[0], correct[0], "%d");
    CHECK(indices[1], correct[1], "%d");
    return PASS;
}

static enum test_result
fhash_test_resize(void)
{
    size_t const prime_start = 5;
    struct val *vals = malloc(sizeof(struct val) * prime_start);
    CHECK(vals != NULL, true, "%d");
    ccc_fhash fh;
    ccc_result const res
        = CCC_FH_INIT(&fh, vals, prime_start, struct val, id, e, realloc,
                      fhash_id_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const to_insert = 1000;
    int const larger_prime = (int)ccc_fh_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = ccc_fh_insert_entry(ccc_fh_entry(&fh, &elem.id), &elem.e);
        CHECK(v->id, shuffled_index, "%d");
        CHECK(v->val, i, "%d");
    }
    CHECK(ccc_fh_size(&fh), to_insert, "%zu");
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = ccc_fh_get(ccc_fh_insert(&fh, &swap_slot.id, &swap_slot.e));
        CHECK(in_table != NULL, true, "%d");
        CHECK(in_table->val, shuffled_index, "%d");
        CHECK(swap_slot.val, i, "%d");
    }
    CHECK(ccc_fh_clear_and_free(&fh, NULL), CCC_OK, "%d");
    return PASS;
}

static enum test_result
fhash_test_resize_macros(void)
{
    size_t const prime_start = 5;
    struct val *vals = malloc(sizeof(struct val) * prime_start);
    CHECK(vals != NULL, true, "%d");
    ccc_fhash fh;
    ccc_result const res
        = CCC_FH_INIT(&fh, vals, prime_start, struct val, id, e, realloc,
                      fhash_id_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const to_insert = 1000;
    int const larger_prime = (int)ccc_fh_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = INSERT_ENTRY(ENTRY(&fh, shuffled_index),
                                     create(shuffled_index, i));
        CHECK(v->id, shuffled_index, "%d");
        CHECK(v->val, i, "%d");
    }
    CHECK(ccc_fh_size(&fh), to_insert, "%zu");
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table
            = OR_INSERT(AND_MODIFY_WITH(ENTRY(&fh, shuffled_index), swap_val,
                                        shuffled_index),
                        (struct val){0});
        CHECK(in_table != NULL, true, "%d");
        CHECK(in_table->val, shuffled_index, "%d");
        OR_INSERT(ENTRY(&fh, shuffled_index), (struct val){0})->val = i;
        struct val const *v = GET(ENTRY(&fh, shuffled_index));
        CHECK(v->val, i, "%d");
    }
    CHECK(ccc_fh_clear_and_free(&fh, NULL), CCC_OK, "%d");
    return PASS;
}

static enum test_result
fhash_test_resize_from_null(void)
{
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, NULL, 0, struct val, id, e, realloc,
                                       fhash_id_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const to_insert = 1000;
    int const larger_prime = (int)ccc_fh_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = ccc_fh_insert_entry(ccc_fh_entry(&fh, &elem.id), &elem.e);
        CHECK(v->id, shuffled_index, "%d");
        CHECK(v->val, i, "%d");
    }
    CHECK(ccc_fh_size(&fh), to_insert, "%zu");
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = ccc_fh_get(ccc_fh_insert(&fh, &swap_slot.id, &swap_slot.e));
        CHECK(in_table != NULL, true, "%d");
        CHECK(in_table->val, shuffled_index, "%d");
        CHECK(swap_slot.val, i, "%d");
    }
    CHECK(ccc_fh_clear_and_free(&fh, NULL), CCC_OK, "%d");
    return PASS;
}

static enum test_result
fhash_test_resize_from_null_macros(void)
{
    size_t const prime_start = 0;
    ccc_fhash fh;
    ccc_result const res
        = CCC_FH_INIT(&fh, NULL, prime_start, struct val, id, e, realloc,
                      fhash_id_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const to_insert = 1000;
    int const larger_prime = (int)ccc_fh_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = INSERT_ENTRY(ENTRY(&fh, shuffled_index),
                                     create(shuffled_index, i));
        CHECK(v->id, shuffled_index, "%d");
        CHECK(v->val, i, "%d");
    }
    CHECK(ccc_fh_size(&fh), to_insert, "%zu");
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table
            = OR_INSERT(AND_MODIFY_WITH(ENTRY(&fh, shuffled_index), swap_val,
                                        shuffled_index),
                        (struct val){0});
        CHECK(in_table != NULL, true, "%d");
        CHECK(in_table->val, shuffled_index, "%d");
        OR_INSERT(ENTRY(&fh, shuffled_index), (struct val){0})->val = i;
        struct val *v = GET_MUT(ENTRY(&fh, shuffled_index));
        CHECK(v->val, i, "%d");
    }
    CHECK(ccc_fh_clear_and_free(&fh, NULL), CCC_OK, "%d");
    return PASS;
}

static inline void
swap_val(ccc_update u)
{
    struct val *v = u.container;
    v->val = *((int *)u.aux);
}

static void
mod(ccc_update const mod)
{
    ((struct val *)mod.container)->val++;
}

static struct val
create(int id, int val)
{
    return (struct val){.id = id, .val = val};
}
