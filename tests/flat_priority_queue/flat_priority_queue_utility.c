#include <stddef.h>
#include <stdlib.h>

#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "buffer.h"
#include "checkers.h"
#include "flat_priority_queue.h"
#include "flat_priority_queue_utility.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

CCC_Order
val_order(CCC_Type_comparator_context const order)
{
    struct Val const *const left = order.type_left;
    struct Val const *const right = order.type_right;
    return (left->val > right->val) - (left->val < right->val);
}

void
val_update(CCC_Type_context const u)
{
    struct Val *const old = u.type;
    old->val = *(int *)u.context;
}

size_t
rand_range(size_t const min, size_t const max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + (rand() / (RAND_MAX / (max - min + 1) + 1));
}

check_begin(insert_shuffled, CCC_Flat_priority_queue *const priority_queue,
            struct Val vals[const], size_t const size, int const larger_prime)
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
        check(push(priority_queue, &vals[i], &(struct Val){}) != NULL,
              CCC_TRUE);
        check(CCC_flat_priority_queue_count(priority_queue).count, i + 1);
        check(validate(priority_queue), true);
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    check(CCC_flat_priority_queue_count(priority_queue).count, size);
    check_end();
}

/* Iterative inorder traversal to check the heap is sorted. */
check_begin(inorder_fill, int vals[const], size_t const size,
            CCC_Flat_priority_queue const *const flat_priority_queue)
{
    if (CCC_flat_priority_queue_count(flat_priority_queue).count != size)
    {
        return CHECK_FAIL;
    }
    CCC_Flat_priority_queue flat_priority_queue_cpy
        = CCC_flat_priority_queue_initialize(NULL, struct Val, CCC_ORDER_LESSER,
                                             val_order, std_allocate, NULL, 0);
    CCC_Result const r = flat_priority_queue_copy(
        &flat_priority_queue_cpy, flat_priority_queue, std_allocate);
    check(r, CCC_RESULT_OK);
    CCC_Buffer b = flat_priority_queue_heapsort(&flat_priority_queue_cpy,
                                                &(struct Val){});
    check(CCC_buffer_is_empty(&b), CCC_FALSE);
    vals[0] = *CCC_buffer_back_as(&b, int);
    size_t i = 1;
    for (struct Val const *prev = reverse_begin(&b),
                          *v = reverse_next(&b, prev);
         v != reverse_end(&b); prev = v, v = reverse_next(&b, v))
    {
        check(prev->val <= v->val, CCC_TRUE);
        vals[i++] = v->val;
    }
    check(i, flat_priority_queue_count(flat_priority_queue).count);
    check_end(clear_and_free(&b, NULL););
}
