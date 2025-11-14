/** File: lru.c
The leetcode lru problem in C. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define HANDLE_BOUNDED_MAP_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "handle_bounded_map.h"
#include "traits.h"
#include "types.h"

enum : size_t
{
    LRU_CAP = 32,
    REQS = 11,
};

struct Lru_cache
{
    CCC_Handle_bounded_map map;
    CCC_Doubly_linked_list l;
    size_t cap;
};

/* This map is pointer stable allowing us to have the lru cache represented
   in the same struct. */
struct Lru_node
{
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

/* Fixed map used for the lru storage. List piggy backs of this array for its
   memory. Map does not need to re-size for this small test. */
handle_bounded_map_declare_fixed_map(lru_fixed_map, struct Lru_node, LRU_CAP);

/*===========================   Prototypes   ================================*/

static CCC_Order order_by_key(CCC_Key_comparator_context order);
static CCC_Order order_list_nodes(CCC_Type_comparator_context corder);
static struct Lru_node *lru_head(struct Lru_cache *lru);
static enum Check_result lru_put(struct Lru_cache *lru, int key, int val);
static enum Check_result lru_get(struct Lru_cache *lru, int key, int *val);
static enum Check_result run_lru_cache(void);

/*===========================   Static Data  ================================*/

/* This is a good opportunity to test the static initialization capabilities
   of the hash table and list. */
static struct Lru_cache lru_cache = {
    .map = handle_bounded_map_initialize(
        &(lru_fixed_map){}, struct Lru_node, key, order_by_key, NULL, NULL,
        handle_bounded_map_fixed_capacity(lru_fixed_map)),
    .l = doubly_linked_list_initialize(lru_cache.l, struct Lru_node, list_node,
                                       order_list_nodes, NULL, NULL),
    .cap = 3,
};

/*===========================     LRU Test   ================================*/

int
main(void)
{
    return check_run(run_lru_cache());
}

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
    check_end({ (void)CCC_handle_bounded_map_clear(&lru_cache.map, NULL); });
}

check_static_begin(lru_put, struct Lru_cache *const lru, int const key,
                   int const val)
{
    CCC_Handle_bounded_map_handle const *const ent = handle_r(&lru->map, &key);
    if (occupied(ent))
    {
        struct Lru_node *const found
            = handle_bounded_map_at(&lru->map, unwrap(ent));
        found->key = key;
        found->val = val;
        CCC_Result r = doubly_linked_list_splice(
            &lru->l, doubly_linked_list_node_begin(&lru->l), &lru->l,
            &found->list_node);
        check(r, CCC_RESULT_OK);
    }
    else
    {
        struct Lru_node *new = handle_bounded_map_at(
            &lru->map,
            insert_handle(ent, &(struct Lru_node){.key = key, .val = val}));
        check(new == NULL, false);
        new = doubly_linked_list_push_front(&lru->l, &new->list_node);
        check(new == NULL, false);
        if (count(&lru->l).count > lru->cap)
        {
            struct Lru_node const *const to_drop = back(&lru->l);
            check(to_drop == NULL, false);
            (void)pop_back(&lru->l);
            CCC_Handle const e
                = remove_handle(handle_r(&lru->map, &to_drop->key));
            check(occupied(&e), true);
        }
    }
    check_end();
}

check_static_begin(lru_get, struct Lru_cache *const lru, int const key,
                   int *val)
{
    check_error(val != NULL, true);
    struct Lru_node *const found
        = handle_bounded_map_at(&lru->map, get_key_val(&lru->map, &key));
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

static struct Lru_node *
lru_head(struct Lru_cache *const lru)
{
    return doubly_linked_list_front(&lru->l);
}

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
