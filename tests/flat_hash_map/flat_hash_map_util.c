#include <stdbool.h>
#include <stdint.h>

#include "fhmap_util.h"
#include "types.h"

uint64_t
fhmap_int_zero(CCC_Key_context const)
{
    return 0;
}

uint64_t
fhmap_int_last_digit(CCC_Key_context const n)
{
    return *((int *)n.any_key) % 10;
}

CCC_Order
fhmap_id_cmp(CCC_Key_comparator_context const cmp)
{
    struct val const *const rhs = cmp.any_type_rhs;
    int const lhs = *((int *)cmp.any_key_lhs);
    return (lhs > rhs->key) - (lhs < rhs->key);
}

uint64_t
fhmap_int_to_u64(CCC_Key_context const k)
{
    int const id_int = *((int *)k.any_key);
    uint64_t x = id_int;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

void
fhmap_modplus(CCC_Type_context const mod)
{
    ((struct val *)mod.any_type)->val++;
}

struct val
fhmap_create(int const id, int const val)
{
    return (struct val){.key = id, .val = val};
}

void
fhmap_swap_val(CCC_Type_context const u)
{
    struct val *v = u.any_type;
    v->val = *((int *)u.aux);
}
