#include "hhmap_util.h"
#include "types.h"

#include <stdint.h>

uint64_t
hhmap_int_zero(ccc_user_key const)
{
    return 0;
}

uint64_t
hhmap_int_last_digit(ccc_user_key const n)
{
    return *((int *)n.user_key) % 10;
}

ccc_tribool
hhmap_id_eq(ccc_key_cmp const cmp)
{
    struct val const *const va = cmp.user_type_rhs;
    return va->key == *((int *)cmp.key_lhs);
}

uint64_t
hhmap_int_to_u64(ccc_user_key const k)
{
    int const id_int = *((int *)k.user_key);
    uint64_t x = id_int;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

void
hhmap_modplus(ccc_user_type const mod)
{
    ((struct val *)mod.user_type)->val++;
}

struct val
hhmap_create(int const id, int const val)
{
    return (struct val){.key = id, .val = val};
}

void
hhmap_swap_val(ccc_user_type const u)
{
    struct val *v = u.user_type;
    v->val = *((int *)u.aux);
}
