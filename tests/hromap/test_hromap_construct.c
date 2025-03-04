#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define HANDLE_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#include "alloc.h"
#include "checkers.h"
#include "handle_realtime_ordered_map.h"
#include "hromap_util.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(hromap_test_empty)
{
    handle_realtime_ordered_map s
        = hrm_init((struct val[3]){}, elem, id, id_cmp, NULL, NULL, 3);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_copy_no_alloc)
{
    handle_realtime_ordered_map src
        = hrm_init((struct val[11]){}, elem, id, id_cmp, NULL, NULL, 11);
    handle_realtime_ordered_map dst
        = hrm_init((struct val[11]){}, elem, id, id_cmp, NULL, NULL, 11);
    (void)insert(&src, &(struct val){.id = 0}.elem);
    (void)insert(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)insert(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src), 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hrm_copy(&dst, &src, NULL);
    CHECK(res, CCC_OK);
    CHECK(size(&dst), size(&src));
    for (int i = 0; i < 3; ++i)
    {
        ccc_handle src_e = remove(&src, &(struct val){.id = i}.elem);
        ccc_handle dst_e = remove(&dst, &(struct val){.id = i}.elem);
        CHECK(occupied(&src_e), occupied(&dst_e));
        struct val *src_v = hrm_at(&src, unwrap(&src_e));
        struct val *dst_v = hrm_at(&dst, unwrap(&dst_e));
        CHECK(src_v != NULL, true);
        CHECK(dst_v != NULL, true);
        CHECK(src_v->id, dst_v->id);
        CHECK(src_v->val, dst_v->val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_copy_no_alloc_fail)
{
    handle_realtime_ordered_map src
        = hrm_init((struct val[11]){}, elem, id, id_cmp, NULL, NULL, 11);
    handle_realtime_ordered_map dst
        = hrm_init((struct val[7]){}, elem, id, id_cmp, NULL, NULL, 7);
    (void)insert(&src, &(struct val){.id = 0}.elem);
    (void)insert(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)insert(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src), 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hrm_copy(&dst, &src, NULL);
    CHECK(res != CCC_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(hromap_test_copy_alloc)
{
    handle_realtime_ordered_map src
        = hrm_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    handle_realtime_ordered_map dst
        = hrm_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    (void)insert(&src, &(struct val){.id = 0}.elem);
    (void)insert(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)insert(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src), 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hrm_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_OK);
    CHECK(size(&dst), size(&src));
    for (int i = 0; i < 3; ++i)
    {
        ccc_handle src_e = remove(&src, &(struct val){.id = i}.elem);
        ccc_handle dst_e = remove(&dst, &(struct val){.id = i}.elem);
        CHECK(occupied(&src_e), occupied(&dst_e));
        struct val *src_v = hrm_at(&src, unwrap(&src_e));
        struct val *dst_v = hrm_at(&dst, unwrap(&dst_e));
        CHECK(src_v != NULL, true);
        CHECK(dst_v != NULL, true);
        CHECK(src_v->id, dst_v->id);
        CHECK(src_v->val, dst_v->val);
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
        = hrm_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    handle_realtime_ordered_map dst
        = hrm_init((struct val *)NULL, elem, id, id_cmp, std_alloc, NULL, 0);
    (void)insert(&src, &(struct val){.id = 0}.elem);
    (void)insert(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)insert(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src), 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = hrm_copy(&dst, &src, NULL);
    CHECK(res != CCC_OK, true);
    CHECK_END_FN({ (void)hrm_clear_and_free(&src, NULL); });
}

int
main()
{
    return CHECK_RUN(hromap_test_empty(), hromap_test_copy_no_alloc(),
                     hromap_test_copy_no_alloc_fail(), hromap_test_copy_alloc(),
                     hromap_test_copy_alloc_fail());
}
