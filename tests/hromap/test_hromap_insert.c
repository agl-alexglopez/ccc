#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "hromap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

static inline struct val
hromap_create(int const id, int const val)
{
    return (struct val){.id = id, .val = val};
}

static inline void
hromap_modplus(ccc_any_type const t)
{
    ((struct val *)t.any_type)->val++;
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[10]){}, elem, id, id_cmp, nullptr, nullptr, 10);

    /* Nothing was there before so nothing is in the handle. */
    ccc_handle *hndl
        = swap_handle_r(&hrm, &(struct val){.id = 137, .val = 99}.elem);
    CHECK(occupied(hndl), false);
    CHECK(size(&hrm).count, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_macros)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[10]){}, elem, id, id_cmp, nullptr, nullptr, 10);

    struct val const *ins
        = hrm_at(&hrm, ccc_hrm_or_insert_w(handle_r(&hrm, &(int){2}),
                                           (struct val){.id = 2, .val = 0}));
    CHECK(ins != nullptr, true);
    CHECK(validate(&hrm), true);
    CHECK(size(&hrm).count, 1);
    ins = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &(int){2}),
                                           (struct val){.id = 2, .val = 0}));
    CHECK(validate(&hrm), true);
    CHECK(ins != nullptr, true);
    ins = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &(int){9}),
                                           (struct val){.id = 9, .val = 1}));
    CHECK(validate(&hrm), true);
    CHECK(ins != nullptr, true);
    ins = hrm_at(
        &hrm, unwrap(hrm_insert_or_assign_w(&hrm, 3, (struct val){.val = 99})));
    CHECK(validate(&hrm), true);
    CHECK(ins == nullptr, false);
    CHECK(validate(&hrm), true);
    CHECK(ins->val, 99);
    CHECK(size(&hrm).count, 3);
    ins = hrm_at(&hrm, ccc_handle_unwrap(hrm_insert_or_assign_w(
                           &hrm, 3, (struct val){.val = 98})));
    CHECK(validate(&hrm), true);
    CHECK(ins == nullptr, false);
    CHECK(ins->val, 98);
    CHECK(size(&hrm).count, 3);
    ins = hrm_at(&hrm,
                 unwrap(hrm_try_insert_w(&hrm, 3, (struct val){.val = 100})));
    CHECK(ins == nullptr, false);
    CHECK(validate(&hrm), true);
    CHECK(ins->val, 98);
    CHECK(size(&hrm).count, 3);
    ins = hrm_at(&hrm, ccc_handle_unwrap(hrm_try_insert_w(
                           &hrm, 4, (struct val){.val = 100})));
    CHECK(ins == nullptr, false);
    CHECK(validate(&hrm), true);
    CHECK(ins->val, 100);
    CHECK(size(&hrm).count, 4);
    CHECK_END_FN(clear_and_free(&hrm, nullptr););
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_overwrite)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[10]){}, elem, id, id_cmp, nullptr, nullptr, 10);

    struct val q = {.id = 137, .val = 99};
    ccc_handle hndl = swap_handle(&hrm, &q.elem);
    CHECK(occupied(&hndl), false);

    struct val const *v = hrm_at(&hrm, unwrap(handle_r(&hrm, &q.id)));
    CHECK(v != nullptr, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_handle in_table = swap_handle(&hrm, &q.elem);
    CHECK(occupied(&in_table), true);

    /* The old contents are now in q and the handle is in the table. */
    v = hrm_at(&hrm, unwrap(&in_table));
    CHECK(v != nullptr, true);
    CHECK(v->val, 100);
    CHECK(q.val, 99);
    v = hrm_at(&hrm, unwrap(handle_r(&hrm, &q.id)));
    CHECK(v != nullptr, true);
    CHECK(v->val, 100);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_then_bad_ideas)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[10]){}, elem, id, id_cmp, nullptr, nullptr, 10);
    struct val q = {.id = 137, .val = 99};
    ccc_handle hndl = swap_handle(&hrm, &q.elem);
    CHECK(occupied(&hndl), false);
    struct val const *v = hrm_at(&hrm, unwrap(handle_r(&hrm, &q.id)));
    CHECK(v != nullptr, true);
    CHECK(v->val, 99);

    q = (struct val){.id = 137, .val = 100};

    hndl = swap_handle(&hrm, &q.elem);
    CHECK(occupied(&hndl), true);
    v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != nullptr, true);
    CHECK(v->val, 100);
    CHECK(q.val, 99);
    q.val -= 9;

    v = hrm_at(&hrm, get_key_val(&hrm, &q.id));
    CHECK(v != nullptr, true);
    CHECK(v->val, 100);
    CHECK(q.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val[200]){}, elem, id, id_cmp, nullptr, nullptr, 200);
    size_t const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = hrm_at(&hrm, or_insert(handle_r(&hrm, &def.id), &def.elem));
        CHECK((d != nullptr), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hrm).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        ccc_handle_i const h
            = or_insert(hrm_and_modify_w(handle_r(&hrm, &def.id), struct val,
                                         { T->val++; }),
                        &def.elem);
        struct val const *const d = hrm_at(&hrm, h);
        /* All values in the array should be odd now */
        CHECK((d != nullptr), true);
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
    CHECK(size(&hrm).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val *const in
            = hrm_at(&hrm, or_insert(handle_r(&hrm, &def.id), &def.elem));
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&hrm).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_via_handle)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val[200]){}, elem, id, id_cmp, nullptr, nullptr, 200);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &def.id), &def.elem));
        CHECK((d != nullptr), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hrm).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &def.id), &def.elem));
        /* All values in the array should be odd now */
        CHECK((d != nullptr), true);
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
    CHECK(size(&hrm).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_via_handle_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val[200]){}, elem, id, id_cmp, nullptr, nullptr, 200);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                         &(struct val){i, i, {}}.elem));
        CHECK((d != nullptr), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hrm).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &i),
                                         &(struct val){i, i + 1, {}}.elem));
        /* All values in the array should be odd now */
        CHECK((d != nullptr), true);
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
    CHECK(size(&hrm).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val[200]){}, elem, id, id_cmp, nullptr, nullptr, 200);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d = hrm_at(
            &hrm, hrm_or_insert_w(handle_r(&hrm, &i), hromap_create(i, i)));
        CHECK((d != nullptr), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hrm).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = hrm_at(
            &hrm,
            hrm_or_insert_w(and_modify(handle_r(&hrm, &i), hromap_modplus),
                            hromap_create(i, i)));
        /* All values in the array should be odd now */
        CHECK((d != nullptr), true);
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
    CHECK(size(&hrm).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v
            = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), (struct val){}));
        CHECK(v != nullptr, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(size(&hrm).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_two_sum)
{
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[20]){}, elem, id, id_cmp, nullptr, nullptr, 20);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (size_t)(sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct val const *const other_addend
            = hrm_at(&hrm, get_key_val(&hrm, &(int){target - addends[i]}));
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_handle const e = insert_or_assign(
            &hrm, &(struct val){.id = addends[i], .val = i}.elem);
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_resize)
{
    size_t const prime_start = 11;
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val *)malloc(sizeof(struct val) * prime_start), elem,
                   id, id_cmp, std_alloc, nullptr, prime_start);
    CHECK(hrm_data(&hrm) != nullptr, true);

    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &elem.id), &elem.elem));
        CHECK(v != nullptr, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&hrm), true);
    }
    CHECK(size(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &swap_slot.id),
                                         &swap_slot.elem));
        CHECK(in_table != nullptr, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(clear_and_free(&hrm, nullptr), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_reserve)
{
    int const to_insert = 1000;
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val *)nullptr, elem, id, id_cmp, nullptr, nullptr, 0);
    ccc_result const r = hrm_reserve(&hrm, to_insert, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &elem.id), &elem.elem));
        CHECK(v != nullptr, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&hrm), true);
    }
    CHECK(size(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &swap_slot.id),
                                         &swap_slot.elem));
        CHECK(in_table != nullptr, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK_END_FN(clear_and_free_reserve(&hrm, nullptr, std_alloc););
}

