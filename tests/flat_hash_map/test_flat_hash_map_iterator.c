#include <stddef.h>
#include <stdlib.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "flat_hash_map.h"
#include "flat_hash_map_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

struct Owner
{
    int key;
    void *allocation;
};

CCC_Order
owners_eq(CCC_Key_comparator_context const order)
{
    int const *const left = order.key_left;
    struct Owner const *const right = order.type_right;
    return (*left > right->key) - (*left < right->key);
}

void
destroy_owner_allocation(CCC_Type_context const t)
{
    struct Owner const *const o = t.type;
    free(o->allocation);
}

check_static_begin(flat_hash_map_test_insert_then_iterate)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(standard_fixed_map){}, struct Val, key, flat_hash_map_int_to_u64,
        flat_hash_map_id_order, NULL, NULL, STANDARD_FIXED_CAP);
    int const size = STANDARD_FIXED_CAP;
    for (int i = 0; i < size; i += 2)
    {
        CCC_Entry e = try_insert(&fh, &(struct Val){.key = i, .val = i});
        check(occupied(&e), false);
        check(validate(&fh), true);
        e = try_insert(&fh, &(struct Val){.key = i, .val = i});
        check(occupied(&e), true);
        check(validate(&fh), true);
        struct Val const *const v = unwrap(&e);
        check(v == NULL, false);
        check(v->key, i);
        check(v->val, i);
    }
    int seen = 0;
    for (int i = 0; i < size; i += 2)
    {
        check(contains(&fh, &i), true);
        check(occupied(entry_wrap(&fh, &i)), true);
        check(validate(&fh), true);
        ++seen;
    }
    check((size_t)seen, count(&fh).count);
    int seen2 = 0;
    for (struct Val const *i = begin(&fh); i != end(&fh); i = next(&fh, i))
    {
        check(i->val % 2 == 0, true);
        ++seen2;
    }
    check(seen, seen2);
    check_end();
}

/** We want to make sure the clear and free method that uses the more
efficient iterator is able to free all elements allocated with no leaks when
run under sanitizers. */
check_static_begin(flat_hash_map_test_insert_allocate_clear_free)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        NULL, struct Owner, key, flat_hash_map_int_to_u64, owners_eq,
        std_allocate, NULL, 0);
    int const size = 32;
    for (int i = 0; i < size; ++i)
    {
        CCC_Entry *e = flat_hash_map_try_insert_with(
            &fh, i, (struct Owner){.allocation = malloc(sizeof(size_t))});
        check(occupied(e), CCC_FALSE);
        struct Owner const *const o = unwrap(e);
        check(o != NULL, CCC_TRUE);
        check(o->allocation != NULL, CCC_TRUE);
    }
    check_end(CCC_flat_hash_map_clear_and_free(&fh, destroy_owner_allocation););
}

int
main(void)
{
    return check_run(flat_hash_map_test_insert_then_iterate(),
                     flat_hash_map_test_insert_allocate_clear_free());
}
