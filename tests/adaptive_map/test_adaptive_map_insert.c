#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define ADAPTIVE_MAP_USING_NAMESPACE_CCC

#include "adaptive_map.h"
#include "adaptive_map_utility.h"
#include "checkers.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

static inline struct Val
adaptive_map_create(int const id, int const val)
{
    return (struct Val){.key = id, .val = val};
}

static inline void
adaptive_map_modplus(CCC_Type_context const t)
{
    ((struct Val *)t.type)->val++;
}

check_static_begin(adaptive_map_test_insert)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, NULL, NULL);

    /* Nothing was there before so nothing is in the entry. */
    CCC_Entry ent = swap_entry(&om, &(struct Val){.key = 137, .val = 99}.elem,
                               &(struct Val){}.elem);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    check(count(&om).count, 1);
    check_end();
}

check_static_begin(adaptive_map_test_insert_macros)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);

    struct Val const *ins = CCC_adaptive_map_or_insert_w(
        entry_r(&om, &(int){2}), (struct Val){.key = 2, .val = 0});
    check(ins != NULL, true);
    check(validate(&om), true);
    check(count(&om).count, 1);
    ins = adaptive_map_insert_entry_w(entry_r(&om, &(int){2}),
                                      (struct Val){.key = 2, .val = 0});
    check(validate(&om), true);
    check(ins != NULL, true);
    ins = adaptive_map_insert_entry_w(entry_r(&om, &(int){9}),
                                      (struct Val){.key = 9, .val = 1});
    check(validate(&om), true);
    check(ins != NULL, true);
    ins = CCC_entry_unwrap(
        adaptive_map_insert_or_assign_w(&om, 3, (struct Val){.val = 99}));
    check(validate(&om), true);
    check(ins == NULL, false);
    check(validate(&om), true);
    check(ins->val, 99);
    check(count(&om).count, 3);
    ins = CCC_entry_unwrap(
        adaptive_map_insert_or_assign_w(&om, 3, (struct Val){.val = 98}));
    check(validate(&om), true);
    check(ins == NULL, false);
    check(ins->val, 98);
    check(count(&om).count, 3);
    ins = CCC_entry_unwrap(
        adaptive_map_try_insert_w(&om, 3, (struct Val){.val = 100}));
    check(ins == NULL, false);
    check(validate(&om), true);
    check(ins->val, 98);
    check(count(&om).count, 3);
    ins = CCC_entry_unwrap(
        adaptive_map_try_insert_w(&om, 4, (struct Val){.val = 100}));
    check(ins == NULL, false);
    check(validate(&om), true);
    check(ins->val, 100);
    check(count(&om).count, 4);
    check_end(CCC_adaptive_map_clear(&om, NULL););
}

check_static_begin(adaptive_map_test_insert_overwrite)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, NULL, NULL);

    struct Val q = {.key = 137, .val = 99};
    CCC_Entry ent = swap_entry(&om, &q.elem, &(struct Val){}.elem);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);

    struct Val const *v = unwrap(entry_r(&om, &q.key));
    check(v != NULL, true);
    check(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    struct Val r = (struct Val){.key = 137, .val = 100};

    /* The contents of q are now in the table. */
    CCC_Entry old_ent = swap_entry(&om, &r.elem, &(struct Val){}.elem);
    check(occupied(&old_ent), true);

    /* The old contents are now in q and the entry is in the table. */
    v = unwrap(&old_ent);
    check(v != NULL, true);
    check(v->val, 99);
    check(r.val, 99);
    v = unwrap(entry_r(&om, &r.key));
    check(v != NULL, true);
    check(v->val, 100);
    check_end();
}

check_static_begin(adaptive_map_test_insert_then_bad_ideas)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, NULL, NULL);
    struct Val q = {.key = 137, .val = 99};
    CCC_Entry ent = swap_entry(&om, &q.elem, &(struct Val){}.elem);
    check(occupied(&ent), false);
    check(unwrap(&ent), NULL);
    struct Val const *v = unwrap(entry_r(&om, &q.key));
    check(v != NULL, true);
    check(v->val, 99);

    struct Val r = (struct Val){.key = 137, .val = 100};

    ent = swap_entry(&om, &r.elem, &(struct Val){}.elem);
    check(occupied(&ent), true);
    v = unwrap(&ent);
    check(v != NULL, true);
    check(v->val, 99);
    check(r.val, 99);
    r.val -= 9;

    v = get_key_val(&om, &q.key);
    check(v != NULL, true);
    check(v->val, 100);
    check(r.val, 90);
    check_end();
}

