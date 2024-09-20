#include "dll_util.h"

ccc_threeway_cmp
val_cmp(ccc_cmp const *const c)
{
    struct val const *const a = c->container_a;
    struct val const *const b = c->container_b;
    return (a->val > b->val) - (a->val < b->val);
}
