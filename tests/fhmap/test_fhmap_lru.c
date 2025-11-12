/** File: lru.c
The leetcode lru problem in C. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "checkers.h"
#include "doubly_linked_list.h"
#include "fhmap/fhmap_util.h"
#include "flat_hash_map.h"
#include "traits.h"
#include "types.h"
#include "util/alloc.h"

#define REQS 11

struct lru_cache
{
    CCC_flat_hash_map fh;
    CCC_doubly_linked_list l;
    size_t cap;
};

struct key_val
{
    int key;
    int val;
    dll_elem list_elem;
};

struct lru_lookup
{
    int key;
    struct key_val *kv_in_list;
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
        struct key_val *(*header)(struct lru_cache *);
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

static CCC_threeway_cmp
lru_lookup_cmp(CCC_any_key_cmp const cmp)
{
    struct lru_lookup const *const rhs = cmp.any_type_rhs;
    int const lhs = *((int *)cmp.any_key_lhs);
    return (lhs > rhs->key) - (lhs < rhs->key);
}

static CCC_threeway_cmp
cmp_by_key(CCC_any_type_cmp const cmp)
{
    struct key_val const *const kv_a = cmp.any_type_lhs;
    struct key_val const *const kv_b = cmp.any_type_rhs;
    return (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);
}

static struct key_val *
lru_head(struct lru_cache *const lru)
{
    return dll_front(&lru->l);
}

enum : size_t
{
    CAP = 3,
};

static_assert(CAP * 1UL < SMALL_FIXED_CAP * 1UL);

/* This is a good opportunity to test the static initialization capabilities
   of the hash table and list. */
static struct lru_cache lru_cache = {
    .cap = CAP,
    .l = dll_init(lru_cache.l, struct key_val, list_elem, cmp_by_key, std_alloc,
                  NULL),
    .fh = fhm_init(&(small_fixed_map){}, struct val, key, fhmap_int_to_u64,
                   lru_lookup_cmp, NULL, NULL, SMALL_FIXED_CAP),
};

CHECK_BEGIN_STATIC_FN(lru_put, struct lru_cache *const lru, int const key,
                      int const val)
{
    CCC_fhmap_entry *const ent = entry_r(&lru->fh, &key);
    if (occupied(ent))
    {
        struct lru_lookup const *const found = unwrap(ent);
        found->kv_in_list->key = key;
        found->kv_in_list->val = val;
        CCC_result r = dll_splice(&lru->l, dll_begin_elem(&lru->l), &lru->l,
                                  &found->kv_in_list->list_elem);
        CHECK(r, CCC_RESULT_OK);
    }
    else
    {
        struct lru_lookup *const new
            = insert_entry(ent, &(struct lru_lookup){.key = key});
        CHECK(new == NULL, false);
        new->kv_in_list = dll_emplace_front(
            &lru->l, (struct key_val){.key = key, .val = val});
        CHECK(new->kv_in_list == NULL, false);
        if (count(&lru->l).count > lru->cap)
        {
            struct key_val const *const to_drop = back(&lru->l);
            CHECK(to_drop == NULL, false);
            CCC_entry const e = remove_entry(entry_r(&lru->fh, &to_drop->key));
            CHECK(occupied(&e), true);
            (void)pop_back(&lru->l);
        }
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(lru_get, struct lru_cache *const lru, int const key,
                      int *val)
{
    CHECK_ERROR(val != NULL, true);
    struct lru_lookup const *const found = get_key_val(&lru->fh, &key);
    if (!found)
    {
        *val = -1;
    }
    else
    {
        CCC_result r = dll_splice(&lru->l, dll_begin_elem(&lru->l), &lru->l,
                                  &found->kv_in_list->list_elem);
        CHECK(r, CCC_RESULT_OK);
        *val = found->kv_in_list->val;
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
                CHECK(validate(&lru_cache.fh), true);
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
                struct key_val const *const kv = requests[i].header(&lru_cache);
                CHECK(kv != NULL, true);
                CHECK(kv->key, requests[i].key);
                CHECK(kv->val, requests[i].val);
            }
            break;
            default:
                break;
        }
    }
    CHECK_END_FN({
        (void)CCC_fhm_clear_and_free(&lru_cache.fh, NULL);
        (void)dll_clear(&lru_cache.l, NULL);
    });
}

int
main()
{
    return CHECK_RUN(run_lru_cache());
}
