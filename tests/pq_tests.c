#include "pqueue.h"
#include "tree.h"

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *const pass_msg = "pass";
const char *const fail_msg = "fail";

typedef bool (*test_fn)(void);

struct val
{
    int id;
    int val;
    pq_elem elem;
};

static bool pq_test_empty(void);
static bool pq_test_insert_one(void);
static bool pq_test_insert_three(void);
static bool pq_test_struct_getter(void);
static bool pq_test_insert_three_dups(void);
static bool pq_test_insert_remove_four_dups(void);
static bool pq_test_read_max_min(void);
static bool pq_test_insert_shuffle(void);
static bool pq_test_insert_erase_shuffled(void);
static bool pq_test_pop_max(void);
static bool pq_test_pop_min(void);
static bool pq_test_max_round_robin(void);
static bool pq_test_min_round_robin(void);
static bool pq_test_delete_prime_shuffle_duplicates(void);
static bool pq_test_prime_shuffle(void);
static bool pq_test_weak_srand(void);
static bool pq_test_forward_iter_unique_vals(void);
static bool pq_test_forward_iter_all_vals(void);
static bool pq_test_insert_iterate_pop(void);
static int run_tests(void);
static void insert_shuffled(pqueue *, struct val[], size_t, int);
static size_t inorder_fill(int vals[], size_t, pqueue *);
static bool iterate_check(pqueue *);
static threeway_cmp val_cmp(const pq_elem *, const pq_elem *, void *);

#define NUM_TESTS (size_t)19
const test_fn all_tests[NUM_TESTS] = {
    pq_test_empty,
    pq_test_insert_one,
    pq_test_insert_three,
    pq_test_struct_getter,
    pq_test_insert_three_dups,
    pq_test_insert_remove_four_dups,
    pq_test_read_max_min,
    pq_test_insert_shuffle,
    pq_test_insert_erase_shuffled,
    pq_test_pop_max,
    pq_test_pop_min,
    pq_test_max_round_robin,
    pq_test_min_round_robin,
    pq_test_delete_prime_shuffle_duplicates,
    pq_test_prime_shuffle,
    pq_test_weak_srand,
    pq_test_forward_iter_unique_vals,
    pq_test_forward_iter_all_vals,
    pq_test_insert_iterate_pop,
};

/* Set this breakpoint on any line where you wish
   execution to stop. Under normal program runs the program
   will simply exit. If triggered in GDB execution will stop
   while able to explore the surrounding context, varialbes,
   and stack frames. Be sure to step "(gdb) up" out of the
   raise function to wherever it triggered. */
#define breakpoint()                                                           \
    do                                                                         \
    {                                                                          \
        (void)fprintf(stderr, "\n!!Break. Line: %d File: %s, Func: %s\n ",     \
                      __LINE__, __FILE__, __func__);                           \
        (void)raise(SIGTRAP);                                                  \
    } while (0)

int
main()
{
    return run_tests();
}

static int
run_tests(void)
{
    printf("\n");
    size_t pass_count = 0;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        const bool passed = all_tests[i]();
        pass_count += passed;
        passed ? printf("...%s\n", pass_msg) : printf("...%s\n", fail_msg);
    }
    printf("PASSED %zu/%zu %s\n\n", pass_count, NUM_TESTS,
           (pass_count == NUM_TESTS) ? "\\(*.*)/\n" : ">:(\n");
    return 0;
}

static bool
pq_test_empty(void)
{
    printf("pq_test_empty");
    pqueue pq;
    pq_init(&pq);
    return pq_empty(&pq);
}

static bool
pq_test_insert_one(void)
{
    printf("pq_test_insert_one");
    pqueue pq;
    pq_init(&pq);
    struct val single;
    single.val = 0;
    pq_insert(&pq, &single.elem, val_cmp, NULL);
    return !pq_empty(&pq)
           && pq_entry(pq_root(&pq), struct val, elem)->val == single.val;
}

static bool
pq_test_insert_three(void)
{
    printf("pq_test_insert_three");
    pqueue pq;
    pq_init(&pq);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        pq_insert(&pq, &three_vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        const size_t size = i + 1;
        assert(pq_size(&pq) == size);
    }
    return pq_size(&pq) == 3;
}

