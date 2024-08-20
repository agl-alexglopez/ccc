#include "set.h"
#include "test.h"
#include "tree.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct val
{
    int id;
    int val;
    ccc_set_elem elem;
};

static enum test_result set_test_insert_one(void);
static enum test_result set_test_insert_three(void);
static enum test_result set_test_struct_getter(void);
static enum test_result set_test_insert_shuffle(void);
static enum test_result insert_shuffled(ccc_set *, struct val[], size_t, int);
static size_t inorder_fill(int vals[], size_t, ccc_set *);
static ccc_threeway_cmp val_cmp(void const *, void const *, void *);

#define NUM_TESTS ((size_t)4)
test_fn const all_tests[NUM_TESTS] = {
    set_test_insert_one,
    set_test_insert_three,
    set_test_struct_getter,
    set_test_insert_shuffle,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        bool const fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
set_test_insert_one(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
    struct val single;
    single.val = 0;
    CHECK(ccc_set_insert(&s, &single.elem), true, bool, "%d");
    CHECK(ccc_set_empty(&s), false, bool, "%d");
    CHECK(((struct val *)ccc_set_root(&s))->val == single.val, true, bool,
          "%d");
    return PASS;
}

static enum test_result
set_test_insert_three(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        CHECK(ccc_set_insert(&s, &three_vals[i].elem), true, bool, "%d");
        CHECK(ccc_tree_validate(&s.t), true, bool, "%d");
    }
    CHECK(ccc_set_size(&s), 3ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_struct_getter(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
    ccc_set set_tester_clone
        = CCC_SET_INIT(struct val, elem, set_tester_clone, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        CHECK(ccc_set_insert(&s, &vals[i].elem), true, bool, "%d");
        CHECK(ccc_set_insert(&set_tester_clone, &tester_clone[i].elem), true,
              bool, "%d");
        CHECK(ccc_tree_validate(&s.t), true, bool, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our
           get to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(ccc_set_size(&s), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_insert_shuffle(void)
{
    ccc_set s = CCC_SET_INIT(struct val, elem, s, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS, enum test_result, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &s), size, size_t, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], int, "%d");
    }
    return PASS;
}

static enum test_result
insert_shuffled(ccc_set *s, struct val vals[], size_t const size,
                int const larger_prime)
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
        ccc_set_insert(s, &vals[shuffled_index].elem);
        CHECK(ccc_set_size(s), i + 1, size_t, "%zu");
        CHECK(ccc_tree_validate(&s->t), true, bool, "%d");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(ccc_set_size(s), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, ccc_set *s)
{
    if (ccc_set_size(s) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct val *e = ccc_set_begin(s); e; e = ccc_set_next(s, &e->elem))
    {
        vals[i++] = e->val;
    }
    return i;
}

static ccc_threeway_cmp
val_cmp(void const *a, void const *b, void *aux)
{
    (void)aux;
    struct val const *const lhs = a;
    struct val const *const rhs = b;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
