#ifndef CCC_HHMAP_UTIL_H
#define CCC_HHMAP_UTIL_H

#include "handle_hash_map.h"
#include "types.h"

#include <stdint.h>

struct val
{
    int key;
    ccc_hhmap_elem e;
    int val;
};

uint64_t hhmap_int_zero(ccc_user_key);
uint64_t hhmap_int_last_digit(ccc_user_key);
uint64_t hhmap_int_to_u64(ccc_user_key);
bool hhmap_id_eq(ccc_key_cmp);

void hhmap_modplus(ccc_user_type);
struct val hhmap_create(int id, int val);
void hhmap_swap_val(ccc_user_type u);

#endif /* CCC_HHMAP_UTIL_H */
