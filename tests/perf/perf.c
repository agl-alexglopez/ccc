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
    ccc_fpq_elem fpq_elem;
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
static ccc_depq_threeway_cmp depq_val_cmp(ccc_depq_elem const *,
                                          ccc_depq_elem const *, void *);
static ccc_fpq_threeway_cmp fpq_val_cmp(ccc_fpq_elem const *,
                                        ccc_fpq_elem const *, void *);
static ccc_pq_threeway_cmp pq_val_cmp(ccc_pq_elem const *, ccc_pq_elem const *,
                                      void *);
static void depq_update_val(ccc_depq_elem *, void *);
static void fpq_update_val(ccc_fpq_elem *, void *);
static void pq_update_val(ccc_pq_elem *, void *);

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
        ccc_depqueue depq = CCC_DEPQ_INIT(depq, depq_val_cmp, NULL);
        ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, pq_val_cmp, NULL);
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
        ccc_buf buf = CCC_BUF_INIT(val_array, sizeof(struct val), n, NULL);
        ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, fpq_elem),
                                           CCC_FPQ_LES, fpq_val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *const v = ccc_buf_alloc(&buf);
            ccc_fpq_push(&fpq, &v->fpq_elem);
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
        ccc_depqueue depq = CCC_DEPQ_INIT(depq, depq_val_cmp, NULL);
        ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, pq_val_cmp, NULL);
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
        ccc_buf buf = CCC_BUF_INIT(val_array, sizeof(struct val), n, NULL);
        ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, fpq_elem),
                                           CCC_FPQ_LES, fpq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            struct val *const v = ccc_buf_alloc(&buf);
            ccc_fpq_push(&fpq, &v->fpq_elem);
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
        ccc_depqueue depq = CCC_DEPQ_INIT(depq, depq_val_cmp, NULL);
        ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, pq_val_cmp, NULL);
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
        ccc_buf buf = CCC_BUF_INIT(val_array, sizeof(struct val), n, NULL);
        ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, fpq_elem),
                                           CCC_FPQ_LES, fpq_val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *const v = ccc_buf_alloc(&buf);
            ccc_fpq_push(&fpq, &v->fpq_elem);
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
        ccc_depqueue depq = CCC_DEPQ_INIT(depq, depq_val_cmp, NULL);
        ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, pq_val_cmp, NULL);
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
        ccc_buf buf = CCC_BUF_INIT(val_array, sizeof(struct val), n, NULL);
        ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, fpq_elem),
                                           CCC_FPQ_LES, fpq_val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *const v = ccc_buf_alloc(&buf);
            ccc_fpq_push(&fpq, &v->fpq_elem);
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
        ccc_depqueue depq = CCC_DEPQ_INIT(depq, depq_val_cmp, NULL);
        ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, pq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].depq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v
                = CCC_DEPQ_OF(struct val, depq_elem, ccc_depq_pop_min(&depq));
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
            struct val *v = CCC_PQ_OF(struct val, pq_elem, ccc_pq_pop(&pq));
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_rand_range);
                ccc_pq_push(&pq, &v->pq_elem);
            }
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_buf buf = CCC_BUF_INIT(val_array, sizeof(struct val), n, NULL);
        ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, fpq_elem),
                                           CCC_FPQ_LES, fpq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            struct val *const v = ccc_buf_alloc(&buf);
            ccc_fpq_push(&fpq, &v->fpq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = CCC_FPQ_OF(struct val, fpq_elem, ccc_fpq_pop(&fpq));
            if (i % 10 == 0)
            {
                v = ccc_buf_alloc(&buf);
                v->val = rand_range(0, max_rand_range);
                ccc_fpq_push(&fpq, &v->fpq_elem);
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
        ccc_depqueue depq = CCC_DEPQ_INIT(depq, depq_val_cmp, NULL);
        ccc_pqueue pq = CCC_PQ_INIT(CCC_PQ_LES, pq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].depq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_rand_range);
            (void)ccc_depq_update(&depq, &val_array[i].depq_elem,
                                  depq_update_val, &new_val);
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
            ccc_pq_decrease(&pq, &val_array[i].pq_elem, pq_update_val,
                            &new_val);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_buf buf = CCC_BUF_INIT(val_array, sizeof(struct val), n, NULL);
        ccc_flat_pqueue fpq = CCC_FPQ_INIT(&buf, offsetof(struct val, fpq_elem),
                                           CCC_FPQ_LES, fpq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            struct val *const v = ccc_buf_alloc(&buf);
            ccc_fpq_push(&fpq, &v->fpq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_rand_range);
            (void)ccc_fpq_update(&fpq, &val_array[i].fpq_elem, fpq_update_val,
                                 &new_val);
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

static ccc_depq_threeway_cmp
depq_val_cmp(ccc_depq_elem const *const a, ccc_depq_elem const *const b,
             void *const aux)
{
    (void)aux;
    struct val const *const x = CCC_DEPQ_OF(struct val, depq_elem, a);
    struct val const *const y = CCC_DEPQ_OF(struct val, depq_elem, b);
    if (x->val < y->val)
    {
        return CCC_DEPQ_LES;
    }
    if (x->val > y->val)
    {
        return CCC_DEPQ_GRT;
    }
    return CCC_DEPQ_EQL;
}

static ccc_fpq_threeway_cmp
fpq_val_cmp(ccc_fpq_elem const *a, ccc_fpq_elem const *b, void *const aux)
{
    (void)aux;
    struct val const *const x = CCC_FPQ_OF(struct val, fpq_elem, a);
    struct val const *const y = CCC_FPQ_OF(struct val, fpq_elem, b);
    if (x->val < y->val)
    {
        return CCC_FPQ_LES;
    }
    if (x->val > y->val)
    {
        return CCC_FPQ_GRT;
    }
    return CCC_FPQ_EQL;
}

static void
depq_update_val(ccc_depq_elem *e, void *aux)
{
    struct val *v = CCC_DEPQ_OF(struct val, depq_elem, e);
    v->val = *((int *)aux);
}

static void
fpq_update_val(ccc_fpq_elem *e, void *aux)
{
    struct val *v = CCC_FPQ_OF(struct val, fpq_elem, e);
    v->val = *((int *)aux);
}

static void
pq_update_val(ccc_pq_elem *e, void *aux)
{
    struct val *v = CCC_PQ_OF(struct val, pq_elem, e);
    v->val = *((int *)aux);
}

static ccc_pq_threeway_cmp
pq_val_cmp(ccc_pq_elem const *a, ccc_pq_elem const *const b, void *const aux)
{
    (void)aux;
    struct val const *const x = CCC_PQ_OF(struct val, pq_elem, a);
    struct val const *const y = CCC_PQ_OF(struct val, pq_elem, b);
    if (x->val < y->val)
    {
        return CCC_PQ_LES;
    }
    if (x->val > y->val)
    {
        return CCC_PQ_GRT;
    }
    return CCC_PQ_EQL;
}
