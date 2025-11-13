#ifndef CCC_FLAT_HASH_MAP_UTIL_H
#define CCC_FLAT_HASH_MAP_UTIL_H

#include <stddef.h>
#include <stdint.h>

#include "flat_hash_map.h"
#include "types.h"

struct Val
{
    int key;
    int val;
};

/** A small fixed map is good when 64 is a desirable upper bound on capacity.
Insertions can continue for about 87.5% of the capacity so about 56. Play it
safe and avoid this limit unless testing insertion failure is important. */
CCC_flat_hash_map_declare_fixed_map(small_fixed_map, struct Val, 64);

/** A standard fixed map is good when 1024 is a desirable upper bound on
capacity. Insertions can continue for 87.5% of the capacity so about 896. Play
it safe and avoid this limit unless testing insertion failure is important. */
CCC_flat_hash_map_declare_fixed_map(standard_fixed_map, struct Val, 1024);

enum : size_t
{
    SMALL_FIXED_CAP = CCC_flat_hash_map_fixed_capacity(small_fixed_map),
    STANDARD_FIXED_CAP = CCC_flat_hash_map_fixed_capacity(standard_fixed_map),
};

uint64_t flat_hash_map_int_zero(CCC_Key_context);
uint64_t flat_hash_map_int_last_digit(CCC_Key_context);
uint64_t flat_hash_map_int_to_u64(CCC_Key_context);
CCC_Order flat_hash_map_id_order(CCC_Key_comparator_context);

void flat_hash_map_modplus(CCC_Type_context);
struct Val flat_hash_map_create(int id, int val);
void flat_hash_map_swap_val(CCC_Type_context u);

#endif /* CCC_FLAT_HASH_MAP_UTIL_H */