check_static_begin(adaptive_map_test_entry_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
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
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val const *const d = or_insert(
            adaptive_map_and_modify_w(entry_r(&om, &def.key), struct Val,
                                      {
                                          T->val++;
                                      }),
            &def.elem);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->key, i);
        if (i % 2)
        {
            check(d->val, i);
        }
        else
        {
            check(d->val, i + 1);
        }
        check(d->val % 2, true);
    }
    check(count(&om).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i;
        struct Val *const in = or_insert(entry_r(&om, &def.key), &def.elem);
        in->val++;
        /* All values in the array should be odd now */
        check((in->val % 2 == 0), true);
    }
    check(count(&om).count, (size / 2));
    check_end(adaptive_map_clear(&om, NULL););
}

check_static_begin(adaptive_map_test_insert_via_entry)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
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
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.key = (int)i;
        def.val = (int)i + 1;
        struct Val const *const d
            = insert_entry(entry_r(&om, &def.key), &def.elem);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->val, i + 1);
        if (i % 2)
        {
            check(d->val % 2 == 0, true);
        }
        else
        {
            check(d->val % 2, true);
        }
    }
    check(count(&om).count, (size / 2));
    check_end(adaptive_map_clear(&om, NULL););
}

check_static_begin(adaptive_map_test_insert_via_entry_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    size_t const size = 200;
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct Val const *const d
            = insert_entry(entry_r(&om, &i), &(struct Val){i, i, {}}.elem);
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct Val const *const d
            = insert_entry(entry_r(&om, &i), &(struct Val){i, i + 1, {}}.elem);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->val, i + 1);
        if (i % 2)
        {
            check(d->val % 2 == 0, true);
        }
        else
        {
            check(d->val % 2, true);
        }
    }
    check(count(&om).count, (size / 2));
    check_end(adaptive_map_clear(&om, NULL););
}

