/** File: lru.c
The leetcode lru problem in C. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define ADAPTIVE_MAP_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "adaptive_map.h"
#include "checkers.h"
#include "doubly_linked_list.h"
#include "traits.h"
#include "types.h"
#include "utility/allocate.h"

#define REQS 11

struct Lru_cache
{
    CCC_Adaptive_map map;
    CCC_Doubly_linked_list l;
    size_t cap;
};

/* This map is pointer stable allowing us to have the lru cache represented
   in the same struct. */
struct Lru_node
{
    CCC_Adaptive_map_node map_node;
    Doubly_linked_list_node list_node;
    int key;
    int val;
};

enum Lru_call
{
    PUT,
    GET,
    HED,
};

struct Lru_request
{
    enum Lru_call call;
    int key;
    int val;
    union
    {
        enum Check_result (*putter)(struct Lru_cache *, int, int);
        enum Check_result (*getter)(struct Lru_cache *, int, int *);
        struct Lru_node *(*header)(struct Lru_cache *);
    };
};

/* Disable me if tests start failing! */
static bool const quiet = true;
#define quiet_print(format_string...)                                          \
    do                                                                         \
    {                                                                          \
        if (!quiet)                                                            \
        {                                                                      \
            printf(format_string);                                             \
        }                                                                      \
    }                                                                          \
    while (0)

static CCC_Order
order_by_key(CCC_Key_comparator_context const order)
{
    int const key_lhs = *(int *)order.key_lhs;
    struct Lru_node const *const kv = order.type_rhs;
    return (key_lhs > kv->key) - (key_lhs < kv->key);
}

static CCC_Order
order_list_nodes(CCC_Type_comparator_context const order)
{
    struct Lru_node const *const kv_a = order.type_lhs;
    struct Lru_node const *const kv_b = order.type_rhs;
    return (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);
}

static struct Lru_node *
lru_head(struct Lru_cache *const lru)
{
    return doubly_linked_list_front(&lru->l);
}

/* This is a good opportunity to test the static initialization capabilities
   of the hash table and list. */
static struct Lru_cache lru_cache = {
    .cap = 3,
    .l = doubly_linked_list_initialize(lru_cache.l, struct Lru_node, list_node,
                                       order_list_nodes, NULL, NULL),
    .map = adaptive_map_initialize(lru_cache.map, struct Lru_node, map_node,
                                   key, order_by_key, std_allocate, NULL),
};

check_static_begin(lru_put, struct Lru_cache *const lru, int const key,
                   int const val)
{
    CCC_Adaptive_map_entry *const ent = entry_r(&lru->map, &key);
    if (occupied(ent))
    {
        struct Lru_node *const found = unwrap(ent);
        found->key = key;
        found->val = val;
        CCC_Result r = doubly_linked_list_splice(
            &lru->l, doubly_linked_list_node_begin(&lru->l), &lru->l,
            &found->list_node);
        check(r, CCC_RESULT_OK);
    }
    else
    {
        struct Lru_node *new = insert_entry(
            ent, &(struct Lru_node){.key = key, .val = val}.map_node);
        check(new == NULL, false);
        new = doubly_linked_list_push_front(&lru->l, &new->list_node);
        check(new == NULL, false);
        if (count(&lru->l).count > lru->cap)
        {
            struct Lru_node const *const to_drop = back(&lru->l);
            check(to_drop == NULL, false);
            (void)pop_back(&lru->l);
            CCC_Entry const e = remove_entry(entry_r(&lru->map, &to_drop->key));
            check(occupied(&e), true);
        }
    }
    check_end();
}

check_static_begin(lru_get, struct Lru_cache *const lru, int const key,
                   int *val)
{
    check_error(val != NULL, true);
    struct Lru_node *const found = get_key_val(&lru->map, &key);
    if (!found)
    {
        *val = -1;
    }
    else
    {
        CCC_Result r = doubly_linked_list_splice(
            &lru->l, doubly_linked_list_node_begin(&lru->l), &lru->l,
            &found->list_node);
        check(r, CCC_RESULT_OK);
        *val = found->val;
    }
    check_end();
}

check_static_begin(run_lru_cache)
{
    quiet_print("LRU CAPACITY -> %zu\n", lru_cache.cap);
    struct Lru_request requests[REQS] = {
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
                check(requests[i].putter(&lru_cache, requests[i].key,
                                         requests[i].val),
                      CHECK_PASS);
                quiet_print("PUT -> {key: %d, val: %d}\n", requests[i].key,
                            requests[i].val);
                check(validate(&lru_cache.map), true);
                check(validate(&lru_cache.l), true);
            }
            break;
            case GET:
            {
                quiet_print("GET -> {key: %d, val: %d}\n", requests[i].key,
                            requests[i].val);
                int val = 0;
                check(requests[i].getter(&lru_cache, requests[i].key, &val),
                      CHECK_PASS);
                check(val, requests[i].val);
                check(validate(&lru_cache.l), true);
            }
            break;
            case HED:
            {
                quiet_print("HED -> {key: %d, val: %d}\n", requests[i].key,
                            requests[i].val);
                struct Lru_node const *const kv
                    = requests[i].header(&lru_cache);
                check(kv != NULL, true);
                check(kv->key, requests[i].key);
                check(kv->val, requests[i].val);
            }
            break;
            default:
                break;
        }
    }
    check_end({ (void)CCC_adaptive_map_clear(&lru_cache.map, NULL); });
}

int
main()
{
    return check_run(run_lru_cache());
}
