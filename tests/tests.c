#include "pqueue.h"

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
  pq_elem elem;
};

static bool pq_test_empty (void);
static bool pq_test_insert_one (void);
static bool pq_test_insert_three (void);
static bool pq_test_struct_getter (void);
static bool pq_test_insert_three_dups (void);
static bool pq_test_read_max_min (void);
static bool pq_test_insert_shuffle (void);
static bool pq_test_insert_erase_shuffled (void);
static bool pq_test_pop_max (void);
static bool pq_test_pop_min (void);
static bool pq_test_max_round_robin (void);
static bool pq_test_min_round_robin (void);
static int run_tests (void);
static void insert_shuffled (pqueue *, struct val[], int, int);
static void inorder_fill (int vals[], size_t size, pqueue *pq);
static threeway_cmp val_cmp (const pq_elem *, const pq_elem *, void *);

#define NUM_TESTS 12
const test_fn all_tests[NUM_TESTS] = {
  pq_test_empty,
  pq_test_insert_one,
  pq_test_insert_three,
  pq_test_struct_getter,
  pq_test_insert_three_dups,
  pq_test_read_max_min,
  pq_test_insert_shuffle,
  pq_test_insert_erase_shuffled,
  pq_test_pop_max,
  pq_test_pop_min,
  pq_test_max_round_robin,
  pq_test_min_round_robin,
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
  return pq_size (&pq) == 3;
}

static bool
pq_test_struct_getter (void)
{
  printf ("pq_test_getter_macro");
  pqueue pq;
  pq_init (&pq);
  pqueue pq_tester_clone;
  pq_init (&pq_tester_clone);
  struct val vals[10];
  struct val tester_clone[10];
  for (int i = 0; i < 10; ++i)
    {
      vals[i].val = i;
      tester_clone[i].val = i;
      pq_insert (&pq, &vals[i].elem, val_cmp, NULL);
      pq_insert (&pq_tester_clone, &tester_clone[i].elem, val_cmp, NULL);
      if (!validate_tree (&pq, val_cmp))
        {
          breakpoint ();
          return false;
        }
      /* Because the getter returns a pointer, if the casting returned
         misaligned data and we overwrote something we need to compare our get
         to uncorrupted data. */
      const struct val *get
          = tree_entry (&tester_clone[i].elem, struct val, elem);
      if (get->val != vals[i].val)
        {
          breakpoint ();
          return false;
        }
    }
  return pq_size (&pq) == 10;
}

static bool
pq_test_insert_three_dups (void)
{
  printf ("pq_test_insert_three_duplicates");
  pqueue pq;
  pq_init (&pq);
  struct val three_vals[3];
  for (int i = 0; i < 3; ++i)
    {
      three_vals[i].val = 0;
      pq_insert (&pq, &three_vals[i].elem, val_cmp, NULL);
      if (!validate_tree (&pq, val_cmp))
        {
          breakpoint ();
          return false;
        }
    }
  return pq_size (&pq) == 3;
}

static bool
pq_test_read_max_min (void)
{
  printf ("pq_test_read_max_min");
  pqueue pq;
  pq_init (&pq);
  struct val vals[10];
  for (int i = 0; i < 10; ++i)
    {
      vals[i].val = i;
      pq_insert (&pq, &vals[i].elem, val_cmp, NULL);
      if (!validate_tree (&pq, val_cmp))
        {
          breakpoint ();
          return false;
        }
    }
  if (10 != pq_size (&pq))
    {
      breakpoint ();
      return false;
    }
  const struct val *max = tree_entry (pq_max (&pq), struct val, elem);
  if (max->val != 9)
    {
      breakpoint ();
      return false;
    }
  const struct val *min = tree_entry (pq_min (&pq), struct val, elem);
  if (min->val != 0)
    {
      breakpoint ();
      return false;
    }
  return true;
}

