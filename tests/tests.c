#include "pqueue.h"

#include <stdbool.h>
#include <stdio.h>

#define NUM_TESTS 1
const char *const pass = "pass";
const char *const fail = "pass";

typedef bool (*test_fn) (void);

struct val
{
  int val;
  pq_elem elem;
};

static bool pq_test_empty (void);
static int run_tests (void);

int
main ()
{
  return run_tests ();
}

static int
run_tests (void)
{
  test_fn success[NUM_TESTS] = { pq_test_empty };
  printf ("\n");
  int pass_count = 0;
  for (int i = 0; i < NUM_TESTS; ++i)
    {
      const bool passed = success[i]();
      pass_count += passed;
      passed ? printf ("...%s\n", pass) : printf ("...%s\n", fail);
    }
  printf ("PASSED %d/%d\n\n", pass_count, NUM_TESTS);
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
