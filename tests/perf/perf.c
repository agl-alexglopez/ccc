#define TRAITS_USING_NAMESPACE_CCC

#include "cli.h"
#include "flat_priority_queue.h"
#include "ordered_multimap.h"
#include "priority_queue.h"
#include "random.h"
#include "str_view/str_view.h"
#include "traits.h"
#include "types.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct val
{
    int val;
    ccc_omm_elem omm_elem;
    ccc_pq_elem pq_elem;
};

size_t const step = 100000;
size_t const end_size = 1100000;
int const max_rand_range = RAND_MAX;

typedef void (*perf_fn)(void);

static void test_push(void);
static void test_pop(void);
static void test_push_pop(void);
static void test_push_intermittent_pop(void);
static void test_pop_intermittent_push(void);
static void test_update(void);

static void *valid_malloc(size_t bytes);
static struct val *create_rand_vals(size_t);
static ccc_threeway_cmp val_key_cmp(ccc_key_cmp);
static ccc_threeway_cmp val_cmp(ccc_cmp);
static void val_update(ccc_user_type_mut);

#define NUM_TESTS (size_t)6
static perf_fn const perf_tests[NUM_TESTS] = {test_push,
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
    printf("push N elements, pq, vs omm, vs fpq \n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n + 1);
        ccc_ordered_multimap omm = ccc_omm_init(struct val, omm_elem, val, omm,
                                                NULL, val_key_cmp, NULL);
        ccc_priority_queue pq
            = ccc_pq_init(struct val, pq_elem, CCC_LES, NULL, val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&omm, &val_array[i].omm_elem);
        }
        clock_t end = clock();
        double const omm_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&pq, &val_array[i].pq_elem);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_flat_priority_queue fpq
            = ccc_fpq_init(val_array, n + 1, CCC_LES, NULL, val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&fpq, &val_array[i]);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, omm_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_pop(void)
{
    printf("pop N elements, pq, vs omm, vs fpq \n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n + 1);
        ccc_ordered_multimap omm = ccc_omm_init(struct val, omm_elem, val, omm,
                                                NULL, val_key_cmp, NULL);
        ccc_priority_queue pq
            = ccc_pq_init(struct val, pq_elem, CCC_LES, NULL, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&omm, &val_array[i].omm_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            ccc_omm_pop_min(&omm);
        }
        clock_t end = clock();
        double const omm_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&pq, &val_array[i].pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)pop(&pq);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_flat_priority_queue fpq
            = ccc_fpq_init(val_array, n + 1, CCC_LES, NULL, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&fpq, &val_array[i]);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)pop(&fpq);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, omm_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_push_pop(void)
{
    printf("push N elements then pop N elements, pq, vs omm, vs fpq \n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n + 1);
        ccc_ordered_multimap omm = ccc_omm_init(struct val, omm_elem, val, omm,
                                                NULL, val_key_cmp, NULL);
        ccc_priority_queue pq
            = ccc_pq_init(struct val, pq_elem, CCC_LES, NULL, val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&omm, &val_array[i].omm_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            ccc_omm_pop_min(&omm);
        }
        clock_t end = clock();
        double const omm_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&pq, &val_array[i].pq_elem);
        }
        for (size_t i = 0; i < n; ++i)
        {
            (void)pop(&pq);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_flat_priority_queue fpq
            = ccc_fpq_init(val_array, n + 1, CCC_LES, NULL, val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&fpq, &val_array[i]);
        }
        for (size_t i = 0; i < n; ++i)
        {
            (void)pop(&fpq);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, omm_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_push_intermittent_pop(void)
{
    printf("push N elements pop every 10, pq, vs omm, vs fpq \n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n + 1);
        ccc_ordered_multimap omm = ccc_omm_init(struct val, omm_elem, val, omm,
                                                NULL, val_key_cmp, NULL);
        ccc_priority_queue pq
            = ccc_pq_init(struct val, pq_elem, CCC_LES, NULL, val_cmp, NULL);
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&omm, &val_array[i].omm_elem);
            if (i % 10 == 0)
            {
                ccc_omm_pop_min(&omm);
            }
        }
        clock_t end = clock();
        double const omm_time = (double)(end - begin) / CLOCKS_PER_SEC;
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&pq, &val_array[i].pq_elem);
            if (i % 10 == 0)
            {
                (void)pop(&pq);
            }
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_flat_priority_queue fpq
            = ccc_fpq_init(val_array, n + 1, CCC_LES, NULL, val_cmp, NULL);
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&fpq, &val_array[i]);
            if (i % 10 == 0)
            {
                (void)pop(&fpq);
            }
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, omm_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_pop_intermittent_push(void)
{
    printf("pop N elements push every 10, pq, vs omm, vs fpq \n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n + 1);
        ccc_ordered_multimap omm = ccc_omm_init(struct val, omm_elem, val, omm,
                                                NULL, val_key_cmp, NULL);
        ccc_priority_queue pq
            = ccc_pq_init(struct val, pq_elem, CCC_LES, NULL, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&omm, &val_array[i].omm_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = ccc_omm_min(&omm);
            ccc_omm_pop_min(&omm);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_rand_range);
                (void)push(&omm, &v->omm_elem);
            }
        }
        clock_t end = clock();
        double const omm_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&pq, &val_array[i].pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            struct val *v = front(&pq);
            (void)pop(&pq);
            if (i % 10 == 0)
            {
                v->val = rand_range(0, max_rand_range);
                (void)push(&pq, &v->pq_elem);
            }
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_flat_priority_queue fpq
            = ccc_fpq_init(val_array, n + 1, CCC_LES, NULL, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&fpq, &val_array[i]);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            (void)pop(&fpq);
            if (i % 10 == 0)
            {
                (void)ccc_fpq_emplace(
                    &fpq, (struct val){.val = rand_range(0, max_rand_range)});
            }
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, omm_time, fpq_time,
               pq_time);
        free(val_array);
    }
}

