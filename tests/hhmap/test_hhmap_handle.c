/** This file dedicated to testing the handle Interface. The interface has
grown significantly requiring a dedicated file to test all code paths in all
the handle functions. */
#define HANDLE_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_hash_map.h"
#include "hhmap_util.h"
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
idval(int const key, int const val)
{
    return (struct val){.key = key, .val = val};
}

static inline void
plus(ccc_user_type const t)
{
    ((struct val *)t.user_type)->val++;
}

static inline void
plusaux(ccc_user_type const t)
{
    ((struct val *)t.user_type)->val += *(int *)t.aux;
}

/* Every test should have three uses of each tested function: one when the
   container is empty, one when the container has a few elements and one when
   the container has many elements. If the function has different behavior
   given an element being present or absent, each possibility should be
   tested at each of those three stages. */

/* Fills the container with n elements with id and val starting at the provided
   value and incrementing by 1 until n is reached. Assumes id_and_val are
   not present by key in the table and all subsequent inserts are unique. */
CHECK_BEGIN_STATIC_FN(fill_n, ccc_handle_hash_map *const hh, ptrdiff_t const n,
                      int id_and_val)
{
    for (ptrdiff_t i = 0; i < n; ++i, ++id_and_val)
    {
        ccc_handle ent = swap_handle(
            hh, &(struct val){.key = id_and_val, .val = id_and_val}.e);
        CHECK(insert_error(&ent), false);
        CHECK(occupied(&ent), false);
        CHECK(validate(hh), true);
    }
    CHECK_END_FN();
}

/* Internally there is some maintenance to perform when swapping values for
   the user on insert. Leave this test here to always catch this. */
