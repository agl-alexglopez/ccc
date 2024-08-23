#include "buf.h"
#include "cli.h"
#include "depqueue.h"
#include "flat_pqueue.h"
#include "pqueue.h"
#include "random.h"
#include "str_view/str_view.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int val;
    ccc_depq_elem depq_elem;
    ccc_pq_elem pq_elem;
};

size_t const step = 100000;
size_t const end_size = 1100000;
int const max_rand_range = RAND_MAX;

typedef void (*depq_perf_fn)(void);

static void test_push(void);
static void test_pop(void);
static void test_push_pop(void);
static void test_push_intermittent_pop(void);
static void test_pop_intermittent_push(void);
static void test_update(void);

static void *valid_malloc(size_t bytes);
static struct val *create_rand_vals(size_t);
static ccc_threeway_cmp val_cmp(ccc_cmp);
static void val_update(ccc_update);

#define NUM_TESTS (size_t)6
static depq_perf_fn const perf_tests[NUM_TESTS] = {test_push,
                                                   test_pop,
                                                   test_push_pop,
                                                   test_push_intermittent_pop,
                                                   test_pop_intermittent_push,
                                                   test_update};

int
main(int argc, char **argv)
{
    if (argc > 1)
    {
        str_view arg = sv(argv[1]);
        if (sv_cmp(arg, SV("push")) == SV_EQL)
        {
            test_push();
        }
        else if (sv_cmp(arg, SV("pop")) == SV_EQL)
        {
            test_pop();
        }
        else if (sv_cmp(arg, SV("push-pop")) == SV_EQL)
        {
            test_push_pop();
        }
        else if (sv_cmp(arg, SV("push-intermittent-pop")) == SV_EQL)
        {
            test_push_intermittent_pop();
        }
        else if (sv_cmp(arg, SV("pop-intermittent-push")) == SV_EQL)
        {
            test_pop_intermittent_push();
        }
        else if (sv_cmp(arg, SV("update")) == SV_EQL)
        {
            test_update();
        }
        else
        {
            quit("Unknown test request\n", 1);
        }
        return 0;
    }
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        perf_tests[i]();
    }
}

/*=======================    Test Cases     =================================*/

