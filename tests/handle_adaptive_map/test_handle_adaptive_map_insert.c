#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_ADAPTIVE_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_adaptive_map.h"
#include "handle_adaptive_map_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"
#include "utility/stack_allocator.h"

static inline struct Val
handle_adaptive_map_create(int const id, int const val)
{
    return (struct Val){.id = id, .val = val};
}

static inline void
handle_adaptive_map_modplus(CCC_Type_context const t)
{
    ((struct Val *)t.type)->val++;
}

check_static_begin(handle_adaptive_map_test_insert)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);

    /* Nothing was there before so nothing is in the handle. */
    CCC_Handle *hndl = swap_handle_wrap(&handle_adaptive_map,
                                        &(struct Val){.id = 137, .val = 99});
    check(occupied(hndl), false);
    check(count(&handle_adaptive_map).count, 1);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_macros)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);

    struct Val const *ins = handle_adaptive_map_at(
        &handle_adaptive_map, CCC_handle_adaptive_map_or_insert_with(
                                  handle_wrap(&handle_adaptive_map, &(int){2}),
                                  (struct Val){.id = 2, .val = 0}));
    check(ins != NULL, true);
    check(validate(&handle_adaptive_map), true);
    check(count(&handle_adaptive_map).count, 1);
    ins = handle_adaptive_map_at(
        &handle_adaptive_map, handle_adaptive_map_insert_handle_with(
                                  handle_wrap(&handle_adaptive_map, &(int){2}),
                                  (struct Val){.id = 2, .val = 0}));
    check(validate(&handle_adaptive_map), true);
    check(ins != NULL, true);
    ins = handle_adaptive_map_at(
        &handle_adaptive_map, handle_adaptive_map_insert_handle_with(
                                  handle_wrap(&handle_adaptive_map, &(int){9}),
                                  (struct Val){.id = 9, .val = 1}));
    check(validate(&handle_adaptive_map), true);
    check(ins != NULL, true);
    ins = handle_adaptive_map_at(
        &handle_adaptive_map,
        unwrap(handle_adaptive_map_insert_or_assign_with(
            &handle_adaptive_map, 3, (struct Val){.val = 99})));
    check(validate(&handle_adaptive_map), true);
    check(ins == NULL, false);
    check(validate(&handle_adaptive_map), true);
    check(ins->val, 99);
    check(count(&handle_adaptive_map).count, 3);
    ins = handle_adaptive_map_at(
        &handle_adaptive_map,
        CCC_handle_unwrap(handle_adaptive_map_insert_or_assign_with(
            &handle_adaptive_map, 3, (struct Val){.val = 98})));
    check(validate(&handle_adaptive_map), true);
    check(ins == NULL, false);
    check(ins->val, 98);
    check(count(&handle_adaptive_map).count, 3);
    ins = handle_adaptive_map_at(
        &handle_adaptive_map,
        unwrap(handle_adaptive_map_try_insert_with(&handle_adaptive_map, 3,
                                                   (struct Val){.val = 100})));
    check(ins == NULL, false);
    check(validate(&handle_adaptive_map), true);
    check(ins->val, 98);
    check(count(&handle_adaptive_map).count, 3);
    ins = handle_adaptive_map_at(
        &handle_adaptive_map,
        CCC_handle_unwrap(handle_adaptive_map_try_insert_with(
            &handle_adaptive_map, 4, (struct Val){.val = 100})));
    check(ins == NULL, false);
    check(validate(&handle_adaptive_map), true);
    check(ins->val, 100);
    check(count(&handle_adaptive_map).count, 4);
    check_end(clear_and_free(&handle_adaptive_map, NULL););
}

