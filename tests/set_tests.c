#include "set.h"
#include "tree.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

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
static int run_tests(void);
static threeway_cmp val_cmp(const set_elem *, const set_elem *, void *);

#define NUM_TESTS 4
const test_fn all_tests[NUM_TESTS] = {
    set_test_empty,
    set_test_insert_one,
    set_test_insert_three,
    set_test_struct_getter,
};

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

static threeway_cmp
val_cmp(const set_elem *a, const set_elem *b, void *aux)
{
    (void)aux;
    struct val *lhs = set_entry(a, struct val, elem);
    struct val *rhs = set_entry(b, struct val, elem);
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