static bool
pq_test_struct_getter(void)
{
    printf("pq_test_getter_macro");
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
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        const struct val *get
            = pq_entry(&tester_clone[i].elem, struct val, elem);
        if (get->val != vals[i].val)
        {
            breakpoint();
            return false;
        }
    }
    return pq_size(&pq) == 10;
}

static bool
pq_test_insert_three_dups(void)
{
    printf("pq_test_insert_three_duplicates");
    pqueue pq;
    pq_init(&pq);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        pq_insert(&pq, &three_vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        const size_t size = i + 1;
        assert(pq_size(&pq) == size);
    }
    return pq_size(&pq) == 3;
}

static bool
pq_test_read_max_min(void)
{
    printf("pq_test_read_max_min");
    pqueue pq;
    pq_init(&pq);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        const size_t size = i + 1;
        assert(pq_size(&pq) == size);
    }
    if (10 != pq_size(&pq))
    {
        breakpoint();
        return false;
    }
    const struct val *max = pq_entry(pq_max(&pq), struct val, elem);
    if (max->val != 9)
    {
        breakpoint();
        return false;
    }
    const struct val *min = pq_entry(pq_min(&pq), struct val, elem);
    if (min->val != 0)
    {
        breakpoint();
        return false;
    }
    return true;
}

static bool
pq_test_insert_shuffle(void)
{
    printf("pq_test_insert_shuffle");
    pqueue pq;
    pq_init(&pq);
    /* Math magic ahead... */
    const int size = 50;
    const int prime = 53;
    struct val vals[size];
    insert_shuffled(&pq, vals, size, prime);
    const struct val *max = pq_entry(pq_max(&pq), struct val, elem);
    if (max->val != size - 1)
    {
        breakpoint();
        return false;
    }
    const struct val *min = pq_entry(pq_min(&pq), struct val, elem);
    if (min->val != 0)
    {
        breakpoint();
        return false;
    }
    int sorted_check[size];
    inorder_fill(sorted_check, size, &pq);
    for (int i = 0; i < size; ++i)
    {
        if (vals[i].val != sorted_check[i])
        {
            return false;
        }
    }
    return true;
}

static bool
pq_test_insert_remove_four_dups(void)
{
    printf("pq_test_insert_remove_four_duplicates");
    pqueue pq;
    pq_init(&pq);
    struct val three_vals[4];
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        pq_insert(&pq, &three_vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        const size_t size = i + 1;
        assert(pq_size(&pq) == size);
    }
    if (pq_size(&pq) != 4)
    {
        breakpoint();
        return false;
    }
    for (int i = 0; i < 4; ++i)
    {
        three_vals[i].val = 0;
        pq_pop_max(&pq);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
    }
    return pq_empty(&pq);
}

static bool
pq_test_insert_erase_shuffled(void)
{
    printf("pq_test_insert_erase_shuffle");
    pqueue pq;
    pq_init(&pq);
    const int size = 50;
    const int prime = 53;
    struct val vals[size];
    insert_shuffled(&pq, vals, size, prime);
    const struct val *max = pq_entry(pq_max(&pq), struct val, elem);
    if (max->val != size - 1)
    {
        breakpoint();
        return false;
    }
    const struct val *min = pq_entry(pq_min(&pq), struct val, elem);
    if (min->val != 0)
    {
        breakpoint();
        return false;
    }
    int sorted_check[size];
    inorder_fill(sorted_check, size, &pq);
    for (int i = 0; i < size; ++i)
    {
        if (vals[i].val != sorted_check[i])
        {
            breakpoint();
            return false;
        }
    }

    /* Now let's delete everything with no errors. */
    for (int i = 0; i < size; ++i)
    {
        const struct val *removed = pq_entry(
            pq_erase(&pq, &vals[i].elem, val_cmp, NULL), struct val, elem);
        if (removed->val != vals[i].val)
        {
            breakpoint();
            return false;
        }
    }
    if (!pq_empty(&pq))
    {
        breakpoint();
        return false;
    }
    return true;
}

