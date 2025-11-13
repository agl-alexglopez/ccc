#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "hromap_util.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

static inline struct val
hromap_create(int const id, int const val)
{
    return (struct val){.id = id, .val = val};
}

static inline void
hromap_modplus(CCC_Type_context const t)
{
    ((struct val *)t.any_type)->val++;
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);

    /* Nothing was there before so nothing is in the handle. */
    CCC_Handle *hndl = swap_handle_r(&hrm, &(struct val){.id = 137, .val = 99});
    CHECK(occupied(hndl), false);
    CHECK(count(&hrm).count, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_macros)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);

    struct val const *ins
        = hrm_at(&hrm, CCC_hrm_or_insert_w(handle_r(&hrm, &(int){2}),
                                           (struct val){.id = 2, .val = 0}));
    CHECK(ins != NULL, true);
    CHECK(validate(&hrm), true);
    CHECK(count(&hrm).count, 1);
    ins = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &(int){2}),
                                           (struct val){.id = 2, .val = 0}));
    CHECK(validate(&hrm), true);
    CHECK(ins != NULL, true);
    ins = hrm_at(&hrm, hrm_insert_handle_w(handle_r(&hrm, &(int){9}),
                                           (struct val){.id = 9, .val = 1}));
    CHECK(validate(&hrm), true);
    CHECK(ins != NULL, true);
    ins = hrm_at(
        &hrm, unwrap(hrm_insert_or_assign_w(&hrm, 3, (struct val){.val = 99})));
    CHECK(validate(&hrm), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&hrm), true);
    CHECK(ins->val, 99);
    CHECK(count(&hrm).count, 3);
    ins = hrm_at(&hrm, CCC_handle_unwrap(hrm_insert_or_assign_w(
                           &hrm, 3, (struct val){.val = 98})));
    CHECK(validate(&hrm), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(count(&hrm).count, 3);
    ins = hrm_at(&hrm,
                 unwrap(hrm_try_insert_w(&hrm, 3, (struct val){.val = 100})));
    CHECK(ins == NULL, false);
    CHECK(validate(&hrm), true);
    CHECK(ins->val, 98);
    CHECK(count(&hrm).count, 3);
    ins = hrm_at(&hrm, CCC_handle_unwrap(hrm_try_insert_w(
                           &hrm, 4, (struct val){.val = 100})));
    CHECK(ins == NULL, false);
    CHECK(validate(&hrm), true);
    CHECK(ins->val, 100);
    CHECK(count(&hrm).count, 4);
    CHECK_END_FN(clear_and_free(&hrm, NULL););
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_overwrite)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);

    struct val q = {.id = 137, .val = 99};
    CCC_Handle hndl = swap_handle(&hrm, &q);
    CHECK(occupied(&hndl), false);

    struct val const *v = hrm_at(&hrm, unwrap(handle_r(&hrm, &q.id)));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    CCC_Handle in_table = swap_handle(&hrm, &q);
    CHECK(occupied(&in_table), true);

    /* The old contents are now in q and the handle is in the table. */
    v = hrm_at(&hrm, unwrap(&in_table));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 99);
    v = hrm_at(&hrm, unwrap(handle_r(&hrm, &q.id)));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_then_bad_ideas)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    struct val q = {.id = 137, .val = 99};
    CCC_Handle hndl = swap_handle(&hrm, &q);
    CHECK(occupied(&hndl), false);
    struct val const *v = hrm_at(&hrm, unwrap(handle_r(&hrm, &q.id)));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    q = (struct val){.id = 137, .val = 100};

    hndl = swap_handle(&hrm, &q);
    CHECK(occupied(&hndl), true);
    v = hrm_at(&hrm, unwrap(&hndl));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 99);
    q.val -= 9;

    v = hrm_at(&hrm, get_key_val(&hrm, &q.id));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(standard_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, STANDARD_FIXED_CAP);
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
            = hrm_at(&hrm, or_insert(handle_r(&hrm, &def.id), &def));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(count(&hrm).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        CCC_Handle_index const h
            = or_insert(hrm_and_modify_w(handle_r(&hrm, &def.id), struct val,
                                         { T->val++; }),
                        &def);
        struct val const *const d = hrm_at(&hrm, h);
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
    CHECK(count(&hrm).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val *const in
            = hrm_at(&hrm, or_insert(handle_r(&hrm, &def.id), &def));
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(count(&hrm).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_via_handle)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(standard_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, STANDARD_FIXED_CAP);
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
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &def.id), &def));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(count(&hrm).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &def.id), &def));
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
    CHECK(count(&hrm).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_via_handle_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(standard_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, STANDARD_FIXED_CAP);
    size_t const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d = hrm_at(
            &hrm, insert_handle(handle_r(&hrm, &i), &(struct val){i, i}));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(count(&hrm).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d = hrm_at(
            &hrm, insert_handle(handle_r(&hrm, &i), &(struct val){i, i + 1}));
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
    CHECK(count(&hrm).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_handle_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(standard_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, STANDARD_FIXED_CAP);
    int const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d = hrm_at(
            &hrm, hrm_or_insert_w(handle_r(&hrm, &i), hromap_create(i, i)));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(count(&hrm).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = hrm_at(
            &hrm,
            hrm_or_insert_w(and_modify(handle_r(&hrm, &i), hromap_modplus),
                            hromap_create(i, i)));
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
    CHECK(count(&hrm).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v
            = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &i), (struct val){}));
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(count(&hrm).count, (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_two_sum)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
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
        CCC_Handle const e
            = insert_or_assign(&hrm, &(struct val){.id = addends[i], .val = i});
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_resize)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &elem.id), &elem));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&hrm), true);
    }
    CHECK(count(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index};
        struct val const *const in_table = hrm_at(
            &hrm, insert_handle(handle_r(&hrm, &swap_slot.id), &swap_slot));
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(clear_and_free(&hrm, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_reserve)
{
    int const to_insert = 1000;
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(NULL, struct val, id, id_cmp, NULL, NULL, 0);
    CCC_Result const r = hrm_reserve(&hrm, to_insert, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &elem.id), &elem));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&hrm), true);
    }
    CHECK(count(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index};
        struct val const *const in_table = hrm_at(
            &hrm, insert_handle(handle_r(&hrm, &swap_slot.id), &swap_slot));
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK_END_FN(clear_and_free_reserve(&hrm, NULL, std_alloc););
}

