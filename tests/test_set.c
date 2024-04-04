#include "set.h"
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
    set_elem elem;
};

static enum test_result set_test_empty(void);
static enum test_result set_test_insert_one(void);
static enum test_result set_test_insert_three(void);
static enum test_result set_test_struct_getter(void);
static enum test_result set_test_insert_shuffle(void);
static enum test_result set_test_insert_erase_shuffled(void);
static enum test_result set_test_prime_shuffle(void);
static enum test_result set_test_weak_srand(void);
static enum test_result set_test_forward_iter(void);
static enum test_result set_test_iterate_removal(void);
static enum test_result set_test_iterate_remove_reinsert(void);
static enum test_result set_test_valid_range(void);
static enum test_result set_test_invalid_range(void);
static enum test_result set_test_empty_range(void);
static int run_tests(void);
static enum test_result insert_shuffled(set *, struct val[], size_t, int);
static size_t inorder_fill(int vals[], size_t, set *);
static enum test_result iterator_check(set *);
static void set_printer_fn(const set_elem *e);
static threeway_cmp val_cmp(const set_elem *, const set_elem *, void *);

#define NUM_TESTS ((size_t)14)
const struct fn_name all_tests[NUM_TESTS] = {
    {set_test_empty, "set_test_empty"},
    {set_test_insert_one, "set_test_insert_one"},
    {set_test_insert_three, "set_test_insert_three"},
    {set_test_struct_getter, "set_test_struct_getter"},
    {set_test_insert_shuffle, "set_test_insert_shuffle"},
    {set_test_insert_erase_shuffled, "set_test_insert_erase_shuffled"},
    {set_test_prime_shuffle, "set_test_prime_shuffle"},
    {set_test_weak_srand, "set_test_weak_srand"},
    {set_test_forward_iter, "set_test_forward_iter"},
    {set_test_iterate_removal, "set_test_iterate_removal"},
    {set_test_valid_range, "set_test_valid_range"},
    {set_test_invalid_range, "set_test_invalid_range"},
    {set_test_empty_range, "set_test_empty_range"},
    {set_test_iterate_remove_reinsert, "set_test_iterate_remove_reinsert"},
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
            (void)fprintf(stderr, "failure in tests_set.c: %s\n",
                          all_tests[i].name);
        }
    }
    return res;
}
static enum test_result
set_test_empty(void)
{
    set s;
    set_init(&s);
    CHECK(set_empty(&s), true, bool, "%b");
    return PASS;
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
set_test_prime_shuffle(void)
{
    set s;
    set_init(&s);
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
        if (set_insert(&s, &vals[i].elem, val_cmp, NULL))
        {
            repeats[i] = true;
        }
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    set_print(&s, s.root, set_printer_fn);
    CHECK(set_size(&s) < size, true, bool, "%b");
    for (size_t i = 0; i < size; ++i)
    {
        const set_elem *elem = set_erase(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(elem != set_end(&s) || !repeats[i], true, bool, "%b");
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    return PASS;
}

static enum test_result
set_test_insert_erase_shuffled(void)
{
    set s;
    set_init(&s);
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
        (void)set_erase(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    CHECK(set_empty(&s), true, bool, "%b");
    return PASS;
}

static enum test_result
set_test_weak_srand(void)
{
    set s;
    set_init(&s);
    /* Seed the test with any integer for reproducible randome test sequence
       currently this will change every test. NOLINTNEXTLINE */
    srand(time(NULL));
    const int num_nodes = 1000;
    struct val vals[num_nodes];
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = rand(); // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        (void)set_erase(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    CHECK(set_empty(&s), true, bool, "%b");
    return PASS;
}

static enum test_result
set_test_forward_iter(void)
{
    set s;
    set_init(&s);
    /* We should have the expected behavior iteration over empty tree. */
    int j = 0;
    for (set_elem *e = set_begin(&s); e != set_end(&s);
         e = set_next(&s, e), ++j)
    {}
    CHECK(j, 0, int, "%d");
    const int num_nodes = 33;
    const int prime = 37;
    struct val vals[num_nodes];
    size_t shuffled_index = prime % num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        vals[i].val = (int)shuffled_index;
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
        shuffled_index = (shuffled_index + prime) % num_nodes;
    }
    int val_keys_inorder[num_nodes];
    CHECK(inorder_fill(val_keys_inorder, num_nodes, &s), set_size(&s), size_t,
          "%zu");
    j = 0;
    for (set_elem *e = set_begin(&s); e != set_end(&s) && j < num_nodes;
         e = set_next(&s, e), ++j)
    {
        const struct val *v = set_entry(e, struct val, elem);
        CHECK(v->val, val_keys_inorder[j], int, "%d");
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
    CHECK(iterator_check(s), PASS, enum test_result, "%d");
    CHECK(set_size(s), size, size_t, "%zu");
    return PASS;
}

static enum test_result
iterator_check(set *s)
{
    const size_t size = set_size(s);
    size_t iter_count = 0;
    for (set_elem *e = set_begin(s); e != set_end(s); e = set_next(s, e))
    {
        ++iter_count;
        CHECK(iter_count != size || set_is_max(s, e), true, bool, "%b");
        CHECK(iter_count == size || !set_is_max(s, e), true, bool, "%b");
    }
    CHECK(iter_count, size, size_t, "%zu");
    iter_count = 0;
    for (set_elem *e = set_rbegin(s); e != set_end(s); e = set_rnext(s, e))
    {
        ++iter_count;
        CHECK(iter_count != size || set_is_min(s, e), true, bool, "%b");
        CHECK(iter_count == size || !set_is_min(s, e), true, bool, "%b");
    }
    CHECK(iter_count, size, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_iterate_removal(void)
{
    set s;
    set_init(&s);
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
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&s), PASS, enum test_result, "%d");
    const int limit = 400;
    for (set_elem *i = set_begin(&s), *next = NULL; i != set_end(&s); i = next)
    {
        next = set_next(&s, i);
        struct val *cur = set_entry(i, struct val, elem);
        if (cur->val > limit)
        {
            (void)set_erase(&s, i, val_cmp, NULL);
            CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
        }
    }
    return PASS;
}

static enum test_result
set_test_iterate_remove_reinsert(void)
{
    set s;
    set_init(&s);
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
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    CHECK(iterator_check(&s), PASS, enum test_result, "%d");
    const size_t old_size = set_size(&s);
    const int limit = 400;
    int new_unique_entry_val = 1001;
    for (set_elem *i = set_begin(&s), *next = NULL; i != set_end(&s); i = next)
    {
        next = set_next(&s, i);
        struct val *cur = set_entry(i, struct val, elem);
        if (cur->val < limit)
        {
            (void)set_erase(&s, i, val_cmp, NULL);
            struct val *v = set_entry(i, struct val, elem);
            v->val = new_unique_entry_val;
            CHECK(set_insert(&s, i, val_cmp, NULL), true, bool, "%b");
            CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
            ++new_unique_entry_val;
        }
    }
    CHECK(set_size(&s), old_size, size_t, "%zu");
    return PASS;
}

static enum test_result
set_test_valid_range(void)
{
    set s;
    set_init(&s);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    struct val b = {.id = 0, .val = 6};
    struct val e = {.id = 0, .val = 44};
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    const int range_vals[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    const set_range range
        = set_equal_range(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(range.begin, struct val, elem)->val, range_vals[0], int,
          "%d");
    CHECK(set_entry(range.end, struct val, elem)->val, range_vals[7], int,
          "%d");
    size_t index = 0;
    set_elem *i1 = range.begin;
    for (; i1 != range.end; i1 = set_next(&s, i1))
    {
        const int cur_val = set_entry(i1, struct val, elem)->val;
        CHECK(range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1, range.end, set_elem *, "%p");
    CHECK(set_entry(i1, struct val, elem)->val, range_vals[7], int, "%d");
    b.val = 119;
    e.val = 84;
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    const int rev_range_vals[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    const set_rrange rev_range
        = set_equal_rrange(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(rev_range.rbegin, struct val, elem)->val, rev_range_vals[0],
          int, "%d");
    CHECK(set_entry(rev_range.end, struct val, elem)->val, rev_range_vals[7],
          int, "%d");
    index = 0;
    set_elem *i2 = rev_range.rbegin;
    for (; i2 != rev_range.end; i2 = set_rnext(&s, i2))
    {
        const int cur_val = set_entry(i2, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2, rev_range.end, set_elem *, "%p");
    CHECK(set_entry(i2, struct val, elem)->val, rev_range_vals[7], int, "%d");
    return PASS;
}

static enum test_result
set_test_invalid_range(void)
{
    set s;
    set_init(&s);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    struct val b = {.id = 0, .val = 95};
    struct val e = {.id = 0, .val = 999};
    /* This should be the following range [95,999). 95 should raise to
       next value not less than 95, 95 and 999 should be the first
       value greater than 999, none or the end. */
    const int forward_range_vals[6] = {95, 100, 105, 110, 115, 120};
    const set_range rev_range
        = set_equal_range(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(rev_range.begin, struct val, elem)->val
              == forward_range_vals[0],
          true, bool, "%b");
    CHECK(rev_range.end, set_end(&s), set_elem *, "%p");
    size_t index = 0;
    set_elem *i1 = rev_range.begin;
    for (; i1 != rev_range.end; i1 = set_next(&s, i1))
    {
        const int cur_val = set_entry(i1, struct val, elem)->val;
        CHECK(forward_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i1, rev_range.end, set_elem *, "%p");
    CHECK(i1, set_end(&s), set_elem *, "%p");
    b.val = 36;
    e.val = -999;
    /* This should be the following range [36,-999). 36 should be
       dropped to first value not greater than 36 and last should
       be dropped to first value less than -999 which is end. */
    const int rev_range_vals[8] = {35, 30, 25, 20, 15, 10, 5, 0};
    const set_rrange range
        = set_equal_rrange(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(range.rbegin, struct val, elem)->val, rev_range_vals[0],
          int, "%d");
    CHECK(range.end, set_end(&s), set_elem *, "%p");
    index = 0;
    set_elem *i2 = range.rbegin;
    for (; i2 != range.end; i2 = set_rnext(&s, i2))
    {
        const int cur_val = set_entry(i2, struct val, elem)->val;
        CHECK(rev_range_vals[index], cur_val, int, "%d");
        ++index;
    }
    CHECK(i2, range.end, set_elem *, "%p");
    CHECK(i2, set_end(&s), set_elem *, "%p");
    return PASS;
}

static enum test_result
set_test_empty_range(void)
{
    set s;
    set_init(&s);

    const int num_nodes = 25;
    struct val vals[num_nodes];
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, val = 0; i < num_nodes; ++i, val += 5)
    {
        vals[i].val = val; // NOLINT
        vals[i].id = i;
        set_insert(&s, &vals[i].elem, val_cmp, NULL);
        CHECK(validate_tree(&s, val_cmp), true, bool, "%b");
    }
    /* Nonexistant range returns end [begin, end) in both positions.
       which may not be the end element but a value in the tree. However,
       Normal iteration patterns would consider this empty. */
    struct val b = {.id = 0, .val = -50};
    struct val e = {.id = 0, .val = -25};
    const set_range forward_range
        = set_equal_range(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(forward_range.begin, struct val, elem)->val, vals[0].val,
          int, "%d");
    CHECK(set_entry(forward_range.end, struct val, elem)->val, vals[0].val, int,
          "%d");
    b.val = 150;
    e.val = 999;
    const set_rrange rev_range
        = set_equal_rrange(&s, &b.elem, &e.elem, val_cmp, NULL);
    CHECK(set_entry(rev_range.rbegin, struct val, elem)->val,
          vals[num_nodes - 1].val, int, "%d");
    CHECK(set_entry(rev_range.end, struct val, elem)->val,
          vals[num_nodes - 1].val, int, "%d");
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

static void
set_printer_fn(const set_elem *const e) // NOLINT
{
    const struct val *const v = set_entry(e, struct val, elem);
    printf("{id:%d,val:%d}", v->id, v->val);
}

static threeway_cmp
val_cmp(const set_elem *a, const set_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = set_entry(a, struct val, elem);
    struct val *rhs = set_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
