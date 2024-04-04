#include "pqueue.h"
#include "test.h"
#include "tree.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *const pass_msg = "pass";
const char *const fail_msg = "fail";

struct val
{
    int id;
    int val;
    pq_elem elem;
};

static enum test_result pq_test_empty(void);
static enum test_result pq_test_insert_one(void);
static enum test_result pq_test_insert_three(void);
static enum test_result pq_test_struct_getter(void);
static enum test_result pq_test_insert_three_dups(void);
static enum test_result pq_test_insert_remove_four_dups(void);
static enum test_result pq_test_read_max_min(void);
static enum test_result pq_test_insert_shuffle(void);
static enum test_result pq_test_insert_erase_shuffled(void);
static enum test_result pq_test_pop_max(void);
static enum test_result pq_test_pop_min(void);
static enum test_result pq_test_max_round_robin(void);
static enum test_result pq_test_min_round_robin(void);
static enum test_result pq_test_delete_prime_shuffle_duplicates(void);
static enum test_result pq_test_prime_shuffle(void);
static enum test_result pq_test_weak_srand(void);
static enum test_result pq_test_forward_iter_unique_vals(void);
static enum test_result pq_test_forward_iter_all_vals(void);
static enum test_result pq_test_insert_iterate_pop(void);
static enum test_result pq_test_priority_update(void);
static enum test_result pq_test_priority_removal(void);
static enum test_result pq_test_priority_valid_range(void);
static enum test_result pq_test_priority_invalid_range(void);
static enum test_result pq_test_priority_empty_range(void);
static int run_tests(void);
static enum test_result insert_shuffled(pqueue *, struct val[], size_t, int);
static size_t inorder_fill(int vals[], size_t, pqueue *);
static enum test_result iterator_check(pqueue *);
static threeway_cmp val_cmp(const pq_elem *, const pq_elem *, void *);
static void val_update(pq_elem *a, void *aux);
static void pq_printer_fn(const pq_elem *e);

#define NUM_TESTS (size_t)24
const struct fn_name all_tests[NUM_TESTS] = {
    {pq_test_empty, "pq_test_empty"},
    {pq_test_insert_one, "pq_test_insert_one"},
    {pq_test_insert_three, "pq_test_insert_three"},
    {pq_test_struct_getter, "pq_test_struct_getter"},
    {pq_test_insert_three_dups, "pq_test_insert_three_dups"},
    {pq_test_insert_remove_four_dups, "pq_test_insert_remove_four_dups"},
    {pq_test_read_max_min, "pq_test_read_max_min"},
    {pq_test_insert_shuffle, "pq_test_insert_shuffle"},
    {pq_test_insert_erase_shuffled, "pq_test_insert_erase_shuffled"},
    {pq_test_pop_max, "pq_test_pop_max"},
    {pq_test_pop_min, "pq_test_pop_min"},
    {pq_test_max_round_robin, "pq_test_max_round_robin"},
    {pq_test_min_round_robin, "pq_test_min_round_robin"},
    {pq_test_delete_prime_shuffle_duplicates,
     "pq_test_delete_prime_shuffle_duplicates"},
    {pq_test_prime_shuffle, "pq_test_prime_shuffle"},
    {pq_test_weak_srand, "pq_test_weak_srand"},
    {pq_test_forward_iter_unique_vals, "pq_test_forward_iter_unique_vals"},
    {pq_test_forward_iter_all_vals, "pq_test_forward_iter_all_vals"},
    {pq_test_insert_iterate_pop, "pq_test_insert_iterate_pop"},
    {pq_test_priority_update, "pq_test_priority_update"},
    {pq_test_priority_removal, "pq_test_priority_removal"},
    {pq_test_priority_valid_range, "pq_test_priority_valid_range"},
    {pq_test_priority_invalid_range, "pq_test_priority_invalid_range"},
    {pq_test_priority_empty_range, "pq_test_priority_empty_range"},
};

int
main()
{
    return run_tests();
}

static int
run_tests(void)
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        const bool fail = all_tests[i].fn() == FAIL;
        if (fail)
        {
            res = FAIL;
            (void)fprintf(stderr, "failure in tests_pq.c: %s\n",
                          all_tests[i].name);
        }
    }
    return res;
}

