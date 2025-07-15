#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include <stddef.h>
#include <stdlib.h>

#include "alloc.h"
#include "buffer.h"
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

CHECK_BEGIN_FN(insert_shuffled, ccc_flat_priority_queue *const pq,
               struct val vals[const], size_t const size,
               int const larger_prime)
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
        CHECK(push(pq, &vals[i], &(struct val){}) != NULL, CCC_TRUE);
        CHECK(ccc_fpq_count(pq).count, i + 1);
        CHECK(validate(pq), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_fpq_count(pq).count, size);
    CHECK_END_FN();
}

/* Iterative inorder traversal to check the heap is sorted. */
CHECK_BEGIN_FN(inorder_fill, int vals[const], size_t const size,
               ccc_flat_priority_queue const *const fpq)
{
    if (ccc_fpq_count(fpq).count != size)
    {
        return FAIL;
    }
    ccc_flat_priority_queue fpq_cpy
        = ccc_fpq_init(NULL, struct val, CCC_LES, val_cmp, std_alloc, NULL, 0);
    ccc_result const r = fpq_copy(&fpq_cpy, fpq, std_alloc);
    CHECK(r, CCC_RESULT_OK);
    ccc_buffer b = fpq_heapsort(&fpq_cpy, &(struct val){});
    CHECK(ccc_buf_is_empty(&b), CCC_FALSE);
    vals[0] = *ccc_buf_back_as(&b, int);
    size_t i = 1;
    for (struct val const *prev = rbegin(&b), *v = rnext(&b, prev);
         v != rend(&b); prev = v, v = rnext(&b, v))
    {
        CHECK(prev->val <= v->val, CCC_TRUE);
        vals[i++] = v->val;
    }
    CHECK(i, fpq_count(fpq).count);
    CHECK_END_FN(clear_and_free(&b, NULL););
}
