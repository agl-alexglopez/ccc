/** File: lru.c
The leetcode lru problem in C. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "realtime_ordered_map.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

#define REQS 11

struct lru_cache
{
    CCC_Realtime_ordered_map map;
    CCC_Doubly_linked_listl;
    size_t cap;
};

/* This map is pointer stable allowing us to have the lru cache represented
   in the same struct. */
struct lru_node
{
    CCC_Realtime_ordered_map_node map_node;
    doubly_linked_list_node list_node;
    int key;
    int val;
};

enum lru_call
{
    PUT,
    GET,
    HED,
};

struct lru_request
{
    enum lru_call call;
    int key;
    int val;
    union
    {
        enum check_result (*putter)(struct lru_cache *, int, int);
        enum check_result (*getter)(struct lru_cache *, int, int *);
        struct lru_node *(*header)(struct lru_cache *);
    };
};

/* Disable me if tests start failing! */
static bool const quiet = true;
#define QUIET_PRINT(format_string...)                                          \
    do                                                                         \
    {                                                                          \
        if (!quiet)                                                            \
        {                                                                      \
            printf(format_string);                                             \
        }                                                                      \
    }                                                                          \
    while (0)

static CCC_Order
cmp_by_key(CCC_Key_comparator_context const cmp)
{
    int const key_lhs = *(int *)cmp.any_key_lhs;
    struct lru_node const *const kv = cmp.any_type_rhs;
    return (key_lhs > kv->key) - (key_lhs < kv->key);
}

static CCC_Order
cmp_list_nodes(CCC_Type_comparator_context const cmp)
{
    struct lru_node const *const kv_a = cmp.any_type_lhs;
    struct lru_node const *const kv_b = cmp.any_type_rhs;
    return (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);
}

static struct lru_node *
lru_head(struct lru_cache *const lru)
{
    return doubly_linked_list_front(&lru->l);
}

/* This is a good opportunity to test the static initialization capabilities
   of the hash table and list. */
static struct lru_cache lru_cache = {
    .cap = 3,
    .l = doubly_linked_list_initialize(lru_cache.l, struct lru_node, list_node,
                                       cmp_list_nodes, NULL, NULL),
    .map = rom_initialize(lru_cache.map, struct lru_node, map_node, key,
                          cmp_by_key, std_alloc, NULL),
};

CHECK_BEGIN_STATIC_FN(lru_put, struct lru_cache *const lru, int const key,
                      int const val)
{
    CCC_Realtime_ordered_map_entry *const ent = entry_r(&lru->map, &key);
    if (occupied(ent))
    {
        struct lru_node *const found = unwrap(ent);
        found->key = key;
        found->val = val;
        CCC_Result r = doubly_linked_list_splice(
            &lru->l, doubly_linked_list_node_begin(&lru->l), &lru->l,
            &found->list_node);
        CHECK(r, CCC_RESULT_OK);
    }
    else
    {
        struct lru_node *new = insert_entry(
            ent, &(struct lru_node){.key = key, .val = val}.map_node);
        CHECK(new == NULL, false);
        new = doubly_linked_list_push_front(&lru->l, &new->list_node);
        CHECK(new == NULL, false);
        if (count(&lru->l).count > lru->cap)
        {
            struct lru_node const *const to_drop = back(&lru->l);
            CHECK(to_drop == NULL, false);
            (void)pop_back(&lru->l);
            CCC_Entry const e = remove_entry(entry_r(&lru->map, &to_drop->key));
            CHECK(occupied(&e), true);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(lru_get, struct lru_cache *const lru, int const key,
                      int *val)
{
    CHECK_ERROR(val != NULL, true);
    struct lru_node *const found = get_key_val(&lru->map, &key);
    if (!found)
    {
        *val = -1;
    }
    else
    {
        CCC_Result r = doubly_linked_list_splice(
            &lru->l, doubly_linked_list_node_begin(&lru->l), &lru->l,
            &found->list_node);
        CHECK(r, CCC_RESULT_OK);
        *val = found->val;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(run_lru_cache)
{
    QUIET_PRINT("LRU CAPACITY -> %zu\n", lru_cache.cap);
    struct lru_request requests[REQS] = {
        {PUT, .key = 1, .val = 1, .putter = lru_put},
        {PUT, .key = 2, .val = 2, .putter = lru_put},
        {GET, .key = 1, .val = 1, .getter = lru_get},
        {PUT, .key = 3, .val = 3, .putter = lru_put},
        {HED, .key = 3, .val = 3, .header = lru_head},
        {PUT, .key = 4, .val = 4, .putter = lru_put},
        {GET, .key = 2, .val = -1, .getter = lru_get},
        {GET, .key = 3, .val = 3, .getter = lru_get},
        {GET, .key = 4, .val = 4, .getter = lru_get},
        {GET, .key = 2, .val = -1, .getter = lru_get},
        {HED, .key = 4, .val = 4, .header = lru_head},
    };
    for (size_t i = 0; i < REQS; ++i)
    {
        switch (requests[i].call)
        {
            case PUT:
            {
                CHECK(requests[i].putter(&lru_cache, requests[i].key,
                                         requests[i].val),
                      PASS);
                QUIET_PRINT("PUT -> {key: %d, val: %d}\n", requests[i].key,
                            requests[i].val);
                CHECK(validate(&lru_cache.map), true);
                CHECK(validate(&lru_cache.l), true);
            }
            break;
            case GET:
            {
                QUIET_PRINT("GET -> {key: %d, val: %d}\n", requests[i].key,
                            requests[i].val);
                int val = 0;
                CHECK(requests[i].getter(&lru_cache, requests[i].key, &val),
                      PASS);
                CHECK(val, requests[i].val);
                CHECK(validate(&lru_cache.l), true);
            }
            break;
            case HED:
            {
                QUIET_PRINT("HED -> {key: %d, val: %d}\n", requests[i].key,
                            requests[i].val);
                struct lru_node const *const kv
                    = requests[i].header(&lru_cache);
                CHECK(kv != NULL, true);
                CHECK(kv->key, requests[i].key);
                CHECK(kv->val, requests[i].val);
            }
            break;
            default:
                break;
        }
    }
    CHECK_END_FN(
        { (void)CCC_realtime_ordered_map_clear(&lru_cache.map, NULL); });
}

int
main()
{
    return CHECK_RUN(run_lru_cache());
}
