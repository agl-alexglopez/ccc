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

    /* Nothing was there before so nothing is in the handle. */
    ccc_handle ent = insert(&hh, &(struct val){.key = 137, .val = 99}.e);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(size(&hh), 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_macros)
{
    ccc_handle_hash_map hh = hhm_init((struct val[10]){}, 10, e, key, NULL,
                                      hhmap_int_zero, hhmap_id_eq, NULL);

    ccc_handle_i h = ccc_hhm_or_insert_w(handle_r(&hh, &(int){2}),
                                         (struct val){.key = 2, .val = 0});
    struct val const *ins = hhm_at(&hh, h);
    CHECK(ins != NULL, true);
    CHECK(validate(&hh), true);
    CHECK(size(&hh), 1);
    h = hhm_insert_handle_w(handle_r(&hh, &(int){2}),
                            (struct val){.key = 2, .val = 0});
    ins = hhm_at(&hh, h);
    CHECK(validate(&hh), true);
    CHECK(ins != NULL, true);
    h = hhm_insert_handle_w(handle_r(&hh, &(int){9}),
                            (struct val){.key = 9, .val = 1});
    ins = hhm_at(&hh, h);
    CHECK(validate(&hh), true);
    CHECK(ins != NULL, true);
    h = ccc_handle_unwrap(
        hhm_insert_or_assign_w(&hh, 3, (struct val){.val = 99}));
    ins = hhm_at(&hh, h);
    CHECK(validate(&hh), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&hh), true);
    CHECK(ins->val, 99);
    CHECK(size(&hh), 3);
    h = ccc_handle_unwrap(
        hhm_insert_or_assign_w(&hh, 3, (struct val){.val = 98}));
    ins = hhm_at(&hh, h);
    CHECK(validate(&hh), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&hh), 3);
    h = ccc_handle_unwrap(hhm_try_insert_w(&hh, 3, (struct val){.val = 100}));
    ins = hhm_at(&hh, h);
    CHECK(ins == NULL, false);
    CHECK(validate(&hh), true);
    CHECK(ins->val, 98);
    CHECK(size(&hh), 3);
    h = ccc_handle_unwrap(hhm_try_insert_w(&hh, 4, (struct val){.val = 100}));
    ins = hhm_at(&hh, h);
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
    ccc_handle ent = insert(&hh, &q.e);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);

    ccc_handle_i h = unwrap(handle_r(&hh, &q.key));
    struct val const *v = hhm_at(&hh, h);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    ent = insert(&hh, &q.e);
    CHECK(occupied(&ent), true);

    /* The old contents are now in q and the handle is in the table. */
    h = unwrap(&ent);
    v = hhm_at(&hh, h);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 99);
    h = unwrap(handle_r(&hh, &q.key));
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
    ccc_handle ent = insert(&hh, &q.e);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    ccc_handle_i h = unwrap(handle_r(&hh, &q.key));
    struct val const *v = hhm_at(&hh, h);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    q = (struct val){.key = 137, .val = 100};

    ent = insert(&hh, &q.e);
    CHECK(occupied(&ent), true);
    h = unwrap(&ent);
    v = hhm_at(&hh, h);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 99);
    q.val -= 9;

    h = get_key_val(&hh, &q.key);
    v = hhm_at(&hh, h);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_handle_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ccc_handle_hash_map hh = hhm_init((struct val[200]){}, 200, e, key, NULL,
                                      hhmap_int_last_digit, hhmap_id_eq, NULL);
    size_t const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        ccc_handle_i h = or_insert(handle_r(&hh, &def.key), &def.e);
        struct val const *const d = hhm_at(&hh, h);
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
        ccc_handle_i h = or_insert(
            and_modify(handle_r(&hh, &def.key), hhmap_modplus), &def.e);
        struct val const *const d = hhm_at(&hh, h);
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
        ccc_handle_i h = or_insert(handle_r(&hh, &def.key), &def.e);
        struct val *const in = hhm_at(&hh, h);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&hh), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_via_handle)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_handle_hash_map hh = hhm_init((struct val[200]){}, 200, e, key, NULL,
                                      hhmap_int_last_digit, hhmap_id_eq, NULL);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        ccc_handle_i h = insert_handle(handle_r(&hh, &def.key), &def.e);
        struct val const *const d = hhm_at(&hh, h);
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
        ccc_handle_i h = insert_handle(handle_r(&hh, &def.key), &def.e);
        struct val const *const d = hhm_at(&hh, h);
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

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_via_handle_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_handle_hash_map hh = hhm_init((struct val[200]){}, 200, e, key, NULL,
                                      hhmap_int_last_digit, hhmap_id_eq, NULL);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        ccc_handle_i h
            = insert_handle(handle_r(&hh, &i), &(struct val){i, {}, i}.e);
        struct val const *const d = hhm_at(&hh, h);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        ccc_handle_i h
            = insert_handle(handle_r(&hh, &i), &(struct val){i, {}, i + 1}.e);
        struct val const *const d = hhm_at(&hh, h);
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

