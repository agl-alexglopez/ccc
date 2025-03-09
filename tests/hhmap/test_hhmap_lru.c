/** File: lru.c
The leetcode lru problem in C. */
#define HANDLE_HASH_MAP_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "checkers.h"
#include "doubly_linked_list.h"
#include "handle_hash_map.h"
#include "hhmap/hhmap_util.h"
#include "traits.h"
#include "types.h"

#define REQS 11

struct lru_cache
{
    ccc_handle_hash_map hh;
    ccc_doubly_linked_list l;
    size_t cap;
};

struct lru_elem
{
    ccc_hhmap_elem hash_elem;
    dll_elem list_elem;
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
        struct lru_elem *(*header)(struct lru_cache *);
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
    } while (0)

static ccc_tribool
lru_elem_cmp(ccc_key_cmp const cmp)
{
    struct lru_elem const *const lookup = cmp.user_type_rhs;
    return lookup->key == *((int *)cmp.key_lhs);
}

static ccc_threeway_cmp
cmp_by_key(ccc_cmp const cmp)
{
    struct lru_elem const *const kv_a = cmp.user_type_lhs;
    struct lru_elem const *const kv_b = cmp.user_type_rhs;
    return (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);
}

static struct lru_elem *
lru_head(struct lru_cache *const lru)
{
    return dll_front(&lru->l);
}

#define CAP 3
#define PRIME_HASH_SIZE 11
/* This should have used the new c23 lifetime initialized compound literals
   as a static array of structs directly in the initializer like this:
   (static struct lru_elem)[PRIME_HASH_SIZE]{}, but the github workflow
   compilers don't support this syntax yet. */
static struct lru_elem map_buf[PRIME_HASH_SIZE];
static_assert(PRIME_HASH_SIZE > CAP);

/* This is a good opportunity to test the static initialization capabilities
   of the hash table and list. */
static struct lru_cache lru_cache = {
    .cap = CAP,
    .l
    = dll_init(lru_cache.l, struct lru_elem, list_elem, cmp_by_key, NULL, NULL),
    .hh = hhm_init(map_buf, hash_elem, key, hhmap_int_to_u64, lru_elem_cmp,
                   NULL, NULL, sizeof(map_buf) / sizeof(map_buf[0])),
};

CHECK_BEGIN_STATIC_FN(lru_put, struct lru_cache *const lru, int const key,
                      int const val)
{
    ccc_hhmap_handle *const ent = handle_r(&lru->hh, &key);
    if (occupied(ent))
    {
        struct lru_elem *const found = hhm_at(&lru->hh, unwrap(ent));
        found->key = key;
        found->val = val;
        ccc_result r = dll_splice(&lru->l, dll_begin_elem(&lru->l), &lru->l,
                                  &found->list_elem);
        CHECK(r, CCC_OK);
    }
    else
    {
        struct lru_elem *const new = hhm_at(
            &lru->hh,
            insert_handle(ent, &(struct lru_elem){.key = key}.hash_elem));
        CHECK(new == NULL, false);
        struct lru_elem *l_elem = dll_push_front(&lru->l, &new->list_elem);
        CHECK(l_elem, new);

        new->val = val;
        if (size(&lru->l) > lru->cap)
        {
            struct lru_elem const *const to_drop = back(&lru->l);
            CHECK(to_drop == NULL, false);
            (void)pop_back(&lru->l);
            ccc_handle const e
                = remove_handle(handle_r(&lru->hh, &to_drop->key));
            CHECK(occupied(&e), true);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(lru_get, struct lru_cache *const lru, int const key,
                      int *val)
{
    CHECK_ERROR(val != NULL, true);
    struct lru_elem *const found
        = hhm_at(&lru->hh, get_key_val(&lru->hh, &key));
    if (!found)
    {
        *val = -1;
    }
    else
    {
        ccc_result r = dll_splice(&lru->l, dll_begin_elem(&lru->l), &lru->l,
                                  &found->list_elem);
        CHECK(r, CCC_OK);
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
            CHECK(validate(&lru_cache.hh), true);
            CHECK(validate(&lru_cache.l), true);
        }
        break;
        case GET:
        {
            QUIET_PRINT("GET -> {key: %d, val: %d}\n", requests[i].key,
                        requests[i].val);
            int val = 0;
            CHECK(requests[i].getter(&lru_cache, requests[i].key, &val), PASS);
            CHECK(val, requests[i].val);
            CHECK(validate(&lru_cache.l), true);
        }
        break;
        case HED:
        {
            QUIET_PRINT("HED -> {key: %d, val: %d}\n", requests[i].key,
                        requests[i].val);
            struct lru_elem const *const kv = requests[i].header(&lru_cache);
            CHECK(kv != NULL, true);
            CHECK(kv->key, requests[i].key);
            CHECK(kv->val, requests[i].val);
        }
        break;
        default:
            break;
        }
    }
    CHECK_END_FN();
}

int
main()
{
    return CHECK_RUN(run_lru_cache());
}