static void
test_push(void)
{
    printf("push N elements, priority queue vs heap priority queue:\n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n);
        ccc_depqueue depq
            = CCC_DEPQ_INIT(struct val, depq_elem, depq, val_cmp, NULL);
        ccc_pqueue pq
            = CCC_PQ_INIT(struct val, pq_elem, CCC_LES, val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].depq_elem);
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].pq_elem);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_buf buf = CCC_BUF_INIT(val_array, struct val, n, NULL);
        ccc_flat_pqueue fpq
            = CCC_FPQ_INIT(&buf, struct val, fpq_elem, CCC_LES, val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&fpq, &val_array[i]);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_pop(void)
{
    printf("pop N elements, priority queue vs heap priority queue:\n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n);
        ccc_depqueue depq
            = CCC_DEPQ_INIT(struct val, depq_elem, depq, val_cmp, NULL);
        ccc_pqueue pq
            = CCC_PQ_INIT(struct val, pq_elem, CCC_LES, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].depq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)ccc_depq_pop_min(&depq);
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_pop(&pq);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_buf buf = CCC_BUF_INIT(val_array, struct val, n, NULL);
        ccc_flat_pqueue fpq
            = CCC_FPQ_INIT(&buf, struct val, fpq_elem, CCC_LES, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&fpq, &val_array[i]);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_pop(&fpq);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_push_pop(void)
{
    printf(
        "push N elements then pop N elements, priority queue vs heap priority "
        "queue:\n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n);
        ccc_depqueue depq
            = CCC_DEPQ_INIT(struct val, depq_elem, depq, val_cmp, NULL);
        ccc_pqueue pq
            = CCC_PQ_INIT(struct val, pq_elem, CCC_LES, val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].depq_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            (void)ccc_depq_pop_min(&depq);
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].pq_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_pop(&pq);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_buf buf = CCC_BUF_INIT(val_array, struct val, n, NULL);
        ccc_flat_pqueue fpq
            = CCC_FPQ_INIT(&buf, struct val, fpq_elem, CCC_LES, val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&fpq, &val_array[i]);
        }
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_pop(&fpq);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_push_intermittent_pop(void)
{
    printf("push N elements pop every 10, priority queue vs heap priority "
           "queue:\n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n);
        ccc_depqueue depq
            = CCC_DEPQ_INIT(struct val, depq_elem, depq, val_cmp, NULL);
        ccc_pqueue pq
            = CCC_PQ_INIT(struct val, pq_elem, CCC_LES, val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].depq_elem);
            if (i % 10 == 0)
            {
                ccc_depq_pop_min(&depq);
            }
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].pq_elem);
            if (i % 10 == 0)
            {
                ccc_pq_pop(&pq);
            }
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_buf buf = CCC_BUF_INIT(val_array, struct val, n, NULL);
        ccc_flat_pqueue fpq
            = CCC_FPQ_INIT(&buf, struct val, fpq_elem, CCC_LES, val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&fpq, &val_array[i]);
            if (i % 10 == 0)
            {
                ccc_fpq_pop(&fpq);
            }
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_pop_intermittent_push(void)
{
    printf("pop N elements push every 10, priority queue vs heap priority "
           "queue:\n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n);
        ccc_depqueue depq
            = CCC_DEPQ_INIT(struct val, depq_elem, depq, val_cmp, NULL);
        ccc_pqueue pq
            = CCC_PQ_INIT(struct val, pq_elem, CCC_LES, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].depq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = ccc_depq_pop_min(&depq);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_rand_range);
                ccc_depq_push(&depq, &v->depq_elem);
            }
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = ccc_pq_pop(&pq);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_rand_range);
                ccc_pq_push(&pq, &v->pq_elem);
            }
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_buf buf = CCC_BUF_INIT(val_array, struct val, n, NULL);
        ccc_flat_pqueue fpq
            = CCC_FPQ_INIT(&buf, struct val, fpq_elem, CCC_LES, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&fpq, &val_array[i]);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)ccc_fpq_pop(&fpq);
            if (i % 10 == 0)
            {
                CCC_FPQ_EMPLACE(&fpq, struct val,
                                {.val = rand_range(0, max_rand_range)});
            }
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_update(void)
{
    printf("push N elements update N elements, priority queue vs heap priority "
           "queue:\n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n);
        ccc_depqueue depq
            = CCC_DEPQ_INIT(struct val, depq_elem, depq, val_cmp, NULL);
        ccc_pqueue pq
            = CCC_PQ_INIT(struct val, pq_elem, CCC_LES, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].depq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_rand_range);
            (void)ccc_depq_update(&depq, &val_array[i].depq_elem, val_update,
                                  &new_val);
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val
                = rand_range(0, val_array[i].val - (val_array[i].val != 0));
            ccc_pq_decrease(&pq, &val_array[i].pq_elem, val_update, &new_val);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_buf buf = CCC_BUF_INIT(val_array, struct val, n, NULL);
        ccc_flat_pqueue fpq
            = CCC_FPQ_INIT(&buf, struct val, fpq_elem, CCC_LES, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&fpq, &val_array[i]);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_rand_range);
            (void)ccc_fpq_update(&fpq, &val_array[i], val_update, &new_val);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

/*=======================  Static Helpers  =================================*/

static struct val *
create_rand_vals(size_t n)
{
    struct val *vals = valid_malloc(n * sizeof(struct val));
    while (n--)
    {
        vals[n].val = rand_range(0, max_rand_range);
    }
    return vals;
}

static void *
valid_malloc(size_t bytes)
{
    void *mem = malloc(bytes);
    if (!mem)
    {
        (void)fprintf(stderr, "heap is exhausted in perf, exiting program.\n");
        exit(1);
    }
    return mem;
}

static ccc_threeway_cmp
val_cmp(ccc_cmp const cmp)
{
    struct val const *const x = cmp.container_a;
    struct val const *const y = cmp.container_b;
    if (x->val < y->val)
    {
        return CCC_LES;
    }
    if (x->val > y->val)
    {
        return CCC_GRT;
    }
    return CCC_EQL;
}

static void
val_update(ccc_update const u)
{
    ((struct val *)u.container)->val = *((int *)u.aux);
}