check_static_begin(handle_adaptive_map_test_insert_overwrite)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);

    struct Val q = {.id = 137, .val = 99};
    CCC_Handle hndl = swap_handle(&handle_adaptive_map, &q);
    check(occupied(&hndl), false);

    struct Val const *v = handle_adaptive_map_at(
        &handle_adaptive_map, unwrap(handle_wrap(&handle_adaptive_map, &q.id)));
    check(v != NULL, true);
    check(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct Val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    CCC_Handle in_table = swap_handle(&handle_adaptive_map, &q);
    check(occupied(&in_table), true);

    /* The old contents are now in q and the handle is in the table. */
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&in_table));
    check(v != NULL, true);
    check(v->val, 100);
    check(q.val, 99);
    v = handle_adaptive_map_at(
        &handle_adaptive_map, unwrap(handle_wrap(&handle_adaptive_map, &q.id)));
    check(v != NULL, true);
    check(v->val, 100);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_then_bad_ideas)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    struct Val q = {.id = 137, .val = 99};
    CCC_Handle hndl = swap_handle(&handle_adaptive_map, &q);
    check(occupied(&hndl), false);
    struct Val const *v = handle_adaptive_map_at(
        &handle_adaptive_map, unwrap(handle_wrap(&handle_adaptive_map, &q.id)));
    check(v != NULL, true);
    check(v->val, 99);

    q = (struct Val){.id = 137, .val = 100};

    hndl = swap_handle(&handle_adaptive_map, &q);
    check(occupied(&hndl), true);
    v = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&hndl));
    check(v != NULL, true);
    check(v->val, 100);
    check(q.val, 99);
    q.val -= 9;

    v = handle_adaptive_map_at(&handle_adaptive_map,
                               get_key_value(&handle_adaptive_map, &q.id));
    check(v != NULL, true);
    check(v->val, 100);
    check(q.val, 90);
    check_end();
}

check_static_begin(handle_adaptive_map_test_handle_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Standard_fixed_map){}, struct Val,
                                         id, id_order, NULL, NULL,
                                         STANDARD_FIXED_CAP);
    size_t const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct Val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct Val const *const d = handle_adaptive_map_at(
            &handle_adaptive_map,
            or_insert(handle_wrap(&handle_adaptive_map, &def.id), &def));
        check((d != NULL), true);
        check(d->id, i);
        check(d->val, i);
    }
    check(count(&handle_adaptive_map).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        CCC_Handle_index const h
            = or_insert(handle_adaptive_map_and_modify_with(
                            handle_wrap(&handle_adaptive_map, &def.id),
                            struct Val, { T->val++; }),
                        &def);
        struct Val const *const d
            = handle_adaptive_map_at(&handle_adaptive_map, h);
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->id, i);
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
    check(count(&handle_adaptive_map).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct Val *const in = handle_adaptive_map_at(
            &handle_adaptive_map,
            or_insert(handle_wrap(&handle_adaptive_map, &def.id), &def));
        in->val++;
        /* All values in the array should be odd now */
        check((in->val % 2 == 0), true);
    }
    check(count(&handle_adaptive_map).count, (size / 2));
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_via_handle)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Standard_fixed_map){}, struct Val,
                                         id, id_order, NULL, NULL,
                                         STANDARD_FIXED_CAP);
    size_t const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct Val def = {0};
    for (size_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct Val const *const d = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &def.id), &def));
        check((d != NULL), true);
        check(d->id, i);
        check(d->val, i);
    }
    check(count(&handle_adaptive_map).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i + 1;
        struct Val const *const d = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &def.id), &def));
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
    check(count(&handle_adaptive_map).count, (size / 2));
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_via_handle_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Standard_fixed_map){}, struct Val,
                                         id, id_order, NULL, NULL,
                                         STANDARD_FIXED_CAP);
    size_t const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (size_t i = 0; i < size / 2; i += 2)
    {
        struct Val const *const d = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &i),
                          &(struct Val){i, i}));
        check((d != NULL), true);
        check(d->id, i);
        check(d->val, i);
    }
    check(count(&handle_adaptive_map).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (size_t i = 0; i < size / 2; ++i)
    {
        struct Val const *const d = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &i),
                          &(struct Val){i, i + 1}));
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
    check(count(&handle_adaptive_map).count, (size / 2));
    check_end();
}

