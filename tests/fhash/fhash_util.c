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
fhash_id_eq(void const *const val_struct, void const *const key_id, void *aux)
{

    (void)aux;
    struct val const *const va = val_struct;
    return va->id == *((int *)key_id);
}
