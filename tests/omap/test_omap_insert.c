#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "omap_util.h"
#include "ordered_map.h"
#include "traits.h"
#include "types.h"

static inline struct val
omap_create(int const id, int const val)
{
    return (struct val){.key = id, .val = val};
}

static inline void
omap_modplus(ccc_any_type const t)
{
    ((struct val *)t.any_type)->val++;
}

CHECK_BEGIN_STATIC_FN(omap_test_insert)
{
    ccc_ordered_map om = om_init(om, struct val, elem, key, id_cmp, NULL, NULL);

    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = swap_entry(&om, &(struct val){.key = 137, .val = 99}.elem,
                               &(struct val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&om).count, 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_macros)
{
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);

    struct val const *ins = ccc_om_or_insert_w(
        entry_r(&om, &(int){2}), (struct val){.key = 2, .val = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&om), true);
    CHECK(size(&om).count, 1);
    ins = om_insert_entry_w(entry_r(&om, &(int){2}),
                            (struct val){.key = 2, .val = 0});
    CHECK(validate(&om), true);
    CHECK(ins != NULL, true);
    ins = om_insert_entry_w(entry_r(&om, &(int){9}),
                            (struct val){.key = 9, .val = 1});
    CHECK(validate(&om), true);
    CHECK(ins != NULL, true);
    ins = ccc_entry_unwrap(
        om_insert_or_assign_w(&om, 3, (struct val){.val = 99}));
    CHECK(validate(&om), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&om), true);
    CHECK(ins->val, 99);
    CHECK(size(&om).count, 3);
    ins = ccc_entry_unwrap(
        om_insert_or_assign_w(&om, 3, (struct val){.val = 98}));
    CHECK(validate(&om), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&om).count, 3);
    ins = ccc_entry_unwrap(om_try_insert_w(&om, 3, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&om), true);
    CHECK(ins->val, 98);
    CHECK(size(&om).count, 3);
    ins = ccc_entry_unwrap(om_try_insert_w(&om, 4, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&om), true);
    CHECK(ins->val, 100);
    CHECK(size(&om).count, 4);
    CHECK_END_FN(ccc_om_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_overwrite)
{
    ccc_ordered_map om = om_init(om, struct val, elem, key, id_cmp, NULL, NULL);

    struct val q = {.key = 137, .val = 99};
    ccc_entry ent = swap_entry(&om, &q.elem, &(struct val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);

    struct val const *v = unwrap(entry_r(&om, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    struct val r = (struct val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_entry old_ent = swap_entry(&om, &r.elem, &(struct val){}.elem);
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

CHECK_BEGIN_STATIC_FN(omap_test_insert_then_bad_ideas)
{
    ccc_ordered_map om = om_init(om, struct val, elem, key, id_cmp, NULL, NULL);
    struct val q = {.key = 137, .val = 99};
    ccc_entry ent = swap_entry(&om, &q.elem, &(struct val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    struct val const *v = unwrap(entry_r(&om, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    struct val r = (struct val){.key = 137, .val = 100};

    ent = swap_entry(&om, &r.elem, &(struct val){}.elem);
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

CHECK_BEGIN_STATIC_FN(omap_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);
    size_t const size = 200;

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d
            = or_insert(entry_r(&om, &def.key), &def.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d
            = or_insert(om_and_modify_w(entry_r(&om, &def.key), struct val,
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
    CHECK(size(&om).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val *const in = or_insert(entry_r(&om, &def.key), &def.elem);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&om).count, (size / 2));
    CHECK_END_FN(om_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d
            = insert_entry(entry_r(&om, &def.key), &def.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
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
    CHECK(size(&om).count, (size / 2));
    CHECK_END_FN(om_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = insert_entry(entry_r(&om, &i), &(struct val){i, i, {}}.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = insert_entry(entry_r(&om, &i), &(struct val){i, i + 1, {}}.elem);
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
    CHECK(size(&om).count, (size / 2));
    CHECK_END_FN(om_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d
            = om_or_insert_w(entry_r(&om, &i), omap_create(i, i));
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = om_or_insert_w(
            and_modify(entry_r(&om, &i), omap_modplus), omap_create(i, i));
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
    CHECK(size(&om).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v = om_or_insert_w(entry_r(&om, &i), (struct val){});
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(size(&om).count, (size / 2));
    CHECK_END_FN(om_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_two_sum)
{
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (size_t)(sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct val const *const other_addend
            = get_key_val(&om, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_entry const e = insert_or_assign(
            &om, &(struct val){.key = addends[i], .val = i}.elem);
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN(om_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_resize)
{
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);

    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&om, &elem.key), &elem.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&om), true);
    }
    CHECK(size(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_r(&om, &swap_slot.key), &swap_slot.elem);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(om_clear(&om, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_resize_macros)
{
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&om, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = om_or_insert_w(
            om_and_modify_w(entry_r(&om, &shuffled_index), struct val,
                            {
                                T->val = shuffled_index;
                            }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = om_or_insert_w(entry_r(&om, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&om, &shuffled_index);
        CHECK(v != NULL, true);
        CHECK(v->val, i);
    }
    CHECK(om_clear(&om, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_resize_fom_null)
{
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&om, &elem.key), &elem.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_r(&om, &swap_slot.key), &swap_slot.elem);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(om_clear(&om, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_resize_fom_null_macros)
{
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&om, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = om_or_insert_w(
            om_and_modify_w(entry_r(&om, &shuffled_index), struct val,
                            {
                                T->val = shuffled_index;
                            }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = om_or_insert_w(entry_r(&om, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&om, &shuffled_index);
        CHECK(v == NULL, false);
        CHECK(v->val, i);
    }
    CHECK(om_clear(&om, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_and_find)
{
    int const size = 101;
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);

    for (int i = 0; i < size; i += 2)
    {
        ccc_entry e = try_insert(&om, &(struct val){.key = i, .val = i}.elem);
        CHECK(occupied(&e), false);
        CHECK(validate(&om), true);
        e = try_insert(&om, &(struct val){.key = i, .val = i}.elem);
        CHECK(occupied(&e), true);
        CHECK(validate(&om), true);
        struct val const *const v = unwrap(&e);
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
    CHECK_END_FN(om_clear(&om, NULL););
}

CHECK_BEGIN_STATIC_FN(omap_test_insert_shuffle)
{
    size_t const size = 50;
    ccc_ordered_map om = om_init(om, struct val, elem, key, id_cmp, NULL, NULL);
    struct val vals[50] = {};
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

CHECK_BEGIN_STATIC_FN(omap_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    ccc_ordered_map om
        = om_init(om, struct val, elem, key, id_cmp, std_alloc, NULL);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        ccc_entry const e = swap_entry(
            &om, &(struct val){.key = rand() /* NOLINT */, .val = i}.elem,
            &(struct val){}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&om), true);
    }
    CHECK(size(&om).count, (size_t)num_nodes);
    CHECK_END_FN(om_clear(&om, NULL););
}

int
main()
{
    return CHECK_RUN(
        omap_test_insert(), omap_test_insert_macros(),
        omap_test_insert_and_find(), omap_test_insert_overwrite(),
        omap_test_insert_then_bad_ideas(), omap_test_insert_via_entry(),
        omap_test_insert_via_entry_macros(), omap_test_entry_api_functional(),
        omap_test_entry_api_macros(), omap_test_two_sum(), omap_test_resize(),
        omap_test_resize_macros(), omap_test_resize_fom_null(),
        omap_test_resize_fom_null_macros(), omap_test_insert_weak_srand(),
        omap_test_insert_shuffle());
}
