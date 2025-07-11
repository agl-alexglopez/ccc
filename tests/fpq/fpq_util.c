#define TRAITS_USING_NAMESPACE_CCC

#include <stddef.h>
#include <stdlib.h>

#include "checkers.h"
#include "flat_priority_queue.h"
#include "fpq_util.h"
#include "traits.h"
#include "types.h"

ccc_threeway_cmp
val_cmp(ccc_any_type_cmp const cmp)
{
    struct val const *const lhs = cmp.any_type_lhs;
    struct val const *const rhs = cmp.any_type_rhs;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

void
val_update(ccc_any_type const u)
{
    struct val *const old = u.any_type;
    old->val = *(int *)u.aux;
}

size_t
rand_range(size_t const min, size_t const max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + (rand() / (RAND_MAX / (max - min + 1) + 1));
}

CHECK_BEGIN_FN(insert_shuffled, ccc_flat_priority_queue *pq, struct val vals[],
               size_t const size, int const larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       random but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].id = vals[i].val = (int)shuffled_index;
        CHECK(push(pq, &vals[i]) != NULL, CCC_TRUE);
        CHECK(ccc_fpq_size(pq).count, i + 1);
        CHECK(validate(pq), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_fpq_size(pq).count, size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
CHECK_BEGIN_FN(inorder_fill, int vals[], size_t size,
               ccc_flat_priority_queue *fpq)
{
    if (ccc_fpq_size(fpq).count != size)
    {
        return FAIL;
    }
    size_t i = 0;
    struct val *copy_buf
        = malloc(sizeof(struct val) * (ccc_fpq_size(fpq).count + 1));
    CHECK(copy_buf == NULL, false);
    ccc_flat_priority_queue fpq_copy
        = ccc_fpq_init(copy_buf, struct val, CCC_LES, val_cmp, NULL, NULL,
                       ccc_fpq_size(fpq).count + 1);
    while (!ccc_fpq_is_empty(fpq) && i < size)
    {
        struct val *const front = front(fpq);
        vals[i++] = front->val;
        size_t const prev = ccc_fpq_size(&fpq_copy).count;
        struct val *v = ccc_fpq_emplace(
            &fpq_copy, (struct val){.id = front->id, .val = front->val});
        CHECK(v != NULL, true);
        CHECK(prev < ccc_fpq_size(&fpq_copy).count, true);
        (void)pop(fpq);
    }
    i = 0;
    while (!ccc_fpq_is_empty(&fpq_copy) && i < size)
    {
        struct val *const v = front(&fpq_copy);
        size_t const prev = ccc_fpq_size(fpq).count;
        struct val *e
            = ccc_fpq_emplace(fpq, (struct val){.id = v->id, .val = v->val});
        CHECK(e != NULL, true);
        CHECK(prev < ccc_fpq_size(fpq).count, true);
        CHECK(vals[i++], v->val);
        (void)pop(&fpq_copy);
    };
    CHECK_END_FN(free(copy_buf););
}
