#define TRAITS_USING_NAMESPACE_CCC
#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "realtime_ordered_map.h"
#include "romap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static inline struct val
romap_create(int const id, int const val)
{
    return (struct val){.key = id, .val = val};
}

static inline void
romap_modplus(ccc_user_type const t)
{
    ((struct val *)t.user_type)->val++;
}

CHECK_BEGIN_STATIC_FN(romap_test_insert)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, NULL, NULL);

    /* Nothing was there before so nothing is in the entry. */
    ccc_entry ent = insert(&rom, &(struct val){.key = 137, .val = 99}.elem,
                           &(struct val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    CHECK(size(&rom), 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_macros)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);

    struct val const *ins = ccc_rom_or_insert_w(
        entry_r(&rom, &(int){2}), (struct val){.key = 2, .val = 0});
    CHECK(ins != NULL, true);
    CHECK(validate(&rom), true);
    CHECK(size(&rom), 1);
    ins = rom_insert_entry_w(entry_r(&rom, &(int){2}),
                             (struct val){.key = 2, .val = 0});
    CHECK(validate(&rom), true);
    CHECK(ins != NULL, true);
    ins = rom_insert_entry_w(entry_r(&rom, &(int){9}),
                             (struct val){.key = 9, .val = 1});
    CHECK(validate(&rom), true);
    CHECK(ins != NULL, true);
    ins = ccc_entry_unwrap(
        rom_insert_or_assign_w(&rom, 3, (struct val){.val = 99}));
    CHECK(validate(&rom), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&rom), true);
    CHECK(ins->val, 99);
    CHECK(size(&rom), 3);
    ins = ccc_entry_unwrap(
        rom_insert_or_assign_w(&rom, 3, (struct val){.val = 98}));
    CHECK(validate(&rom), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&rom), 3);
    ins = ccc_entry_unwrap(rom_try_insert_w(&rom, 3, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&rom), true);
    CHECK(ins->val, 98);
    CHECK(size(&rom), 3);
    ins = ccc_entry_unwrap(rom_try_insert_w(&rom, 4, (struct val){.val = 100}));
    CHECK(ins == NULL, false);
    CHECK(validate(&rom), true);
    CHECK(ins->val, 100);
    CHECK(size(&rom), 4);
    CHECK_END_FN(ccc_rom_clear(&rom, NULL););
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_overwrite)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, NULL, NULL);

    struct val q = {.key = 137, .val = 99};
    ccc_entry ent = insert(&rom, &q.elem, &(struct val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);

    struct val const *v = unwrap(entry_r(&rom, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    struct val r = (struct val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_entry old_ent = insert(&rom, &r.elem, &(struct val){}.elem);
    CHECK(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(r.val, 99);
    v = unwrap(entry_r(&rom, &r.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_then_bad_ideas)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, NULL, NULL);
    struct val q = {.key = 137, .val = 99};
    ccc_entry ent = insert(&rom, &q.elem, &(struct val){}.elem);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent), NULL);
    struct val const *v = unwrap(entry_r(&rom, &q.key));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    struct val r = (struct val){.key = 137, .val = 100};

    ent = insert(&rom, &r.elem, &(struct val){}.elem);
    CHECK(occupied(&ent), true);
    v = unwrap(&ent);
    CHECK(v != NULL, true);
    CHECK(v->val, 99);
    CHECK(r.val, 99);
    r.val -= 9;

    v = get_key_val(&rom, &q.key);
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(r.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);
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
            = or_insert(entry_r(&rom, &def.key), &def.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&rom), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d
            = or_insert(rom_and_modify_w(entry_r(&rom, &def.key), struct val,
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
    CHECK(size(&rom), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val *const in = or_insert(entry_r(&rom, &def.key), &def.elem);
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&rom), (size / 2));
    CHECK_END_FN(rom_clear(&rom, NULL););
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct val const *const d
            = insert_entry(entry_r(&rom, &def.key), &def.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&rom), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = insert_entry(entry_r(&rom, &def.key), &def.elem);
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
    CHECK(size(&rom), (size / 2));
    CHECK_END_FN(rom_clear(&rom, NULL););
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = insert_entry(entry_r(&rom, &i), &(struct val){i, i, {}}.elem);
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&rom), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = insert_entry(entry_r(&rom, &i), &(struct val){i, i + 1, {}}.elem);
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
    CHECK(size(&rom), (size / 2));
    CHECK_END_FN(rom_clear(&rom, NULL););
}

CHECK_BEGIN_STATIC_FN(romap_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d
            = rom_or_insert_w(entry_r(&rom, &i), romap_create(i, i));
        CHECK((d != NULL), true);
        CHECK(d->key, i);
        CHECK(d->val, i);
    }
    CHECK(size(&rom), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = rom_or_insert_w(
            and_modify(entry_r(&rom, &i), romap_modplus), romap_create(i, i));
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
    CHECK(size(&rom), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v = rom_or_insert_w(entry_r(&rom, &i), (struct val){});
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(size(&rom), (size / 2));
    CHECK_END_FN(rom_clear(&rom, NULL););
}

CHECK_BEGIN_STATIC_FN(romap_test_two_sum)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct val const *const other_addend
            = get_key_val(&rom, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_entry const e = insert_or_assign(
            &rom, &(struct val){.key = addends[i], .val = i}.elem);
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN(rom_clear(&rom, NULL););
}

CHECK_BEGIN_STATIC_FN(romap_test_resize)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);

    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&rom, &elem.key), &elem.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&rom), true);
    }
    CHECK(size(&rom), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_r(&rom, &swap_slot.key), &swap_slot.elem);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(rom_clear(&rom, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_resize_macros)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&rom, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&rom), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = rom_or_insert_w(
            rom_and_modify_w(entry_r(&rom, &shuffled_index), struct val,
                             {
                                 T->val = shuffled_index;
                             }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = rom_or_insert_w(entry_r(&rom, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&rom, &shuffled_index);
        CHECK(v != NULL, true);
        CHECK(v->val, i);
    }
    CHECK(rom_clear(&rom, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_resize_from_null)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.key = shuffled_index, .val = i};
        struct val *v = insert_entry(entry_r(&rom, &elem.key), &elem.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&rom), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = insert_entry(entry_r(&rom, &swap_slot.key), &swap_slot.elem);
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(rom_clear(&rom, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_resize_from_null_macros)
{
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = insert_entry(entry_r(&rom, &shuffled_index),
                                     &(struct val){shuffled_index, i, {}}.elem);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&rom), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = rom_or_insert_w(
            rom_and_modify_w(entry_r(&rom, &shuffled_index), struct val,
                             {
                                 T->val = shuffled_index;
                             }),
            (struct val){});
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = rom_or_insert_w(entry_r(&rom, &shuffled_index), (struct val){});
        CHECK(v == NULL, false);
        v->val = i;
        v = get_key_val(&rom, &shuffled_index);
        CHECK(v == NULL, false);
        CHECK(v->val, i);
    }
    CHECK(rom_clear(&rom, NULL), CCC_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_and_find)
{
    int const size = 101;
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);

    for (int i = 0; i < size; i += 2)
    {
        ccc_entry e = try_insert(&rom, &(struct val){.key = i, .val = i}.elem);
        CHECK(occupied(&e), false);
        CHECK(validate(&rom), true);
        e = try_insert(&rom, &(struct val){.key = i, .val = i}.elem);
        CHECK(occupied(&e), true);
        CHECK(validate(&rom), true);
        struct val const *const v = unwrap(&e);
        CHECK(v == NULL, false);
        CHECK(v->key, i);
        CHECK(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&rom, &i), true);
        CHECK(occupied(entry_r(&rom, &i)), true);
        CHECK(validate(&rom), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&rom, &i), false);
        CHECK(occupied(entry_r(&rom, &i)), false);
        CHECK(validate(&rom), true);
    }
    CHECK_END_FN(rom_clear(&rom, NULL););
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_shuffle)
{
    size_t const size = 50;
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, NULL, NULL);
    struct val vals[50] = {};
    CHECK(size > 1, true);
    int const prime = 53;
    CHECK(insert_shuffled(&rom, vals, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &rom), size);
    for (size_t i = 1; i < size; ++i)
    {
        CHECK(sorted_check[i - 1] <= sorted_check[i], true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(romap_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    ccc_realtime_ordered_map rom
        = rom_init(rom, struct val, elem, key, id_cmp, std_alloc, NULL);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        ccc_entry const e = insert(
            &rom, &(struct val){.key = rand() /* NOLINT */, .val = i}.elem,
            &(struct val){}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&rom), true);
    }
    CHECK(size(&rom), (size_t)num_nodes);
    CHECK_END_FN(rom_clear(&rom, NULL););
}

int
main()
{
    return CHECK_RUN(
        romap_test_insert(), romap_test_insert_macros(),
        romap_test_insert_and_find(), romap_test_insert_overwrite(),
        romap_test_insert_then_bad_ideas(), romap_test_insert_via_entry(),
        romap_test_insert_via_entry_macros(), romap_test_entry_api_functional(),
        romap_test_entry_api_macros(), romap_test_two_sum(),
        romap_test_resize(), romap_test_resize_macros(),
        romap_test_resize_from_null(), romap_test_resize_from_null_macros(),
        romap_test_insert_weak_srand(), romap_test_insert_shuffle());
}
