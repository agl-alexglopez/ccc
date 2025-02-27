#include <stdbool.h>
#include <stddef.h>

#define TRAITS_USING_NAMESPACE_CCC
#define FLAT_ORDERED_MAP_USING_NAMESPACE_CCC
#include "alloc.h"
#include "checkers.h"
#include "flat_ordered_map.h"
#include "fomap_util.h"
#include "traits.h"
#include "types.h"

CHECK_BEGIN_STATIC_FN(fomap_test_empty)
{
    flat_ordered_map s
        = fom_init((struct val[3]){}, elem, id, NULL, id_cmp, NULL, 3);
    CHECK(is_empty(&s), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_copy_no_alloc)
{
    flat_ordered_map src
        = fom_init((struct val[11]){}, elem, id, NULL, id_cmp, NULL, 11);
    flat_ordered_map dst
        = fom_init((struct val[11]){}, elem, id, NULL, id_cmp, NULL, 11);
    (void)insert(&src, &(struct val){.id = 0}.elem);
    (void)insert(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)insert(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src), 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fom_copy(&dst, &src, NULL);
    CHECK(res, CCC_OK);
    CHECK(size(&dst), size(&src));
    for (int i = 0; i < 3; ++i)
    {
        ccc_entry src_e = remove(&src, &(struct val){.id = i}.elem);
        ccc_entry dst_e = remove(&dst, &(struct val){.id = i}.elem);
        CHECK(occupied(&src_e), occupied(&dst_e));
        struct val *src_v = unwrap(&src_e);
        struct val *dst_v = unwrap(&dst_e);
        CHECK(src_v != NULL, true);
        CHECK(dst_v != NULL, true);
        CHECK(src_v->id, dst_v->id);
        CHECK(src_v->val, dst_v->val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_copy_no_alloc_fail)
{
    flat_ordered_map src
        = fom_init((struct val[11]){}, elem, id, NULL, id_cmp, NULL, 11);
    flat_ordered_map dst
        = fom_init((struct val[7]){}, elem, id, NULL, id_cmp, NULL, 7);
    (void)insert(&src, &(struct val){.id = 0}.elem);
    (void)insert(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)insert(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src), 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fom_copy(&dst, &src, NULL);
    CHECK(res != CCC_OK, true);
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(fomap_test_copy_alloc)
{
    flat_ordered_map src
        = fom_init((struct val *)NULL, elem, id, std_alloc, id_cmp, NULL, 0);
    flat_ordered_map dst
        = fom_init((struct val *)NULL, elem, id, std_alloc, id_cmp, NULL, 0);
    (void)insert(&src, &(struct val){.id = 0}.elem);
    (void)insert(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)insert(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src), 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fom_copy(&dst, &src, std_alloc);
    CHECK(res, CCC_OK);
    CHECK(size(&dst), size(&src));
    for (int i = 0; i < 3; ++i)
    {
        ccc_entry src_e = remove(&src, &(struct val){.id = i}.elem);
        ccc_entry dst_e = remove(&dst, &(struct val){.id = i}.elem);
        CHECK(occupied(&src_e), occupied(&dst_e));
        struct val *src_v = unwrap(&src_e);
        struct val *dst_v = unwrap(&dst_e);
        CHECK(src_v != NULL, true);
        CHECK(dst_v != NULL, true);
        CHECK(src_v->id, dst_v->id);
        CHECK(src_v->val, dst_v->val);
    }
    CHECK(is_empty(&src), is_empty(&dst));
    CHECK(is_empty(&dst), true);
    CHECK_END_FN({
        (void)fom_clear_and_free(&src, NULL);
        (void)fom_clear_and_free(&dst, NULL);
    });
}

CHECK_BEGIN_STATIC_FN(fomap_test_copy_alloc_fail)
{
    flat_ordered_map src
        = fom_init((struct val *)NULL, elem, id, std_alloc, id_cmp, NULL, 0);
    flat_ordered_map dst
        = fom_init((struct val *)NULL, elem, id, std_alloc, id_cmp, NULL, 0);
    (void)insert(&src, &(struct val){.id = 0}.elem);
    (void)insert(&src, &(struct val){.id = 1, .val = 1}.elem);
    (void)insert(&src, &(struct val){.id = 2, .val = 2}.elem);
    CHECK(size(&src), 3);
    CHECK(is_empty(&dst), true);
    ccc_result res = fom_copy(&dst, &src, NULL);
    CHECK(res != CCC_OK, true);
    CHECK_END_FN({ (void)fom_clear_and_free(&src, NULL); });
}

int
main()
{
    return CHECK_RUN(fomap_test_empty(), fomap_test_copy_no_alloc(),
                     fomap_test_copy_no_alloc_fail(), fomap_test_copy_alloc(),
                     fomap_test_copy_alloc_fail());
}
