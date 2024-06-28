#include "cli.h"
#include "heap_pqueue.h"
#include "pair_pqueue.h"
#include "pqueue.h"
#include "random.h"
#include "str_view.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int val;
    struct depq_elem depq_elem;
    struct hpq_elem hpq_elem;
    struct ppq_elem ppq_elem;
};

const size_t step = 100000;
const size_t end_size = 1100000;
const int max_range = 999999;

typedef void (*depq_perf_fn)(void);

static void test_push(void);
static void test_pop(void);
static void test_push_pop(void);
static void test_push_intermittent_pop(void);
static void test_pop_intermittent_push(void);
static void test_update(void);

static void *valid_malloc(size_t bytes);
static struct val *create_rand_vals(size_t);
static node_threeway_cmp depq_val_cmp(const struct depq_elem *,
                                      const struct depq_elem *, void *);
static enum heap_pq_threeway_cmp hpq_val_cmp(const struct hpq_elem *,
                                             const struct hpq_elem *, void *);
static enum ppq_threeway_cmp ppq_val_cmp(const struct ppq_elem *,
                                         const struct ppq_elem *, void *);
static void depq_update_val(struct depq_elem *, void *);
static void hpq_update_val(struct hpq_elem *, void *);
static void ppq_update_val(struct ppq_elem *, void *);
static void hpq_destroy_val(struct hpq_elem *);
static void ppq_destroy_val(struct ppq_elem *);

#define NUM_TESTS (size_t)6
static const depq_perf_fn perf_tests[NUM_TESTS] = {test_push,
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
        struct depqueue pq;
        struct heap_pqueue hpq;
        struct pair_pqueue ppq;
        depq_init(&pq, depq_val_cmp, NULL);
        hpq_init(&hpq, HPQLES, hpq_val_cmp, NULL);
        ppq_init(&ppq, PPQLES, ppq_val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            depq_push(&pq, &val_array[i].depq_elem);
        }
        clock_t end = clock();
        const double depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            hpq_push(&hpq, &val_array[i].hpq_elem);
        }
        end = clock();
        const double hpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ppq_push(&ppq, &val_array[i].ppq_elem);
        }
        end = clock();
        const double ppq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: PQ=%f, HPQ=%f, PPQ=%f\n", n, depq_time, hpq_time,
               ppq_time);
        hpq_clear(&hpq, hpq_destroy_val);
        ppq_clear(&ppq, ppq_destroy_val);
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
        struct depqueue pq;
        struct heap_pqueue hpq;
        struct pair_pqueue ppq;
        depq_init(&pq, depq_val_cmp, NULL);
        hpq_init(&hpq, HPQLES, hpq_val_cmp, NULL);
        ppq_init(&ppq, PPQLES, ppq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            depq_push(&pq, &val_array[i].depq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)depq_pop_min(&pq);
        }
        clock_t end = clock();
        const double depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            hpq_push(&hpq, &val_array[i].hpq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            hpq_pop(&hpq);
        }
        end = clock();
        const double hpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ppq_push(&ppq, &val_array[i].ppq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ppq_pop(&ppq);
        }
        end = clock();
        const double ppq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: PQ=%f, HPQ=%f, PPQ=%f\n", n, depq_time, hpq_time,
               ppq_time);
        hpq_clear(&hpq, hpq_destroy_val);
        ppq_clear(&ppq, ppq_destroy_val);
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
        struct depqueue pq;
        struct heap_pqueue hpq;
        struct pair_pqueue ppq;
        depq_init(&pq, depq_val_cmp, NULL);
        hpq_init(&hpq, HPQLES, hpq_val_cmp, NULL);
        ppq_init(&ppq, PPQLES, ppq_val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            depq_push(&pq, &val_array[i].depq_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            (void)depq_pop_min(&pq);
        }
        clock_t end = clock();
        const double depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            hpq_push(&hpq, &val_array[i].hpq_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            hpq_pop(&hpq);
        }
        end = clock();
        const double hpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ppq_push(&ppq, &val_array[i].ppq_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            ppq_pop(&ppq);
        }
        end = clock();
        const double ppq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: PQ=%f, HPQ=%f, PPQ=%f\n", n, depq_time, hpq_time,
               ppq_time);
        hpq_clear(&hpq, hpq_destroy_val);
        ppq_clear(&ppq, ppq_destroy_val);
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
        struct depqueue pq;
        struct heap_pqueue hpq;
        struct pair_pqueue ppq;
        depq_init(&pq, depq_val_cmp, NULL);
        hpq_init(&hpq, HPQLES, hpq_val_cmp, NULL);
        ppq_init(&ppq, PPQLES, ppq_val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            depq_push(&pq, &val_array[i].depq_elem);
            if (i % 10 == 0)
            {
                depq_pop_min(&pq);
            }
        }
        clock_t end = clock();
        const double depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            hpq_push(&hpq, &val_array[i].hpq_elem);
            if (i % 10 == 0)
            {
                hpq_pop(&hpq);
            }
        }
        end = clock();
        const double hpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ppq_push(&ppq, &val_array[i].ppq_elem);
            if (i % 10 == 0)
            {
                ppq_pop(&ppq);
            }
        }
        end = clock();
        const double ppq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: PQ=%f, HPQ=%f, PPQ=%f\n", n, depq_time, hpq_time,
               ppq_time);
        hpq_clear(&hpq, hpq_destroy_val);
        ppq_clear(&ppq, ppq_destroy_val);
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
        struct depqueue pq;
        struct heap_pqueue hpq;
        struct pair_pqueue ppq;
        depq_init(&pq, depq_val_cmp, NULL);
        hpq_init(&hpq, HPQLES, hpq_val_cmp, NULL);
        ppq_init(&ppq, PPQLES, ppq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            depq_push(&pq, &val_array[i].depq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v
                = depq_entry(depq_pop_min(&pq), struct val, depq_elem);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_range);
                depq_push(&pq, &v->depq_elem);
            }
        }
        clock_t end = clock();
        const double depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            hpq_push(&hpq, &val_array[i].hpq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = hpq_entry(hpq_pop(&hpq), struct val, hpq_elem);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_range);
                hpq_push(&hpq, &v->hpq_elem);
            }
        }
        end = clock();
        const double hpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ppq_push(&ppq, &val_array[i].ppq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = ppq_entry(ppq_pop(&ppq), struct val, ppq_elem);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_range);
                ppq_push(&ppq, &v->ppq_elem);
            }
        }
        end = clock();
        const double ppq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: PQ=%f, HPQ=%f, PPQ=%f\n", n, depq_time, hpq_time,
               ppq_time);
        hpq_clear(&hpq, hpq_destroy_val);
        ppq_clear(&ppq, ppq_destroy_val);
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
        struct depqueue pq;
        struct heap_pqueue hpq;
        struct pair_pqueue ppq;
        depq_init(&pq, depq_val_cmp, NULL);
        hpq_init(&hpq, HPQLES, hpq_val_cmp, NULL);
        ppq_init(&ppq, PPQLES, ppq_val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            depq_push(&pq, &val_array[i].depq_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_range);
            (void)depq_update(&pq, &val_array[i].depq_elem, depq_update_val,
                              &new_val);
        }
        clock_t end = clock();
        const double depq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            hpq_push(&hpq, &val_array[i].hpq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_range);
            (void)hpq_update(&hpq, &val_array[i].hpq_elem, hpq_update_val,
                             &new_val);
        }
        end = clock();
        const double hpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            ppq_push(&ppq, &val_array[i].ppq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val
                = rand_range(0, val_array[i].val - (val_array[i].val != 0));
            ppq_decrease(&ppq, &val_array[i].ppq_elem, ppq_update_val,
                         &new_val);
        }
        end = clock();
        const double ppq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: PQ=%f, HPQ=%f, PPQ=%f\n", n, depq_time, hpq_time,
               ppq_time);
        hpq_clear(&hpq, hpq_destroy_val);
        ppq_clear(&ppq, ppq_destroy_val);
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
        vals[n].val = rand_range(0, max_range);
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