static void
test_update(void)
{
    printf("push N elements update N elements, pq, vs omm, vs fpq \n");
    for (size_t n = step; n < end_size; n += step)
    {
        struct val *val_array = create_rand_vals(n + 1);
        ccc_ordered_multimap omm = ccc_omm_init(struct val, omm_elem, val, omm,
                                                NULL, val_key_cmp, NULL);
        ccc_priority_queue pq
            = ccc_pq_init(struct val, pq_elem, CCC_LES, NULL, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&omm, &val_array[i].omm_elem);
        }
        clock_t begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_rand_range);
            (void)ccc_omm_update(&omm, &val_array[i].omm_elem, val_update,
                                 &new_val);
        }
        clock_t end = clock();
        double const omm_time = (double)(end - begin) / CLOCKS_PER_SEC;
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&pq, &val_array[i].pq_elem);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val
                = rand_range(0, val_array[i].val - (val_array[i].val != 0));
            (void)ccc_pq_decrease(&pq, &val_array[i].pq_elem, val_update,
                                  &new_val);
        }
        end = clock();
        double const pq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        ccc_flat_priority_queue fpq
            = ccc_fpq_init(val_array, n + 1, CCC_LES, NULL, val_cmp, NULL);
        for (size_t i = 0; i < n; ++i)
        {
            (void)push(&fpq, &val_array[i]);
        }
        begin = clock();
        for (size_t i = 0; i < n; ++i)
        {
            int new_val = rand_range(0, max_rand_range);
            (void)ccc_fpq_update(&fpq, &val_array[i], val_update, &new_val);
        }
        end = clock();
        double const fpq_time = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("N=%zu: DEPQ=%f, FPQ=%f, PQ=%f\n", n, omm_time, fpq_time,
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
val_key_cmp(ccc_key_cmp const cmp)
{
    struct val const *const x = cmp.user_type_rhs;
    int const key = *((int *)cmp.key_lhs);
    return (key > x->val) - (key < x->val);
}

static ccc_threeway_cmp
val_cmp(ccc_cmp const cmp)
{
    struct val const *const a = cmp.user_type_lhs;
    struct val const *const b = cmp.user_type_rhs;
    return (a->val > b->val) - (a->val < b->val);
}

static void
val_update(ccc_user_type_mut const u)
{
    ((struct val *)u.user_type)->val = *((int *)u.aux);
}
