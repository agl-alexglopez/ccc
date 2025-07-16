#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_ORDERED_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_ordered_map.h"
#include "homap_util.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(homap_test_empty)
{
    handle_ordered_map s = hom_init(&(small_fixed_map){}, struct val, id,
                                    id_cmp, NULL, NULL, SMALL_FIXED_CAP);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_copy_no_alloc)
{
    handle_ordered_map src = hom_init(&(small_fixed_map){}, struct val, id,
                                      id_cmp, NULL, NULL, SMALL_FIXED_CAP);
    handle_ordered_map dst = hom_init(&(small_fixed_map){}, struct val, id,
                                      id_cmp, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_handle(&src, &(struct val){.id = 0});
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hom_copy(&dst, &src, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct val src_v = {.id = i};
        struct val dst_v = {.id = i};
        ccc_handle src_e = ccc_remove(&src, &src_v);
        ccc_handle dst_e = ccc_remove(&dst, &dst_v);
        CHECK(occupied(&src_e), occupied(&dst_e));
        CHECK(src_v.id, dst_v.id);
        CHECK(src_v.val, dst_v.val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_copy_no_alloc_fail)
{
    handle_ordered_map src = hom_init(&(standard_fixed_map){}, struct val, id,
                                      id_cmp, NULL, NULL, STANDARD_FIXED_CAP);
    handle_ordered_map dst = hom_init(&(small_fixed_map){}, struct val, id,
                                      id_cmp, NULL, NULL, SMALL_FIXED_CAP);
    (void)swap_handle(&src, &(struct val){.id = 0});
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hom_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_copy_alloc)
{
    handle_ordered_map src
        = hom_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    handle_ordered_map dst
        = hom_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    (void)swap_handle(&src, &(struct val){.id = 0});
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hom_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct val src_v = {.id = i};
        struct val dst_v = {.id = i};
        ccc_handle src_e = ccc_remove(&src, &src_v);
        ccc_handle dst_e = ccc_remove(&dst, &dst_v);
        CHECK(occupied(&src_e), occupied(&dst_e));
        CHECK(src_v.id, dst_v.id);
        CHECK(src_v.val, dst_v.val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN({
        (void)hom_clear_and_free(&src, NULL);
        (void)hom_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(homap_test_copy_alloc_fail)
{
    handle_ordered_map src
        = hom_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    handle_ordered_map dst
        = hom_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    (void)swap_handle(&src, &(struct val){.id = 0});
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hom_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)hom_clear_and_free(&src, NULL); });
}

int
main()
{
    return CHECK_RUN(homap_test_empty(), homap_test_copy_no_alloc(),
                     homap_test_copy_no_alloc_fail(), homap_test_copy_alloc(),
                     homap_test_copy_alloc_fail());
}
