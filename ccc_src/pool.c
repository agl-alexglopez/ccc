#include "pool.h"
#include "attrib.h"

#include <alloca.h>
#include <stdint.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

static void *at(ccc_pool const[static 1], size_t) ATTRIB_NONNULL(1);

ccc_pool_result
ccc_pool_init(ccc_pool pool[static const 1], size_t const elem_sz,
              size_t const capacity, ccc_pool_realloc_fn *const fn)
{
    pool->mem = NULL;
    pool->elem_sz = elem_sz;
    pool->capacity = capacity;
    pool->fn = fn;
    pool->sz = 0;
    if (!capacity)
    {
        return CCC_POOL_OK;
    }
    pool->mem = pool->fn(pool->mem, capacity * sizeof(elem_sz));
    if (!pool->mem)
    {
        return CCC_POOL_ERR;
    }
    return CCC_POOL_OK;
}

ccc_pool_result
ccc_pool_realloc(ccc_pool pool[static const 1], size_t const new_capacity)
{
    void *const new_mem = pool->fn(pool->mem, new_capacity);
    if (!new_mem)
    {
        return CCC_POOL_ERR;
    }
    pool->mem = new_mem;
    pool->capacity = new_capacity;
    return CCC_POOL_OK;
}

void *
ccc_pool_at(ccc_pool const pool[static 1], size_t const i)
{
    if (i >= pool->sz)
    {
        return NULL;
    }
    return ((uint8_t *)pool->mem + (i * pool->elem_sz));
}

void *
ccc_pool_alloc(ccc_pool pool[static const 1])
{
    if (pool->sz == pool->capacity)
    {
        return NULL;
    }
    return ((uint8_t *)pool->mem + (pool->elem_sz * pool->sz++));
}

ccc_pool_result
ccc_pool_swap(ccc_pool pool[static const 1], uint8_t tmp[static const 1],
              size_t const i, size_t const j)
{
    if (!pool->sz || i >= pool->sz || j >= pool->sz || j == i)
    {
        return CCC_POOL_ERR;
    }
    (void)memcpy(tmp, at(pool, i), pool->elem_sz);
    (void)memcpy(at(pool, i), at(pool, j), pool->elem_sz);
    (void)memcpy(at(pool, j), tmp, pool->elem_sz);
    return CCC_POOL_OK;
}

void *
ccc_pool_copy(ccc_pool pool[static 1], size_t const dst, size_t const src)
{
    if (!pool->sz || dst >= pool->sz || src >= pool->sz || dst == src)
    {
        return NULL;
    }
    void *const ret = at(pool, dst);
    (void)memcpy(ret, at(pool, src), pool->elem_sz);
    return ret;
}

ccc_pool_result
ccc_pool_free(ccc_pool pool[static 1], size_t const i)
{
    if (!pool->sz || i >= pool->sz)
    {
        return CCC_POOL_ERR;
    }
    if (1 == pool->sz)
    {
        pool->sz = 0;
        return CCC_POOL_OK;
    }
    (void)memcpy(at(pool, i), at(pool, pool->sz - 1), pool->elem_sz);
    pool->sz--;
    return CCC_POOL_OK;
}

ccc_pool_result
ccc_pool_pop_n(ccc_pool pool[static 1], size_t n)
{
    if (n > pool->sz)
    {
        return CCC_POOL_ERR;
    }
    pool->sz -= n;
    return CCC_POOL_OK;
}

ccc_pool_result
ccc_pool_pop(ccc_pool pool[static 1])
{
    return ccc_pool_pop_n(pool, 1);
}

size_t
ccc_pool_size(ccc_pool const pool[static const 1])
{
    return pool->sz;
}

size_t
ccc_pool_capacity(ccc_pool const pool[static const 1])
{
    return pool->capacity;
}

bool
ccc_pool_full(ccc_pool const pool[static const 1])
{
    return pool->sz == pool->capacity;
}

bool
ccc_pool_empty(ccc_pool const pool[static 1])
{
    return !pool->sz;
}

void *
ccc_pool_base(ccc_pool pool[static 1])
{
    return pool->mem;
}

static inline void *
at(ccc_pool const pool[static 1], size_t const i)
{
    return ((uint8_t *)pool->mem + (i * pool->elem_sz));
}