check_static_begin(adaptive_map_test_entry_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);

    /* Test entry or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct Val const *const d = adaptive_map_or_insert_w(
            entry_r(&om, &i), adaptive_map_create(i, i));
        check((d != NULL), true);
        check(d->key, i);
        check(d->val, i);
    }
    check(count(&om).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val const *const d = adaptive_map_or_insert_w(
            and_modify(entry_r(&om, &i), adaptive_map_modplus),
            adaptive_map_create(i, i));
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->key, i);
        if (i % 2)
        {
            check(d->val, i);
        }
        else
        {
            check(d->val, i + 1);
        }
        check(d->val % 2, true);
    }
    check(count(&om).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val *v
            = adaptive_map_or_insert_w(entry_r(&om, &i), (struct Val){});
        check(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        check(v->val % 2 == 0, true);
    }
    check(count(&om).count, (size / 2));
    check_end(adaptive_map_clear(&om, NULL););
}

check_static_begin(adaptive_map_test_two_sum)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
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
        check(insert_error(&e), false);
    }
    check(solution_indices[0], 8);
    check(solution_indices[1], 2);
    check_end(adaptive_map_clear(&om, NULL););
}

check_static_begin(adaptive_map_test_resize)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);

    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.key = shuffled_index, .val = i};
        struct Val *v = insert_entry(entry_r(&om, &elem.key), &elem.elem);
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
        check(validate(&om), true);
    }
    check(count(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index, {}};
        struct Val const *const in_table
            = insert_entry(entry_r(&om, &swap_slot.key), &swap_slot.elem);
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
    }
    check(adaptive_map_clear(&om, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(adaptive_map_test_resize_macros)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val *v = insert_entry(entry_r(&om, &shuffled_index),
                                     &(struct Val){shuffled_index, i, {}}.elem);
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
    }
    check(count(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val const *const in_table = adaptive_map_or_insert_w(
            adaptive_map_and_modify_w(entry_r(&om, &shuffled_index), struct Val,
                                      {
                                          T->val = shuffled_index;
                                      }),
            (struct Val){});
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
        struct Val *v = adaptive_map_or_insert_w(entry_r(&om, &shuffled_index),
                                                 (struct Val){});
        check(v == NULL, false);
        v->val = i;
        v = get_key_val(&om, &shuffled_index);
        check(v != NULL, true);
        check(v->val, i);
    }
    check(adaptive_map_clear(&om, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(adaptive_map_test_resize_fadaptive_map_null)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.key = shuffled_index, .val = i};
        struct Val *v = insert_entry(entry_r(&om, &elem.key), &elem.elem);
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
    }
    check(count(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index, {}};
        struct Val const *const in_table
            = insert_entry(entry_r(&om, &swap_slot.key), &swap_slot.elem);
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
    }
    check(adaptive_map_clear(&om, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(adaptive_map_test_resize_fadaptive_map_null_macros)
{
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val *v = insert_entry(entry_r(&om, &shuffled_index),
                                     &(struct Val){shuffled_index, i, {}}.elem);
        check(v != NULL, true);
        check(v->key, shuffled_index);
        check(v->val, i);
    }
    check(count(&om).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val const *const in_table = adaptive_map_or_insert_w(
            adaptive_map_and_modify_w(entry_r(&om, &shuffled_index), struct Val,
                                      {
                                          T->val = shuffled_index;
                                      }),
            (struct Val){});
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
        struct Val *v = adaptive_map_or_insert_w(entry_r(&om, &shuffled_index),
                                                 (struct Val){});
        check(v == NULL, false);
        v->val = i;
        v = get_key_val(&om, &shuffled_index);
        check(v == NULL, false);
        check(v->val, i);
    }
    check(adaptive_map_clear(&om, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(adaptive_map_test_insert_and_find)
{
    int const size = 101;
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);

    for (int i = 0; i < size; i += 2)
    {
        CCC_Entry e = try_insert(&om, &(struct Val){.key = i, .val = i}.elem);
        check(occupied(&e), false);
        check(validate(&om), true);
        e = try_insert(&om, &(struct Val){.key = i, .val = i}.elem);
        check(occupied(&e), true);
        check(validate(&om), true);
        struct Val const *const v = unwrap(&e);
        check(v == NULL, false);
        check(v->key, i);
        check(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        check(contains(&om, &i), true);
        check(occupied(entry_r(&om, &i)), true);
        check(validate(&om), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        check(contains(&om, &i), false);
        check(occupied(entry_r(&om, &i)), false);
        check(validate(&om), true);
    }
    check_end(adaptive_map_clear(&om, NULL););
}

check_static_begin(adaptive_map_test_insert_shuffle)
{
    size_t const size = 50;
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, NULL, NULL);
    struct Val vals[50] = {};
    check(size > 1, true);
    int const prime = 53;
    check(insert_shuffled(&om, vals, size, prime), CHECK_PASS);
    int sorted_check[50];
    check(inorder_fill(sorted_check, size, &om), size);
    for (size_t i = 1; i < size; ++i)
    {
        check(sorted_check[i - 1] <= sorted_check[i], true);
    }
    check_end();
}

check_static_begin(adaptive_map_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    CCC_Adaptive_map om = adaptive_map_initialize(om, struct Val, elem, key,
                                                  id_order, std_allocate, NULL);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Entry const e = swap_entry(
            &om, &(struct Val){.key = rand() /* NOLINT */, .val = i}.elem,
            &(struct Val){}.elem);
        check(insert_error(&e), false);
        check(validate(&om), true);
    }
    check(count(&om).count, (size_t)num_nodes);
    check_end(adaptive_map_clear(&om, NULL););
}

int
main()
{
    return check_run(
        adaptive_map_test_insert(), adaptive_map_test_insert_macros(),
        adaptive_map_test_insert_and_find(),
        adaptive_map_test_insert_overwrite(),
        adaptive_map_test_insert_then_bad_ideas(),
        adaptive_map_test_insert_via_entry(),
        adaptive_map_test_insert_via_entry_macros(),
        adaptive_map_test_entry_api_functional(),
        adaptive_map_test_entry_api_macros(), adaptive_map_test_two_sum(),
        adaptive_map_test_resize(), adaptive_map_test_resize_macros(),
        adaptive_map_test_resize_fadaptive_map_null(),
        adaptive_map_test_resize_fadaptive_map_null_macros(),
        adaptive_map_test_insert_weak_srand(),
        adaptive_map_test_insert_shuffle());
}
