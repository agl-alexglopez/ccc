/** File: lru.c
The leetcode lru problem in C. */
#include "doubly_linked_list.h"
#include "fhash/fhash_util.h"
#include "flat_hash_map.h"
#include "test.h"

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
    ccc_fhash_elem hash_elem;
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
        void (*put)(struct lru_cache *, int, int);
        int (*get)(struct lru_cache *, int);
        struct key_val *(*hed)(struct lru_cache *);
    };
};

static enum test_result run_lru_cache(void);
static void put(struct lru_cache *, int key, int val);
static int get(struct lru_cache *, int key);
static struct key_val *hed(struct lru_cache *);
static bool lru_lookup_cmp(ccc_key_cmp);

static ccc_threeway_cmp cmp_by_key(ccc_cmp cmp);

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
    printf("LRU CAPACITY -> %zu\n", lru.cap);
    CCC_FHM_INIT(&lru.fh, NULL, 0, struct lru_lookup, key, hash_elem, realloc,
                 fhash_int_to_u64, lru_lookup_cmp, NULL);
    struct lru_request requests[REQS] = {
        {PUT, .key = 1, .val = 1, .put = put},
        {PUT, .key = 2, .val = 2, .put = put},
        {GET, .key = 1, .val = 1, .get = get},
        {PUT, .key = 3, .val = 3, .put = put},
        {HED, .key = 3, .val = 3, .hed = hed},
        {PUT, .key = 4, .val = 4, .put = put},
        {GET, .key = 2, .val = -1, .get = get},
        {GET, .key = 3, .val = 3, .get = get},
        {GET, .key = 4, .val = 4, .get = get},
        {GET, .key = 2, .val = -1, .get = get},
        {HED, .key = 4, .val = 4, .hed = hed},
    };
    for (size_t i = 0; i < REQS; ++i)
    {
        switch (requests[i].call)
        {
        case PUT:
            requests[i].put(&lru, requests[i].key, requests[i].val);
            printf("PUT -> {key: %d, val: %d}\n", requests[i].key,
                   requests[i].val);
            CHECK(ccc_fhm_validate(&lru.fh), true, "%d");
            CHECK(ccc_dll_validate(&lru.l), true, "%d");
            break;
        case GET:
            printf("GET -> {key: %d, val: %d}\n", requests[i].key,
                   requests[i].val);
            CHECK(requests[i].get(&lru, requests[i].key), requests[i].val,
                  "%d");
            CHECK(ccc_dll_validate(&lru.l), true, "%d");
            break;
        case HED:
            printf("HED -> {key: %d, val: %d}\n", requests[i].key,
                   requests[i].val);
            CHECK(requests[i].hed(&lru)->key, requests[i].key, "%d");
            CHECK(requests[i].hed(&lru)->val, requests[i].val, "%d");
            break;
        default:
            break;
        }
    }
    ccc_fhm_clear_and_free(&lru.fh, NULL);
    ccc_dll_clear(&lru.l, NULL);
    return PASS;
}

static void
put(struct lru_cache *const lru, int key, int val)
{
    ccc_fhash_entry const ent = FHM_ENTRY(&lru->fh, key);
    struct lru_lookup const *found = FHM_UNWRAP(ent);
    if (found)
    {
        found->kv_in_list->key = key;
        found->kv_in_list->val = val;
        ccc_dll_splice(ccc_dll_head(&lru->l), &found->kv_in_list->list_elem);
        return;
    }
    FHM_INSERT_ENTRY(ent,
                     (struct lru_lookup){
                         .key = key,
                         .kv_in_list = DLL_EMPLACE_FRONT(
                             &lru->l, (struct key_val){.key = key, .val = val}),
                     });
    if (ccc_dll_size(&lru->l) > lru->cap)
    {
        struct key_val const *const to_drop = ccc_dll_back(&lru->l);
        ccc_fhm_remove_entry(FHM_ENTRY(&lru->fh, to_drop->key));
        ccc_dll_pop_back(&lru->l);
    }
}

static int
get(struct lru_cache *const lru, int key)
{
    struct lru_lookup const *found = FHM_UNWRAP(FHM_ENTRY(&lru->fh, key));
    if (!found)
    {
        return -1;
    }
    ccc_dll_splice(ccc_dll_head(&lru->l), &found->kv_in_list->list_elem);
    return found->kv_in_list->val;
}

static struct key_val *
hed(struct lru_cache *const lru)
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
