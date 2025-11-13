#include <stddef.h>
#include <stdlib.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "fhmap_util.h"
#include "flat_hash_map.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

struct owner
{
    int key;
    void *allocation;
};

CCC_Order
owners_eq(CCC_Key_comparator_context const cmp)
{
    int const *const lhs = cmp.any_key_lhs;
    struct owner const *const rhs = cmp.any_type_rhs;
    return (*lhs > rhs->key) - (*lhs < rhs->key);
}

void
destroy_owner_allocation(CCC_Type_context const t)
{
    struct owner const *const o = t.any_type;
    free(o->allocation);
}

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_then_iterate)
{
    CCC_flat_hash_map fh = fhm_initialize(&(standard_fixed_map){}, struct val,
                                          key, fhmap_int_to_u64, fhmap_id_cmp,
                                          NULL, NULL, STANDARD_FIXED_CAP);
    int const size = STANDARD_FIXED_CAP;
    for (int i = 0; i < size; i += 2)
    {
        CCC_Entry e = try_insert(&fh, &(struct val){.key = i, .val = i});
        CHECK(occupied(&e), false);
        CHECK(validate(&fh), true);
        e = try_insert(&fh, &(struct val){.key = i, .val = i});
        CHECK(occupied(&e), true);
        CHECK(validate(&fh), true);
        struct val const *const v = unwrap(&e);
        CHECK(v == NULL, false);
        CHECK(v->key, i);
        CHECK(v->val, i);
    }
    int seen = 0;
    for (int i = 0; i < size; i += 2)
    {
        CHECK(contains(&fh, &i), true);
        CHECK(occupied(entry_r(&fh, &i)), true);
        CHECK(validate(&fh), true);
        ++seen;
    }
    CHECK((size_t)seen, count(&fh).count);
    int seen2 = 0;
    for (struct val const *i = begin(&fh); i != end(&fh); i = next(&fh, i))
    {
        CHECK(i->val % 2 == 0, true);
        ++seen2;
    }
    CHECK(seen, seen2);
    CHECK_END_FN();
}

/** We want to make sure the clear and free method that uses the more
efficient iterator is able to free all elements allocated with no leaks when
run under sanitizers. */
CHECK_BEGIN_STATIC_FN(fhmap_test_insert_allocate_clear_free)
{
    CCC_flat_hash_map fh
        = fhm_initialize(NULL, struct owner, key, fhmap_int_to_u64, owners_eq,
                         std_alloc, NULL, 0);
    int const size = 32;
    for (int i = 0; i < size; ++i)
    {
        CCC_Entry *e = fhm_try_insert_w(
            &fh, i, (struct owner){.allocation = malloc(sizeof(size_t))});
        CHECK(occupied(e), CCC_FALSE);
        struct owner const *const o = unwrap(e);
        CHECK(o != NULL, CCC_TRUE);
        CHECK(o->allocation != NULL, CCC_TRUE);
    }
    CHECK_END_FN(CCC_fhm_clear_and_free(&fh, destroy_owner_allocation););
}

int
main(void)
{
    return CHECK_RUN(fhmap_test_insert_then_iterate(),
                     fhmap_test_insert_allocate_clear_free());
}