static bool
pq_test_insert_shuffle (void)
{
  printf ("pq_test_insert_shuffle");
  pqueue pq;
  pq_init (&pq);
  /* Math magic ahead... */
  const int size = 50;
  const int prime = 53;
  struct val vals[size];
  insert_shuffled (&pq, vals, size, prime);
  const struct val *max = tree_entry (pq_max (&pq), struct val, elem);
  if (max->val != size - 1)
    {
      breakpoint ();
      return false;
    }
  const struct val *min = tree_entry (pq_min (&pq), struct val, elem);
  if (min->val != 0)
    {
      breakpoint ();
      return false;
    }
  int sorted_check[size];
  inorder_fill (sorted_check, size, &pq);
  for (int i = 0; i < size; ++i)
    if (vals[i].val != sorted_check[i])
      return false;
  return true;
}

static bool
pq_test_insert_erase_shuffled (void)
{
  printf ("pq_test_insert_erase_shuffle");
  pqueue pq;
  pq_init (&pq);
  const int size = 50;
  const int prime = 53;
  struct val vals[size];
  insert_shuffled (&pq, vals, size, prime);
  const struct val *max = tree_entry (pq_max (&pq), struct val, elem);
  if (max->val != size - 1)
    {
      breakpoint ();
      return false;
    }
  const struct val *min = tree_entry (pq_min (&pq), struct val, elem);
  if (min->val != 0)
    {
      breakpoint ();
      return false;
    }
  int sorted_check[size];
  inorder_fill (sorted_check, size, &pq);
  for (int i = 0; i < size; ++i)
    if (vals[i].val != sorted_check[i])
      {
        breakpoint ();
        return false;
      }

  /* Now let's delete everything with no errors. */

  for (int i = 0; i < size; ++i)
    {
      const struct val *removed = tree_entry (
          pq_erase (&pq, &vals[i].elem, val_cmp, NULL), struct val, elem);
      if (removed->val != vals[i].val)
        {
          breakpoint ();
          return false;
        }
    }
  if (!pq_empty (&pq))
    {
      breakpoint ();
      return false;
    }
  return true;
}

static bool
pq_test_pop_max (void)
{
  printf ("pq_test_pop_max");
  pqueue pq;
  pq_init (&pq);
  const int size = 50;
  const int prime = 53;
  struct val vals[size];
  insert_shuffled (&pq, vals, size, prime);
  const struct val *max = tree_entry (pq_max (&pq), struct val, elem);
  if (max->val != size - 1)
    {
      breakpoint ();
      return false;
    }
  const struct val *min = tree_entry (pq_min (&pq), struct val, elem);
  if (min->val != 0)
    {
      breakpoint ();
      return false;
    }
  int sorted_check[size];
  inorder_fill (sorted_check, size, &pq);
  for (int i = 0; i < size; ++i)
    if (vals[i].val != sorted_check[i])
      {
        breakpoint ();
        return false;
      }

  /* Now let's pop from the front of the queue until empty. */

  for (int i = size - 1; i >= 0; --i)
    {
      const struct val *front
          = tree_entry (pq_pop_max (&pq), struct val, elem);
      if (front->val != vals[i].val)
        {
          breakpoint ();
          return false;
        }
    }
  if (!pq_empty (&pq))
    {
      breakpoint ();
      return false;
    }
  return true;
}

static bool
pq_test_pop_min (void)
{
  printf ("pq_test_pop_min");
  pqueue pq;
  pq_init (&pq);
  const int size = 50;
  const int prime = 53;
  struct val vals[size];
  insert_shuffled (&pq, vals, size, prime);
  const struct val *max = tree_entry (pq_max (&pq), struct val, elem);
  if (max->val != size - 1)
    {
      breakpoint ();
      return false;
    }
  const struct val *min = tree_entry (pq_min (&pq), struct val, elem);
  if (min->val != 0)
    {
      breakpoint ();
      return false;
    }
  int sorted_check[size];
  inorder_fill (sorted_check, size, &pq);
  for (int i = 0; i < size; ++i)
    if (vals[i].val != sorted_check[i])
      {
        breakpoint ();
        return false;
      }

  /* Now let's pop from the front of the queue until empty. */

  for (int i = 0; i < size; ++i)
    {
      const struct val *front
          = tree_entry (pq_pop_min (&pq), struct val, elem);
      if (front->val != vals[i].val)
        {
          breakpoint ();
          return false;
        }
    }
  if (!pq_empty (&pq))
    {
      breakpoint ();
      return false;
    }
  return true;
}