CHECK_BEGIN_STATIC_FN(hhmap_test_handle_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    ccc_handle_hash_map hh = hhm_init((struct val[200]){}, 200, e, key, NULL,
                                      hhmap_int_last_digit, hhmap_id_eq, NULL);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        ccc_handle_i h = hhm_or_insert_w(handle_r(&hh, &i), hhmap_create(i, i));
        struct val const *const d = hhm_at(&hh, h);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hh), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        ccc_handle_i h = hhm_or_insert_w(
            and_modify(handle_r(&hh, &i), hhmap_modplus), hhmap_create(i, i));
        struct val const *const d = hhm_at(&hh, h);
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
        ccc_handle_i h = hhm_or_insert_w(handle_r(&hh, &i), (struct val){});
        struct val *v = hhm_at(&hh, h);
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
        ccc_handle_i h = get_key_val(&hh, &(int){target - addends[i]});
        struct val const *const other_addend = hhm_at(&hh, h);
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_handle const e = insert_or_assign(
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
    CHECK(size(&hh), 0);

    int const to_insert = 1000;
    int const larger_prime = (int)hhm_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        ccc_handle_i h = insert_handle(handle_r(&hh, &elem.key), &elem.e);
        CHECK(size(&hh), i + 1);
        struct val *v = hhm_at(&hh, h);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        bool const valid = validate(&hh);
        CHECK(valid, true);
        bool const contain = contains(&hh, &shuffled_index);
        CHECK(contain, true);
    }
    CHECK(size(&hh), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, {}, shuffled_index};
        bool const contain = contains(&hh, &shuffled_index);
        CHECK(contain, true);
        ccc_handle_i h
            = insert_handle(handle_r(&hh, &swap_slot.key), &swap_slot.e);
        struct val const *const in_table = hhm_at(&hh, h);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        CHECK(size(&hh), to_insert);
    }
    CHECK_END_FN(hhm_clear_and_free(&hh, NULL););
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
        ccc_handle_i h = insert_handle(handle_r(&hh, &shuffled_index),
                                       &(struct val){shuffled_index, {}, i}.e);
        struct val *v = hhm_at(&hh, h);
        bool const valid = validate(&hh);
        CHECK(valid, true);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        ccc_handle e = try_insert(&hh, &(struct val){.key = shuffled_index}.e);
        CHECK(occupied(&e), true);
        bool const contain = contains(&hh, &shuffled_index);
        CHECK(contain, true);
    }
    CHECK(size(&hh), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        bool const contain = contains(&hh, &shuffled_index);
        CHECK(contain, true);
    }
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        ccc_handle_i h = hhm_or_insert_w(
            hhm_and_modify_w(handle_r(&hh, &shuffled_index), struct val,
                             { T->val = shuffled_index; }),
            (struct val){});
        bool const valid = validate(&hh);
        CHECK(valid, true);
        struct val const *const in_table = hhm_at(&hh, h);
        CHECK(in_table != NULL, true);
        CHECK(in_table->key, shuffled_index);
        CHECK(in_table->val, shuffled_index);
        h = hhm_or_insert_w(handle_r(&hh, &shuffled_index), (struct val){});
        struct val *v = hhm_at(&hh, h);
        CHECK(v == NULL, false);
        v->val = i;
        h = get_key_val(&hh, &shuffled_index);
        v = hhm_at(&hh, h);
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
        ccc_handle_i h = insert_handle(handle_r(&hh, &elem.key), &elem.e);
        bool const valid = validate(&hh);
        CHECK(valid, true);
        struct val *v = hhm_at(&hh, h);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hh), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, {}, shuffled_index};
        ccc_handle_i h
            = insert_handle(handle_r(&hh, &swap_slot.key), &swap_slot.e);
        bool const valid = validate(&hh);
        CHECK(valid, true);
        struct val const *const in_table = hhm_at(&hh, h);
        CHECK(in_table != NULL, true);
        CHECK(in_table->key, shuffled_index);
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
        ccc_handle_i h = insert_handle(handle_r(&hh, &shuffled_index),
                                       &(struct val){shuffled_index, {}, i}.e);
        struct val *v = hhm_at(&hh, h);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hh), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        ccc_handle_i h = hhm_or_insert_w(
            hhm_and_modify_w(handle_r(&hh, &shuffled_index), struct val,
                             { T->val = shuffled_index; }),
            (struct val){});
        struct val const *const in_table = hhm_at(&hh, h);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        h = hhm_or_insert_w(handle_r(&hh, &shuffled_index), (struct val){});
        struct val *v = hhm_at(&hh, h);
        CHECK(v == NULL, false);
        v->val = i;
        h = get_key_val(&hh, &shuffled_index);
        v = hhm_at(&hh, h);
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
        ccc_handle_i h = insert_handle(handle_r(&hh, &shuffled_index),
                                       &(struct val){shuffled_index, {}, i}.e);
        struct val *v = hhm_at(&hh, h);
        if (!v)
        {
            break;
        }
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = size(&hh);
    /* The last successful handle is still in the table and is overwritten.
     */
    struct val v = {.key = last_index, .val = -1};
    ccc_handle ent = insert(&hh, &v.e);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(insert_error(&ent), false);
    CHECK(size(&hh), final_size);

    v = (struct val){.key = last_index, .val = -2};
    ccc_handle_i h = insert_handle(handle_r(&hh, &v.key), &v.e);
    struct val *in_table = hhm_at(&hh, h);
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -2);
    CHECK(size(&hh), final_size);

    h = insert_handle(handle_r(&hh, &last_index),
                      &(struct val){.key = last_index, .val = -3}.e);
    in_table = hhm_at(&hh, h);
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -3);
    CHECK(size(&hh), final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.key = shuffled_index, .val = -4};
    h = insert_handle(handle_r(&hh, &v.key), &v.e);
    in_table = hhm_at(&hh, h);
    CHECK(in_table == NULL, true);
    CHECK(size(&hh), final_size);

    h = insert_handle(handle_r(&hh, &shuffled_index),
                      &(struct val){.key = shuffled_index, .val = -4}.e);
    in_table = hhm_at(&hh, h);
    CHECK(in_table == NULL, true);
    CHECK(size(&hh), final_size);

    ent = insert(&hh, &v.e);
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
        ccc_handle e = try_insert(&hh, &(struct val){.key = i, .val = i}.e);
        CHECK(occupied(&e), false);
        bool valid = validate(&hh);
        CHECK(valid, true);
        struct val v = {.key = i, .val = i};
        e = try_insert(&hh, &v.e);
        CHECK(occupied(&e), true);
        valid = validate(&hh);
        CHECK(valid, true);
        CHECK(v.key, i);
        CHECK(v.val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&hh, &i), true);
        CHECK(occupied(handle_r(&hh, &i)), true);
        CHECK(validate(&hh), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&hh, &i), false);
        CHECK(occupied(handle_r(&hh, &i)), false);
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
        hhmap_test_insert_then_bad_ideas(), hhmap_test_insert_via_handle(),
        hhmap_test_insert_via_handle_macros(),
        hhmap_test_handle_api_functional(), hhmap_test_handle_api_macros(),
        hhmap_test_two_sum(), hhmap_test_resize(), hhmap_test_resize_macros(),
        hhmap_test_resize_from_null(), hhmap_test_resize_from_null_macros(),
        hhmap_test_insert_limit());
}
