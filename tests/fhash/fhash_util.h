#ifndef CCC_FHASH_UTIL_H
#define CCC_FHASH_UTIL_H

#include "flat_hash_map.h"

#include <stdint.h>

struct val
{
    int id;
    int val;
    ccc_fh_map_elem e;
};

uint64_t fhash_int_zero(void const *);
uint64_t fhash_int_last_digit(void const *);
uint64_t fhash_int_to_u64(void const *);
bool fhash_id_eq(ccc_key_cmp);
void fhash_print_val(void const *val);

void fhash_modplus(ccc_update);
struct val fhash_create(int id, int val);
void fhash_swap_val(ccc_update u);

#endif /* CCC_FHASH_UTIL_H */
