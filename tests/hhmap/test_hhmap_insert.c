#define HANDLE_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "handle_hash_map.h"
#include "hhmap_util.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

CHECK_BEGIN_STATIC_FN(hhmap_test_insert)
{
    ccc_handle_hash_map hh = hhm_init((struct val[10]){}, 10, e, key, NULL,
                                      hhmap_int_zero, hhmap_id_eq, NULL);

    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = insert(&hh, &(struct val){.key = 137, .val = 99}.e);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&hh), 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_macros)
{
    ccc_handle_hash_map hh = hhm_init((struct val[10]){}, 10, e, key, NULL,
                                      hhmap_int_zero, hhmap_id_eq, NULL);

    ccc_handle h = ccc_hhm_or_insert_w(entry_r(&hh, &(int){2}),
                                       (struct val){.key = 2, .val = 0});
    struct val const *ins = hhm_at(&hh, h);
    CHECK(ins != NULL, true);
    CHECK(validate(&hh), true);
    CHECK(size(&hh), 1);
    h = hhm_insert_entry_w(entry_r(&hh, &(int){2}),
                           (struct val){.key = 2, .val = 0});
    ins = hhm_at(&hh, h);
    CHECK(validate(&hh), true);
    CHECK(ins != NULL, true);
    h = hhm_insert_entry_w(entry_r(&hh, &(int){9}),
                           (struct val){.key = 9, .val = 1});
    ins = hhm_at(&hh, h);
    CHECK(validate(&hh), true);
    CHECK(ins != NULL, true);
    ins = ccc_entry_unwrap(
        hhm_insert_or_assign_w(&hh, 3, (struct val){.val = 99}));
    ins = hhm_at(&hh, h);
    CHECK(validate(&hh), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&hh), true);
    CHECK(ins->val, 99);
    CHECK(size(&hh), 3);
    ins = ccc_entry_unwrap(
        hhm_insert_or_assign_w(&hh, 3, (struct val){.val = 98}));
    CHECK(validate(&hh), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&hh), 3);
    ins = ccc_entry_unwrap(hhm_try_insert_w(&hh, 3, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&hh), true);
    CHECK(ins->val, 98);
    CHECK(size(&hh), 3);
    ins = ccc_entry_unwrap(hhm_try_insert_w(&hh, 4, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&hh), true);
    CHECK(ins->val, 100);
    CHECK(size(&hh), 4);
    CHECK_END_FN(ccc_hhm_clear_and_free(&hh, NULL););
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_overwrite)
{
    ccc_handle_hash_map hh = hhm_init((struct val[10]){}, 10, e, key, NULL,
                                      hhmap_int_zero, hhmap_id_eq, NULL);

    struct val q = {.key = 137, .val = 99};
    ccc_entry ent = insert(&hh, &q.e);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);

    ccc_handle h = unwrap(entry_r(&hh, &q.key));
    struct val const *v = hhm_at(&hh, h);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_entry old_ent = insert(&hh, &q.e);
    CHECK(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(q.val, 99);
    h = unwrap(entry_r(&hh, &q.key));
    v = hhm_at(&hh, h);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_then_bad_ideas)
{
    ccc_handle_hash_map hh = hhm_init((struct val[10]){}, 10, e, key, NULL,
                                      hhmap_int_zero, hhmap_id_eq, NULL);
    struct val q = {.key = 137, .val = 99};
    ccc_entry ent = insert(&hh, &q.e);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    struct val const *v = unwrap(entry_r(&hh, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    q = (struct val){.key = 137, .val = 100};

    ent = insert(&hh, &q.e);
    CHECK(occupied(&ent), true);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(q.val, 99);
    q.val -= 9;

    v = get_key_val(&hh, &q.key);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ccc_handle_hash_map hh = hhm_init((struct val[200]){}, 200, e, key, NULL,
                                      hhmap_int_last_digit, hhmap_id_eq, NULL);
    size_t const size = 200;

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d = or_insert(entry_r(&hh, &def.key), &def.e);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d = or_insert(
            and_modify(entry_r(&hh, &def.key), hhmap_modplus), &def.e);
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->key, i);
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
    CHECK(size(&hh), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val *const in = or_insert(entry_r(&hh, &def.key), &def.e);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&hh), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_handle_hash_map hh = hhm_init((struct val[200]){}, 200, e, key, NULL,
                                      hhmap_int_last_digit, hhmap_id_eq, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d
            = insert_entry(entry_r(&hh, &def.key), &def.e);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = insert_entry(entry_r(&hh, &def.key), &def.e);
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
    CHECK(size(&hh), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_handle_hash_map hh = hhm_init((struct val[200]){}, 200, e, key, NULL,
                                      hhmap_int_last_digit, hhmap_id_eq, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = insert_entry(entry_r(&hh, &i), &(struct val){i, i, {}}.e);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = insert_entry(entry_r(&hh, &i), &(struct val){i, i + 1, {}}.e);
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
    CHECK(size(&hh), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    ccc_handle_hash_map hh = hhm_init((struct val[200]){}, 200, e, key, NULL,
                                      hhmap_int_last_digit, hhmap_id_eq, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d
            = hhm_or_insert_w(entry_r(&hh, &i), hhmap_create(i, i));
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = hhm_or_insert_w(
            and_modify(entry_r(&hh, &i), hhmap_modplus), hhmap_create(i, i));
        /* All values in the array should be odd now */
        CHECK((d != NULL), true);
        CHECK(d->key, i);
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
    CHECK(size(&hh), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v = hhm_or_insert_w(entry_r(&hh, &i), (struct val){});
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(size(&hh), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_two_sum)
{
    ccc_handle_hash_map hh = hhm_init((struct val[20]){}, 20, e, key, NULL,
                                      hhmap_int_to_u64, hhmap_id_eq, NULL);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct val const *const other_addend
            = get_key_val(&hh, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_entry const e = insert_or_assign(
            &hh, &(struct val){.key = addends[i], .val = i}.e);
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_resize)
{
    size_t const prime_start = 11;
    ccc_handle_hash_map hh = hhm_init(
        (struct val *)malloc(sizeof(struct val) * prime_start), prime_start, e,
        key, std_alloc, hhmap_int_to_u64, hhmap_id_eq, NULL);
    CHECK(hhm_data(&hh) != NULL, true);

    int const to_insert = 1000;
    int const larger_prime = (int)hhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&hh, &elem.key), &elem.e);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&hh), true);
    }
    CHECK(size(&hh), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_r(&hh, &swap_slot.key), &swap_slot.e);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(hhm_clear_and_free(&hh, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_resize_macros)
{
    size_t const prime_start = 11;
    ccc_handle_hash_map hh = hhm_init(
        (struct val *)malloc(sizeof(struct val) * prime_start), prime_start, e,
        key, std_alloc, hhmap_int_to_u64, hhmap_id_eq, NULL);
    CHECK(hhm_data(&hh) != NULL, true);
    int const to_insert = 1000;
    int const larger_prime = (int)hhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&hh, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.e);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hh), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = hhm_or_insert_w(
            hhm_and_modify_w(entry_r(&hh, &shuffled_index), struct val,
                             {
                                 T->val = shuffled_index;
                             }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = hhm_or_insert_w(entry_r(&hh, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&hh, &shuffled_index);
        CHECK(v != NULL, true);
        CHECK(v->val, i);
    }
    CHECK(hhm_clear_and_free(&hh, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_resize_from_null)
{
    ccc_handle_hash_map hh = hhm_init((struct val *)NULL, 0, e, key, std_alloc,
                                      hhmap_int_to_u64, hhmap_id_eq, NULL);
    int const to_insert = 1000;
    int const larger_prime = (int)hhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&hh, &elem.key), &elem.e);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hh), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_r(&hh, &swap_slot.key), &swap_slot.e);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(hhm_clear_and_free(&hh, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_resize_from_null_macros)
{
    ccc_handle_hash_map hh = hhm_init((struct val *)NULL, 0, e, key, std_alloc,
                                      hhmap_int_to_u64, hhmap_id_eq, NULL);
    int const to_insert = 1000;
    int const larger_prime = (int)hhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&hh, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.e);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hh), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = hhm_or_insert_w(
            hhm_and_modify_w(entry_r(&hh, &shuffled_index), struct val,
                             {
                                 T->val = shuffled_index;
                             }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = hhm_or_insert_w(entry_r(&hh, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&hh, &shuffled_index);
        CHECK(v == NULL, false);
        CHECK(v->val, i);
    }
    CHECK(hhm_clear_and_free(&hh, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_limit)
{
    int const size = 101;
    ccc_handle_hash_map hh = hhm_init((struct val[101]){}, 101, e, key, NULL,
                                      hhmap_int_to_u64, hhmap_id_eq, NULL);

    int const larger_prime = (int)hhm_next_prime(size);
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct val *v = insert_entry(entry_r(&hh, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.e);
        if (!v)
        {
            break;
        }
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = size(&hh);
    /* The last successful entry is still in the table and is overwritten. */
    struct val v = {.key = last_index, .val = -1};
    ccc_entry ent = insert(&hh, &v.e);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(insert_error(&ent), false);
    CHECK(size(&hh), final_size);

    v = (struct val){.key = last_index, .val = -2};
    struct val *in_table = insert_entry(entry_r(&hh, &v.key), &v.e);
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -2);
    CHECK(size(&hh), final_size);

    in_table = insert_entry(entry_r(&hh, &last_index),
                            &(struct val){.key = last_index, .val = -3}.e);
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -3);
    CHECK(size(&hh), final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.key = shuffled_index, .val = -4};
    in_table = insert_entry(entry_r(&hh, &v.key), &v.e);
    CHECK(in_table == NULL, true);
    CHECK(size(&hh), final_size);

    in_table = insert_entry(entry_r(&hh, &shuffled_index),
                            &(struct val){.key = shuffled_index, .val = -4}.e);
    CHECK(in_table == NULL, true);
    CHECK(size(&hh), final_size);

    ent = insert(&hh, &v.e);
    CHECK(unwrap(&ent) == NULL, true);
    CHECK(insert_error(&ent), true);
    CHECK(size(&hh), final_size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_and_find)
{
    int const size = 101;
    ccc_handle_hash_map hh = hhm_init((struct val[101]){}, 101, e, key, NULL,
                                      hhmap_int_to_u64, hhmap_id_eq, NULL);

    for (int i = 0; i < size; i += 2)
    {
        ccc_entry e = try_insert(&hh, &(struct val){.key = i, .val = i}.e);
        CHECK(occupied(&e), false);
        CHECK(validate(&hh), true);
        e = try_insert(&hh, &(struct val){.key = i, .val = i}.e);
        CHECK(occupied(&e), true);
        CHECK(validate(&hh), true);
        struct val const *const v = unwrap(&e);
        CHECK(v == NULL, false);
        CHECK(v->key, i);
        CHECK(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&hh, &i), true);
        CHECK(occupied(entry_r(&hh, &i)), true);
        CHECK(validate(&hh), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&hh, &i), false);
        CHECK(occupied(entry_r(&hh, &i)), false);
        CHECK(validate(&hh), true);
    }
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        hhmap_test_insert(), hhmap_test_insert_macros(),
        hhmap_test_insert_and_find(), hhmap_test_insert_overwrite(),
        hhmap_test_insert_then_bad_ideas(), hhmap_test_insert_via_entry(),
        hhmap_test_insert_via_entry_macros(), hhmap_test_entry_api_functional(),
        hhmap_test_entry_api_macros(), hhmap_test_two_sum(),
        hhmap_test_resize(), hhmap_test_resize_macros(),
        hhmap_test_resize_from_null(), hhmap_test_resize_from_null_macros(),
        hhmap_test_insert_limit());
}
