#include "set.h"
#include "test.h"
#include "tree.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct val
{
    int id;
    int val;
    struct set_elem elem;
};

static enum test_result set_test_insert_erase_shuffled(void);
static enum test_result set_test_prime_shuffle(void);
static enum test_result set_test_weak_srand(void);
static enum test_result insert_shuffled(struct set *, struct val[], size_t,
                                        int);
static size_t inorder_fill(int[], size_t, struct set *);
static node_threeway_cmp val_cmp(const struct set_elem *,
                                 const struct set_elem *, void *);
static void set_printer_fn(const struct set_elem *);

#define NUM_TESTS ((size_t)3)
const test_fn all_tests[NUM_TESTS] = {
    set_test_insert_erase_shuffled,
    set_test_prime_shuffle,
    set_test_weak_srand,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        const bool fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
set_test_prime_shuffle(void)
{
    struct set s;
    set_init(&s, val_cmp, NULL);
    const size_t size = 50;
    const size_t prime = 53;
    const size_t less = 10;
    /* We want the tree to have a smattering of duplicates so
       reduce the shuffle range so it will repeat some values. */
    size_t shuffled_index = prime % (size - less);
    struct val vals[size];
    bool repeats[size];
    memset(repeats, false, sizeof(bool) * size);
    for (size_t i = 0; i < size; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = (int)shuffled_index;
        if (set_insert(&s, &vals[i].elem))
        {
            repeats[i] = true;
        }
        CHECK(validate_tree(&s.t), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    set_print(&s, set_root(&s), set_printer_fn);
    CHECK(set_size(&s) < size, true, bool, "%b");
    for (size_t i = 0; i < size; ++i)
    {
        const struct set_elem *elem = set_erase(&s, &vals[i].elem);
        CHECK(elem != set_end(&s) || !repeats[i], true, bool, "%b");
        CHECK(validate_tree(&s.t), true, bool, "%b");
    }
    return PASS;
}

static enum test_result
set_test_insert_erase_shuffled(void)
{
    struct set s;
    set_init(&s, val_cmp, NULL);
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
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)set_erase(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%b");
    }
    CHECK(set_empty(&s), true, bool, "%b");
    return PASS;
}

static enum test_result
set_test_weak_srand(void)
{
    struct set s;
    set_init(&s, val_cmp, NULL);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const int num_nodes = 1000;
    struct val vals[num_nodes];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%b");
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        (void)set_erase(&s, &vals[i].elem);
        CHECK(validate_tree(&s.t), true, bool, "%b");
    }
    CHECK(set_empty(&s), true, bool, "%b");
    return PASS;
}

static enum test_result
insert_shuffled(struct set *s, struct val vals[], const size_t size,
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
        set_insert(s, &vals[shuffled_index].elem);
        CHECK(set_size(s), i + 1, size_t, "%zu");
        CHECK(validate_tree(&s->t), true, bool, "%b");
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    CHECK(set_size(s), size, size_t, "%zu");
    return PASS;
}

static size_t
inorder_fill(int vals[], size_t size, struct set *s)
{
    if (set_size(s) != size)
    {
        return 0;
    }
    size_t i = 0;
    for (struct set_elem *e = set_begin(s); e != set_end(s); e = set_next(s, e))
    {
        vals[i++] = SET_ENTRY(e, struct val, elem)->val;
    }
    return i;
}

static node_threeway_cmp
val_cmp(const struct set_elem *a, const struct set_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = SET_ENTRY(a, struct val, elem);
    struct val *rhs = SET_ENTRY(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

static void
set_printer_fn(const struct set_elem *const e) // NOLINT
{
    const struct val *const v = SET_ENTRY(e, struct val, elem);
    printf("{id:%d,val:%d}", v->id, v->val);
}
