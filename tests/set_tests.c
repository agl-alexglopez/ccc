#include "set.h"
#include "tree.h"

#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  int id;
  int val;
  set_elem elem;
};

static bool set_test_empty (void);
static bool set_test_insert_one (void);
static bool set_test_insert_three (void);
static bool set_test_struct_getter (void);
static bool set_test_insert_shuffle (void);
static bool set_test_insert_erase_shuffled (void);
static int run_tests (void);
static void insert_shuffled (set *, struct val[], int, int);
static void inorder_fill (int vals[], size_t size, set *set);
static threeway_cmp val_cmp (const set_elem *, const set_elem *, void *);

#define NUM_TESTS 6
const test_fn all_tests[NUM_TESTS] = {
  set_test_empty,          set_test_insert_one,
  set_test_insert_three,   set_test_struct_getter,
  set_test_insert_shuffle, set_test_insert_erase_shuffled,
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
  return NUM_TESTS - pass_count;
}
static bool
set_test_empty (void)
{
  printf ("set_test_empty");
  set s;
  set_init (&s);
  return set_empty (&s);
}

static bool
set_test_insert_one (void)
{
  printf ("set_test_insert_one");
  set s;
  set_init (&s);
  struct val single;
  single.val = 0;
  assert (set_insert (&s, &single.elem, val_cmp, NULL));
  return !set_empty (&s)
         && set_entry (set_root (&s), struct val, elem)->val == single.val;
}

static bool
set_test_insert_three (void)
{
  printf ("set_test_insert_three");
  set s;
  set_init (&s);
  struct val three_vals[3];
  for (int i = 0; i < 3; ++i)
    {
      three_vals[i].val = i;
      assert (set_insert (&s, &three_vals[i].elem, val_cmp, NULL));
      if (!validate_tree (&s, val_cmp))
        {
          breakpoint ();
          return false;
        }
    }
  return set_size (&s) == 3;
}

static bool
set_test_struct_getter (void)
{
  printf ("set_test_getter_macro");
  set s;
  set_init (&s);
  set set_tester_clone;
  set_init (&set_tester_clone);
  struct val vals[10];
  struct val tester_clone[10];
  for (int i = 0; i < 10; ++i)
    {
      vals[i].val = i;
      tester_clone[i].val = i;
      assert (set_insert (&s, &vals[i].elem, val_cmp, NULL));
      assert (set_insert (&set_tester_clone, &tester_clone[i].elem, val_cmp,
                          NULL));
      if (!validate_tree (&s, val_cmp))
        {
          breakpoint ();
          return false;
        }
      /* Because the getter returns a pointer, if the casting returned
         misaligned data and we overwrote something we need to compare our get
         to uncorrupted data. */
      const struct val *get
          = set_entry (&tester_clone[i].elem, struct val, elem);
      if (get->val != vals[i].val)
        {
          breakpoint ();
          return false;
        }
    }
  return set_size (&s) == 10;
}

static bool
set_test_insert_shuffle (void)
{
  printf ("set_test_insert_shuffle");
  set s;
  set_init (&s);
  /* Math magic ahead... */
  const int size = 50;
  const int prime = 53;
  struct val vals[size];
  insert_shuffled (&s, vals, size, prime);
  int sorted_check[size];
  inorder_fill (sorted_check, size, &s);
  for (int i = 0; i < size; ++i)
    if (vals[i].val != sorted_check[i])
      return false;
  return true;
}

static bool
set_test_insert_erase_shuffled (void)
{
  printf ("set_test_insert_erase_shuffle");
  set s;
  set_init (&s);
  const int size = 50;
  const int prime = 53;
  struct val vals[size];
  insert_shuffled (&s, vals, size, prime);
  int sorted_check[size];
  inorder_fill (sorted_check, size, &s);
  for (int i = 0; i < size; ++i)
    if (vals[i].val != sorted_check[i])
      {
        breakpoint ();
        return false;
      }

  /* Now let's delete everything with no errors. */

  for (int i = 0; i < size; ++i)
    {
      const set_elem *elem = set_erase (&s, &vals[i].elem, val_cmp, NULL);
      if (elem == set_end (&s))
        {
          breakpoint ();
          return false;
        }
      const struct val *removed = set_entry (elem, struct val, elem);
      if (removed->val != vals[i].val)
        {
          breakpoint ();
          return false;
        }
    }
  if (!set_empty (&s))
    {
      breakpoint ();
      return false;
    }
  return true;
}

static void
insert_shuffled (set *s, struct val vals[], const int size,
                 const int larger_prime)
{
  /* Math magic ahead so that we iterate over every index
     eventually but in a shuffled order. Not necessarily
     randome but a repeatable sequence that makes it
     easier to debug if something goes wrong. Think
     of the prime number as a random seed, kind of. */
  int shuffled_index = larger_prime % size;
  for (int i = 0; i < size; ++i)
    {
      vals[shuffled_index].val = shuffled_index;
      assert (set_insert (s, &vals[shuffled_index].elem, val_cmp, NULL));
      assert (validate_tree (s, val_cmp));
      shuffled_index = (shuffled_index + larger_prime) % size;
    }
  const size_t catch_size = size;
  assert (catch_size == set_size (s));
}

/* Iterative inorder traversal to check the set is sorted. */
static void
inorder_fill (int vals[], size_t size, set *s)
{
  assert (set_size (s) == size);
  struct node *iter = s->root;
  struct node *inorder_pred = NULL;
  size_t sz = 0;
  while (iter != &s->end)
    {
      if (&s->end == iter->link[L])
        {
          /* This is where we climb back up a link if it exists */
          vals[sz++] = set_entry (iter, struct val, elem)->val;
          iter = iter->link[R];
          continue;
        }
      inorder_pred = iter->link[L];
      while (&s->end != inorder_pred->link[R] && iter != inorder_pred->link[R])
        inorder_pred = inorder_pred->link[R];
      if (&s->end == inorder_pred->link[R])
        {
          /* The right field is a temporary traversal helper. */
          inorder_pred->link[R] = iter;
          iter = iter->link[L];
          continue;
        }
      /* Here is our last chance to count this value. */
      vals[sz++] = set_entry (iter, struct val, elem)->val;
      /* Here is our last chance to repair our wreckage */
      inorder_pred->link[R] = &s->end;
      /* This is how we get to a right subtree if any exists. */
      iter = iter->link[R];
    }
}

static threeway_cmp
val_cmp (const set_elem *a, const set_elem *b, void *aux)
{
  (void)aux;
  struct val *lhs = set_entry (a, struct val, elem);
  struct val *rhs = set_entry (b, struct val, elem);
  return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
