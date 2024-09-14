/** File: lru.c
The leetcode lru problem in C. */
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "doubly_linked_list.h"
#include "fhash/fhash_util.h"
#include "flat_hash_map.h"
#include "test.h"
#include "traits.h"

#include <assert.h>
#include <stdlib.h>

#define REQS 11

struct key_val
{
    int key;
    int val;
    ccc_dll_elem list_elem;
};

struct lru_cache
{
    ccc_flat_hash_map fh;
    ccc_doubly_linked_list l;
    size_t cap;
};

struct lru_lookup
{
    int key;
    struct key_val *kv_in_list;
    ccc_fh_map_elem hash_elem;
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
    union {
        void (*putter)(struct lru_cache *, int, int);
        int (*getter)(struct lru_cache *, int);
        struct key_val *(*header)(struct lru_cache *);
    };
};

static enum test_result run_lru_cache(void);
static void lru_put(struct lru_cache *, int key, int val);
static int lru_get(struct lru_cache *, int key);
static struct key_val *lru_head(struct lru_cache *);
static bool lru_lookup_cmp(ccc_key_cmp);

static ccc_threeway_cmp cmp_by_key(ccc_cmp cmp);

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

int
main()
{
    return run_lru_cache();
}

static enum test_result
run_lru_cache(void)
{
    struct lru_cache lru = {
        .cap = 3,
        .l = CCC_DLL_INIT(&lru.l, lru.l, struct key_val, list_elem, realloc,
                          cmp_by_key, NULL),
    };
    QUIET_PRINT("LRU CAPACITY -> %zu\n", lru.cap);
    FHM_INIT(&lru.fh, NULL, 0, struct lru_lookup, key, hash_elem, realloc,
             fhash_int_to_u64, lru_lookup_cmp, NULL);
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
            requests[i].putter(&lru, requests[i].key, requests[i].val);
            QUIET_PRINT("PUT -> {key: %d, val: %d}\n", requests[i].key,
                        requests[i].val);
            CHECK(fhm_validate(&lru.fh), true, "%d");
            CHECK(ccc_dll_validate(&lru.l), true, "%d");
            break;
        case GET:
            QUIET_PRINT("GET -> {key: %d, val: %d}\n", requests[i].key,
                        requests[i].val);
            CHECK(requests[i].getter(&lru, requests[i].key), requests[i].val,
                  "%d");
            CHECK(ccc_dll_validate(&lru.l), true, "%d");
            break;
        case HED:
            QUIET_PRINT("HED -> {key: %d, val: %d}\n", requests[i].key,
                        requests[i].val);
            CHECK(requests[i].header(&lru)->key, requests[i].key, "%d");
            CHECK(requests[i].header(&lru)->val, requests[i].val, "%d");
            break;
        default:
            break;
        }
    }
    fhm_clear_and_free(&lru.fh, NULL);
    ccc_dll_clear(&lru.l, NULL);
    return PASS;
}

static void
lru_put(struct lru_cache *const lru, int const key, int const val)
{
    ccc_fh_map_entry *ent = entry_vr(&lru->fh, &key);
    struct lru_lookup const *const found = unwrap(ent);
    if (found)
    {
        found->kv_in_list->key = key;
        found->kv_in_list->val = val;
        ccc_dll_splice(ccc_dll_head(&lru->l), &found->kv_in_list->list_elem);
        return;
    }
    struct lru_lookup *const new
        = FHM_INSERT_ENTRY(ent, (struct lru_lookup){.key = key});
    assert(new);
    new->kv_in_list = CCC_DLL_EMPLACE_FRONT(
        &lru->l, (struct key_val){.key = key, .val = val});
    if (size(&lru->l) > lru->cap)
    {
        struct key_val const *const to_drop = back(&lru->l);
        assert(to_drop);
        remove_entry(entry_vr(&lru->fh, &to_drop->key));
        pop_back(&lru->l);
    }
}

static int
lru_get(struct lru_cache *const lru, int const key)
{
    struct lru_lookup const *const found = get_key_val(&lru->fh, &key);
    if (!found)
    {
        return -1;
    }
    ccc_dll_splice(ccc_dll_head(&lru->l), &found->kv_in_list->list_elem);
    return found->kv_in_list->val;
}

static struct key_val *
lru_head(struct lru_cache *const lru)
{
    return ccc_dll_front(&lru->l);
}

static bool
lru_lookup_cmp(ccc_key_cmp const cmp)
{
    struct lru_lookup const *const lookup = cmp.container;
    return lookup->key == *((int *)cmp.key);
}

static ccc_threeway_cmp
cmp_by_key(ccc_cmp const cmp)
{
    struct key_val const *const kv_a = cmp.container_a;
    struct key_val const *const kv_b = cmp.container_b;
    return (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);
}
