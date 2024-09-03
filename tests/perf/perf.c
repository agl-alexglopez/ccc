#include "cli.h"
#include "depqueue.h"
#include "heap_pqueue.h"
#include "pqueue.h"
#include "random.h"
#include "str_view/str_view.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int val;
    ccc_depq_elem deccc_pq_elem;
    ccc_fpq_elem hccc_pq_elem;
    ccc_pq_elem ccc_pq_elem;
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
static ccc_deccc_pq_threeway_cmp depq_val_cmp(ccc_depq_elem const *,
                                              ccc_depq_elem const *, void *);
static ccc_fpq_threeway_cmp fpq_val_cmp(ccc_fpq_elem const *,
                                        ccc_fpq_elem const *, void *);
static ccc_pq_threeway_cmp pq_val_cmp(ccc_pq_elem const *, ccc_pq_elem const *,
                                      void *);
static void depq_update_val(ccc_depq_elem *, void *);
static void fpq_update_val(ccc_fpq_elem *, void *);
static void pq_update_val(ccc_pq_elem *, void *);
static void fpq_destroy_val(ccc_fpq_elem *);
static void pq_destroy_val(ccc_pq_elem *);

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
        ccc_flat_pqueue hpq;
        ccc_fpq_init(&hpq, HPQLES, fpq_val_cmp, NULL);
        ccc_pqueue pq = CCC_PQ_INIT(PQLES, pq_val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].deccc_pq_elem);
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&hpq, &val_array[i].hccc_pq_elem);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].ccc_pq_elem);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, HPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        ccc_fpq_clear(&hpq, fpq_destroy_val);
        ccc_pq_clear(&pq, pq_destroy_val);
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
        ccc_flat_pqueue hpq;
        ccc_pqueue pq = CCC_PQ_INIT(PQLES, pq_val_cmp, NULL);
        ccc_fpq_init(&hpq, HPQLES, fpq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].deccc_pq_elem);
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
            ccc_fpq_push(&hpq, &val_array[i].hccc_pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_pop(&hpq);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].ccc_pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_pop(&pq);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, HPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        ccc_fpq_clear(&hpq, fpq_destroy_val);
        ccc_pq_clear(&pq, pq_destroy_val);
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
        ccc_flat_pqueue hpq;
        ccc_pqueue pq = CCC_PQ_INIT(PQLES, pq_val_cmp, NULL);
        ccc_fpq_init(&hpq, HPQLES, fpq_val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].deccc_pq_elem);
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
            ccc_fpq_push(&hpq, &val_array[i].hccc_pq_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_pop(&hpq);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].ccc_pq_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_pop(&pq);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, HPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        ccc_fpq_clear(&hpq, fpq_destroy_val);
        ccc_pq_clear(&pq, pq_destroy_val);
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
        ccc_flat_pqueue hpq;
        ccc_fpq_init(&hpq, HPQLES, fpq_val_cmp, NULL);
        ccc_pqueue pq = CCC_PQ_INIT(PQLES, pq_val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].deccc_pq_elem);
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
            ccc_fpq_push(&hpq, &val_array[i].hccc_pq_elem);
            if (i % 10 == 0)
            {
                ccc_fpq_pop(&hpq);
            }
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].ccc_pq_elem);
            if (i % 10 == 0)
            {
                ccc_pq_pop(&pq);
            }
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, HPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        ccc_fpq_clear(&hpq, fpq_destroy_val);
        ccc_pq_clear(&pq, pq_destroy_val);
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
        ccc_flat_pqueue hpq;
        ccc_pqueue pq = CCC_PQ_INIT(PQLES, pq_val_cmp, NULL);
        ccc_fpq_init(&hpq, HPQLES, fpq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].deccc_pq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = CCC_DEPQ_OF(ccc_depq_pop_min(&depq), struct val,
                                        deccc_pq_elem);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_rand_range);
                ccc_depq_push(&depq, &v->deccc_pq_elem);
            }
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&hpq, &val_array[i].hccc_pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v
                = HPQ_ENTRY(ccc_fpq_pop(&hpq), struct val, hccc_pq_elem);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_rand_range);
                ccc_fpq_push(&hpq, &v->hccc_pq_elem);
            }
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].ccc_pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = CCC_PQ_OF(ccc_pq_pop(&pq), struct val, ccc_pq_elem);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_rand_range);
                ccc_pq_push(&pq, &v->ccc_pq_elem);
            }
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, HPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        ccc_fpq_clear(&hpq, fpq_destroy_val);
        ccc_pq_clear(&pq, pq_destroy_val);
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
        ccc_flat_pqueue hpq;
        ccc_pqueue pq = CCC_PQ_INIT(PQLES, pq_val_cmp, NULL);
        ccc_fpq_init(&hpq, HPQLES, fpq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            ccc_depq_push(&depq, &val_array[i].deccc_pq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_rand_range);
            (void)ccc_depq_update(&depq, &val_array[i].deccc_pq_elem,
                                  depq_update_val, &new_val);
        }
        clock_t end = clock();
        double const depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ccc_fpq_push(&hpq, &val_array[i].hccc_pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_rand_range);
            (void)ccc_fpq_update(&hpq, &val_array[i].hccc_pq_elem,
                                 fpq_update_val, &new_val);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ccc_pq_push(&pq, &val_array[i].ccc_pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val
                = rand_range(0, val_array[i].val - (val_array[i].val != 0));
            ccc_pq_decrease(&pq, &val_array[i].ccc_pq_elem, pq_update_val,
                            &new_val);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, HPQ=%f, PQ=%f\n", n, depq_time, fpq_time,
               pq_time);
        ccc_fpq_clear(&hpq, fpq_destroy_val);
        ccc_pq_clear(&pq, pq_destroy_val);
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

