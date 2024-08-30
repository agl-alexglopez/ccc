#include "fpq_util.h"
#include <stdlib.h>

ccc_threeway_cmp
val_cmp(ccc_cmp const cmp)
{
    struct val const *const lhs = cmp.container_a;
    struct val const *const rhs = cmp.container_b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

void
val_print(void const *e)
{
    struct val const *const v = e;
    printf("{%d,%d}", v->id, v->val);
}

void
val_update(ccc_update const u)
{
    struct val *const old = u.container;
    old->val = *(int *)u.aux;
}

size_t
rand_range(size_t const min, size_t const max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

enum test_result
insert_shuffled(ccc_flat_priority_queue *pq, struct val vals[],
                size_t const size, int const larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       randome but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].id = vals[i].val = (int)shuffled_index;
        ccc_fpq_push(pq, &vals[i]);
        CHECK(ccc_fpq_size(pq), i + 1, "%zu");
        CHECK(ccc_fpq_validate(pq), true, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_fpq_size(pq), size, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
enum test_result
inorder_fill(int vals[], size_t size, ccc_flat_priority_queue *fpq)
{
    if (ccc_fpq_size(fpq) != size)
    {
        return FAIL;
    }
    size_t i = 0;
    struct val copy_buf[sizeof(struct val) * ccc_fpq_size(fpq)];
    ccc_flat_priority_queue fpq_copy = CCC_FPQ_INIT(
        &copy_buf, ccc_fpq_size(fpq), struct val, CCC_LES, NULL, val_cmp, NULL);
    while (!ccc_fpq_empty(fpq) && i < size)
    {
        struct val *const front = ccc_fpq_pop(fpq);
        vals[i++] = front->val;
        size_t const prev = ccc_fpq_size(&fpq_copy);
        struct val *v = FPQ_EMPLACE(
            &fpq_copy, (struct val){.id = front->id, .val = front->val});
        CHECK(v != NULL, true, "%d");
        CHECK(prev < ccc_fpq_size(&fpq_copy), true, "%d");
    }
    i = 0;
    while (!ccc_fpq_empty(&fpq_copy) && i < size)
    {
        struct val *const v = ccc_fpq_pop(&fpq_copy);
        size_t const prev = ccc_fpq_size(fpq);
        struct val *e
            = FPQ_EMPLACE(fpq, (struct val){.id = v->id, .val = v->val});
        CHECK(e != NULL, true, "%d");
        CHECK(prev < ccc_fpq_size(fpq), true, "%d");
        CHECK(vals[i++], v->val, "%d");
    }
    return PASS;
}
