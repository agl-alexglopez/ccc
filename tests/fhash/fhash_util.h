#ifndef CCC_FHASH_UTIL_H
#define CCC_FHASH_UTIL_H

#include "flat_hash_map.h"
#include "types.h"

#include <stdint.h>

struct val
{
    int id;
    int val;
    ccc_fh_map_elem e;
};

uint64_t fhash_int_zero(ccc_user_key);
uint64_t fhash_int_last_digit(ccc_user_key);
uint64_t fhash_int_to_u64(ccc_user_key);
bool fhash_id_eq(ccc_key_cmp);

void fhash_modplus(ccc_user_type_mut);
struct val fhash_create(int id, int val);
void fhash_swap_val(ccc_user_type_mut u);

#endif /* CCC_FHASH_UTIL_H */
