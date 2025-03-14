#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_ORDERED_MAP_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "handle_ordered_map.h"
#include "homap_util.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

static inline struct val
homap_create(int const id, int const val)
{
    return (struct val){.id = id, .val = val};
}

static inline void
homap_modplus(ccc_user_type const t)
{
    ((struct val *)t.user_type)->val++;
}

CHECK_BEGIN_STATIC_FN(homap_test_insert)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[10]){}, elem, id, id_cmp, NULL, NULL, 10);

    /* Nothing was there before so nothing is in the handle. */
    ccc_handle ent
        = swap_handle(&hom, &(struct val){.id = 137, .val = 99}.elem);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom), 1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_macros)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[10]){}, elem, id, id_cmp, NULL, NULL, 10);

    struct val const *ins
        = hom_at(&hom, hom_or_insert_w(handle_r(&hom, &(int){2}),
                                       (struct val){.id = 2, .val = 0}));
    CHECK(ins != NULL, true);
    CHECK(validate(&hom), true);
    CHECK(size(&hom), 1);
    ins = hom_at(&hom, hom_insert_handle_w(handle_r(&hom, &(int){2}),
                                           (struct val){.id = 2, .val = 0}));
    CHECK(validate(&hom), true);
    CHECK(ins != NULL, true);
    ins = hom_at(&hom, hom_insert_handle_w(handle_r(&hom, &(int){9}),
                                           (struct val){.id = 9, .val = 1}));
    CHECK(validate(&hom), true);
    CHECK(ins != NULL, true);
    ins = hom_at(
        &hom, unwrap(hom_insert_or_assign_w(&hom, 3, (struct val){.val = 99})));
    CHECK(validate(&hom), true);
    CHECK(ins == NULL, false);
    CHECK(validate(&hom), true);
    CHECK(ins->val, 99);
    CHECK(size(&hom), 3);
    ins = hom_at(
        &hom, unwrap(hom_insert_or_assign_w(&hom, 3, (struct val){.val = 98})));
    CHECK(validate(&hom), true);
    CHECK(ins == NULL, false);
    CHECK(ins->val, 98);
    CHECK(size(&hom), 3);
    ins = hom_at(&hom,
                 unwrap(hom_try_insert_w(&hom, 3, (struct val){.val = 100})));
    CHECK(ins == NULL, false);
    CHECK(validate(&hom), true);
    CHECK(ins->val, 98);
    CHECK(size(&hom), 3);
    ins = hom_at(&hom,
                 unwrap(hom_try_insert_w(&hom, 4, (struct val){.val = 100})));
    CHECK(ins == NULL, false);
    CHECK(validate(&hom), true);
    CHECK(ins->val, 100);
    CHECK(size(&hom), 4);
    CHECK_END_FN(hom_clear_and_free(&hom, NULL););
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_overwrite)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[10]){}, elem, id, id_cmp, NULL, NULL, 10);

    struct val q = {.id = 137, .val = 99};
    ccc_handle ent = swap_handle(&hom, &q.elem);
    CHECK(occupied(&ent), false);

    struct val const *v = hom_at(&hom, unwrap(handle_r(&hom, &q.id)));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    /* Now the second insertion will take place and the old occupying value
       will be written into our struct we used to make the query. */
    q = (struct val){.id = 137, .val = 100};

    /* The contents of q are now in the table. */
    ccc_handle in_table = swap_handle(&hom, &q.elem);
    CHECK(occupied(&in_table), true);

    /* The old contents are now in q and the handle is in the table. */
    v = hom_at(&hom, unwrap(&in_table));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 99);
    v = hom_at(&hom, unwrap(handle_r(&hom, &q.id)));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_then_bad_ideas)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[10]){}, elem, id, id_cmp, NULL, NULL, 10);
    struct val q = {.id = 137, .val = 99};
    ccc_handle ent = swap_handle(&hom, &q.elem);
    CHECK(occupied(&ent), false);
    struct val const *v = hom_at(&hom, unwrap(handle_r(&hom, &q.id)));
    CHECK(v != NULL, true);
    CHECK(v->val, 99);

    q = (struct val){.id = 137, .val = 100};

    ent = swap_handle(&hom, &q.elem);
    CHECK(occupied(&ent), true);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 99);
    q.val -= 9;

    v = hom_at(&hom, get_key_val(&hom, &q.id));
    CHECK(v != NULL, true);
    CHECK(v->val, 100);
    CHECK(q.val, 90);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_handle_api_functional)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ccc_handle_ordered_map hom
        = hom_init((struct val[200]){}, elem, id, id_cmp, NULL, NULL, 200);
    ptrdiff_t const size = 200;

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (ptrdiff_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = hom_at(&hom, or_insert(handle_r(&hom, &def.id), &def.elem));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hom), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (ptrdiff_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = hom_at(&hom, or_insert(hom_and_modify_w(handle_r(&hom, &def.id),
                                                      struct val,
                                                      {
                                                          T->val++;
                                                      }),
                                     &def.elem));
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
    CHECK(size(&hom), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (ptrdiff_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val *const in
            = hom_at(&hom, or_insert(handle_r(&hom, &def.id), &def.elem));
        in->val++;
        /* All values in the array should be odd now */
        CHECK((in->val % 2 == 0), true);
    }
    CHECK(size(&hom), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_via_handle)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ptrdiff_t const size = 200;
    ccc_handle_ordered_map hom
        = hom_init((struct val[200]){}, elem, id, id_cmp, NULL, NULL, 200);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    struct val def = {0};
    for (ptrdiff_t i = 0; i < size / 2; i += 2)
    {
        def.id = (int)i;
        def.val = (int)i;
        struct val const *const d
            = hom_at(&hom, insert_handle(handle_r(&hom, &def.id), &def.elem));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hom), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (ptrdiff_t i = 0; i < size / 2; ++i)
    {
        def.id = (int)i;
        def.val = (int)i + 1;
        struct val const *const d
            = hom_at(&hom, insert_handle(handle_r(&hom, &def.id), &def.elem));
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
    CHECK(size(&hom), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_via_handle_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    ptrdiff_t const size = 200;
    ccc_handle_ordered_map hom
        = hom_init((struct val[200]){}, elem, id, id_cmp, NULL, NULL, 200);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (ptrdiff_t i = 0; i < size / 2; i += 2)
    {
        struct val const *const d
            = hom_at(&hom, insert_handle(handle_r(&hom, &i),
                                         &(struct val){i, i, {}}.elem));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hom), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (ptrdiff_t i = 0; i < size / 2; ++i)
    {
        struct val const *const d
            = hom_at(&hom, insert_handle(handle_r(&hom, &i),
                                         &(struct val){i, i + 1, {}}.elem));
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
    CHECK(size(&hom), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_handle_api_macros)
{
    /* Over allocate size now because we don't want to worry about resizing. */
    int const size = 200;
    ccc_handle_ordered_map hom
        = hom_init((struct val[200]){}, elem, id, id_cmp, NULL, NULL, 200);

    /* Test handle or insert with for all even values. Default should be
       inserted. All entries are hashed to last digit so many spread out
       collisions. */
    for (int i = 0; i < size / 2; i += 2)
    {
        /* The macros support functions that will only execute if the or
           insert branch executes. */
        struct val const *const d = hom_at(
            &hom, hom_or_insert_w(handle_r(&hom, &i), homap_create(i, i)));
        CHECK((d != NULL), true);
        CHECK(d->id, i);
        CHECK(d->val, i);
    }
    CHECK(size(&hom), (size / 2) / 2);
    /* The default insertion should not occur every other element. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val const *const d = hom_at(
            &hom, hom_or_insert_w(and_modify(handle_r(&hom, &i), homap_modplus),
                                  homap_create(i, i)));
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
    CHECK(size(&hom), (size / 2));
    /* More simply modifications don't require the and modify function. All
       should be switched back to even now. */
    for (int i = 0; i < size / 2; ++i)
    {
        struct val *v
            = hom_at(&hom, hom_or_insert_w(handle_r(&hom, &i), (struct val){}));
        CHECK(v != NULL, true);
        v->val++;
        /* All values in the array should be odd now */
        CHECK(v->val % 2 == 0, true);
    }
    CHECK(size(&hom), (size / 2));
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_two_sum)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[20]){}, elem, id, id_cmp, NULL, NULL, 20);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (ptrdiff_t i = 0; i < (ptrdiff_t)(sizeof(addends) / sizeof(addends[0]));
         ++i)
    {
        struct val const *const other_addend
            = hom_at(&hom, get_key_val(&hom, &(int){target - addends[i]}));
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        ccc_handle const e = insert_or_assign(
            &hom, &(struct val){.id = addends[i], .val = i}.elem);
        CHECK(insert_error(&e), false);
    }
    CHECK(solution_indices[0], 8);
    CHECK(solution_indices[1], 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_resize)
{
    ptrdiff_t const prime_start = 11;
    ccc_handle_ordered_map hom
        = hom_init((struct val *)malloc(sizeof(struct val) * prime_start), elem,
                   id, id_cmp, std_alloc, NULL, prime_start);
    CHECK(hom_data(&hom) != NULL, true);

    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = hom_at(&hom, insert_handle(handle_r(&hom, &elem.id), &elem.elem));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        CHECK(validate(&hom), true);
    }
    CHECK(size(&hom), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = hom_at(&hom, insert_handle(handle_r(&hom, &swap_slot.id),
                                         &swap_slot.elem));
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(hom_clear_and_free(&hom, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_resize_macros)
{
    ptrdiff_t const prime_start = 11;
    ccc_handle_ordered_map hom
        = hom_init((struct val *)malloc(sizeof(struct val) * prime_start), elem,
                   id, id_cmp, std_alloc, NULL, prime_start);
    CHECK(hom_data(&hom) != NULL, true);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = hom_at(
            &hom, insert_handle(handle_r(&hom, &shuffled_index),
                                &(struct val){shuffled_index, i, {}}.elem));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hom), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = hom_at(
            &hom,
            hom_or_insert_w(hom_and_modify_w(handle_r(&hom, &shuffled_index),
                                             struct val,
                                             {
                                                 T->val = shuffled_index;
                                             }),
                            (struct val){}));
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = hom_at(&hom, hom_or_insert_w(handle_r(&hom, &shuffled_index),
                                           (struct val){}));
        CHECK(v == NULL, false);
        v->val = i;
        v = hom_at(&hom, get_key_val(&hom, &shuffled_index));
        CHECK(v != NULL, true);
        CHECK(v->val, i);
    }
    CHECK(hom_clear_and_free(&hom, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_resize_from_null)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = hom_at(&hom, insert_handle(handle_r(&hom, &elem.id), &elem.elem));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hom), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val swap_slot = {shuffled_index, shuffled_index, {}};
        struct val const *const in_table
            = hom_at(&hom, insert_handle(handle_r(&hom, &swap_slot.id),
                                         &swap_slot.elem));
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
    }
    CHECK(hom_clear_and_free(&hom, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_resize_from_null_macros)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    int const to_insert = 1000;
    int const larger_prime = 1009;
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val *v = hom_at(
            &hom, insert_handle(handle_r(&hom, &shuffled_index),
                                &(struct val){shuffled_index, i, {}}.elem));
        CHECK(v != NULL, true);
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
    }
    CHECK(size(&hom), to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val const *const in_table = hom_at(
            &hom,
            hom_or_insert_w(hom_and_modify_w(handle_r(&hom, &shuffled_index),
                                             struct val,
                                             {
                                                 T->val = shuffled_index;
                                             }),
                            (struct val){}));
        CHECK(in_table != NULL, true);
        CHECK(in_table->val, shuffled_index);
        struct val *v
            = hom_at(&hom, hom_or_insert_w(handle_r(&hom, &shuffled_index),
                                           (struct val){}));
        CHECK(v == NULL, false);
        v->val = i;
        v = hom_at(&hom, get_key_val(&hom, &shuffled_index));
        CHECK(v == NULL, false);
        CHECK(v->val, i);
    }
    CHECK(hom_clear_and_free(&hom, NULL), CCC_RESULT_OK);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_limit)
{
    int const size = 101;
    ccc_handle_ordered_map hom
        = hom_init((struct val[101]){}, elem, id, id_cmp, NULL, NULL, 101);

    int const larger_prime = 103;
    int last_index = 0;
    int shuffled_index = larger_prime % size;
    for (int i = 0; i < size;
         ++i, shuffled_index = (shuffled_index + larger_prime) % size)
    {
        struct val *v = hom_at(
            &hom, insert_handle(handle_r(&hom, &shuffled_index),
                                &(struct val){shuffled_index, i, {}}.elem));
        if (!v)
        {
            break;
        }
        CHECK(v->id, shuffled_index);
        CHECK(v->val, i);
        last_index = shuffled_index;
    }
    ptrdiff_t const final_size = size(&hom);
    /* The last successful handle is still in the table and is overwritten. */
    struct val v = {.id = last_index, .val = -1};
    ccc_handle ent = swap_handle(&hom, &v.elem);
    CHECK(insert_error(&ent), false);
    CHECK(size(&hom), final_size);

    v = (struct val){.id = last_index, .val = -2};
    struct val *in_table
        = hom_at(&hom, insert_handle(handle_r(&hom, &v.id), &v.elem));
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -2);
    CHECK(size(&hom), final_size);

    in_table = hom_at(
        &hom, insert_handle(handle_r(&hom, &last_index),
                            &(struct val){.id = last_index, .val = -3}.elem));
    CHECK(in_table != NULL, true);
    CHECK(in_table->val, -3);
    CHECK(size(&hom), final_size);

    /* The shuffled index key that failed insertion should fail again. */
    v = (struct val){.id = shuffled_index, .val = -4};
    in_table = hom_at(&hom, insert_handle(handle_r(&hom, &v.id), &v.elem));
    CHECK(in_table == NULL, true);
    CHECK(size(&hom), final_size);

    in_table = hom_at(
        &hom,
        insert_handle(handle_r(&hom, &shuffled_index),
                      &(struct val){.id = shuffled_index, .val = -4}.elem));
    CHECK(in_table == NULL, true);
    CHECK(size(&hom), final_size);

    ent = swap_handle(&hom, &v.elem);
    CHECK(insert_error(&ent), true);
    CHECK(size(&hom), final_size);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_and_find)
{
    int const size = 101;
    ccc_handle_ordered_map hom
        = hom_init((struct val[101]){}, elem, id, id_cmp, NULL, NULL, 101);

    for (int i = 0; i < size; i += 2)
    {
        ccc_handle e = try_insert(&hom, &(struct val){.id = i, .val = i}.elem);
        CHECK(occupied(&e), false);
        CHECK(validate(&hom), true);
        e = try_insert(&hom, &(struct val){.id = i, .val = i}.elem);
        CHECK(occupied(&e), true);
        CHECK(validate(&hom), true);
        struct val const *const v = hom_at(&hom, unwrap(&e));
        CHECK(v == NULL, false);
        CHECK(v->id, i);
        CHECK(v->val, i);
    }
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&hom, &i), true);
        CHECK(occupied(handle_r(&hom, &i)), true);
        CHECK(validate(&hom), true);
    }
    for (int i = 1; i < size; i += 2)
    {
        CHECK(contains(&hom, &i), false);
        CHECK(occupied(handle_r(&hom, &i)), false);
        CHECK(validate(&hom), true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_shuffle)
{
    ptrdiff_t const size = 50;
    ccc_handle_ordered_map s
        = hom_init((struct val[51]){}, elem, id, id_cmp, NULL, NULL, 51);
    CHECK(size > 1, true);
    int const prime = 53;
    CHECK(insert_shuffled(&s, size, prime), PASS);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &s), size);
    for (ptrdiff_t i = 1; i < size; ++i)
    {
        CHECK(sorted_check[i - 1] <= sorted_check[i], true);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_weak_srand)
{
    int const num_nodes = 1000;
    ccc_handle_ordered_map s
        = hom_init((struct val[1001]){}, elem, id, id_cmp, NULL, NULL, 1001);
    srand(time(NULL)); /* NOLINT */
    for (int i = 0; i < num_nodes; ++i)
    {
        ccc_handle const e = swap_handle(
            &s, &(struct val){.id = rand() /* NOLINT */, .val = i}.elem);
        CHECK(insert_error(&e), false);
        CHECK(validate(&s), true);
    }
    CHECK(size(&s), (ptrdiff_t)num_nodes);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(
        homap_test_insert(), homap_test_insert_macros(),
        homap_test_insert_and_find(), homap_test_insert_overwrite(),
        homap_test_insert_then_bad_ideas(), homap_test_insert_via_handle(),
        homap_test_insert_via_handle_macros(),
        homap_test_handle_api_functional(), homap_test_handle_api_macros(),
        homap_test_two_sum(), homap_test_resize(), homap_test_resize_macros(),
        homap_test_resize_from_null(), homap_test_resize_from_null_macros(),
        homap_test_insert_limit(), homap_test_insert_weak_srand(),
        homap_test_insert_shuffle());
}
