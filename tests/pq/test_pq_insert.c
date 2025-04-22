#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "pq_util.h"
#include "priority_queue.h"
#include "traits.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

CHECK_BEGIN_STATIC_FN(pq_test_insert_one)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, nullptr, nullptr);
    struct val single;
    single.val = 0;
    CHECK(push(&pq, &single.elem) != nullptr, true);
    CHECK(ccc_pq_is_empty(&pq), false);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_insert_three)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, nullptr, nullptr);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = i;
        CHECK(push(&pq, &three_vals[i].elem) != nullptr, true);
        CHECK(validate(&pq), true);
        CHECK(ccc_pq_size(&pq).count, (size_t)i + 1);
    }
    CHECK(ccc_pq_size(&pq).count, (size_t)3);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_struct_getter)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, nullptr, nullptr);
    ccc_priority_queue pq_tester_clone
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, nullptr, nullptr);
    struct val vals[10];
    struct val tester_clone[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        tester_clone[i].val = i;
        CHECK(push(&pq, &vals[i].elem) != nullptr, true);
        CHECK(push(&pq_tester_clone, &tester_clone[i].elem) != nullptr, true);
        CHECK(validate(&pq), true);
        /* Because the getter returns a pointer, if the casting returned
           misaligned data and we overwrote something we need to compare our get
           to uncorrupted data. */
        struct val const *get = &tester_clone[i];
        CHECK(get->val, vals[i].val);
    }
    CHECK(ccc_pq_size(&pq).count, (size_t)10);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_insert_three_dups)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, nullptr, nullptr);
    struct val three_vals[3];
    for (int i = 0; i < 3; ++i)
    {
        three_vals[i].val = 0;
        CHECK(push(&pq, &three_vals[i].elem) != nullptr, true);
        CHECK(validate(&pq), true);
        CHECK(ccc_pq_size(&pq).count, (size_t)i + 1);
    }
    CHECK(ccc_pq_size(&pq).count, (size_t)3);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_insert_shuffle)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, nullptr, nullptr);
    /* Math magic ahead... */
    size_t const size = 50;
    int const prime = 53;
    struct val vals[50];
    CHECK(insert_shuffled(&pq, vals, size, prime), PASS);
    struct val const *min = front(&pq);
    CHECK(min->val, 0);
    int sorted_check[50];
    CHECK(inorder_fill(sorted_check, size, &pq), PASS);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(pq_test_read_max_min)
{
    ccc_priority_queue pq
        = ccc_pq_init(struct val, elem, CCC_LES, val_cmp, nullptr, nullptr);
    struct val vals[10];
    for (int i = 0; i < 10; ++i)
    {
        vals[i].val = i;
        CHECK(push(&pq, &vals[i].elem) != nullptr, true);
        CHECK(validate(&pq), true);
        CHECK(ccc_pq_size(&pq).count, (size_t)i + 1);
    }
    CHECK(ccc_pq_size(&pq).count, (size_t)10);
    struct val const *min = front(&pq);
    CHECK(min->val, 0);
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(pq_test_insert_one(), pq_test_insert_three(),
                     pq_test_struct_getter(), pq_test_insert_three_dups(),
                     pq_test_insert_shuffle(), pq_test_read_max_min());
}