CHECK_BEGIN_STATIC_FN(hromap_test_resize_macros)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &shuffled_index),
                                         &(struct val){shuffled_index, i}));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(count(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        CCC_Handle_index const h = hrm_or_insert_w(
            hrm_and_modify_w(handle_r(&hrm, &shuffled_index), struct val,
                             { T->val = shuffled_index; }),
            (struct val){});
        struct val const *const in_table = hrm_at(&hrm, h);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &shuffled_index),
                                           (struct val){}));
        CHECK(v == NULL, false);
        v->val = i;
        v = hrm_at(&hrm, get_key_val(&hrm, &shuffled_index));
        CHECK(v != NULL, true);
        CHECK(v->val, i);
    }
    CHECK(clear_and_free(&hrm, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_resize_from_null)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &elem.id), &elem));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(count(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index};
        struct val const *const in_table = hrm_at(
            &hrm, insert_handle(handle_r(&hrm, &swap_slot.id), &swap_slot));
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(clear_and_free(&hrm, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_resize_from_null_macros)
{
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &shuffled_index),
                                         &(struct val){shuffled_index, i}));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(count(&hrm).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        CCC_Handle_index const h = hrm_or_insert_w(
            hrm_and_modify_w(handle_r(&hrm, &shuffled_index), struct val,
                             { T->val = shuffled_index; }),
            (struct val){});
        struct val const *const in_table = hrm_at(&hrm, h);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = hrm_at(&hrm, hrm_or_insert_w(handle_r(&hrm, &shuffled_index),
                                           (struct val){}));
        CHECK(v == NULL, false);
        v->val = i;
        v = hrm_at(&hrm, get_key_val(&hrm, &shuffled_index));
        CHECK(v == NULL, false);
        CHECK(v->val, i);
    }
    CHECK(clear_and_free(&hrm, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_limit)
{
    int const size = SMALL_FIXED_CAP;
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);

    int const larger_prime = 103;
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct val *v
            = hrm_at(&hrm, insert_handle(handle_r(&hrm, &shuffled_index),
                                         &(struct val){shuffled_index, i}));
        if (!v)
        {
            break;
        }
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = count(&hrm).count;
    /* The last successful handle is still in the table and is overwritten. */
    struct val v = {.id = last_index, .val = -1};
    CCC_Handle hndl = swap_handle(&hrm, &v);
    CHECK(unwrap(&hndl) != 0, true);
    CHECK(insert_error(&hndl), false);
    CHECK(count(&hrm).count, final_size);

    v = (struct val){.id = last_index, .val = -2};
    struct val *in_table
        = hrm_at(&hrm, insert_handle(handle_r(&hrm, &v.id), &v));
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -2);
    CHECK(count(&hrm).count, final_size);

    in_table = hrm_at(
        &hrm, insert_handle(handle_r(&hrm, &last_index),
                            &(struct val){.id = last_index, .val = -3}));
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -3);
    CHECK(count(&hrm).count, final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.id = shuffled_index, .val = -4};
    in_table = hrm_at(&hrm, insert_handle(handle_r(&hrm, &v.id), &v));
    CHECK(in_table == NULL, true);
    CHECK(count(&hrm).count, final_size);

    in_table = hrm_at(
        &hrm, insert_handle(handle_r(&hrm, &shuffled_index),
                            &(struct val){.id = shuffled_index, .val = -4}));
    CHECK(in_table == NULL, true);
    CHECK(count(&hrm).count, final_size);

    hndl = swap_handle(&hrm, &v);
    CHECK(unwrap(&hndl) == 0, true);
    CHECK(insert_error(&hndl), true);
    CHECK(count(&hrm).count, final_size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_and_find)
{
    int const size = SMALL_FIXED_CAP;
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);

    for (int i = 0; i < size; i += 2)
    {
        CCC_Handle e = try_insert(&hrm, &(struct val){.id = i, .val = i});
        CHECK(occupied(&e), false);
        CHECK(validate(&hrm), true);
        e = try_insert(&hrm, &(struct val){.id = i, .val = i});
        CHECK(occupied(&e), true);
        CHECK(validate(&hrm), true);
        struct val const *const v = hrm_at(&hrm, unwrap(&e));
        CHECK(v == NULL, false);
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
    size_t const size = SMALL_FIXED_CAP - 1;
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(small_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, SMALL_FIXED_CAP);
    CHECK(size > 1, true);
    int const prime = 67;
    CHECK(insert_shuffled(&hrm, size, prime), PASS);
    int sorted_check[SMALL_FIXED_CAP - 1];
    CHECK(inorder_fill(sorted_check, size, &hrm), size);
    for (size_t i = 1; i < size; ++i)
    {
        CHECK(sorted_check[i - 1] <= sorted_check[i], true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_insert_weak_srand)
{
    int const num_nodes = STANDARD_FIXED_CAP - 1;
    CCC_handle_realtime_ordered_map hrm
        = hrm_initialize(&(standard_fixed_map){}, struct val, id, id_cmp, NULL,
                         NULL, STANDARD_FIXED_CAP);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Handle const e = swap_handle(
            &hrm, &(struct val){.id = rand() /* NOLINT */, .val = i});
        CHECK(insert_error(&e), false);
        CHECK(validate(&hrm), true);
    }
    CHECK(count(&hrm).count, (size_t)num_nodes);
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
