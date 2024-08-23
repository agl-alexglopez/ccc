#include "fhash_util.h"

#include <stdbool.h>
#include <stdint.h>

uint64_t
fhash_int_zero(void const *const n)
{
    (void)n;
    return 0;
}

uint64_t
fhash_int_last_digit(void const *n)
{
    return *((int *)n) % 10;
}

bool
fhash_id_eq(ccc_key_cmp const cmp)
{
    struct val const *const va = cmp.container;
    return va->id == *((int *)cmp.key);
}
