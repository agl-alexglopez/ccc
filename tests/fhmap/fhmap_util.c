#include <stdbool.h>
#include <stdint.h>

#include "fhmap_util.h"
#include "types.h"

uint64_t
fhmap_int_zero(ccc_any_key const)
{
    return 0;
}

uint64_t
fhmap_int_last_digit(ccc_any_key const n)
{
    return *((int *)n.any_key) % 10;
}

ccc_tribool
fhmap_id_eq(ccc_any_key_cmp const cmp)
{
    struct val const *const va = cmp.any_type_rhs;
    return va->key == *((int *)cmp.any_key_lhs);
}

uint64_t
fhmap_int_to_u64(ccc_any_key const k)
{
    int const id_int = *((int *)k.any_key);
    uint64_t x = id_int;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

void
fhmap_modplus(ccc_any_type const mod)
{
    ((struct val *)mod.any_type)->val++;
}

struct val
fhmap_create(int const id, int const val)
{
    return (struct val){.key = id, .val = val};
}

void
fhmap_swap_val(ccc_any_type const u)
{
    struct val *v = u.any_type;
    v->val = *((int *)u.aux);
}