static bool
pq_test_max_round_robin (void)
{
  printf ("pq_test_max_round_robin");
  pqueue pq;
  pq_init (&pq);
  const int size = 50;
  struct val vals[size];
  vals[0].id = 0;
  vals[0].val = 0;
  for (int i = 1; i < size; ++i)
    {
      vals[i].val = 99;
      vals[i].id = i;
      pq_insert (&pq, &vals[i].elem, val_cmp, NULL);
      assert (validate_tree (&pq, val_cmp));
    }

  /* Now let's make sure we pop round robin. */
  for (int i = 1; i < size; ++i)
    {
      const struct val *front
          = tree_entry (pq_pop_max (&pq), struct val, elem);
      if (front->id != vals[i].id)
        {
          breakpoint ();
          return false;
        }
    }
  if (!pq_empty (&pq))
    {
      breakpoint ();
      return false;
    }
  return true;
}

static bool
pq_test_min_round_robin (void)
{
  printf ("pq_test_min_round_robin");
  pqueue pq;
  pq_init (&pq);
  const int size = 50;
  struct val vals[size];
  vals[0].id = 99;
  vals[0].val = 99;
  for (int i = 1; i < size; ++i)
    {
      vals[i].val = 0;
      vals[i].id = i;
      pq_insert (&pq, &vals[i].elem, val_cmp, NULL);
      assert (validate_tree (&pq, val_cmp));
    }

  /* Now let's make sure we pop round robin. */
  for (int i = 1; i < size; ++i)
    {
      const struct val *front
          = tree_entry (pq_pop_min (&pq), struct val, elem);
      if (front->id != vals[i].id)
        {
          breakpoint ();
          return false;
        }
    }
  if (!pq_empty (&pq))
    {
      breakpoint ();
      return false;
    }
  return true;
}

static void
insert_shuffled (pqueue *pq, struct val vals[], const int size,
                 const int larger_prime)
{
  /* Math magic ahead... */
  int shuffled_index = larger_prime % size;
  for (int i = 0; i < size; ++i)
    {
      vals[shuffled_index].val = shuffled_index;
      pq_insert (pq, &vals[shuffled_index].elem, val_cmp, NULL);
      assert (validate_tree (pq, val_cmp));
      shuffled_index = (shuffled_index + larger_prime) % size;
    }
  const size_t catch_size = size;
  assert (catch_size == pq_size (pq));
}

/* Iterative inorder traversal to check the heap is sorted. */
static void
inorder_fill (int vals[], size_t size, pqueue *pq)
{
  assert (pq_size (pq) == size);
  struct node *iter = pq->root;
  struct node *inorder_pred = iter;
  size_t s = 0;
  while (iter != &pq->nil)
    {
      if (&pq->nil == iter->links[L])
        {
          /* This is where we climb back up a link if it exists */
          vals[s++] = tree_entry (iter, struct val, elem)->val;
          iter = iter->links[R];
          continue;
        }
      inorder_pred = iter->links[L];
      while (&pq->nil != inorder_pred->links[R]
             && iter != inorder_pred->links[R])
        inorder_pred = inorder_pred->links[R];
      if (&pq->nil == inorder_pred->links[R])
        {
          /* The right field is a temporary traversal helper. */
          inorder_pred->links[R] = iter;
          iter = iter->links[L];
          continue;
        }
      /* Here is our last chance to count this value. */
      vals[s++] = tree_entry (iter, struct val, elem)->val;
      /* Here is our last chance to repair our wreckage */
      inorder_pred->links[R] = &pq->nil;
      /* This is how we get to a right subtree if any exists. */
      iter = iter->links[R];
    }
}

static threeway_cmp
val_cmp (const pq_elem *a, const pq_elem *b, void *aux)
{
  (void)aux;
  struct val *lhs = tree_entry (a, struct val, elem);
  struct val *rhs = tree_entry (b, struct val, elem);
  return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}
