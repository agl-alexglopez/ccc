/** This file dedicated to testing the Entry Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the handle functions. */
#define HANDLE_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_ordered_map.h"
#include "homap_util.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

static inline struct val
val(int const val)
{
    return (struct val){.val = val};
}

static inline struct val
idval(int const id, int const val)
{
    return (struct val){.id = id, .val = val};
}

static inline void
plus(ccc_any_type const t)
{
    ((struct val *)t.any_type)->val++;
}

static inline void
plusaux(ccc_any_type const t)
{
    ((struct val *)t.any_type)->val += *(int *)t.aux;
}

/* Every test should have three uses of each tested function: one when the
   container is empty, one when the container has a few elements and one when
   the container has many elements. If the function has different behavior
   given an element being present or absent, each possibility should be
   tested at each of those three stages. */

/* Fills the container with n elements with id and val starting at the provided
   value and incrementing by 1 until n is reached. Assumes id_and_val are
   not present by key in the table and all subsequent inserts are unique. */
CHECK_BEGIN_STATIC_FN(fill_n, ccc_handle_ordered_map *const hom, size_t const n,
                      int id_and_val)
{
    for (size_t i = 0; i < n; ++i, ++id_and_val)
    {
        ccc_handle ent = swap_handle(
            hom, &(struct val){.id = id_and_val, .val = id_and_val}.elem);
        CHECK(insert_error(&ent), false);
        CHECK(occupied(&ent), false);
        CHECK(validate(hom), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(homap_test_validate)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[3]){}, elem, id, id_cmp, nullptr, nullptr, 3);
    ccc_handle ent = swap_handle(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, 1);
    ent = swap_handle(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, 1);
    struct val *v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert)
{
    int size = 30;
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    ccc_handle ent = swap_handle(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, 1);
    ent = swap_handle(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, 1);
    struct val *v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = swap_handle(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = swap_handle(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = swap_handle(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = swap_handle(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_remove)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    ccc_handle ent = ccc_remove(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, 0);
    ent = swap_handle(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, 1);
    ent = ccc_remove(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, 0);
    struct val *v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = ccc_remove(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i);
    ent = swap_handle(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i + 1);
    ent = ccc_remove(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, i);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i);
    CHECK(v->id, i);

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = ccc_remove(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i);
    ent = swap_handle(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i + 1);
    ent = ccc_remove(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, i);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_try_insert)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    ccc_handle ent = try_insert(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, 1);
    ent = try_insert(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, 1);
    struct val *v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = try_insert(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = try_insert(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = try_insert(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = try_insert(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_try_insert_with)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    ccc_handle *ent = hom_try_insert_w(&hom, -1, val(-1));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, 1);
    ent = hom_try_insert_w(&hom, -1, val(-1));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, 1);
    struct val *v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = hom_try_insert_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = hom_try_insert_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = hom_try_insert_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = hom_try_insert_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_or_assign)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    ccc_handle ent
        = insert_or_assign(&hom, &(struct val){.id = -1, .val = -1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, 1);
    ent = insert_or_assign(&hom, &(struct val){.id = -1, .val = -2}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, 1);
    struct val *v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = insert_or_assign(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = insert_or_assign(&hom, &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = insert_or_assign(&hom, &(struct val){.id = i, .val = i}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = insert_or_assign(&hom, &(struct val){.id = i, .val = i + 1}.elem);
    CHECK(validate(&hom), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(&ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_or_assign_with)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    ccc_handle *ent = hom_insert_or_assign_w(&hom, -1, val(-1));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, 1);
    ent = hom_insert_or_assign_w(&hom, -1, val(-2));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, 1);
    struct val *v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -2);
    CHECK(v->id, -1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = hom_insert_or_assign_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = hom_insert_or_assign_w(&hom, i, val(i + 1));
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = hom_insert_or_assign_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 2);
    ent = hom_insert_or_assign_w(&hom, i, val(i + 1));
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_handle_and_modify)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    ccc_homap_handle *ent = handle_r(&hom, &(int){-1});
    CHECK(validate(&hom), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, 0);
    ent = and_modify(ent, plus);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, 0);
    (void)hom_insert_or_assign_w(&hom, -1, val(-1));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, 1);
    struct val *v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = and_modify(ent, plus);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hom, &i);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 1);
    (void)hom_insert_or_assign_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, i + 2);
    ent = and_modify(ent, plus);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = handle_r(&hom, &i);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 1);
    (void)hom_insert_or_assign_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, i + 2);
    ent = and_modify(ent, plus);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_handle_and_modify_aux)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    int aux = 1;
    ccc_homap_handle *ent = handle_r(&hom, &(int){-1});
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, 0);
    (void)hom_insert_or_assign_w(&hom, -1, val(-1));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&hom).count, 1);
    struct val *v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hom, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 1);
    (void)hom_insert_or_assign_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&hom).count, i + 2);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = handle_r(&hom, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 1);
    (void)hom_insert_or_assign_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&hom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_handle_and_modify_with)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    ccc_homap_handle *ent = handle_r(&hom, &(int){-1});
    ent = hom_and_modify_w(ent, struct val, { T->val++; });
    CHECK(size(&hom).count, 0);
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, 0);
    (void)hom_insert_or_assign_w(&hom, -1, val(-1));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &(int){-1});
    struct val *v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, -1);
    CHECK(v->id, -1);
    ent = hom_and_modify_w(ent, struct val, { T->val++; });
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, 0);
    CHECK(size(&hom).count, 1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hom, &i);
    ent = hom_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 1);
    (void)hom_insert_or_assign_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &i);
    ent = hom_and_modify_w(ent, struct val, { T->val++; });
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&hom).count, i + 2);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    ent = handle_r(&hom, &i);
    ent = hom_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(size(&hom).count, i + 1);
    (void)hom_insert_or_assign_w(&hom, i, val(i));
    CHECK(validate(&hom), true);
    ent = handle_r(&hom, &i);
    ent = hom_and_modify_w(ent, struct val, { T->val++; });
    v = hom_at(&hom, unwrap(ent));
    CHECK(v != nullptr, true);
    CHECK(v->val, i + 1);
    CHECK(v->id, i);
    CHECK(size(&hom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_or_insert)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    struct val *v
        = hom_at(&hom, or_insert(handle_r(&hom, &(int){-1}),
                                 &(struct val){.id = -1, .val = -1}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hom).count, 1);
    v = hom_at(&hom, or_insert(handle_r(&hom, &(int){-1}),
                               &(struct val){.id = -1, .val = -2}.elem));
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hom).count, 1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    v = hom_at(&hom, or_insert(handle_r(&hom, &i),
                               &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, or_insert(handle_r(&hom, &i),
                               &(struct val){.id = i, .val = i + 1}.elem));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    v = hom_at(&hom, or_insert(handle_r(&hom, &i),
                               &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, or_insert(handle_r(&hom, &i),
                               &(struct val){.id = i, .val = i + 1}.elem));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_or_insert_with)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    struct val *v = hom_at(
        &hom, hom_or_insert_w(handle_r(&hom, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hom).count, 1);
    v = hom_at(&hom,
               hom_or_insert_w(handle_r(&hom, &(int){-1}), idval(-1, -2)));
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hom).count, 1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    v = hom_at(&hom, hom_or_insert_w(handle_r(&hom, &i), idval(i, i)));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, hom_or_insert_w(handle_r(&hom, &i), idval(i, i + 1)));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    v = hom_at(&hom, hom_or_insert_w(handle_r(&hom, &i), idval(i, i)));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, hom_or_insert_w(handle_r(&hom, &i), idval(i, i + 1)));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_handle)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    struct val *v
        = hom_at(&hom, insert_handle(handle_r(&hom, &(int){-1}),
                                     &(struct val){.id = -1, .val = -1}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hom).count, 1);
    v = hom_at(&hom, insert_handle(handle_r(&hom, &(int){-1}),
                                   &(struct val){.id = -1, .val = -2}.elem));
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(size(&hom).count, 1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    v = hom_at(&hom, insert_handle(handle_r(&hom, &i),
                                   &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, insert_handle(handle_r(&hom, &i),
                                   &(struct val){.id = i, .val = i + 1}.elem));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hom).count, i + 2);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    v = hom_at(&hom, insert_handle(handle_r(&hom, &i),
                                   &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, insert_handle(handle_r(&hom, &i),
                                   &(struct val){.id = i, .val = i + 1}.elem));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_insert_handle_with)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    struct val *v = hom_at(
        &hom, hom_insert_handle_w(handle_r(&hom, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hom).count, 1);
    v = hom_at(&hom,
               hom_insert_handle_w(handle_r(&hom, &(int){-1}), idval(-1, -2)));
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -2);
    CHECK(size(&hom).count, 1);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    v = hom_at(&hom, hom_insert_handle_w(handle_r(&hom, &i), idval(i, i)));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, hom_insert_handle_w(handle_r(&hom, &i), idval(i, i + 1)));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hom).count, i + 2);
    ++i;

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    v = hom_at(&hom, hom_insert_handle_w(handle_r(&hom, &i), idval(i, i)));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 2);
    v = hom_at(&hom, hom_insert_handle_w(handle_r(&hom, &i), idval(i, i + 1)));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hom).count, i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_remove_handle)
{
    ccc_handle_ordered_map hom
        = hom_init((struct val[33]){}, elem, id, id_cmp, nullptr, nullptr, 33);
    int size = 30;
    struct val *v
        = hom_at(&hom, or_insert(handle_r(&hom, &(int){-1}),
                                 &(struct val){.id = -1, .val = -1}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hom).count, 1);
    ccc_handle *e = remove_handle_r(handle_r(&hom, &(int){-1}));
    CHECK(validate(&hom), true);
    CHECK(occupied(e), true);
    v = hom_at(&hom, unwrap(e));
    CHECK(v != nullptr, true);
    CHECK(v->id, -1);
    CHECK(v->val, -1);
    CHECK(size(&hom).count, 0);
    int i = 0;

    CHECK(fill_n(&hom, size / 2, i), PASS);

    i += (size / 2);
    v = hom_at(&hom, or_insert(handle_r(&hom, &i),
                               &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 1);
    e = remove_handle_r(handle_r(&hom, &i));
    CHECK(validate(&hom), true);
    CHECK(occupied(e), true);
    v = hom_at(&hom, unwrap(e));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i);

    CHECK(fill_n(&hom, size - i, i), PASS);

    i = size;
    v = hom_at(&hom, or_insert(handle_r(&hom, &i),
                               &(struct val){.id = i, .val = i}.elem));
    CHECK(validate(&hom), true);
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i + 1);
    e = remove_handle_r(handle_r(&hom, &i));
    CHECK(validate(&hom), true);
    CHECK(occupied(e), true);
    v = hom_at(&hom, unwrap(e));
    CHECK(v != nullptr, true);
    CHECK(v->id, i);
    CHECK(v->val, i);
    CHECK(size(&hom).count, i);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(
        homap_test_insert(), homap_test_remove(), homap_test_validate(),
        homap_test_try_insert(), homap_test_try_insert_with(),
        homap_test_insert_or_assign(), homap_test_insert_or_assign_with(),
        homap_test_handle_and_modify(), homap_test_handle_and_modify_aux(),
        homap_test_handle_and_modify_with(), homap_test_or_insert(),
        homap_test_or_insert_with(), homap_test_insert_handle(),
        homap_test_insert_handle_with(), homap_test_remove_handle());
}
