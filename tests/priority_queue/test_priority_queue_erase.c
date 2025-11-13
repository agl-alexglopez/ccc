#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "pq_util.h"
#include "priority_queue.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(pq_test_insert_remove_four_dups)
{
    CCC_priority_queue ppq = CCC_pq_initialize(struct val, elem, CCC_ORDER_LESS,
                                               val_cmp, NULL, NULL);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        CHECK(push(&ppq, &three_vals[i].elem) != NULL, true);
        CHECK(validate(&ppq), true);
        size_t const size = i + 1;
        CHECK(CCC_pq_count(&ppq).count, size);
    }
    CHECK(CCC_pq_count(&ppq).count, (size_t)4);
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        CHECK(pop(&ppq), CCC_RESULT_OK);
        CHECK(validate(&ppq), true);
    }
    CHECK(CCC_pq_count(&ppq).count, (size_t)0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_insert_extract_shuffled)
{
    CCC_priority_queue ppq = CCC_pq_initialize(struct val, elem, CCC_ORDER_LESS,
                                               val_cmp, NULL, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS);
    struct val const *min = front(&ppq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS);
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)CCC_pq_extract(&ppq, &vals[i].elem);
        CHECK(validate(&ppq), true);
    }
    CHECK(CCC_pq_count(&ppq).count, (size_t)0);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_pop_max)
{
    CCC_priority_queue ppq = CCC_pq_initialize(struct val, elem, CCC_ORDER_LESS,
                                               val_cmp, NULL, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS);
    struct val const *min = front(&ppq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *front = front(&ppq);
        CHECK(front->val, vals[i].val);
        CHECK(pop(&ppq), CCC_RESULT_OK);
    }
    CHECK(CCC_pq_is_empty(&ppq), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_pop_min)
{
    CCC_priority_queue ppq = CCC_pq_initialize(struct val, elem, CCC_ORDER_LESS,
                                               val_cmp, NULL, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&ppq, vals, size, prime), PASS);
    struct val const *min = front(&ppq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &ppq), PASS);
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        struct val const *front = front(&ppq);
        CHECK(front->val, vals[i].val);
        CHECK(pop(&ppq), CCC_RESULT_OK);
    }
    CHECK(CCC_pq_is_empty(&ppq), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_delete_prime_shuffle_duplicates)
{
    CCC_priority_queue ppq = CCC_pq_initialize(struct val, elem, CCC_ORDER_LESS,
                                               val_cmp, NULL, NULL);
    int const size = 99;
    int const prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    int const less = 77;
    struct val vals[99];
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        CHECK(push(&ppq, &vals[i].elem) != NULL, true);
        CHECK(validate(&ppq), true);
        size_t const s = i + 1;
        CHECK(CCC_pq_count(&ppq).count, s);
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)CCC_pq_extract(&ppq, &vals[shuffled_index].elem);
        CHECK(validate(&ppq), true);
        --cur_size;
        CHECK(CCC_pq_count(&ppq).count, cur_size);
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_prime_shuffle)
{
    CCC_priority_queue ppq = CCC_pq_initialize(struct val, elem, CCC_ORDER_LESS,
                                               val_cmp, NULL, NULL);
    int const size = 50;
    int const prime = 53;
    int const less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct val vals[50];
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        CHECK(push(&ppq, &vals[i].elem) != NULL, true);
        CHECK(validate(&ppq), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)CCC_pq_extract(&ppq, &vals[i].elem);
        CHECK(validate(&ppq), true);
        --cur_size;
        CHECK(CCC_pq_count(&ppq).count, cur_size);
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_weak_srand)
{
    CCC_priority_queue ppq = CCC_pq_initialize(struct val, elem, CCC_ORDER_LESS,
                                               val_cmp, NULL, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_heap_elems = 1000;
    struct val vals[1000];
    for (int i = 0; i < num_heap_elems; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        CHECK(push(&ppq, &vals[i].elem) != NULL, true);
        CHECK(validate(&ppq), true);
    }
    for (int i = 0; i < num_heap_elems; ++i)
    {
        (void)CCC_pq_extract(&ppq, &vals[i].elem);
        CHECK(validate(&ppq), true);
    }
    CHECK(CCC_pq_is_empty(&ppq), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_weak_srand_alloc)
{
    CCC_priority_queue ppq = CCC_pq_initialize(struct val, elem, CCC_ORDER_LESS,
                                               val_cmp, std_alloc, NULL);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_heap_elems = 100;
    for (int i = 0; i < num_heap_elems; ++i)
    {
        CHECK(push(&ppq,
                   &(struct val){
                       .id = i,
                       .val = rand() /*NOLINT*/,
                   }
                        .elem)
                  != NULL,
              true);
        CHECK(validate(&ppq), true);
    }
    CHECK_END_FN(CCC_pq_clear(&ppq, NULL););
}

int
main()
{
    return CHECK_RUN(
        pq_test_insert_remove_four_dups(), pq_test_insert_extract_shuffled(),
        pq_test_pop_max(), pq_test_pop_min(),
        pq_test_delete_prime_shuffle_duplicates(), pq_test_prime_shuffle(),
        pq_test_weak_srand(), pq_test_weak_srand_alloc());
}
