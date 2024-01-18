#include "pqueue.h"

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_TESTS 3

/* Set this breakpoint on any line where you wish
   execution to stop. Under normal program runs the program
   will simply exit. If triggered in GDB execution will stop
   while able to explore the surrounding context, varialbes,
   and stack frames. Be sure to step "(gdb) up" out of the
   raise function to wherever it triggered. */
#define breakpoint()                                                          \
  do                                                                          \
    {                                                                         \
      (void)fprintf (stderr, "\n!!Break. Line: %d File: %s, Func: %s\n ",     \
                     __LINE__, __FILE__, __func__);                           \
      (void)raise (SIGTRAP);                                                  \
    }                                                                         \
  while (0)

const char *const pass_msg = "pass";
const char *const fail_msg = "fail";

typedef bool (*test_fn) (void);

struct val
{
  int val;
  pq_elem elem;
};

static bool pq_test_empty (void);
static bool pq_test_insert_one (void);
static bool pq_test_insert_three (void);
static int run_tests (void);
static threeway_cmp val_cmp (const pq_elem *, const pq_elem *, void *);

const test_fn all_tests[NUM_TESTS] = {
  pq_test_empty,
  pq_test_insert_one,
  pq_test_insert_three,
};

int
main ()
{
  return run_tests ();
}

static int
run_tests (void)
{
  printf ("\n");
  int pass_count = 0;
  for (int i = 0; i < NUM_TESTS; ++i)
    {
      const bool passed = all_tests[i]();
      pass_count += passed;
      passed ? printf ("...%s\n", pass_msg) : printf ("...%s\n", fail_msg);
    }
  printf ("PASSED %d/%d %s\n\n", pass_count, NUM_TESTS,
          (pass_count == NUM_TESTS) ? "\\(*.*)/\n" : ">:(\n");
  return 0;
}

static bool
pq_test_empty (void)
{
  printf ("pq_test_empty");
  pqueue pq;
  pq_init (&pq);
  return pq_empty (&pq);
}

static bool
pq_test_insert_one (void)
{
  printf ("pq_test_insert_one");
  pqueue pq;
  pq_init (&pq);
  struct val single;
  single.val = 0;
  pq_insert (&pq, &single.elem, val_cmp, NULL);
  return !pq_empty (&pq)
         && tree_entry (pq_root (&pq), struct val, elem)->val == single.val;
}

static bool
pq_test_insert_three (void)
{
  printf ("pq_test_insert_three");
  pqueue pq;
  pq_init (&pq);
  struct val three_vals[3];
  for (int i = 0; i < 3; ++i)
    {
      three_vals[i].val = i;
      pq_insert (&pq, &three_vals[i].elem, val_cmp, NULL);
      if (!validate_tree (&pq, val_cmp))
        {
          breakpoint ();
          return false;
        }
    }
  return true;
}

static threeway_cmp
val_cmp (const pq_elem *a, const pq_elem *b, void *aux)
{
  (void)aux;
  struct val *lhs = tree_entry (a, struct val, elem);
  struct val *rhs = tree_entry (b, struct val, elem);
  return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
