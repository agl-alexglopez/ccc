#ifndef CCC_FHMAP_UTIL_H
#define CCC_FHMAP_UTIL_H

#include "flat_hash_map.h"
#include "types.h"

#include <stddef.h>
#include <stdint.h>

struct val
{
    int key;
    int val;
};

/** A small fixed map is good when 64 is a desirable upper bound on capacity.
Insertions can continue for about 87.5% of the capacity so about 56. Play it
safe and avoid this limit unless testing insertion failure is important. */
ccc_fhm_declare_fixed_map(small_fixed_map, struct val, 64);

/** A standard fixed map is good when 1024 is a desirable upper bound on
capacity. Insertions can continue for 87.5% of the capacity so about 896. Play
it safe and avoid this limit unless testing insertion failure is important. */
ccc_fhm_declare_fixed_map(standard_fixed_map, struct val, 1024);

enum : size_t
{
    SMALL_FIXED_CAP = ccc_fhm_fixed_capacity(small_fixed_map),
    STANDARD_FIXED_CAP = ccc_fhm_fixed_capacity(standard_fixed_map),
};

uint64_t fhmap_int_zero(ccc_user_key);
uint64_t fhmap_int_last_digit(ccc_user_key);
uint64_t fhmap_int_to_u64(ccc_user_key);
ccc_tribool fhmap_id_eq(ccc_key_cmp);

void fhmap_modplus(ccc_user_type);
struct val fhmap_create(int id, int val);
void fhmap_swap_val(ccc_user_type u);

#endif /* CCC_FHMAP_UTIL_H */