CHECK_BEGIN_STATIC_FN(hhmap_test_validate)
{
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);

    ccc_handle ent = swap_handle(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), 1);
    ent = swap_handle(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), 1);
    struct val *v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    ccc_handle ent = swap_handle(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), 1);
    ent = swap_handle(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), 1);
    struct val *v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = swap_handle(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i + 2);
    ent = swap_handle(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = swap_handle(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i + 2);
    ent = swap_handle(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_remove)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    ccc_handle ent = remove(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), 0);
    ent = swap_handle(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), 1);
    struct val rem = {.key = -1, .val = -1};
    ent = remove(&hh, &rem.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), 0);
    CHECK(rem.val, -1);
    CHECK(rem.key, -1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = remove(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i);
    ent = swap_handle(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i + 1);
    rem = (struct val){.key = i, .val = i};
    ent = remove(&hh, &rem.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), i);
    CHECK(rem.val, i);
    CHECK(rem.key, i);

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = remove(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i);
    ent = swap_handle(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i + 1);
    rem = (struct val){.key = i, .val = i};
    ent = remove(&hh, &rem.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), i);
    CHECK(rem.val, i);
    CHECK(rem.key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_try_insert)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    ccc_handle ent = try_insert(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), 1);
    ent = try_insert(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), 1);
    struct val *v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = try_insert(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i + 2);
    ent = try_insert(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = try_insert(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i + 2);
    ent = try_insert(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_try_insert_with)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    ccc_handle *ent = hhm_try_insert_w(&hh, -1, val(-1));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), 1);
    ent = hhm_try_insert_w(&hh, -1, val(-1));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hh), 1);
    struct val *v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = hhm_try_insert_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 2);
    ent = hhm_try_insert_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = hhm_try_insert_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 2);
    ent = hhm_try_insert_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_or_assign)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    ccc_handle ent
        = insert_or_assign(&hh, &(struct val){.key = -1, .val = -1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), 1);
    ent = insert_or_assign(&hh, &(struct val){.key = -1, .val = -2}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), 1);
    struct val *v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -2);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = insert_or_assign(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i + 2);
    ent = insert_or_assign(&hh, &(struct val){.key = i, .val = i + 1}.e);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = insert_or_assign(&hh, &(struct val){.key = i, .val = i}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), i + 2);
    ent = insert_or_assign(&hh, &(struct val){.key = i, .val = i + 1}.e);
    CHECK(validate(&hh), true);
    CHECK(occupied(&ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(&ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_or_assign_with)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    ccc_handle *ent = hhm_insert_or_assign_w(&hh, -1, val(-1));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), 1);
    ent = hhm_insert_or_assign_w(&hh, -1, val(0));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hh), 1);
    struct val *v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, 0);
    CHECK(v->key, -1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = hhm_insert_or_assign_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 2);
    ent = hhm_insert_or_assign_w(&hh, i, val(i + 1));
    CHECK(occupied(ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = hhm_insert_or_assign_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 2);
    ent = hhm_insert_or_assign_w(&hh, i, val(i + 1));
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), true);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_handle_and_modify)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    ccc_hhmap_handle *ent = handle_r(&hh, &(int){-1});
    CHECK(validate(&hh), true);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), 0);
    ent = and_modify(ent, plus);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), 0);
    (void)hhm_insert_or_assign_w(&hh, -1, val(-1));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&hh), 1);
    struct val *v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = and_modify(ent, plus);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hh, &i);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 1);
    (void)hhm_insert_or_assign_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&hh), i + 2);
    ent = and_modify(ent, plus);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = handle_r(&hh, &i);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 1);
    (void)hhm_insert_or_assign_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &i);
    CHECK(occupied(ent), true);
    CHECK(size(&hh), i + 2);
    ent = and_modify(ent, plus);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_handle_and_modify_aux)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    int aux = 1;
    ccc_hhmap_handle *ent = handle_r(&hh, &(int){-1});
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), 0);
    (void)hhm_insert_or_assign_w(&hh, -1, val(-1));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &(int){-1});
    CHECK(occupied(ent), true);
    CHECK(size(&hh), 1);
    struct val *v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hh, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 1);
    (void)hhm_insert_or_assign_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(size(&hh), i + 2);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = handle_r(&hh, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 1);
    (void)hhm_insert_or_assign_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &i);
    ent = and_modify_aux(ent, plusaux, &aux);
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(size(&hh), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_handle_and_modify_with)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    ccc_hhmap_handle *ent = handle_r(&hh, &(int){-1});
    ent = hhm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(size(&hh), 0);
    CHECK(occupied(ent), false);
    CHECK(size(&hh), 0);
    (void)hhm_insert_or_assign_w(&hh, -1, val(-1));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &(int){-1});
    struct val *v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, -1);
    CHECK(v->key, -1);
    ent = hhm_and_modify_w(ent, struct val, { T->val++; });
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, 0);
    CHECK(size(&hh), 1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    ent = handle_r(&hh, &i);
    ent = hhm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 1);
    (void)hhm_insert_or_assign_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &i);
    ent = hhm_and_modify_w(ent, struct val, { T->val++; });
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(size(&hh), i + 2);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    ent = handle_r(&hh, &i);
    ent = hhm_and_modify_w(ent, struct val, { T->val++; });
    CHECK(occupied(ent), false);
    CHECK(size(&hh), i + 1);
    (void)hhm_insert_or_assign_w(&hh, i, val(i));
    CHECK(validate(&hh), true);
    ent = handle_r(&hh, &i);
    ent = hhm_and_modify_w(ent, struct val, { T->val++; });
    v = hhm_at(&hh, unwrap(ent));
    CHECK(v != NULL, true);
    CHECK(v->val, i + 1);
    CHECK(v->key, i);
    CHECK(size(&hh), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_or_insert)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    struct val *v
        = hhm_at(&hh, or_insert(handle_r(&hh, &(int){-1}),
                                &(struct val){.key = -1, .val = -1}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&hh), 1);
    v = hhm_at(&hh, or_insert(handle_r(&hh, &(int){-1}),
                              &(struct val){.key = -1, .val = -2}.e));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&hh), 1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    v = hhm_at(
        &hh, or_insert(handle_r(&hh, &i), &(struct val){.key = i, .val = i}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, or_insert(handle_r(&hh, &i),
                              &(struct val){.key = i, .val = i + 1}.e));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    v = hhm_at(
        &hh, or_insert(handle_r(&hh, &i), &(struct val){.key = i, .val = i}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, or_insert(handle_r(&hh, &i),
                              &(struct val){.key = i, .val = i + 1}.e));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_or_insert_with)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    struct val *v = hhm_at(
        &hh, hhm_or_insert_w(handle_r(&hh, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&hh), 1);
    v = hhm_at(&hh, hhm_or_insert_w(handle_r(&hh, &(int){-1}), idval(-1, -2)));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&hh), 1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    v = hhm_at(&hh, hhm_or_insert_w(handle_r(&hh, &i), idval(i, i)));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, hhm_or_insert_w(handle_r(&hh, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    v = hhm_at(&hh, hhm_or_insert_w(handle_r(&hh, &i), idval(i, i)));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, hhm_or_insert_w(handle_r(&hh, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_handle)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    struct val *v
        = hhm_at(&hh, insert_handle(handle_r(&hh, &(int){-1}),
                                    &(struct val){.key = -1, .val = -1}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&hh), 1);
    v = hhm_at(&hh, insert_handle(handle_r(&hh, &(int){-1}),
                                  &(struct val){.key = -1, .val = -2}.e));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -2);
    CHECK(size(&hh), 1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    v = hhm_at(&hh, insert_handle(handle_r(&hh, &i),
                                  &(struct val){.key = i, .val = i}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, insert_handle(handle_r(&hh, &i),
                                  &(struct val){.key = i, .val = i + 1}.e));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hh), i + 2);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    v = hhm_at(&hh, insert_handle(handle_r(&hh, &i),
                                  &(struct val){.key = i, .val = i}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, insert_handle(handle_r(&hh, &i),
                                  &(struct val){.key = i, .val = i + 1}.e));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hh), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_insert_handle_with)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    struct val *v = hhm_at(
        &hh, hhm_insert_handle_w(handle_r(&hh, &(int){-1}), idval(-1, -1)));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&hh), 1);
    v = hhm_at(&hh,
               hhm_insert_handle_w(handle_r(&hh, &(int){-1}), idval(-1, -2)));
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -2);
    CHECK(size(&hh), 1);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    v = hhm_at(&hh, hhm_insert_handle_w(handle_r(&hh, &i), idval(i, i)));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, hhm_insert_handle_w(handle_r(&hh, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hh), i + 2);
    ++i;

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    v = hhm_at(&hh, hhm_insert_handle_w(handle_r(&hh, &i), idval(i, i)));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 2);
    v = hhm_at(&hh, hhm_insert_handle_w(handle_r(&hh, &i), idval(i, i + 1)));
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i + 1);
    CHECK(size(&hh), i + 2);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_remove_handle)
{
    int size = 30;
    ccc_handle_hash_map hh
        = hhm_init((struct val[50]){}, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   NULL, NULL, 50);
    struct val *v
        = hhm_at(&hh, or_insert(handle_r(&hh, &(int){-1}),
                                &(struct val){.key = -1, .val = -1}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, -1);
    CHECK(v->val, -1);
    CHECK(size(&hh), 1);
    ccc_handle *e = remove_handle_r(handle_r(&hh, &(int){-1}));
    CHECK(validate(&hh), true);
    CHECK(occupied(e), true);
    CHECK(size(&hh), 0);
    int i = 0;

    CHECK(fill_n(&hh, size / 2, i), PASS);

    i += (size / 2);
    v = hhm_at(
        &hh, or_insert(handle_r(&hh, &i), &(struct val){.key = i, .val = i}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 1);
    e = remove_handle_r(handle_r(&hh, &i));
    CHECK(validate(&hh), true);
    CHECK(occupied(e), true);
    CHECK(size(&hh), i);

    CHECK(fill_n(&hh, size - i, i), PASS);

    i = size;
    v = hhm_at(
        &hh, or_insert(handle_r(&hh, &i), &(struct val){.key = i, .val = i}.e));
    CHECK(validate(&hh), true);
    CHECK(v != NULL, true);
    CHECK(v->key, i);
    CHECK(v->val, i);
    CHECK(size(&hh), i + 1);
    e = remove_handle_r(handle_r(&hh, &i));
    CHECK(validate(&hh), true);
    CHECK(occupied(e), true);
    CHECK(size(&hh), i);
    CHECK_END_FN();
}

int
main(void)
{
    return CHECK_RUN(
        hhmap_test_insert(), hhmap_test_remove(), hhmap_test_validate(),
        hhmap_test_try_insert(), hhmap_test_try_insert_with(),
        hhmap_test_insert_or_assign(), hhmap_test_insert_or_assign_with(),
        hhmap_test_handle_and_modify(), hhmap_test_handle_and_modify_aux(),
        hhmap_test_handle_and_modify_with(), hhmap_test_or_insert(),
        hhmap_test_or_insert_with(), hhmap_test_insert_handle(),
        hhmap_test_insert_handle_with(), hhmap_test_remove_handle());
}