check_static_begin(handle_adaptive_map_test_handle_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Standard_fixed_map){}, struct Val,
                                         id, id_order, NULL, NULL,
                                         STANDARD_FIXED_CAP);
    int const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct Val const *const d = handle_adaptive_map_at(
            &handle_adaptive_map, handle_adaptive_map_or_insert_with(
                                      handle_wrap(&handle_adaptive_map, &i),
                                      handle_adaptive_map_create(i, i)));
        check((d != NULL), true);
        check(d->id, i);
        check(d->val, i);
    }
    check(count(&handle_adaptive_map).count, (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val const *const d = handle_adaptive_map_at(
            &handle_adaptive_map,
            handle_adaptive_map_or_insert_with(
                and_modify(handle_wrap(&handle_adaptive_map, &i),
                           handle_adaptive_map_modplus),
                handle_adaptive_map_create(i, i)));
        /* All values in the array should be odd now */
        check((d != NULL), true);
        check(d->id, i);
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
    check(count(&handle_adaptive_map).count, (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            handle_adaptive_map_or_insert_with(
                handle_wrap(&handle_adaptive_map, &i), (struct Val){}));
        check(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        check(v->val % 2 == 0, true);
    }
    check(count(&handle_adaptive_map).count, (size / 2));
    check_end();
}

check_static_begin(handle_adaptive_map_test_two_sum)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (size_t)(sizeof(addends) / sizeof(addends[0])); ++i)
    {
        struct Val const *const other_addend = handle_adaptive_map_at(
            &handle_adaptive_map,
            get_key_value(&handle_adaptive_map, &(int){target - addends[i]}));
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        CCC_Handle const e = insert_or_assign(
            &handle_adaptive_map, &(struct Val){.id = addends[i], .val = i});
        check(insert_error(&e), false);
    }
    check(solution_indices[0], 8);
    check(solution_indices[1], 2);
    check_end();
}

check_static_begin(handle_adaptive_map_test_resize)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(NULL, struct Val, id, id_order,
                                         std_allocate, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.id = shuffled_index, .val = i};
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &elem.id), &elem));
        check(v != NULL, true);
        check(v->id, shuffled_index);
        check(v->val, i);
        check(validate(&handle_adaptive_map), true);
    }
    check(count(&handle_adaptive_map).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index};
        struct Val const *const in_table = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &swap_slot.id),
                          &swap_slot));
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
    }
    check(clear_and_free(&handle_adaptive_map, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(handle_adaptive_map_test_reserve)
{
    struct Stack_allocator allocator
        = stack_allocator_initialize(Standard_fixed_map, 1);
    int const to_insert = 1000;
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_with_capacity(struct Val, id, id_order,
                                            stack_allocator_allocate,
                                            &allocator, STANDARD_FIXED_CAP - 1);
    check(handle_adaptive_map_capacity(&handle_adaptive_map).count
              >= STANDARD_FIXED_CAP - 1,
          true);
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.id = shuffled_index, .val = i};
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &elem.id), &elem));
        check(v != NULL, true);
        check(v->id, shuffled_index);
        check(v->val, i);
        check(validate(&handle_adaptive_map), true);
    }
    check(count(&handle_adaptive_map).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index};
        struct Val const *const in_table = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &swap_slot.id),
                          &swap_slot));
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
    }
    check(count(&handle_adaptive_map).count, to_insert);
    check_end(clear_and_free_reserve(&handle_adaptive_map, NULL,
                                     stack_allocator_allocate););
}

