#include "fhash_util.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
fhash_id_eq(ccc_key_cmp const *const cmp)
{
    struct val const *const va = cmp->container;
    return va->id == *((int *)cmp->key);
}

void
fhash_print_val(void const *const val)
{
    struct val const *const v = val;
    printf("{id:%d,val:%d},", v->id, v->val);
}

uint64_t
fhash_int_to_u64(void const *const id)
{
    int const id_int = *((int *)id);
    uint64_t x = id_int;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

void
fhash_modplus(ccc_update const *const mod)
{
    ((struct val *)mod->container)->val++;
}

struct val
fhash_create(int const id, int const val)
{
    return (struct val){.id = id, .val = val};
}

void
fhash_swap_val(ccc_update const *const u)
{
    struct val *v = u->container;
    v->val = *((int *)u->aux);
}
