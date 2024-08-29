#include "fhash_util.h"
#include "flat_hash.h"
#include "list.h"
#include "test.h"
#include "types.h"

#include <stddef.h>
#include <stdlib.h>

static enum test_result fhash_test_erase(void);
static enum test_result fhash_test_shuffle_insert_erase(void);
static enum test_result fhash_test_lru_cache(void);

#define NUM_TESTS (size_t)3
test_fn const all_tests[NUM_TESTS] = {
    fhash_test_erase,
    fhash_test_shuffle_insert_erase,
    fhash_test_lru_cache,
};

int
main()
{
    enum test_result res = PASS;
    for (size_t i = 0; i < NUM_TESTS; ++i)
    {
        bool const fail = all_tests[i]() == FAIL;
        if (fail)
        {
            res = FAIL;
        }
    }
    return res;
}

struct key_val
{
    int key;
    int val;
    ccc_list_elem list_elem;
};

struct lru_cache
{
    ccc_fhash fh;
    ccc_list l;
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
    FRONT,
};

struct lru_request
{
    enum lru_call call;
    union {
        struct
        {
            int key;
            int val;
            void (*fn)(struct lru_cache *, int, int);
        } put;
        struct
        {
            int key;
            int val;
            int (*fn)(struct lru_cache *, int);
        } get;
        struct
        {
            int key;
            int val;
            struct key_val *(*fn)(struct lru_cache *);
        } front;
    };
};

static void put(struct lru_cache *, int key, int val);
static int get(struct lru_cache *, int key);
static struct key_val *front(struct lru_cache *);
static bool lru_lookup_cmp(ccc_key_cmp);

static enum test_result
fhash_test_erase(void)
{
    struct val vals[2] = {{0}, {0}};
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, vals, 2, struct val, id, e, NULL,
                                       fhash_int_zero, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    struct val query = {.id = 137, .val = 99};
    /* Nothing was there before so nothing is in the entry. */
    ccc_fhash_entry ent = ccc_fh_insert(&fh, &query.id, &query.e);
    CHECK(ccc_fh_occupied(ent), false, "%d");
    CHECK(ccc_fh_get(ent), NULL, "%p");
    CHECK(ccc_fh_size(&fh), 1, "%zu");
    struct val *v = ccc_fh_remove(&fh, &query.id, &query.e);
    CHECK(v != NULL, true, "%d");
    CHECK(v->id, 137, "%d");
    CHECK(v->val, 99, "%d");
    CHECK(ccc_fh_size(&fh), 0, "%zu");
    int absent = 101;
    v = ccc_fh_remove(&fh, &absent, &query.e);
    CHECK(v == NULL, true, "%d");
    CHECK(ccc_fh_size(&fh), 0, "%zu");
    INSERT_ENTRY(ENTRY(&fh, 137), (struct val){.id = 137, .val = 99});
    CHECK(ccc_fh_size(&fh), 1, "%zu");
    CHECK(ccc_fh_remove_entry(ENTRY(&fh, 137)), true, "%d");
    CHECK(ccc_fh_size(&fh), 0, "%zu");
    return PASS;
}

static enum test_result
fhash_test_shuffle_insert_erase(void)
{
    ccc_fhash fh;
    ccc_result const res = CCC_FH_INIT(&fh, NULL, 0, struct val, id, e, realloc,
                                       fhash_int_to_u64, fhash_id_eq, NULL);
    CHECK(res, CCC_OK, "%d");
    int const to_insert = 100;
    int const larger_prime = (int)ccc_fh_next_prime(to_insert);
    for (int i = 0, shuffled_index = larger_prime % to_insert; i < to_insert;
         ++i, shuffled_index = (shuffled_index + larger_prime) % to_insert)
    {
        struct val elem = {.id = shuffled_index, .val = i};
        struct val *v
            = ccc_fh_insert_entry(ccc_fh_entry(&fh, &elem.id), &elem.e);
        CHECK(v->id, shuffled_index, "%d");
        CHECK(v->val, i, "%d");
        CHECK(ccc_fh_validate(&fh), true, "%d");
    }
    CHECK(ccc_fh_size(&fh), to_insert, "%zu");
    size_t cur_size = ccc_fh_size(&fh);
    int i = 0;
    while (!ccc_fh_empty(&fh) && cur_size)
    {
        CHECK(ccc_fh_contains(&fh, &i), true, "%d");
        if (i % 2)
        {
            struct val swap_slot = {.id = i};
            struct val const *const old_val
                = ccc_fh_remove(&fh, &swap_slot.id, &swap_slot.e);
            CHECK(old_val != NULL, true, "%d");
            CHECK(old_val->id, i, "%d");
        }
        else
        {
            bool const removed = ccc_fh_remove_entry(ENTRY(&fh, i));
            CHECK(removed, true, "%d");
        }
        --cur_size;
        ++i;
        CHECK(ccc_fh_size(&fh), cur_size, "%zu");
        CHECK(ccc_fh_validate(&fh), true, "%d");
    }
    CHECK(ccc_fh_size(&fh), 0, "%zu");
    CHECK(ccc_fh_clear_and_free(&fh, NULL), CCC_OK, "%d");
    return PASS;
}