check_static_begin(handle_adaptive_map_test_resize_macros)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(NULL, struct Val, id, id_order,
                                         std_allocate, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &shuffled_index),
                          &(struct Val){shuffled_index, i}));
        check(v != NULL, true);
        check(v->id, shuffled_index);
        check(v->val, i);
    }
    check(count(&handle_adaptive_map).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        CCC_Handle_index const h = handle_adaptive_map_or_insert_with(
            handle_adaptive_map_and_modify_with(
                handle_wrap(&handle_adaptive_map, &shuffled_index), struct Val,
                { T->val = shuffled_index; }),
            (struct Val){});
        struct Val const *const in_table
            = handle_adaptive_map_at(&handle_adaptive_map, h);
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            handle_adaptive_map_or_insert_with(
                handle_wrap(&handle_adaptive_map, &shuffled_index),
                (struct Val){}));
        check(v == NULL, false);
        v->val = i;
        v = handle_adaptive_map_at(
            &handle_adaptive_map,
            get_key_value(&handle_adaptive_map, &shuffled_index));
        check(v != NULL, true);
        check(v->val, i);
    }
    check(clear_and_free(&handle_adaptive_map, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(handle_adaptive_map_test_resize_from_null)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(NULL, struct Val, id, id_order,
                                         std_allocate, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val elem = {.id = shuffled_index, .val = i};
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &elem.id), &elem));
        check(v != NULL, true);
        check(v->id, shuffled_index);
        check(v->val, i);
    }
    check(count(&handle_adaptive_map).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val swap_slot = {shuffled_index, shuffled_index};
        struct Val const *const in_table = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &swap_slot.id),
                          &swap_slot));
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
    }
    check(clear_and_free(&handle_adaptive_map, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(handle_adaptive_map_test_resize_from_null_macros)
{
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(NULL, struct Val, id, id_order,
                                         std_allocate, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &shuffled_index),
                          &(struct Val){shuffled_index, i}));
        check(v != NULL, true);
        check(v->id, shuffled_index);
        check(v->val, i);
    }
    check(count(&handle_adaptive_map).count, to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        CCC_Handle_index const h = handle_adaptive_map_or_insert_with(
            handle_adaptive_map_and_modify_with(
                handle_wrap(&handle_adaptive_map, &shuffled_index), struct Val,
                { T->val = shuffled_index; }),
            (struct Val){});
        struct Val const *const in_table
            = handle_adaptive_map_at(&handle_adaptive_map, h);
        check(in_table != NULL, true);
        check(in_table->val, shuffled_index);
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            handle_adaptive_map_or_insert_with(
                handle_wrap(&handle_adaptive_map, &shuffled_index),
                (struct Val){}));
        check(v == NULL, false);
        v->val = i;
        v = handle_adaptive_map_at(
            &handle_adaptive_map,
            get_key_value(&handle_adaptive_map, &shuffled_index));
        check(v == NULL, false);
        check(v->val, i);
    }
    check(clear_and_free(&handle_adaptive_map, NULL), CCC_RESULT_OK);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_limit)
{
    int const size = SMALL_FIXED_CAP;
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);

    int const larger_prime = 103;
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct Val *v = handle_adaptive_map_at(
            &handle_adaptive_map,
            insert_handle(handle_wrap(&handle_adaptive_map, &shuffled_index),
                          &(struct Val){shuffled_index, i}));
        if (!v)
        {
            break;
        }
        check(v->id, shuffled_index);
        check(v->val, i);
        last_index = shuffled_index;
    }
    size_t const final_size = count(&handle_adaptive_map).count;
    /* The last successful handle is still in the table and is overwritten. */
    struct Val v = {.id = last_index, .val = -1};
    CCC_Handle hndl = swap_handle(&handle_adaptive_map, &v);
    check(unwrap(&hndl) != 0, true);
    check(insert_error(&hndl), false);
    check(count(&handle_adaptive_map).count, final_size);

    v = (struct Val){.id = last_index, .val = -2};
    struct Val *in_table = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &v.id), &v));
    check(in_table != NULL, true);
    check(in_table->val, -2);
    check(count(&handle_adaptive_map).count, final_size);

    in_table = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &last_index),
                      &(struct Val){.id = last_index, .val = -3}));
    check(in_table != NULL, true);
    check(in_table->val, -3);
    check(count(&handle_adaptive_map).count, final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct Val){.id = shuffled_index, .val = -4};
    in_table = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &v.id), &v));
    check(in_table == NULL, true);
    check(count(&handle_adaptive_map).count, final_size);

    in_table = handle_adaptive_map_at(
        &handle_adaptive_map,
        insert_handle(handle_wrap(&handle_adaptive_map, &shuffled_index),
                      &(struct Val){.id = shuffled_index, .val = -4}));
    check(in_table == NULL, true);
    check(count(&handle_adaptive_map).count, final_size);

    hndl = swap_handle(&handle_adaptive_map, &v);
    check(unwrap(&hndl) == 0, true);
    check(insert_error(&hndl), true);
    check(count(&handle_adaptive_map).count, final_size);
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_and_find)
{
    int const size = SMALL_FIXED_CAP;
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);

    for (int i = 0; i < size; i += 2)
    {
        CCC_Handle e = try_insert(&handle_adaptive_map,
                                  &(struct Val){.id = i, .val = i});
        check(occupied(&e), false);
        check(validate(&handle_adaptive_map), true);
        e = try_insert(&handle_adaptive_map, &(struct Val){.id = i, .val = i});
        check(occupied(&e), true);
        check(validate(&handle_adaptive_map), true);
        struct Val const *const v
            = handle_adaptive_map_at(&handle_adaptive_map, unwrap(&e));
        check(v == NULL, false);
        check(v->id, i);
        check(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        check(contains(&handle_adaptive_map, &i), true);
        check(occupied(handle_wrap(&handle_adaptive_map, &i)), true);
        check(validate(&handle_adaptive_map), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        check(contains(&handle_adaptive_map, &i), false);
        check(occupied(handle_wrap(&handle_adaptive_map, &i)), false);
        check(validate(&handle_adaptive_map), true);
    }
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_shuffle)
{
    size_t const size = SMALL_FIXED_CAP - 1;
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Small_fixed_map){}, struct Val, id,
                                         id_order, NULL, NULL, SMALL_FIXED_CAP);
    check(size > 1, true);
    int const prime = 67;
    check(insert_shuffled(&handle_adaptive_map, size, prime), CHECK_PASS);
    int sorted_check[SMALL_FIXED_CAP - 1];
    check(inorder_fill(sorted_check, size, &handle_adaptive_map), size);
    for (size_t i = 1; i < size; ++i)
    {
        check(sorted_check[i - 1] <= sorted_check[i], true);
    }
    check_end();
}

