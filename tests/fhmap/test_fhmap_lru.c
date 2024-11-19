/** File: lru.c
The leetcode lru problem in C. */
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC

#include "alloc.h"
#include "checkers.h"
#include "doubly_linked_list.h"
#include "fhmap/fhmap_util.h"
#include "flat_hash_map.h"
#include "traits.h"
#include "types.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define REQS 11

struct lru_cache
{
    ccc_flat_hash_map fh;
    ccc_doubly_linked_list l;
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
    ccc_fhmap_elem hash_elem;
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
    } while (0)

static bool
lru_lookup_cmp(ccc_key_cmp const cmp)
{
    struct lru_lookup const *const lookup = cmp.user_type_rhs;
    return lookup->key == *((int *)cmp.key_lhs);
}

static ccc_threeway_cmp
cmp_by_key(ccc_cmp const cmp)
{
    struct key_val const *const kv_a = cmp.user_type_lhs;
    struct key_val const *const kv_b = cmp.user_type_rhs;
    return (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);
}

static struct key_val *
lru_head(struct lru_cache *const lru)
{
    return dll_front(&lru->l);
}

CHECK_BEGIN_STATIC_FN(lru_put, struct lru_cache *const lru, int const key,
                      int const val)
{
    ccc_fhmap_entry *const ent = entry_r(&lru->fh, &key);
    if (occupied(ent))
    {
        struct lru_lookup const *const found = unwrap(ent);
        found->kv_in_list->key = key;
        found->kv_in_list->val = val;
        ccc_result r = dll_splice(&lru->l, dll_begin_elem(&lru->l), &lru->l,
                                  &found->kv_in_list->list_elem);
        CHECK(r, CCC_OK);
    }
    else
    {
        struct lru_lookup *const new
            = insert_entry(ent, &(struct lru_lookup){.key = key}.hash_elem);
        CHECK(new == NULL, false);
        new->kv_in_list = dll_emplace_front(
            &lru->l, (struct key_val){.key = key, .val = val});
        CHECK(new->kv_in_list == NULL, false);
        if (size(&lru->l) > lru->cap)
        {
            struct key_val const *const to_drop = back(&lru->l);
            CHECK(to_drop == NULL, false);
            ccc_entry const e = remove_entry(entry_r(&lru->fh, &to_drop->key));
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
        ccc_result r = dll_splice(&lru->l, dll_begin_elem(&lru->l), &lru->l,
                                  &found->kv_in_list->list_elem);
        CHECK(r, CCC_OK);
        *val = found->kv_in_list->val;
    }
    CHECK_END_FN();
}

CHECK_BEGIN_STATIC_FN(run_lru_cache)
{
    struct lru_cache lru = {
        .cap = 3,
        .l = dll_init(lru.l, struct key_val, list_elem, std_alloc, cmp_by_key,
                      NULL),
    };
    QUIET_PRINT("LRU CAPACITY -> %zu\n", lru.cap);
    fhm_init(&lru.fh, (struct lru_lookup *)NULL, 0, key, hash_elem, std_alloc,
             fhmap_int_to_u64, lru_lookup_cmp, NULL);
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
            CHECK(requests[i].putter(&lru, requests[i].key, requests[i].val),
                  PASS);
            QUIET_PRINT("PUT -> {key: %d, val: %d}\n", requests[i].key,
                        requests[i].val);
            CHECK(validate(&lru.fh), true);
            CHECK(validate(&lru.l), true);
        }
        break;
        case GET:
        {
            QUIET_PRINT("GET -> {key: %d, val: %d}\n", requests[i].key,
                        requests[i].val);
            int val = {};
            CHECK(requests[i].getter(&lru, requests[i].key, &val), PASS);
            CHECK(val, requests[i].val);
            CHECK(validate(&lru.l), true);
        }
        break;
        case HED:
        {
            QUIET_PRINT("HED -> {key: %d, val: %d}\n", requests[i].key,
                        requests[i].val);
            struct key_val const *const kv = requests[i].header(&lru);
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
        (void)ccc_fhm_clear_and_free(&lru.fh, NULL);
        (void)dll_clear(&lru.l, NULL);
    });
}

int
main()
{
    return CHECK_RUN(run_lru_cache());
}
