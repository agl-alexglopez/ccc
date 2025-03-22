#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_ORDERED_MAP_USING_NAMESPACE_CCC
#include "alloc.h"
#include "checkers.h"
#include "handle_ordered_map.h"
#include "homap_util.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(homap_test_empty)
{
    handle_ordered_map s
        = hom_init((struct val[3]){}, elem, id, id_cmp, NULL, NULL, 3);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_copy_no_alloc)
{
    handle_ordered_map src
        = hom_init((struct val[11]){}, elem, id, id_cmp, NULL, NULL, 11);
    handle_ordered_map dst
        = hom_init((struct val[11]){}, elem, id, id_cmp, NULL, NULL, 11);
    (void)swap_handle(&src, &(struct val){.id = 0}.elem);
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hom_copy(&dst, &src, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(size(&dst).count, size(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct val src_v = {.id = i};
        struct val dst_v = {.id = i};
        ccc_handle src_e = ccc_remove(&src, &src_v.elem);
        ccc_handle dst_e = ccc_remove(&dst, &dst_v.elem);
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
    handle_ordered_map src
        = hom_init((struct val[11]){}, elem, id, id_cmp, NULL, NULL, 11);
    handle_ordered_map dst
        = hom_init((struct val[7]){}, elem, id, id_cmp, NULL, NULL, 7);
    (void)swap_handle(&src, &(struct val){.id = 0}.elem);
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hom_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(homap_test_copy_alloc)
{
    handle_ordered_map src
        = hom_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    handle_ordered_map dst
        = hom_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    (void)swap_handle(&src, &(struct val){.id = 0}.elem);
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src).count, 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hom_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_RESULT_OK);
    CHECK(size(&dst).count, size(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct val src_v = {.id = i};
        struct val dst_v = {.id = i};
        ccc_handle src_e = ccc_remove(&src, &src_v.elem);
        ccc_handle dst_e = ccc_remove(&dst, &dst_v.elem);
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
        = hom_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    handle_ordered_map dst
        = hom_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    (void)swap_handle(&src, &(struct val){.id = 0}.elem);
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src).count, 3);
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