check_static_begin(handle_adaptive_map_test_insert_weak_srand)
{
    int const num_nodes = STANDARD_FIXED_CAP - 1;
    CCC_Handle_adaptive_map handle_adaptive_map
        = handle_adaptive_map_initialize(&(Standard_fixed_map){}, struct Val,
                                         id, id_order, NULL, NULL,
                                         STANDARD_FIXED_CAP);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        CCC_Handle const e
            = swap_handle(&handle_adaptive_map,
                          &(struct Val){.id = rand() /* NOLINT */, .val = i});
        check(insert_error(&e), false);
        check(validate(&handle_adaptive_map), true);
    }
    check(count(&handle_adaptive_map).count, (size_t)num_nodes);
    check_end();
}

int
main()
{
    return check_run(handle_adaptive_map_test_insert(),
                     handle_adaptive_map_test_insert_macros(),
                     handle_adaptive_map_test_insert_and_find(),
                     handle_adaptive_map_test_insert_overwrite(),
                     handle_adaptive_map_test_insert_then_bad_ideas(),
                     handle_adaptive_map_test_insert_via_handle(),
                     handle_adaptive_map_test_insert_via_handle_macros(),
                     handle_adaptive_map_test_reserve(),
                     handle_adaptive_map_test_handle_api_functional(),
                     handle_adaptive_map_test_handle_api_macros(),
                     handle_adaptive_map_test_two_sum(),
                     handle_adaptive_map_test_resize(),
                     handle_adaptive_map_test_resize_macros(),
                     handle_adaptive_map_test_resize_from_null(),
                     handle_adaptive_map_test_resize_from_null_macros(),
                     handle_adaptive_map_test_insert_limit(),
                     handle_adaptive_map_test_insert_weak_srand(),
                     handle_adaptive_map_test_insert_shuffle());
}
