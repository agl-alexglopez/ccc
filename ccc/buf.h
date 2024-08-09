#ifndef BUF_H
#define BUF_H

#include "attrib.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum ccc_buf_result
{
    CCC_BUF_OK,
    CCC_BUF_FULL,
    CCC_BUF_ERR,
} ccc_buf_result;

typedef void *ccc_buf_realloc_fn(void *, size_t);

typedef struct ccc_buf
{
    void *mem;
    size_t elem_sz;
    size_t sz;
    size_t capacity;
    ccc_buf_realloc_fn *fn;
} ccc_buf;

#define CCC_BUF_INIT(buf, element_size, total_cap, realloc_fn)                 \
    {                                                                          \
        .mem = (buf), .elem_sz = (element_size), .sz = 0,                      \
        .capacity = (total_cap), .fn = (realloc_fn)                            \
    }

ccc_buf_result ccc_buf_init(ccc_buf[static 1], size_t elem_sz, size_t capacity,
                            ccc_buf_realloc_fn *fn) ATTRIB_NONNULL(1);
ccc_buf_result ccc_buf_realloc(ccc_buf[static 1], size_t new_capacity)
    ATTRIB_NONNULL(1);
void *ccc_buf_base(ccc_buf[static 1]) ATTRIB_NONNULL(1);
size_t ccc_buf_size(ccc_buf const[static 1]) ATTRIB_NONNULL(1);
size_t ccc_buf_capacity(ccc_buf const[static 1]) ATTRIB_NONNULL(1);
size_t ccc_buf_elem_size(ccc_buf const[static 1]) ATTRIB_NONNULL(1);
bool ccc_buf_full(ccc_buf const[static 1]) ATTRIB_NONNULL(1);
bool ccc_buf_empty(ccc_buf const[static 1]) ATTRIB_NONNULL(1);
void *ccc_buf_at(ccc_buf const[static 1], size_t) ATTRIB_NONNULL(1);
void *ccc_buf_back(ccc_buf const[static 1]) ATTRIB_NONNULL(1);
void *ccc_buf_front(ccc_buf const[static 1]) ATTRIB_NONNULL(1);
void *ccc_buf_alloc(ccc_buf[static 1]) ATTRIB_NONNULL(1);
ccc_buf_result ccc_buf_pop_back(ccc_buf[static 1]) ATTRIB_NONNULL(1);
ccc_buf_result ccc_buf_pop_back_n(ccc_buf[static 1], size_t n)
    ATTRIB_NONNULL(1);
void *ccc_buf_copy(ccc_buf[static 1], size_t dst, size_t src) ATTRIB_NONNULL(1);
ccc_buf_result ccc_buf_swap(ccc_buf[static 1], uint8_t tmp[static 1], size_t i,
                            size_t j) ATTRIB_NONNULL(1, 2);
ccc_buf_result ccc_buf_free(ccc_buf[static 1], size_t i) ATTRIB_NONNULL(1);

#endif /* BUF_H */