static bool
pq_test_pop_max(void)
{
    printf("pq_test_pop_max");
    pqueue pq;
    pq_init(&pq);
    const int size = 50;
    const int prime = 53;
    struct val vals[size];
    insert_shuffled(&pq, vals, size, prime);
    const struct val *max = pq_entry(pq_max(&pq), struct val, elem);
    if (max->val != size - 1)
    {
        breakpoint();
        return false;
    }
    const struct val *min = pq_entry(pq_min(&pq), struct val, elem);
    if (min->val != 0)
    {
        breakpoint();
        return false;
    }
    int sorted_check[size];
    inorder_fill(sorted_check, size, &pq);
    for (int i = 0; i < size; ++i)
    {
        if (vals[i].val != sorted_check[i])
        {
            breakpoint();
            return false;
        }
    }

    /* Now let's pop from the front of the queue until empty. */

    for (int i = size - 1; i >= 0; --i)
    {
        const struct val *front = pq_entry(pq_pop_max(&pq), struct val, elem);
        if (front->val != vals[i].val)
        {
            breakpoint();
            return false;
        }
    }
    if (!pq_empty(&pq))
    {
        breakpoint();
        return false;
    }
    return true;
}

static bool
pq_test_pop_min(void)
{
    printf("pq_test_pop_min");
    pqueue pq;
    pq_init(&pq);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    insert_shuffled(&pq, vals, size, prime);
    const struct val *max = pq_entry(pq_max(&pq), struct val, elem);

    const size_t max_s = max->val;
    if (max_s != size - 1)
    {
        breakpoint();
        return false;
    }
    const struct val *min = pq_entry(pq_min(&pq), struct val, elem);
    if (min->val != 0)
    {
        breakpoint();
        return false;
    }
    int sorted_check[size];

    if (inorder_fill(sorted_check, size, &pq) != size)
    {
        breakpoint();
        return false;
    }
    for (size_t i = 0; i < size; ++i)
    {
        if (vals[i].val != sorted_check[i])
        {
            breakpoint();
            return false;
        }
    }

    /* Now let's pop from the front of the queue until empty. */
    for (size_t i = 0; i < size; ++i)
    {
        const struct val *front = pq_entry(pq_pop_min(&pq), struct val, elem);
        if (front->val != vals[i].val)
        {
            breakpoint();
            return false;
        }
    }
    if (!pq_empty(&pq))
    {
        breakpoint();
        return false;
    }
    return true;
}

static bool
pq_test_max_round_robin(void)
{
    printf("pq_test_max_round_robin");
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
        assert(validate_tree(&pq, val_cmp));
    }

    /* Now let's make sure we pop round robin. */
    int last_id = 0;
    while (!pq_empty(&pq))
    {
        const struct val *front = pq_entry(pq_pop_max(&pq), struct val, elem);
        if (last_id >= front->id)
        {
            breakpoint();
            return false;
        }
        last_id = front->id;
    }
    return true;
}

static bool
pq_test_min_round_robin(void)
{
    printf("pq_test_min_round_robin");
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
        assert(validate_tree(&pq, val_cmp));
    }

    /* Now let's make sure we pop round robin. */
    int last_id = 0;
    while (!pq_empty(&pq))
    {
        const struct val *front = pq_entry(pq_pop_min(&pq), struct val, elem);
        if (last_id >= front->id)
        {
            breakpoint();
            return false;
        }
        last_id = front->id;
    }
    return true;
}

static bool
pq_test_delete_prime_shuffle_duplicates(void)
{
    printf("pq_test_delete_prime_shuffle_duplicates");
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
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        const size_t s = i + 1;
        if (pq_size(&pq) != s)
        {
            breakpoint();
            return false;
        }
        /* Shuffle like this only on insertions to create more dups. */
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    shuffled_index = prime % (size - less);
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        pq_erase(&pq, &vals[shuffled_index].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        --cur_size;
        const size_t cur_get_size = pq_size(&pq);
        if (cur_get_size != cur_size)
        {
            breakpoint();
            return false;
        }
        /* Shuffle normally here so we only remove each elem once. */
        shuffled_index = (shuffled_index + prime) % size;
    }
    return true;
}

static bool
pq_test_prime_shuffle(void)
{
    printf("pq_test_rand_queue");
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
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        shuffled_index = (shuffled_index + prime) % (size - less);
    }

    /* Now we go through and free all the elements in order but
       their positions in the tree will be somewhat random */
    size_t cur_size = size;
    for (int i = 0; i < size; ++i)
    {
        pq_erase(&pq, &vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        --cur_size;
        const size_t cur_get_size = pq_size(&pq);
        if (cur_get_size != cur_size)
        {
            breakpoint();
            return false;
        }
    }
    return true;
}

