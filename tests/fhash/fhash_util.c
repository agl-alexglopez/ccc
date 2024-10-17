#include "fhash_util.h"
#include "types.h"

#include <stdbool.h>
#include <stdint.h>

uint64_t
fhash_int_zero([[maybe_unused]] ccc_user_key const n)
{
    return 0;
}

uint64_t
fhash_int_last_digit(ccc_user_key const n)
{
    return *((int *)n.user_key) % 10;
}

bool
fhash_id_eq(ccc_key_cmp const cmp)
{
    struct val const *const va = cmp.user_type_rhs;
    return va->id == *((int *)cmp.key_lhs);
}

uint64_t
fhash_int_to_u64(ccc_user_key const k)
{
    int const id_int = *((int *)k.user_key);
    uint64_t x = id_int;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

void
fhash_modplus(ccc_user_type_mut const mod)
{
    ((struct val *)mod.user_type)->val++;
}

struct val
fhash_create(int const id, int const val)
{
    return (struct val){.id = id, .val = val};
}

void
fhash_swap_val(ccc_user_type_mut const u)
{
    struct val *v = u.user_type;
    v->val = *((int *)u.aux);
}
