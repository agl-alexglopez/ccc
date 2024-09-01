#include "map.h"
#include "map_util.h"
#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static enum test_result map_test_insert_one(void);
static enum test_result map_test_insert_three(void);
static enum test_result map_test_struct_getter(void);
static enum test_result map_test_insert_shuffle(void);

#define NUM_TESTS ((size_t)4)
test_fn const all_tests[NUM_TESTS] = {
    map_test_insert_one,
    map_test_insert_three,
    map_test_struct_getter,
    map_test_insert_shuffle,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        bool const fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

static enum test_result
map_test_insert_one(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    struct val single;
    single.val = 0;
    CHECK(ccc_m_insert_entry(ccc_m_entry(&s, &single.val), &single.elem)
              != NULL,
          true, "%d");
    CHECK(ccc_m_empty(&s), false, "%d");
    CHECK(((struct val *)ccc_m_root(&s))->val == single.val, true, "%d");
    return PASS;
}

static enum test_result
map_test_insert_three(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        CHECK(ccc_m_unwrap(ccc_m_insert(&s, &three_vals[i].elem)) == NULL, true,
              "%d");
        CHECK(ccc_m_validate(&s), true, "%d");
    }
    CHECK(ccc_m_size(&s), (size_t)3, "%zu");
    return PASS;
}

static enum test_result
map_test_struct_getter(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    ccc_map map_tester_clone = CCC_M_INIT(
        struct val, elem, val, map_tester_clone, NULL, val_cmp, NULL);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        CHECK(ccc_m_insert_entry(ccc_m_entry(&s, &vals[i].val), &vals[i].elem)
                  != NULL,
              true, "%d");
        CHECK(ccc_m_insert_entry(
                  ccc_m_entry(&map_tester_clone, &tester_clone[i].val),
                  &tester_clone[i].elem)
                  != NULL,
              true, "%d");
        CHECK(ccc_m_validate(&s), true, "%d");
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our
           get to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val, "%d");
    }
    CHECK(ccc_m_size(&s), (size_t)10, "%zu");
    return PASS;
}

static enum test_result
map_test_insert_shuffle(void)
{
    ccc_map s = CCC_M_INIT(struct val, elem, val, s, NULL, val_cmp, NULL);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[size];
    CHECK(insert_shuffled(&s, vals, size, prime), PASS, "%d");
    int sorted_check[size];
    CHECK(inorder_fill(sorted_check, size, &s), size, "%zu");
    for (size_t i = 0; i < size; ++i)
    {
        CHECK(vals[i].val, sorted_check[i], "%d");
    }
    return PASS;
}
