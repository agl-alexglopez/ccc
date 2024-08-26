#ifndef CCC_FHASH_UTIL_H
#define CCC_FHASH_UTIL_H

#include "flat_hash.h"

#include <stdint.h>

struct val
{
    int id;
    int val;
    ccc_fhash_elem e;
};

uint64_t fhash_int_zero(void const *);
uint64_t fhash_int_last_digit(void const *);
uint64_t fhash_id_to_u64(void const *);
bool fhash_id_eq(ccc_key_cmp);
void fhash_print_val(void const *val);

#endif /* CCC_FHASH_UTIL_H */
