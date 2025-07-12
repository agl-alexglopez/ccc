#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "fhmap_util.h"
#include "flat_hash_map.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

CHECK_BEGIN_STATIC_FN(fhmap_test_insert_then_iterate)
{
    ccc_flat_hash_map fh
        = fhm_init(&(standard_fixed_map){}, struct val, key, fhmap_int_to_u64,
                   fhmap_id_eq, NULL, NULL, STANDARD_FIXED_CAP);
    int const size = STANDARD_FIXED_CAP;
    for (int i = 0; i < size; i += 2)
    {
        ccc_entry e = try_insert(&fh, &(struct val){.key = i, .val = i});
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

int
main(void)
{
    return CHECK_RUN(fhmap_test_insert_then_iterate());
}
