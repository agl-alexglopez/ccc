#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define ORDERED_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "ordered_map.h"
#include "ordered_map_util.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

static inline struct Val
ordered_map_create(int const id, int const val)
{
    return (struct Val){.key = id, .val = val};
}

static inline void
ordered_map_modplus(CCC_Type_context const t)
{
    ((struct Val *)t.type)->val++;
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, NULL, NULL);

    /* Nothing was there before so nothing is in the entry. */
    CCC_Entry ent = swap_entry(&om, &(struct Val){.key = 137, .val = 99}.elem,
                               &(struct Val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(count(&om).count, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_macros)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);

    struct Val const *ins = CCC_ordered_map_or_insert_w(
        entry_r(&om, &(int){2}), (struct Val){.key = 2, .val = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&om), true);
    CHECK(count(&om).count, 1);
    ins = ordered_map_insert_entry_w(entry_r(&om, &(int){2}),
                                     (struct Val){.key = 2, .val = 0});
    CHECK(validate(&om), true);
    CHECK(ins != NULL, true);
    ins = ordered_map_insert_entry_w(entry_r(&om, &(int){9}),
                                     (struct Val){.key = 9, .val = 1});
    CHECK(validate(&om), true);
    CHECK(ins != NULL, true);
    ins = CCC_entry_unwrap(
        ordered_map_insert_or_assign_w(&om, 3, (struct Val){.val = 99}));
    CHECK(validate(&om), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&om), true);
    CHECK(ins->val, 99);
    CHECK(count(&om).count, 3);
    ins = CCC_entry_unwrap(
        ordered_map_insert_or_assign_w(&om, 3, (struct Val){.val = 98}));
    CHECK(validate(&om), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(count(&om).count, 3);
    ins = CCC_entry_unwrap(
        ordered_map_try_insert_w(&om, 3, (struct Val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&om), true);
    CHECK(ins->val, 98);
    CHECK(count(&om).count, 3);
    ins = CCC_entry_unwrap(
        ordered_map_try_insert_w(&om, 4, (struct Val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&om), true);
    CHECK(ins->val, 100);
    CHECK(count(&om).count, 4);
    CHECK_END_FN(CCC_ordered_map_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_overwrite)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, NULL, NULL);

    struct Val q = {.key = 137, .val = 99};
    CCC_Entry ent = swap_entry(&om, &q.elem, &(struct Val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);

    struct Val const *v = unwrap(entry_r(&om, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    struct Val r = (struct Val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    CCC_Entry old_ent = swap_entry(&om, &r.elem, &(struct Val){}.elem);
    CHECK(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(r.val, 99);
    v = unwrap(entry_r(&om, &r.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_then_bad_ideas)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, NULL, NULL);
    struct Val q = {.key = 137, .val = 99};
    CCC_Entry ent = swap_entry(&om, &q.elem, &(struct Val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    struct Val const *v = unwrap(entry_r(&om, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    struct Val r = (struct Val){.key = 137, .val = 100};

    ent = swap_entry(&om, &r.elem, &(struct Val){}.elem);
    CHECK(occupied(&ent), true);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(r.val, 99);
    r.val -= 9;

    v = get_key_val(&om, &q.key);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(r.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);
    size_t const size = 200;

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct Val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d
            = or_insert(entry_r(&om, &def.key), &def.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(count(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d = or_insert(
            ordered_map_and_modify_w(entry_r(&om, &def.key), struct Val,
                                     {
                                         T->val++;
                                     }),
            &def.elem);
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
    CHECK(count(&om).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val *const in = or_insert(entry_r(&om, &def.key), &def.elem);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(count(&om).count, (size / 2));
    CHECK_END_FN(ordered_map_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct Val def = {};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d
            = insert_entry(entry_r(&om, &def.key), &def.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(count(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i + 1;
        struct Val const *const d
            = insert_entry(entry_r(&om, &def.key), &def.elem);
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
    CHECK(count(&om).count, (size / 2));
    CHECK_END_FN(ordered_map_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct Val const *const d
            = insert_entry(entry_r(&om, &i), &(struct Val){i, i, {}}.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(count(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct Val const *const d
            = insert_entry(entry_r(&om, &i), &(struct Val){i, i + 1, {}}.elem);
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
    CHECK(count(&om).count, (size / 2));
    CHECK_END_FN(ordered_map_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct Val const *const d = ordered_map_or_insert_w(
            entry_r(&om, &i), ordered_map_create(i, i));
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(count(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val const *const d = ordered_map_or_insert_w(
            and_modify(entry_r(&om, &i), ordered_map_modplus),
            ordered_map_create(i, i));
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
    CHECK(count(&om).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val *v
            = ordered_map_or_insert_w(entry_r(&om, &i), (struct Val){});
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(count(&om).count, (size / 2));
    CHECK_END_FN(ordered_map_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_two_sum)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (size_t)(sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct Val const *const other_addend
            = get_key_val(&om, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        CCC_Entry const e = insert_or_assign(
            &om, &(struct Val){.key = addends[i], .val = i}.elem);
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN(ordered_map_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_resize)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);

    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.key = shuffled_index, .val = i};
        struct Val *v = insert_entry(entry_r(&om, &elem.key), &elem.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&om), true);
    }
    CHECK(count(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index, {}};
        struct Val const *const in_table
            = insert_entry(entry_r(&om, &swap_slot.key), &swap_slot.elem);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(ordered_map_clear(&om, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_resize_macros)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val *v = insert_entry(entry_r(&om, &shuffled_index),
                                     &(struct Val){shuffled_index, i, {}}.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(count(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val const *const in_table = ordered_map_or_insert_w(
            ordered_map_and_modify_w(entry_r(&om, &shuffled_index), struct Val,
                                     {
                                         T->val = shuffled_index;
                                     }),
            (struct Val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct Val *v = ordered_map_or_insert_w(entry_r(&om, &shuffled_index),
                                                (struct Val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&om, &shuffled_index);
        CHECK(v != NULL, true);
        CHECK(v->val, i);
    }
    CHECK(ordered_map_clear(&om, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_resize_fordered_map_null)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.key = shuffled_index, .val = i};
        struct Val *v = insert_entry(entry_r(&om, &elem.key), &elem.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(count(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index, {}};
        struct Val const *const in_table
            = insert_entry(entry_r(&om, &swap_slot.key), &swap_slot.elem);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(ordered_map_clear(&om, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_resize_fordered_map_null_macros)
{
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val *v = insert_entry(entry_r(&om, &shuffled_index),
                                     &(struct Val){shuffled_index, i, {}}.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(count(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val const *const in_table = ordered_map_or_insert_w(
            ordered_map_and_modify_w(entry_r(&om, &shuffled_index), struct Val,
                                     {
                                         T->val = shuffled_index;
                                     }),
            (struct Val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct Val *v = ordered_map_or_insert_w(entry_r(&om, &shuffled_index),
                                                (struct Val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&om, &shuffled_index);
        CHECK(v == NULL, false);
        CHECK(v->val, i);
    }
    CHECK(ordered_map_clear(&om, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_and_find)
{
    int const size = 101;
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);

    for (int i = 0; i < size; i += 2)
    {
        CCC_Entry e = try_insert(&om, &(struct Val){.key = i, .val = i}.elem);
        CHECK(occupied(&e), false);
        CHECK(validate(&om), true);
        e = try_insert(&om, &(struct Val){.key = i, .val = i}.elem);
        CHECK(occupied(&e), true);
        CHECK(validate(&om), true);
        struct Val const *const v = unwrap(&e);
        CHECK(v == NULL, false);
        CHECK(v->key, i);
        CHECK(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&om, &i), true);
        CHECK(occupied(entry_r(&om, &i)), true);
        CHECK(validate(&om), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&om, &i), false);
        CHECK(occupied(entry_r(&om, &i)), false);
        CHECK(validate(&om), true);
    }
    CHECK_END_FN(ordered_map_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_shuffle)
{
    size_t const size = 50;
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, NULL, NULL);
    struct Val vals[50] = {};
    CHECK(size > 1, true);
    int const prime = 53;
    CHECK(insert_shuffled(&om, vals, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &om), size);
    for (size_t i = 1; i < size; ++i)
    {
        CHECK(sorted_check[i - 1] <= sorted_check[i], true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(ordered_map_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    CCC_Ordered_map om = ordered_map_initialize(om, struct Val, elem, key,
                                                id_order, std_allocate, NULL);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Entry const e = swap_entry(
            &om, &(struct Val){.key = rand() /* NOLINT */, .val = i}.elem,
            &(struct Val){}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&om), true);
    }
    CHECK(count(&om).count, (size_t)num_nodes);
    CHECK_END_FN(ordered_map_clear(&om, NULL););
}

int
main()
{
    return CHECK_RUN(
        ordered_map_test_insert(), ordered_map_test_insert_macros(),
        ordered_map_test_insert_and_find(), ordered_map_test_insert_overwrite(),
        ordered_map_test_insert_then_bad_ideas(),
        ordered_map_test_insert_via_entry(),
        ordered_map_test_insert_via_entry_macros(),
        ordered_map_test_entry_api_functional(),
        ordered_map_test_entry_api_macros(), ordered_map_test_two_sum(),
        ordered_map_test_resize(), ordered_map_test_resize_macros(),
        ordered_map_test_resize_fordered_map_null(),
        ordered_map_test_resize_fordered_map_null_macros(),
        ordered_map_test_insert_weak_srand(),
        ordered_map_test_insert_shuffle());
}