static ccc_deccc_pq_threeway_cmp
depq_val_cmp(ccc_depq_elem const *const a, ccc_depq_elem const *const b,
             void *const aux)
{
    (void)aux;
    struct val const *const x = CCC_DEPQ_OF(a, struct val, deccc_pq_elem);
    struct val const *const y = CCC_DEPQ_OF(b, struct val, deccc_pq_elem);
    if (x->val < y->val)
    {
        return DPQLES;
    }
    if (x->val > y->val)
    {
        return DPQGRT;
    }
    return DPQEQL;
}

static ccc_fpq_threeway_cmp
fpq_val_cmp(ccc_fpq_elem const *a, ccc_fpq_elem const *b, void *const aux)
{
    (void)aux;
    struct val const *const x = HPQ_ENTRY(a, struct val, hccc_pq_elem);
    struct val const *const y = HPQ_ENTRY(b, struct val, hccc_pq_elem);
    if (x->val < y->val)
    {
        return HPQLES;
    }
    if (x->val > y->val)
    {
        return HPQGRT;
    }
    return HPQEQL;
}

static void
depq_update_val(ccc_depq_elem *e, void *aux)
{
    struct val *v = CCC_DEPQ_OF(e, struct val, deccc_pq_elem);
    v->val = *((int *)aux);
}

static void
fpq_update_val(ccc_fpq_elem *e, void *aux)
{
    struct val *v = HPQ_ENTRY(e, struct val, hccc_pq_elem);
    v->val = *((int *)aux);
}

static void
pq_update_val(ccc_pq_elem *e, void *aux)
{
    struct val *v = CCC_PQ_OF(e, struct val, ccc_pq_elem);
    v->val = *((int *)aux);
}

static void
fpq_destroy_val(ccc_fpq_elem *e)
{
    (void)e;
}

static void
pq_destroy_val(ccc_pq_elem *e)
{
    (void)e;
}

static ccc_pq_threeway_cmp
pq_val_cmp(ccc_pq_elem const *a, ccc_pq_elem const *const b, void *const aux)
{
    (void)aux;
    struct val const *const x = CCC_PQ_OF(a, struct val, ccc_pq_elem);
    struct val const *const y = CCC_PQ_OF(b, struct val, ccc_pq_elem);
    if (x->val < y->val)
    {
        return PQLES;
    }
    if (x->val > y->val)
    {
        return PQGRT;
    }
    return PQEQL;
}