static bool
pq_test_weak_srand(void)
{
    printf("pq_test_weak_srand");
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
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
    }
    for (int i = 0; i < num_nodes; ++i)

    {
        pq_erase(&pq, &vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
    }
    if (!pq_empty(&pq))
    {
        breakpoint();
        return false;
    }
    return true;
}

static bool
pq_test_forward_iter_unique_vals(void)
{
    printf("pq_test_forward_iter_unique_vals");
    pqueue pq;
    pq_init(&pq);

    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (pq_elem *e = pq_uniq_begin(&pq); e != pq_uniq_end(&pq);
         e = pq_uniq_next(&pq, e), ++j)
    {}
    if (j != 0)
    {
        breakpoint();
        return false;
    }

    const int num_nodes = 33;
    const int prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = shuffled_index; // NOLINT
        vals[i].id = i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    inorder_fill(val_keys_inorder, num_nodes, &pq);
    for (pq_elem *e = pq_uniq_begin(&pq); e != pq_uniq_end(&pq);
         e = pq_uniq_next(&pq, e), ++j)
    {
        const struct val *v = pq_entry(e, struct val, elem);
        if (v->val != val_keys_inorder[j])
        {
            breakpoint();
            return false;
        }
    }
    return true;
}

static bool
pq_test_forward_iter_all_vals(void)
{
    printf("pq_test_forward_iter_all_vals");
    pqueue pq;
    pq_init(&pq);

    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (struct pq_iter i = pq_begin(&pq); !pq_end(&pq, &i);
         pq_next(&pq, &i), ++j)
    {}
    if (j != 0)
    {
        breakpoint();
        return false;
    }

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
            if (!validate_tree(&pq, val_cmp))
            {
                breakpoint();
                return false;
            }
        }
    }
    int val_keys_inorder[num_nodes];
    inorder_fill(val_keys_inorder, num_nodes, &pq);
    for (struct pq_iter i = pq_begin(&pq); !pq_end(&pq, &i);
         pq_next(&pq, &i), ++j)
    {
        const struct val *v = pq_entry(pq_from_iter(&i), struct val, elem);
        if (v->val != val_keys_inorder[j])
        {
            breakpoint();
            return false;
        }
    }
    return true;
}

static bool
pq_test_insert_iterate_pop(void)
{
    printf("pq_test_insert_iterate_pop");
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
        vals[i].val = rand() % (1000 + 1); // NOLINT
        vals[i].id = (int)i;
        pq_insert(&pq, &vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
    }
    if (!iterate_check(&pq))
    {
        breakpoint();
        return false;
    }
    size_t pop_count = 0;
    while (!pq_empty(&pq))
    {
        pq_pop_max(&pq);
        ++pop_count;
        if (!validate_tree(&pq, val_cmp))
        {
            breakpoint();
            return false;
        }
        if (pop_count % 200 && !iterate_check(&pq))
        {
            breakpoint();
            return false;
        }
    }
    if (pop_count != num_nodes)
    {
        breakpoint();
        return false;
    }
    return true;
}

static void
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
        if (pq_size(pq) != i + 1)
        {
            breakpoint();
        }
        if (!validate_tree(pq, val_cmp))
        {
            breakpoint();
        }
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    const size_t catch_size = size;
    assert(catch_size == pq_size(pq));
}

static void
fill_dups(size_t size, int vals[], size_t *i, struct node *n)
{
    if (!n->dups)
    {
        return;
    }
    const pq_elem *start = n->parent_or_dups;
    vals[(*i)++] = pq_entry(start, struct val, elem)->val;
    for (struct node *cur = start->link[N]; *i < size && cur != start;
         cur = cur->link[N])
    {
        vals[(*i)++] = pq_entry(cur, struct val, elem)->val;
    }
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
    for (pq_elem *e = pq_uniq_begin(pq); e != pq_uniq_end(pq);
         e = pq_uniq_next(pq, e))
    {
        vals[i++] = pq_entry(e, struct val, elem)->val;
        fill_dups(size, vals, &i, e);
    }
    return i;
}

static bool
iterate_check(pqueue *pq)
{
    size_t iter_count = 0;
    for (struct pq_iter e = pq_begin(pq); !pq_end(pq, &e); pq_next(pq, &e))
    {
        ++iter_count;
    }
    return iter_count == pq_size(pq);
}

static threeway_cmp
val_cmp(const pq_elem *a, const pq_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = pq_entry(a, struct val, elem);
    struct val *rhs = pq_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