static node_threeway_cmp
depq_val_cmp(const struct depq_elem *const a, const struct depq_elem *const b,
             void *const aux)
{
    (void)aux;
    const struct val *const x = depq_entry(a, struct val, depq_elem);
    const struct val *const y = depq_entry(b, struct val, depq_elem);
    if (x->val < y->val)
    {
        return NODE_LES;
    }
    if (x->val > y->val)
    {
        return NODE_GRT;
    }
    return NODE_EQL;
}

static enum heap_pq_threeway_cmp
hpq_val_cmp(const struct hpq_elem *a, const struct hpq_elem *b, void *const aux)
{
    (void)aux;
    const struct val *const x = hpq_entry(a, struct val, hpq_elem);
    const struct val *const y = hpq_entry(b, struct val, hpq_elem);
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
depq_update_val(struct depq_elem *e, void *aux)
{
    struct val *v = depq_entry(e, struct val, depq_elem);
    v->val = *((int *)aux);
}

static void
hpq_update_val(struct hpq_elem *e, void *aux)
{
    struct val *v = hpq_entry(e, struct val, hpq_elem);
    v->val = *((int *)aux);
}

static void
ppq_update_val(struct ppq_elem *e, void *aux)
{
    struct val *v = ppq_entry(e, struct val, ppq_elem);
    v->val = *((int *)aux);
}

static void
hpq_destroy_val(struct hpq_elem *e)
{
    (void)e;
}

static void
ppq_destroy_val(struct ppq_elem *e)
{
    (void)e;
}

static enum ppq_threeway_cmp
ppq_val_cmp(const struct ppq_elem *a, const struct ppq_elem *const b,
            void *const aux)
{
    (void)aux;
    const struct val *const x = ppq_entry(a, struct val, ppq_elem);
    const struct val *const y = ppq_entry(b, struct val, ppq_elem);
    if (x->val < y->val)
    {
        return PPQLES;
    }
    if (x->val > y->val)
    {
        return PPQGRT;
    }
    return PPQEQL;
}
