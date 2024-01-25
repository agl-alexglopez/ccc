#include "set.h"

#include <signal.h>
#include <stdbool.h>
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
    set_elem elem;
};

static bool set_test_empty(void);
static bool set_test_insert_one(void);
static bool set_test_insert_three(void);
static bool set_test_struct_getter(void);
static bool set_test_insert_shuffle(void);
static bool set_test_insert_erase_shuffled(void);
static bool set_test_prime_shuffle(void);
static bool set_test_weak_srand(void);
static int run_tests(void);
static bool insert_shuffled(set *, struct val[], size_t, int);
static size_t inorder_fill(int vals[], size_t, set *);
static bool iterator_check(set *);
static void set_printer_fn(const set_elem *e);
static threeway_cmp val_cmp(const set_elem *, const set_elem *, void *);

#define NUM_TESTS 8
const test_fn all_tests[NUM_TESTS] = {
    set_test_empty,          set_test_insert_one,
    set_test_insert_three,   set_test_struct_getter,
    set_test_insert_shuffle, set_test_insert_erase_shuffled,
    set_test_prime_shuffle,  set_test_weak_srand,
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
    int pass_count = 0;
    for (int i = 0; i < NUM_TESTS; ++i)
    {
        const bool passed = all_tests[i]();
        pass_count += passed;
        passed ? printf("...%s\n", pass_msg) : printf("...%s\n", fail_msg);
    }
    printf("PASSED %d/%d %s\n\n", pass_count, NUM_TESTS,
           (pass_count == NUM_TESTS) ? "\\(*.*)/\n" : ">:(\n");
    return NUM_TESTS - pass_count;
}
static bool
set_test_empty(void)
{
    printf("set_test_empty");
    set s;
    set_init(&s);
    return set_empty(&s);
}

static bool
set_test_insert_one(void)
{
    printf("set_test_insert_one");
    set s;
    set_init(&s);
    struct val single;
    single.val = 0;
    return set_insert(&s, &single.elem, val_cmp, NULL) && !set_empty(&s)
           && set_entry(set_root(&s), struct val, elem)->val == single.val;
}

static bool
set_test_insert_three(void)
{
    printf("set_test_insert_three");
    set s;
    set_init(&s);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        if (!set_insert(&s, &three_vals[i].elem, val_cmp, NULL)
            || !validate_tree(&s, val_cmp))
        {
            breakpoint();
            return false;
        }
    }
    return set_size(&s) == 3;
}

static bool
set_test_struct_getter(void)
{
    printf("set_test_getter_macro");
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
        if (!set_insert(&s, &vals[i].elem, val_cmp, NULL)
            || !set_insert(&set_tester_clone, &tester_clone[i].elem, val_cmp,
                           NULL)
            || !validate_tree(&s, val_cmp))
        {
            breakpoint();
            return false;
        }
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        const struct val *get
            = set_entry(&tester_clone[i].elem, struct val, elem);
        if (get->val != vals[i].val)
        {
            breakpoint();
            return false;
        }
    }
    return set_size(&s) == 10;
}

static bool
set_test_insert_shuffle(void)
{
    printf("set_test_insert_shuffle");
    set s;
    set_init(&s);
    /* Math magic ahead... */
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    if (!insert_shuffled(&s, vals, size, prime))
    {
        return false;
    }
    int sorted_check[size];
    if (inorder_fill(sorted_check, size, &s) != size)
    {
        return false;
    }
    for (size_t i = 0; i < size; ++i)
    {
        if (vals[i].val != sorted_check[i])
        {
            return false;
        }
    }
    return true;
}

static bool
set_test_prime_shuffle(void)
{
    printf("set_test_prime_shuffle");
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
        if (!set_insert(&s, &vals[i].elem, val_cmp, NULL))
        {
            repeats[i] = true;
        }
        if (!validate_tree(&s, val_cmp))
        {
            return false;
        }
        shuffled_index = (shuffled_index + prime) % (size - less);
    }
    /* One test can use our printer function as test output */
    set_print(&s, s.root, set_printer_fn);
    if (set_size(&s) >= size)
    {
        return false;
    }
    for (size_t i = 0; i < size; ++i)
    {
        const set_elem *elem = set_erase(&s, &vals[i].elem, val_cmp, NULL);
        if (elem == set_end(&s) && repeats[i] == false)
        {
            return false;
        }
        if (!validate_tree(&s, val_cmp))
        {
            return false;
        }
    }
    return true;
}

static bool
set_test_insert_erase_shuffled(void)
{
    printf("set_test_insert_erase_shuffle");
    set s;
    set_init(&s);
    const size_t size = 50;
    const int prime = 53;
    struct val vals[size];
    if (!insert_shuffled(&s, vals, size, prime))
    {
        return false;
    }
    int sorted_check[size];
    if (inorder_fill(sorted_check, size, &s) != size)
    {
        return false;
    }
    for (size_t i = 0; i < size; ++i)
    {
        if (vals[i].val != sorted_check[i])
        {
            return false;
        }
    }
    /* Now let's delete everything with no errors. */
    for (size_t i = 0; i < size; ++i)
    {
        (void)set_erase(&s, &vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&s, val_cmp))
        {
            return false;
        }
    }
    if (!set_empty(&s))
    {
        return false;
    }
    return true;
}

static bool
set_test_weak_srand(void)
{
    printf("set_test_weak_srand");
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
        if (!validate_tree(&s, val_cmp))
        {
            return false;
        }
    }
    for (int i = 0; i < num_nodes; ++i)
    {
        (void)set_erase(&s, &vals[i].elem, val_cmp, NULL);
        if (!validate_tree(&s, val_cmp))
        {
            return false;
        }
    }
    if (!set_empty(&s))
    {
        return false;
    }
    return true;
}

static bool
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
        if (set_size(s) != i + 1)
        {
            return false;
        }
        if (!validate_tree(s, val_cmp))
        {
            return false;
        }
        shuffled_index = (shuffled_index + larger_prime) % size;
    }
    if (!iterator_check(s))
    {
        return false;
    }
    const size_t catch_size = size;
    if (set_size(s) != catch_size)
    {
        return false;
    }
    return true;
}

static bool
iterator_check(set *s)
{
    const size_t size = set_size(s);
    size_t iter_count = 0;
    for (set_elem *e = set_begin(s); e != set_end(s); e = set_next(s, e))
    {
        ++iter_count;
        if (iter_count == size && !set_is_max(s, e))
        {
            return false;
        }
        if (iter_count != size && set_is_max(s, e))
        {
            return false;
        }
    }
    if (iter_count != size)
    {
        return false;
    }
    iter_count = 0;
    for (set_elem *e = set_rbegin(s); e != set_end(s); e = set_rnext(s, e))
    {
        ++iter_count;
        if (iter_count == size && !set_is_min(s, e))
        {
            return false;
        }
        if (iter_count != size && set_is_min(s, e))
        {
            return false;
        }
    }
    return iter_count == size;
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
