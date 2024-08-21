#include "fhash_util.h"

#include <stdbool.h>
#include <stdint.h>

uint64_t
fhash_val_zero(void const *const n)
{
    (void)n;
    return 0;
}

uint64_t
fhash_val_last_digit(void const *n)
{
    return ((struct val *)n)->val % 10;
}

bool
fhash_val_eq(void const *const a, void const *const b, void *aux)
{

    (void)aux;
    struct val const *const va = a;
    struct val const *const vb = b;
    return va->val == vb->val;
}

bool
fhash_id_and_val_eq(void const *const a, void const *const b, void *aux)
{
    (void)aux;
    struct val const *const va = a;
    struct val const *const vb = b;
    return va->val == vb->val && va->id == vb->id;
}