static enum test_result
fhash_test_lru_cache(void)
{
    struct lru_cache lru = {
        .cap = 3,
        .l
        = CCC_L_INIT(&lru.l, lru.l, struct key_val, list_elem, realloc, NULL),
    };
    CCC_FH_INIT(&lru.fh, NULL, 0, struct lru_lookup, key, hash_elem, realloc,
                fhash_int_to_u64, lru_lookup_cmp, NULL);
    struct lru_request requests[10] = {
        {PUT, .put = {.key = 1, .val = 1, put}},
        {PUT, .put = {.key = 2, .val = 2, put}},
        {GET, .get = {.key = 1, .val = 1, get}},
        {PUT, .put = {.key = 3, .val = 3, put}},
        {FRONT, .front = {.key = 3, .val = 3, front}},
        {PUT, .put = {.key = 4, .val = 4, put}},
        {GET, .get = {.key = 2, .val = -1, get}},
        {GET, .get = {.key = 4, .val = 4, get}},
        {GET, .get = {.key = 2, .val = -1, get}},
        {FRONT, .front = {.key = 4, .val = 4, front}},
    };
    for (size_t i = 0; i < 10; ++i)
    {
        switch (requests[i].call)
        {
        case PUT:
        {
            requests[i].put.fn(&lru, requests[i].put.key, requests[i].put.val);
        }
        break;
        case GET:
        {
            CHECK(requests[i].get.fn(&lru, requests[i].put.key),
                  requests[i].get.val, "%d");
        }
        break;
        case FRONT:
        {
            struct key_val const *const kv = requests[i].front.fn(&lru);
            CHECK(kv->key, requests[i].front.key, "%d");
            CHECK(kv->val, requests[i].front.val, "%d");
        }
        break;
        }
    }
    return PASS;
}

static void
put(struct lru_cache *const lru, int key, int val)
{
    ccc_fhash_entry const ent = ENTRY(&lru->fh, key);
    struct lru_lookup const *found = GET(ent);
    if (found)
    {
        found->kv_in_list->key = key;
        found->kv_in_list->val = val;
        ccc_l_splice(&((struct key_val *)ccc_l_begin(&lru->l))->list_elem,
                     &found->kv_in_list->list_elem);
        return;
    }
    if (ccc_l_size(&lru->l) == lru->cap)
    {
        ccc_fh_remove_entry(
            ENTRY(&lru->fh, ((struct key_val *)ccc_l_back(&lru->l))->key));
        ccc_l_pop_back(&lru->l);
    }

    struct key_val *kv
        = EMPLACE_FRONT(&lru->l, (struct key_val){.key = key, .val = val});
    INSERT_ENTRY(ent, (struct lru_lookup){.key = key, .kv_in_list = kv});
}

static int
get(struct lru_cache *const lru, int key)
{
    struct lru_lookup const *found = GET(ENTRY(&lru->fh, key));
    if (!found)
    {
        return -1;
    }
    ccc_l_splice(&((struct key_val *)ccc_l_begin(&lru->l))->list_elem,
                 &found->kv_in_list->list_elem);
    return found->kv_in_list->val;
}

static struct key_val *
front(struct lru_cache *const lru)
{
    return ccc_l_front(&lru->l);
}

static bool
lru_lookup_cmp(ccc_key_cmp const cmp)
{
    struct lru_lookup const *const lookup = cmp.container;
    return lookup->key == *((int *)cmp.key);
}
