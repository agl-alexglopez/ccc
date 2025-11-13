#include <stdbool.h>
#include <stdint.h>

#include "flat_hash_map_utility.h"
#include "types.h"

uint64_t
flat_hash_map_int_zero(CCC_Key_context const)
{
    return 0;
}

uint64_t
flat_hash_map_int_last_digit(CCC_Key_context const n)
{
    return *((int *)n.key) % 10;
}

CCC_Order
flat_hash_map_id_order(CCC_Key_comparator_context const order)
{
    struct Val const *const rhs = order.type_rhs;
    int const lhs = *((int *)order.key_lhs);
    return (lhs > rhs->key) - (lhs < rhs->key);
}

uint64_t
flat_hash_map_int_to_u64(CCC_Key_context const k)
{
    int const id_int = *((int *)k.key);
    uint64_t x = id_int;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

void
flat_hash_map_modplus(CCC_Type_context const mod)
{
    ((struct Val *)mod.type)->val++;
}

struct Val
flat_hash_map_create(int const id, int const val)
{
    return (struct Val){.key = id, .val = val};
}

void
flat_hash_map_swap_val(CCC_Type_context const u)
{
    struct Val *v = u.type;
    v->val = *((int *)u.context);
}
