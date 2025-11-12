#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC

#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "hromap_util.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

CHECK_BEGIN_STATIC_FN(hromap_test_empty)
{
    handle_realtime_ordered_map s
        = hrm_init(&(small_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   SMALL_FIXED_CAP);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_copy_no_alloc)
{
    handle_realtime_ordered_map src
        = hrm_init(&(small_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   SMALL_FIXED_CAP);
    handle_realtime_ordered_map dst
        = hrm_init(&(small_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   SMALL_FIXED_CAP);
    (void)swap_handle(&src, &(struct val){.id = 0});
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_result res = hrm_copy(&dst, &src, NULL);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct val src_v = {.id = i};
        struct val dst_v = {.id = i};
        CCC_handle src_e = CCC_remove(&src, &src_v);
        CCC_handle dst_e = CCC_remove(&dst, &dst_v);
        CHECK(occupied(&src_e), occupied(&dst_e));
        CHECK(src_v.id, dst_v.id);
        CHECK(src_v.val, dst_v.val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_copy_no_alloc_fail)
{
    handle_realtime_ordered_map src
        = hrm_init(&(standard_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   STANDARD_FIXED_CAP);
    handle_realtime_ordered_map dst
        = hrm_init(&(small_fixed_map){}, struct val, id, id_cmp, NULL, NULL,
                   SMALL_FIXED_CAP);
    (void)swap_handle(&src, &(struct val){.id = 0});
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_result res = hrm_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_copy_alloc)
{
    handle_realtime_ordered_map src
        = hrm_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    handle_realtime_ordered_map dst
        = hrm_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    (void)swap_handle(&src, &(struct val){.id = 0});
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_result res = hrm_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_RESULT_OK);
    CHECK(count(&dst).count, count(&src).count);
    for (int i = 0; i < 3; ++i)
    {
        struct val src_v = {.id = i};
        struct val dst_v = {.id = i};
        CCC_handle src_e = CCC_remove(&src, &src_v);
        CCC_handle dst_e = CCC_remove(&dst, &dst_v);
        CHECK(occupied(&src_e), occupied(&dst_e));
        CHECK(src_v.id, dst_v.id);
        CHECK(src_v.val, dst_v.val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN({
        (void)hrm_clear_and_free(&src, NULL);
        (void)hrm_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(hromap_test_copy_alloc_fail)
{
    handle_realtime_ordered_map src
        = hrm_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    handle_realtime_ordered_map dst
        = hrm_init(NULL, struct val, id, id_cmp, std_alloc, NULL, 0);
    (void)swap_handle(&src, &(struct val){.id = 0});
    (void)swap_handle(&src, &(struct val){.id = 1, .val = 1});
    (void)swap_handle(&src, &(struct val){.id = 2, .val = 2});
    CHECK(count(&src).count, 3);
    CHECK(is_empty(&dst), true);
    CCC_result res = hrm_copy(&dst, &src, NULL);
    CHECK(res != CCC_RESULT_OK, true);
    CHECK_END_FN({ (void)hrm_clear_and_free(&src, NULL); });
}

int
main()
{
    return CHECK_RUN(hromap_test_empty(), hromap_test_copy_no_alloc(),
                     hromap_test_copy_no_alloc_fail(), hromap_test_copy_alloc(),
                     hromap_test_copy_alloc_fail());
}