static enum test_result
pq_test_empty(void)
{
    pqueue pq;
    pq_init(&pq);
    CHECK(pq_empty(&pq), true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_insert_one(void)
{
    pqueue pq;
    pq_init(&pq);
    struct val single;
    single.val = 0;
    pq_insert(&pq, &single.elem, val_cmp, NULL);
    CHECK(pq_empty(&pq), false, bool, "%b");
    CHECK(pq_entry(pq_root(&pq), struct val, elem)->val == single.val, true,
          bool, "%b");
    return PASS;
}

static enum test_result
pq_test_insert_three(void)
{
    pqueue pq;
    pq_init(&pq);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        pq_insert(&pq, &three_vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 3, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_struct_getter(void)
{
    pqueue pq;
    pq_init(&pq);
    pqueue pq_tester_clone;
    pq_init(&pq_tester_clone);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        pq_insert(&pq_tester_clone, &tester_clone[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        const struct val *get
            = pq_entry(&tester_clone[i].elem, struct val, elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(pq_size(&pq), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_three_dups(void)
{
    pqueue pq;
    pq_init(&pq);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        pq_insert(&pq, &three_vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 3ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_read_max_min(void)
{
    pqueue pq;
    pq_init(&pq);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        CHECK(pq_size(&pq), i + 1, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 10ULL, size_t, "%zu");
    const struct val *max = pq_entry(pq_const_max(&pq), struct val, elem);
    CHECK(max->val, 9, int, "%d");
    const struct val *min = pq_entry(pq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    return PASS;
}

static enum test_result
pq_test_insert_shuffle(void)
{
    pqueue pq;
    pq_init(&pq);
    /* Math magic ahead... */
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *max = pq_entry(pq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    const struct val *min = pq_entry(pq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    return PASS;
}

static enum test_result
pq_test_insert_remove_four_dups(void)
{
    pqueue pq;
    pq_init(&pq);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        pq_insert(&pq, &three_vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        const size_t size = i + 1;
        CHECK(pq_size(&pq), size, size_t, "%zu");
    }
    CHECK(pq_size(&pq), 4, size_t, "%zu");
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        pq_pop_max(&pq);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    CHECK(pq_size(&pq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_insert_erase_shuffled(void)
{
    pqueue pq;
    pq_init(&pq);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *max = pq_entry(pq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    const struct val *min = pq_entry(pq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)pq_erase(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    CHECK(pq_size(&pq), 0ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_pop_max(void)
{
    pqueue pq;
    pq_init(&pq);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *max = pq_entry(pq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    const struct val *min = pq_entry(pq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = size - 1; i != (size_t)-1; --i)
    {
        const struct val *front = pq_entry(pq_pop_max(&pq), struct val, elem);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(pq_empty(&pq), true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_pop_min(void)
{
    pqueue pq;
    pq_init(&pq);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS, enum test_result,
          "%d");
    const struct val *max = pq_entry(pq_const_max(&pq), struct val, elem);
    CHECK(max->val, size - 1, int, "%d");
    const struct val *min = pq_entry(pq_const_min(&pq), struct val, elem);
    CHECK(min->val, 0, int, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &pq), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        const struct val *front = pq_entry(pq_pop_min(&pq), struct val, elem);
        CHECK(front->val, vals[i].val, int, "%d");
    }
    CHECK(pq_empty(&pq), true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_max_round_robin(void)
{
    pqueue pq;
    pq_init(&pq);
    const int size = 50;
    struct val vals[size];
    vals[0].id = 99;
    vals[0].val = 0;
    pq_insert(&pq, &vals[0].elem, val_cmp, NULL);
    for (int i = 1; i < size; ++i)
    {
        vals[i].val = 99;
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    /* Now let's make sure we pop round robin. */
    int last_id = 0;
    while (!pq_empty(&pq))
    {
        const struct val *front = pq_entry(pq_pop_max(&pq), struct val, elem);
        CHECK(last_id < front->id, true, bool, "%b");
        last_id = front->id;
    }
    return PASS;
}

static enum test_result
pq_test_min_round_robin(void)
{
    pqueue pq;
    pq_init(&pq);
    const int size = 50;
    struct val vals[size];
    vals[0].id = 99;
    vals[0].val = 99;
    pq_insert(&pq, &vals[0].elem, val_cmp, NULL);
    for (int i = 1; i < size; ++i)
    {
        vals[i].val = 1;
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    /* Now let's make sure we pop round robin. */
    int last_id = 0;
    while (!pq_empty(&pq))
    {
        const struct val *front = pq_entry(pq_pop_min(&pq), struct val, elem);
        CHECK(last_id < front->id, true, bool, "%b");
        last_id = front->id;
    }
    return PASS;
}

static enum test_result
pq_test_delete_prime_shuffle_duplicates(void)
{
    pqueue pq;
    pq_init(&pq);
    const int size = 99;
    const int prime = 101;
    /* Make the prime shuffle shorter than size for many duplicates. */
    const int less = 77;
    struct val vals[size];
    int shuffled_index = prime % (size - less);
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        const size_t s = i + 1;
        CHECK(pq_size(&pq), s, size_t, "%zu");
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        (void)pq_erase(&pq, &vals[shuffled_index].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        --cur_size;
        CHECK(pq_size(&pq), cur_size, size_t, "%zu");
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    return PASS;
}

static enum test_result
pq_test_prime_shuffle(void)
{
    pqueue pq;
    pq_init(&pq);
    const int size = 50;
    const int prime = 53;
    const int less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    int shuffled_index = prime % (size - less);
    struct val vals[size];
    for (int i = 0; i < size; ++i)
    {
        vals[i].val = shuffled_index;
        vals[i].id = shuffled_index;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    pq_print(&pq, pq.root, pq_printer_fn);
    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        CHECK(pq_erase(&pq, &vals[i].elem, val_cmp, NULL) != NULL, true, bool,
              "%b");
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        --cur_size;
        CHECK(pq_size(&pq), cur_size, size_t, "%zu");
    }
    return PASS;
}

static enum test_result
pq_test_weak_srand(void)
{
    pqueue pq;
    pq_init(&pq);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const int num_nodes = 1000;
    struct val vals[num_nodes];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        CHECK(pq_erase(&pq, &vals[i].elem, val_cmp, NULL) != NULL, true, bool,
              "%b");
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    CHECK(pq_empty(&pq), true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_forward_iter_unique_vals(void)
{
    pqueue pq;
    pq_init(&pq);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (pq_elem *e = pq_begin(&pq); e != pq_end(&pq); e = pq_next(&pq, e), ++j)
    {}
    CHECK(j, 0, int, "%d");
    const int num_nodes = 33;
    const int prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = shuffled_index; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &pq), pq_size(&pq), size_t,
          "%zu");
    j = num_nodes - 1;
    for (pq_elem *e = pq_begin(&pq); e != pq_end(&pq) && j >= 0;
         e = pq_next(&pq, e), --j)
    {
        const struct val *v = pq_entry(e, struct val, elem);
        CHECK(v->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
pq_test_forward_iter_all_vals(void)
{
    pqueue pq;
    pq_init(&pq);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (pq_elem *i = pq_begin(&pq); i != pq_end(&pq); i = pq_next(&pq, i), ++j)
    {}
    CHECK(j, 0, int, "%d");
    const int num_nodes = 33;
    struct val vals[num_nodes];
    vals[0].val = 0; // NOLINT
    vals[0].id = 0;
    pq_insert(&pq, &vals[0].elem, val_cmp, NULL);
    /* This will test iterating through every possible length list. */
    for (int i = 1, val = 1; i < num_nodes; i += i, ++val)
    {
        for (int repeats = 0, index = i; repeats < i && index < num_nodes;
             ++repeats, ++index)
        {
            vals[index].val = val; // NOLINT
            vals[index].id = index;
            pq_insert(&pq, &vals[index].elem, val_cmp, NULL);
            CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        }
    }
    int val_keys_inorder[num_nodes];
    (void)inorder_fill(val_keys_inorder, num_nodes, &pq);
    j = num_nodes - 1;
    for (pq_elem *i = pq_begin(&pq); i != pq_end(&pq) && j >= 0;
         i = pq_next(&pq, i), --j)
    {
        const struct val *v = pq_entry(i, struct val, elem);
        CHECK(v->val, val_keys_inorder[j], int, "%d");
    }
    return PASS;
}

static enum test_result
pq_test_insert_iterate_pop(void)
{
    pqueue pq;
    pq_init(&pq);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    size_t pop_count = 0;
    while (!pq_empty(&pq))
    {
        pq_pop_max(&pq);
        ++pop_count;
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        if (pop_count % 200)
        {
            CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
        }
    }
    CHECK(pop_count, num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_removal(void)
{
    pqueue pq;
    pq_init(&pq);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    const int limit = 400;
    for (pq_elem *i = pq_begin(&pq); i != pq_end(&pq);)
    {
        struct val *cur = pq_entry(i, struct val, elem);
        if (cur->val > limit)
        {
            i = pq_erase(&pq, i, val_cmp, NULL);
            CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
        }
        else
        {
            i = pq_next(&pq, i);
        }
    }
    return PASS;
}

static enum test_result
pq_test_priority_update(void)
{
    pqueue pq;
    pq_init(&pq);
    /* Seed the test with any integer for reproducible random test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const size_t num_nodes = 1000;
    struct val vals[num_nodes];
    for (size_t i = 0; i < num_nodes; ++i)
    {
        /* Force duplicates. */
        vals[i].val = rand() % (num_nodes + 1); // NOLINT
        vals[i].id = (int)i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&pq), PASS, enum test_result, "%d");
    const int limit = 400;
    for (pq_elem *i = pq_begin(&pq); i != pq_end(&pq);)
    {
        struct val *cur = pq_entry(i, struct val, elem);
        int backoff = cur->val / 2;
        if (cur->val > limit)
        {
            pq_elem *next = pq_next(&pq, i);
            CHECK(pq_update(&pq, i, val_cmp, val_update, &backoff), true, bool,
                  "%b");
            CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
            i = next;
        }
        else
        {
            i = pq_next(&pq, i);
        }
    }
    CHECK(pq_size(&pq), num_nodes, size_t, "%zu");
    return PASS;
}

static enum test_result
pq_test_priority_valid_range(void)
{
    pqueue pq;
    pq_init(&pq);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    struct val b = {.id = 0, .val = 6};
    struct val e = {.id = 0, .val = 44};
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    const int rev_range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    const pq_rrange rev_range
        = pq_equal_rrange(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(rev_range.rbegin, struct val, elem)->val == rev_range_vals[0]
              && pq_entry(rev_range.end, struct val, elem)->val
                     == rev_range_vals[7],
          true, bool, "%b");
    size_t index = 0;
    pq_elem *i1 = rev_range.rbegin;
    for (; i1 != rev_range.end; i1 = pq_rnext(&pq, i1))
    {
        const int cur_val = pq_entry(i1, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1 == rev_range.end
              && pq_entry(i1, struct val, elem)->val == rev_range_vals[7],
          true, bool, "%b");
    b.val = 119;
    e.val = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    const int range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    const struct range range
        = pq_equal_range(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(range.begin, struct val, elem)->val == range_vals[0]
              && pq_entry(range.end, struct val, elem)->val == range_vals[7],
          true, bool, "%b");
    index = 0;
    pq_elem *i2 = range.begin;
    for (; i2 != range.end; i2 = pq_next(&pq, i2))
    {
        const int cur_val = pq_entry(i2, struct val, elem)->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2 == range.end
              && pq_entry(i2, struct val, elem)->val == range_vals[7],
          true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_priority_invalid_range(void)
{
    pqueue pq;
    pq_init(&pq);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    struct val b = {.id = 0, .val = 95};
    struct val e = {.id = 0, .val = 999};
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    const int rev_range_vals[6] = {95, 100, 105, 110, 115, 120};
    const pq_rrange rev_range
        = pq_equal_rrange(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(rev_range.rbegin, struct val, elem)->val == rev_range_vals[0]
              && rev_range.end == pq_end(&pq),
          true, bool, "%b");
    size_t index = 0;
    pq_elem *i1 = rev_range.rbegin;
    for (; i1 != rev_range.end; i1 = pq_rnext(&pq, i1))
    {
        const int cur_val = pq_entry(i1, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1 == rev_range.end && i1 == pq_end(&pq), true, bool, "%b");
    b.val = 36;
    e.val = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    const int range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    const pq_range range = pq_equal_range(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(range.begin, struct val, elem)->val == range_vals[0]
              && range.end == pq_end(&pq),
          true, bool, "%b");
    index = 0;
    pq_elem *i2 = range.begin;
    for (; i2 != range.end; i2 = pq_next(&pq, i2))
    {
        const int cur_val = pq_entry(i2, struct val, elem)->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2 == range.end && i2 == pq_end(&pq), true, bool, "%b");
    return PASS;
}

static enum test_result
pq_test_priority_empty_range(void)
{
    pqueue pq;
    pq_init(&pq);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&pq, val_cmp), true, bool, "%b");
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    const pq_rrange rev_range
        = pq_equal_rrange(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(rev_range.rbegin, struct val, elem)->val == vals[0].val
              && pq_entry(rev_range.end, struct val, elem)->val == vals[0].val,
          true, bool, "%b");
    b.val = 150;
    e.val = 999;
    const pq_range range = pq_equal_range(&pq, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(pq_entry(range.begin, struct val, elem)->val
                  == vals[num_nodes - 1].val
              && pq_entry(range.end, struct val, elem)->val
                     == vals[num_nodes - 1].val,
          true, bool, "%b");
    return PASS;
}

static enum test_result
insert_shuffled(pqueue *pq, struct val vals[], const size_t size,
                const int larger_prime)
{
    /* Math magic ahead so that we iterate over every index
       eventually but in a shuffled order. Not necessarily
       randome but a repeatable sequence that makes it
       easier to debug if something goes wrong. Think
       of the prime number as a random seed, kind of. */
    size_t shuffled_index = larger_prime % size;
    for (size_t i = 0; i < size; ++i)
    {
        vals[shuffled_index].val = (int)shuffled_index;
        pq_insert(pq, &vals[shuffled_index].elem, val_cmp, NULL);
        CHECK(pq_size(pq), i + 1, size_t, "%zu");
        CHECK(validate_tree(pq, val_cmp), true, bool, "%b");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(pq_size(pq), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, pqueue *pq)
{
    if (pq_size(pq) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (pq_elem *e = pq_rbegin(pq); e != pq_end(pq); e = pq_rnext(pq, e))
    {
        vals[i++] = pq_entry(e, struct val, elem)->val;
    }
    return i;
}

static enum test_result
iterator_check(pqueue *pq)
{
    const size_t size = pq_size(pq);
    size_t iter_count = 0;
    for (pq_elem *e = pq_begin(pq); e != pq_end(pq); e = pq_next(pq, e))
    {
        ++iter_count;
        CHECK(iter_count != size || pq_is_min(pq, e), true, bool, "%b");
        CHECK(iter_count == size || !pq_is_min(pq, e), true, bool, "%b");
    }
    CHECK(iter_count, size, size_t, "%zu");
    iter_count = 0;
    for (pq_elem *e = pq_rbegin(pq); e != pq_end(pq); e = pq_rnext(pq, e))
    {
        ++iter_count;
        CHECK(iter_count != size || pq_is_max(pq, e), true, bool, "%b");
        CHECK(iter_count == size || !pq_is_max(pq, e), true, bool, "%b");
    }
    CHECK(iter_count, size, size_t, "%zu");
    return PASS;
}

static threeway_cmp
val_cmp(const pq_elem *a, const pq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = pq_entry(a, struct val, elem);
    struct val *rhs = pq_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
pq_printer_fn(const pq_elem *const e)
{
    const struct val *const v = pq_entry(e, struct val, elem);
    printf("{id:%d,val:%d}", v->id, v->val);
}

static void
val_update(pq_elem *a, void *aux)
{
    struct val *old = pq_entry(a, struct val, elem);
    old->val = *(int *)aux;
}
