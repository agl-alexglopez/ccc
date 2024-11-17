#ifndef CCC_FHMAP_UTIL_H
#define CCC_FHMAP_UTIL_H

#include "flat_hash_map.h"
#include "types.h"

#include <stdint.h>

struct val
{
    int key;
    int val;
    ccc_fhmap_elem e;
};

uint64_t fhmap_int_zero(ccc_user_key);
uint64_t fhmap_int_last_digit(ccc_user_key);
uint64_t fhmap_int_to_u64(ccc_user_key);
bool fhmap_id_eq(ccc_key_cmp);

void fhmap_modplus(ccc_user_type);
struct val fhmap_create(int id, int val);
void fhmap_swap_val(ccc_user_type u);

#endif /* CCC_FHMAP_UTIL_H */
