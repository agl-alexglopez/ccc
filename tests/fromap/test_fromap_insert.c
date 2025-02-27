#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "flat_realtime_ordered_map.h"
#include "fromap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static inline struct val
fromap_create(int const id, int const val)
{
    return (struct val){.id = id, .val = val};
}

static inline void
fromap_modplus(ccc_user_type const t)
{
    ((struct val *)t.user_type)->val++;
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[10]){}, elem, id, id_cmp, NULL, NULL, 10);

    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = insert(&frm, &(struct val){.id = 137, .val = 99}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&frm), 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_macros)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[10]){}, elem, id, id_cmp, NULL, NULL, 10);

    struct val const *ins = ccc_frm_or_insert_w(
        entry_r(&frm, &(int){2}), (struct val){.id = 2, .val = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&frm), true);
    CHECK(size(&frm), 1);
    ins = frm_insert_entry_w(entry_r(&frm, &(int){2}),
                             (struct val){.id = 2, .val = 0});
    CHECK(validate(&frm), true);
    CHECK(ins != NULL, true);
    ins = frm_insert_entry_w(entry_r(&frm, &(int){9}),
                             (struct val){.id = 9, .val = 1});
    CHECK(validate(&frm), true);
    CHECK(ins != NULL, true);
    ins = ccc_entry_unwrap(
        frm_insert_or_assign_w(&frm, 3, (struct val){.val = 99}));
    CHECK(validate(&frm), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&frm), true);
    CHECK(ins->val, 99);
    CHECK(size(&frm), 3);
    ins = ccc_entry_unwrap(
        frm_insert_or_assign_w(&frm, 3, (struct val){.val = 98}));
    CHECK(validate(&frm), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&frm), 3);
    ins = ccc_entry_unwrap(frm_try_insert_w(&frm, 3, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&frm), true);
    CHECK(ins->val, 98);
    CHECK(size(&frm), 3);
    ins = ccc_entry_unwrap(frm_try_insert_w(&frm, 4, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&frm), true);
    CHECK(ins->val, 100);
    CHECK(size(&frm), 4);
    CHECK_END_FN(ccc_frm_clear_and_free(&frm, NULL););
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_overwrite)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[10]){}, elem, id, id_cmp, NULL, NULL, 10);

    struct val q = {.id = 137, .val = 99};
    ccc_entry ent = insert(&frm, &q.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);

    struct val const *v = unwrap(entry_r(&frm, &q.id));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_entry old_ent = insert(&frm, &q.elem);
    CHECK(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(q.val, 99);
    v = unwrap(entry_r(&frm, &q.id));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_then_bad_ideas)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[10]){}, elem, id, id_cmp, NULL, NULL, 10);
    struct val q = {.id = 137, .val = 99};
    ccc_entry ent = insert(&frm, &q.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    struct val const *v = unwrap(entry_r(&frm, &q.id));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    q = (struct val){.id = 137, .val = 100};

    ent = insert(&frm, &q.elem);
    CHECK(occupied(&ent), true);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(q.val, 99);
    q.val -= 9;

    v = get_key_val(&frm, &q.id);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[200]){}, elem, id, id_cmp, NULL, NULL, 200);
    size_t const size = 200;

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = or_insert(entry_r(&frm, &def.id), &def.elem);
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&frm), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = or_insert(frm_and_modify_w(entry_r(&frm, &def.id), struct val,
                                         {
                                             T->val++;
                                         }),
                        &def.elem);
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
    CHECK(size(&frm), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val *const in = or_insert(entry_r(&frm, &def.id), &def.elem);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&frm), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[200]){}, elem, id, id_cmp, NULL, NULL, 200);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = insert_entry(entry_r(&frm, &def.id), &def.elem);
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&frm), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = insert_entry(entry_r(&frm, &def.id), &def.elem);
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
    CHECK(size(&frm), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[200]){}, elem, id, id_cmp, NULL, NULL, 200);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = insert_entry(entry_r(&frm, &i), &(struct val){i, i, {}}.elem);
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&frm), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = insert_entry(entry_r(&frm, &i), &(struct val){i, i + 1, {}}.elem);
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
    CHECK(size(&frm), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[200]){}, elem, id, id_cmp, NULL, NULL, 200);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d
            = frm_or_insert_w(entry_r(&frm, &i), fromap_create(i, i));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&frm), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = frm_or_insert_w(
            and_modify(entry_r(&frm, &i), fromap_modplus), fromap_create(i, i));
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
    CHECK(size(&frm), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v = frm_or_insert_w(entry_r(&frm, &i), (struct val){});
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(size(&frm), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_two_sum)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[20]){}, elem, id, id_cmp, NULL, NULL, 20);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct val const *const other_addend
            = get_key_val(&frm, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_entry const e = insert_or_assign(
            &frm, &(struct val){.id = addends[i], .val = i}.elem);
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_resize)
{
    size_t const prime_start = 11;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val *)malloc(sizeof(struct val) * prime_start), elem,
                   id, id_cmp, std_alloc, NULL, prime_start);
    CHECK(frm_data(&frm) != NULL, true);

    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&frm, &elem.id), &elem.elem);
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&frm), true);
    }
    CHECK(size(&frm), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_r(&frm, &swap_slot.id), &swap_slot.elem);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(frm_clear_and_free(&frm, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_resize_macros)
{
    size_t const prime_start = 11;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val *)malloc(sizeof(struct val) * prime_start), elem,
                   id, id_cmp, std_alloc, NULL, prime_start);
    CHECK(frm_data(&frm) != NULL, true);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&frm, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.elem);
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&frm), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = frm_or_insert_w(
            frm_and_modify_w(entry_r(&frm, &shuffled_index), struct val,
                             {
                                 T->val = shuffled_index;
                             }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = frm_or_insert_w(entry_r(&frm, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&frm, &shuffled_index);
        CHECK(v != NULL, true);
        CHECK(v->val, i);
    }
    CHECK(frm_clear_and_free(&frm, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_resize_from_null)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&frm, &elem.id), &elem.elem);
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&frm), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_r(&frm, &swap_slot.id), &swap_slot.elem);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(frm_clear_and_free(&frm, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_resize_from_null_macros)
{
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&frm, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.elem);
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&frm), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = frm_or_insert_w(
            frm_and_modify_w(entry_r(&frm, &shuffled_index), struct val,
                             {
                                 T->val = shuffled_index;
                             }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = frm_or_insert_w(entry_r(&frm, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&frm, &shuffled_index);
        CHECK(v == NULL, false);
        CHECK(v->val, i);
    }
    CHECK(frm_clear_and_free(&frm, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_limit)
{
    int const size = 101;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[101]){}, elem, id, id_cmp, NULL, NULL, 101);

    int const larger_prime = 103;
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct val *v = insert_entry(entry_r(&frm, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.elem);
        if (!v)
        {
            break;
        }
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = size(&frm);
    /* The last successful entry is still in the table and is overwritten. */
    struct val v = {.id = last_index, .val = -1};
    ccc_entry ent = insert(&frm, &v.elem);
    CHECK(unwrap(&ent) != NULL, true);
    CHECK(insert_error(&ent), false);
    CHECK(size(&frm), final_size);

    v = (struct val){.id = last_index, .val = -2};
    struct val *in_table = insert_entry(entry_r(&frm, &v.id), &v.elem);
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -2);
    CHECK(size(&frm), final_size);

    in_table = insert_entry(entry_r(&frm, &last_index),
                            &(struct val){.id = last_index, .val = -3}.elem);
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -3);
    CHECK(size(&frm), final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.id = shuffled_index, .val = -4};
    in_table = insert_entry(entry_r(&frm, &v.id), &v.elem);
    CHECK(in_table == NULL, true);
    CHECK(size(&frm), final_size);

    in_table
        = insert_entry(entry_r(&frm, &shuffled_index),
                       &(struct val){.id = shuffled_index, .val = -4}.elem);
    CHECK(in_table == NULL, true);
    CHECK(size(&frm), final_size);

    ent = insert(&frm, &v.elem);
    CHECK(unwrap(&ent) == NULL, true);
    CHECK(insert_error(&ent), true);
    CHECK(size(&frm), final_size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_and_find)
{
    int const size = 101;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[101]){}, elem, id, id_cmp, NULL, NULL, 101);

    for (int i = 0; i < size; i += 2)
    {
        ccc_entry e = try_insert(&frm, &(struct val){.id = i, .val = i}.elem);
        CHECK(occupied(&e), false);
        CHECK(validate(&frm), true);
        e = try_insert(&frm, &(struct val){.id = i, .val = i}.elem);
        CHECK(occupied(&e), true);
        CHECK(validate(&frm), true);
        struct val const *const v = unwrap(&e);
        CHECK(v == NULL, false);
        CHECK(v->id, i);
        CHECK(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&frm, &i), true);
        CHECK(occupied(entry_r(&frm, &i)), true);
        CHECK(validate(&frm), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&frm, &i), false);
        CHECK(occupied(entry_r(&frm, &i)), false);
        CHECK(validate(&frm), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_shuffle)
{
    size_t const size = 50;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[51]){}, elem, id, id_cmp, NULL, NULL, 51);
    CHECK(size > 1, true);
    int const prime = 53;
    CHECK(insert_shuffled(&frm, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &frm), size);
    for (size_t i = 1; i < size; ++i)
    {
        CHECK(sorted_check[i - 1] <= sorted_check[i], true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fromap_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    ccc_flat_realtime_ordered_map frm
        = frm_init((struct val[1001]){}, elem, id, id_cmp, NULL, NULL, 1001);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        ccc_entry const e = insert(
            &frm, &(struct val){.id = rand() /* NOLINT */, .val = i}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&frm), true);
    }
    CHECK(size(&frm), (size_t)num_nodes);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        fromap_test_insert(), fromap_test_insert_macros(),
        fromap_test_insert_and_find(), fromap_test_insert_overwrite(),
        fromap_test_insert_then_bad_ideas(), fromap_test_insert_via_entry(),
        fromap_test_insert_via_entry_macros(),
        fromap_test_entry_api_functional(), fromap_test_entry_api_macros(),
        fromap_test_two_sum(), fromap_test_resize(),
        fromap_test_resize_macros(), fromap_test_resize_from_null(),
        fromap_test_resize_from_null_macros(), fromap_test_insert_limit(),
        fromap_test_insert_weak_srand(), fromap_test_insert_shuffle());
}
