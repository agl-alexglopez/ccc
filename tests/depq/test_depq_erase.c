#define TRAITS_USING_NAMESPACE_CCC

#include "depq_util.h"
#include "double_ended_priority_queue.h"
#include "test.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

BEGIN_STATIC_TEST(depq_test_insert_remove_four_dups)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        push(&pq, &three_vals[i].elem);
        CHECK(validate(&pq), true);
        size_t const size = i + 1;
        CHECK(size(&pq), size);
    }
    CHECK(size(&pq), (size_t)4);
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        ccc_depq_pop_max(&pq);
        CHECK(validate(&pq), true);
    }
    CHECK(size(&pq), (size_t)0);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_insert_erase_shuffled)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS);
    struct val const *max = ccc_depq_max(&pq);
    CHECK(max->val, (int)size - 1);
    struct val const *min = ccc_depq_min(&pq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &pq), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i]);
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)ccc_depq_erase(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    CHECK(size(&pq), (size_t)0);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_pop_max)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS);
    struct val const *max = ccc_depq_max(&pq);
    CHECK(max->val, (int)size - 1);
    struct val const *min = ccc_depq_min(&pq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &pq), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i]);
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = size - 1; i != (size_t)-1; --i)
    {
        CHECK(((struct val *)ccc_depq_max(&pq))->val, vals[i].val);
        ccc_depq_pop_max(&pq);
    }
    CHECK(empty(&pq), true);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_pop_min)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS);
    struct val const *max = ccc_depq_max(&pq);
    CHECK(max->val, (int)size - 1);
    struct val const *min = ccc_depq_min(&pq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &pq), size);
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i]);
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(((struct val *)ccc_depq_min(&pq))->val, vals[i].val);
        ccc_depq_pop_min(&pq);
    }
    CHECK(empty(&pq), true);
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_max_round_robin)
{
    ccc_double_ended_priority_queue depq
        = ccc_depq_init(struct val, elem, val, depq, NULL, val_cmp, NULL);
    int const size = 6;
    struct val vals[6];
    struct val const order[6] = {
        {.id = 0, .val = 99}, {.id = 2, .val = 99}, {.id = 4, .val = 99},
        {.id = 1, .val = 1},  {.id = 3, .val = 1},  {.id = 5, .val = 1},
    };
    for (int i = 0; i < size; ++i)
    {
        if (i % 2)
        {
            vals[i].val = 1;
        }
        else
        {
            vals[i].val = 99;
        }
        vals[i].id = i;
        push(&depq, &vals[i].elem);
        CHECK(validate(&depq), true);
    }
    /* Now let's make sure we pop round robin. */
    size_t i = 0;
    while (!empty(&depq))
    {
        struct val const *front = ccc_depq_max(&depq);
        CHECK(front->id, order[i].id);
        CHECK(front->val, order[i].val);
        ccc_depq_pop_max(&depq);
        ++i;
    }
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_min_round_robin)
{
    ccc_double_ended_priority_queue depq
        = ccc_depq_init(struct val, elem, val, depq, NULL, val_cmp, NULL);
    int const size = 6;
    struct val vals[6];
    struct val const order[6] = {
        {.id = 0, .val = 1},  {.id = 2, .val = 1},  {.id = 4, .val = 1},
        {.id = 1, .val = 99}, {.id = 3, .val = 99}, {.id = 5, .val = 99},
    };
    for (int i = 0; i < size; ++i)
    {
        if (i % 2)
        {
            vals[i].val = 99;
        }
        else
        {
            vals[i].val = 1;
        }
        vals[i].id = i;
        push(&depq, &vals[i].elem);
        CHECK(validate(&depq), true);
    }
    /* Now let's make sure we pop round robin. */
    size_t i = 0;
    while (!empty(&depq))
    {
        struct val const *front = ccc_depq_min(&depq);
        CHECK(front->id, order[i].id);
        CHECK(front->val, order[i].val);
        ccc_depq_pop_min(&depq);
        ++i;
    }
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_delete_prime_shuffle_duplicates)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
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
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
        size_t const s = i + 1;
        CHECK(size(&pq), s);
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)ccc_depq_erase(&pq, &vals[shuffled_index].elem);
        CHECK(validate(&pq), true);
        --cur_size;
        CHECK(size(&pq), cur_size);
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_prime_shuffle)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
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
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    ccc_depq_print(&pq, depq_printer_fn);
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        CHECK(ccc_depq_erase(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
        --cur_size;
        CHECK(size(&pq), cur_size);
    }
    END_TEST();
}

BEGIN_STATIC_TEST(depq_test_weak_srand)
{
    ccc_double_ended_priority_queue pq
        = ccc_depq_init(struct val, elem, val, pq, NULL, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    int const num_nodes = 1000;
    struct val vals[1000];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        push(&pq, &vals[i].elem);
        CHECK(validate(&pq), true);
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CHECK(ccc_depq_erase(&pq, &vals[i].elem) != NULL, true);
        CHECK(validate(&pq), true);
    }
    CHECK(empty(&pq), true);
    END_TEST();
}

int
main()
{
    return RUN_TESTS(depq_test_insert_remove_four_dups(),
                     depq_test_insert_erase_shuffled(), depq_test_pop_max(),
                     depq_test_pop_min(), depq_test_max_round_robin(),
                     depq_test_min_round_robin(),
                     depq_test_delete_prime_shuffle_duplicates(),
                     depq_test_prime_shuffle(), depq_test_weak_srand());
}
