#define HANDLE_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "handle_hash_map.h"
#include "hhmap_util.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

CHECK_BEGIN_STATIC_FN(hhmap_test_erase)
{
    ccc_handle_hash_map hh
        = hhm_init((struct val[10]){}, e, key, hhmap_int_zero, hhmap_id_eq,
                   NULL, NULL, 10);

    struct val query = {.key = 137, .val = 99};
    /* Nothing was there before so nothing is in the handle. */
    ccc_handle ent = swap_handle(&hh, &query.e);
    CHECK(occupied(&ent), false);
    CHECK(unwrap(&ent) != 0, true);
    CHECK(size(&hh), 1);
    ent = remove(&hh, &query.e);
    CHECK(occupied(&ent), true);
    struct val *v = hhm_at(&hh, unwrap(&ent));
    CHECK(v == NULL, true);
    CHECK(query.key, 137);
    CHECK(query.val, 99);
    CHECK(size(&hh), 0);
    query.key = 101;
    ent = remove(&hh, &query.e);
    CHECK(occupied(&ent), false);
    CHECK(size(&hh), 0);
    ccc_hhm_insert_handle_w(handle_r(&hh, &(int){137}),
                            (struct val){.key = 137, .val = 99});
    CHECK(size(&hh), 1);
    CHECK(occupied(remove_handle_r(handle_r(&hh, &(int){137}))), true);
    CHECK(size(&hh), 0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hhmap_test_shuffle_insert_erase)
{
    ccc_handle_hash_map h
        = hhm_init((struct val *)NULL, e, key, hhmap_int_to_u64, hhmap_id_eq,
                   std_alloc, NULL, 0);

    int const to_insert = 100;
    int const larger_prime = (int)hhm_next_prime(to_insert);
    for (int i = 0, shuffle = larger_prime % to_insert; i < to_insert;
         ++i, shuffle = (shuffle + larger_prime) % to_insert)
    {
        ccc_handle const *const hndl
            = hhm_insert_or_assign_w(&h, shuffle, (struct val){.val = i});
        ccc_handle_i hndl_i = unwrap(hndl);
        struct val *const v = hhm_at(&h, hndl_i);
        CHECK(v != NULL, true);
        CHECK(v->key, shuffle);
        CHECK(v->val, i);
        bool const valid = validate(&h);
        CHECK(valid, true);
    }
    CHECK(size(&h), to_insert);
    ptrdiff_t cur_size = size(&h);
    int i = 0;
    while (!is_empty(&h) && cur_size)
    {
        CHECK(contains(&h, &i), true);
        if (i % 2)
        {
            struct val const k = {.key = i};
            ccc_handle const *const hndl = remove_r(&h, &k.e);
            CHECK(occupied(hndl), true);
            CHECK(k.key, i);
        }
        else
        {
            ccc_handle removed = remove_handle(handle_r(&h, &i));
            CHECK(occupied(&removed), true);
        }
        --cur_size;
        ++i;
        CHECK(size(&h), cur_size);
        CHECK(validate(&h), true);
    }
    CHECK(size(&h), 0);
    CHECK_END_FN(hhm_clear_and_free(&h, NULL););
}

int
main()
{
    return CHECK_RUN(hhmap_test_erase(), hhmap_test_shuffle_insert_erase());
}
