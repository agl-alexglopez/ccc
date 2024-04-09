#include "set.h"
#include "test.h"
#include "tree.h"

#include <stdio.h>

struct val
{
    int id;
    int val;
    set_elem elem;
};

static enum test_result set_test_insert_one(void);
static enum test_result set_test_insert_three(void);
static enum test_result set_test_struct_getter(void);
static enum test_result set_test_insert_shuffle(void);
static enum test_result insert_shuffled(set *, struct val[], size_t, int);
static size_t inorder_fill(int vals[], size_t, set *);
static threeway_cmp val_cmp(const set_elem *, const set_elem *, void *);

#define NUM_TESTS ((size_t)4)
const struct fn_name all_tests[NUM_TESTS] = {
    {set_test_insert_one, "set_test_insert_one"},
    {set_test_insert_three, "set_test_insert_three"},
    {set_test_struct_getter, "set_test_struct_getter"},
    {set_test_insert_shuffle, "set_test_insert_shuffle"},
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        const bool fail = all_tests[i].fn() == FAIL;
        if (fail)
        {
            res = FAIL;
            (void)fprintf(stderr, "failure in tests_set.c: %s\n",
                          all_tests[i].name);
        }
    }
    return res;
}

static enum test_result
set_test_insert_one(void)
{
    set s;
    set_init(&s);
    struct val single;
    single.val = 0;
    CHECK(set_insert(&s, &single.elem, val_cmp, NULL), true, bool, "%b");
    CHECK(set_empty(&s), false, bool, "%b");
    CHECK(set_entry(set_root(&s), struct val, elem)->val == single.val, true,
          bool, "%b");
    return PASS;
}

static enum test_result
set_test_insert_three(void)
{
    set s;
    set_init(&s);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        CHECK(set_insert(&s, &three_vals[i].elem, val_cmp, NULL), true, bool,
              "%b");
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    CHECK(set_size(&s), 3ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_struct_getter(void)
{
    set s;
    set_init(&s);
    set set_tester_clone;
    set_init(&set_tester_clone);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        CHECK(set_insert(&s, &vals[i].elem, val_cmp, NULL), true, bool, "%b");
        CHECK(
            set_insert(&set_tester_clone, &tester_clone[i].elem, val_cmp, NULL),
            true, bool, "%b");
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our
           get to uncorrupted data. */
        const struct val *get
            = set_entry(&tester_clone[i].elem, struct val, elem);
        CHECK(get->val, vals[i].val, int, "%d");
    }
    CHECK(set_size(&s), 10ULL, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_insert_shuffle(void)
{
    set s;
    set_init(&s);
    /* Math magic ahead... */
    const size_t size = 50;
    const int prime = 53;
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
insert_shuffled(set *s, struct val vals[], const size_t size,
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
        set_insert(s, &vals[shuffled_index].elem, val_cmp, NULL);
        CHECK(set_size(s), i + 1, size_t, "%zu");
        CHECK(validate_tree(s, val_cmp), true, bool, "%b");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(set_size(s), size, size_t, "%zu");
    return PASS;
}

/* Iterative inorder traversal to check the heap is sorted. */
static size_t
inorder_fill(int vals[], size_t size, set *s)
{
    if (set_size(s) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (set_elem *e = set_begin(s); e != set_end(s); e = set_next(s, e))
    {
        vals[i++] = set_entry(e, struct val, elem)->val;
    }
    return i;
}

static threeway_cmp
val_cmp(const set_elem *a, const set_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = set_entry(a, struct val, elem);
    struct val *rhs = set_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