CHECK_BEGIN_STATIC_FN(hromap_test_resize_macros)
{
    size_t const prime_start = 11;
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val *)malloc(sizeof(struct val) * prime_start), elem,
                   id, id_cmp, std_alloc, nullptr, prime_start);
    CHECK(hrm_data(&hrm) != nullptr, true);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = hrm_at(
            &hrm, insert_handle(handle_r(&hrm, &shuffled_index),
                                &(struct val){shuffled_index, i, {}}.elem));
        CHECK(v != nullptr, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        ccc_handle_i const h = hrm_or_insert_w(
            hrm_and_modify_w(handle_r(&hrm, &shuffled_index), struct val,
                             { T->val = shuffled_index; }),
            (struct val){});
        struct val const *const in_table = hrm_at(&hrm, h);
        CHECK(in_table != nullptr, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &shuffled_index),
                                           (struct val){}));
        CHECK(v == nullptr, false);
        v->val = i;
        v = hrm_at(&hrm, get_key_val(&hrm, &shuffled_index));
        CHECK(v != nullptr, true);
        CHECK(v->val, i);
    }
    CHECK(clear_and_free(&hrm, nullptr), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_resize_from_null)
{
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val *)nullptr, elem, id, id_cmp, std_alloc, nullptr, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &elem.id), &elem.elem));
        CHECK(v != nullptr, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &swap_slot.id),
                                         &swap_slot.elem));
        CHECK(in_table != nullptr, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(clear_and_free(&hrm, nullptr), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_resize_from_null_macros)
{
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val *)nullptr, elem, id, id_cmp, std_alloc, nullptr, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = hrm_at(
            &hrm, insert_handle(handle_r(&hrm, &shuffled_index),
                                &(struct val){shuffled_index, i, {}}.elem));
        CHECK(v != nullptr, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        ccc_handle_i const h = hrm_or_insert_w(
            hrm_and_modify_w(handle_r(&hrm, &shuffled_index), struct val,
                             { T->val = shuffled_index; }),
            (struct val){});
        struct val const *const in_table = hrm_at(&hrm, h);
        CHECK(in_table != nullptr, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &shuffled_index),
                                           (struct val){}));
        CHECK(v == nullptr, false);
        v->val = i;
        v = hrm_at(&hrm, get_key_val(&hrm, &shuffled_index));
        CHECK(v == nullptr, false);
        CHECK(v->val, i);
    }
    CHECK(clear_and_free(&hrm, nullptr), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_limit)
{
    int const size = 101;
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val[101]){}, elem, id, id_cmp, nullptr, nullptr, 101);

    int const larger_prime = 103;
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct val *v = hrm_at(
            &hrm, insert_handle(handle_r(&hrm, &shuffled_index),
                                &(struct val){shuffled_index, i, {}}.elem));
        if (!v)
        {
            break;
        }
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = size(&hrm).count;
    /* The last successful handle is still in the table and is overwritten. */
    struct val v = {.id = last_index, .val = -1};
    ccc_handle hndl = swap_handle(&hrm, &v.elem);
    CHECK(unwrap(&hndl) != 0, true);
    CHECK(insert_error(&hndl), false);
    CHECK(size(&hrm).count, final_size);

    v = (struct val){.id = last_index, .val = -2};
    struct val *in_table
        = hrm_at(&hrm, insert_handle(handle_r(&hrm, &v.id), &v.elem));
    CHECK(in_table != nullptr, true);
    CHECK(in_table->val, -2);
    CHECK(size(&hrm).count, final_size);

    in_table = hrm_at(
        &hrm, insert_handle(handle_r(&hrm, &last_index),
                            &(struct val){.id = last_index, .val = -3}.elem));
    CHECK(in_table != nullptr, true);
    CHECK(in_table->val, -3);
    CHECK(size(&hrm).count, final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.id = shuffled_index, .val = -4};
    in_table = hrm_at(&hrm, insert_handle(handle_r(&hrm, &v.id), &v.elem));
    CHECK(in_table == nullptr, true);
    CHECK(size(&hrm).count, final_size);

    in_table = hrm_at(
        &hrm,
        insert_handle(handle_r(&hrm, &shuffled_index),
                      &(struct val){.id = shuffled_index, .val = -4}.elem));
    CHECK(in_table == nullptr, true);
    CHECK(size(&hrm).count, final_size);

    hndl = swap_handle(&hrm, &v.elem);
    CHECK(unwrap(&hndl) == 0, true);
    CHECK(insert_error(&hndl), true);
    CHECK(size(&hrm).count, final_size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_and_find)
{
    int const size = 101;
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val[101]){}, elem, id, id_cmp, nullptr, nullptr, 101);

    for (int i = 0; i < size; i += 2)
    {
        ccc_handle e = try_insert(&hrm, &(struct val){.id = i, .val = i}.elem);
        CHECK(occupied(&e), false);
        CHECK(validate(&hrm), true);
        e = try_insert(&hrm, &(struct val){.id = i, .val = i}.elem);
        CHECK(occupied(&e), true);
        CHECK(validate(&hrm), true);
        struct val const *const v = hrm_at(&hrm, unwrap(&e));
        CHECK(v == nullptr, false);
        CHECK(v->id, i);
        CHECK(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&hrm, &i), true);
        CHECK(occupied(handle_r(&hrm, &i)), true);
        CHECK(validate(&hrm), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&hrm, &i), false);
        CHECK(occupied(handle_r(&hrm, &i)), false);
        CHECK(validate(&hrm), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_shuffle)
{
    size_t const size = 50;
    ccc_handle_realtime_ordered_map hrm
        = hrm_init((struct val[51]){}, elem, id, id_cmp, nullptr, nullptr, 51);
    CHECK(size > 1, true);
    int const prime = 53;
    CHECK(insert_shuffled(&hrm, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &hrm), size);
    for (size_t i = 1; i < size; ++i)
    {
        CHECK(sorted_check[i - 1] <= sorted_check[i], true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    ccc_handle_realtime_ordered_map hrm = hrm_init(
        (struct val[1001]){}, elem, id, id_cmp, nullptr, nullptr, 1001);
    srand(time(nullptr)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        ccc_handle const e = swap_handle(
            &hrm, &(struct val){.id = rand() /* NOLINT */, .val = i}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&hrm), true);
    }
    CHECK(size(&hrm).count, (size_t)num_nodes);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        hromap_test_insert(), hromap_test_insert_macros(),
        hromap_test_insert_and_find(), hromap_test_insert_overwrite(),
        hromap_test_insert_then_bad_ideas(), hromap_test_insert_via_handle(),
        hromap_test_insert_via_handle_macros(), hromap_test_reserve(),
        hromap_test_handle_api_functional(), hromap_test_handle_api_macros(),
        hromap_test_two_sum(), hromap_test_resize(),
        hromap_test_resize_macros(), hromap_test_resize_from_null(),
        hromap_test_resize_from_null_macros(), hromap_test_insert_limit(),
        hromap_test_insert_weak_srand(), hromap_test_insert_shuffle());
}
