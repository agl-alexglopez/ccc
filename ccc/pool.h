#ifndef POOL_H
#define POOL_H

#include "attrib.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum ccc_pool_result
{
    CCC_POOL_OK,
    CCC_POOL_FULL,
    CCC_POOL_ERR,
} ccc_pool_result;

typedef void *ccc_pool_realloc_fn(void *, size_t);

typedef struct ccc_pool
{
    void *mem ATTRIB_PRIVATE;
    size_t elem_sz ATTRIB_PRIVATE;
    size_t sz ATTRIB_PRIVATE;
    size_t capacity ATTRIB_PRIVATE;
    ccc_pool_realloc_fn *fn ATTRIB_PRIVATE;
} ccc_pool;

ccc_pool_result ccc_pool_init(ccc_pool[static 1], size_t elem_sz,
                              size_t capacity, ccc_pool_realloc_fn *fn)
    ATTRIB_NONNULL(1);
void *ccc_pool_base(ccc_pool[static 1]) ATTRIB_NONNULL(1);
size_t ccc_pool_size(ccc_pool const[static 1]) ATTRIB_NONNULL(1);
size_t ccc_pool_capacity(ccc_pool const[static 1]) ATTRIB_NONNULL(1);
bool ccc_pool_full(ccc_pool const[static 1]) ATTRIB_NONNULL(1);
bool ccc_pool_empty(ccc_pool const[static 1]) ATTRIB_NONNULL(1);
ccc_pool_result ccc_pool_realloc(ccc_pool[static 1], size_t new_capacity)
    ATTRIB_NONNULL(1);
void *ccc_pool_at(ccc_pool const[static 1], size_t) ATTRIB_NONNULL(1);
void *ccc_pool_alloc(ccc_pool[static 1]) ATTRIB_NONNULL(1);
ccc_pool_result ccc_pool_pop(ccc_pool[static 1]) ATTRIB_NONNULL(1);
ccc_pool_result ccc_pool_pop_n(ccc_pool[static 1], size_t n) ATTRIB_NONNULL(1);
void *ccc_pool_copy(ccc_pool[static 1], size_t dst, size_t src)
    ATTRIB_NONNULL(1);
ccc_pool_result ccc_pool_swap(ccc_pool[static 1], uint8_t tmp[static 1],
                              size_t i, size_t j) ATTRIB_NONNULL(1, 2);
ccc_pool_result ccc_pool_free(ccc_pool[static 1], size_t i) ATTRIB_NONNULL(1);

#endif /* POOL_H */
