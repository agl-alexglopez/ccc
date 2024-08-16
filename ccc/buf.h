#ifndef BUF_H
#define BUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    CCC_BUF_OK,
    CCC_BUF_FULL,
    CCC_BUF_ERR,
} ccc_buf_result;

typedef void *ccc_buf_realloc_fn(void *, size_t);

typedef struct
{
    void *mem;
    size_t const elem_sz;
    size_t sz;
    size_t capacity;
    ccc_buf_realloc_fn *realloc_fn;
} ccc_buf;

#define CCC_BUF_INIT(mem, type, capacity, realloc_fn)                          \
    {                                                                          \
        (mem), sizeof(type), 0, (capacity), (realloc_fn)                       \
    }

ccc_buf_result ccc_buf_realloc(ccc_buf *, size_t new_capacity,
                               ccc_buf_realloc_fn *);
void *ccc_buf_base(ccc_buf *);
size_t ccc_buf_size(ccc_buf const *);
size_t ccc_buf_capacity(ccc_buf const *);
size_t ccc_buf_elem_size(ccc_buf const *);
bool ccc_buf_full(ccc_buf const *);
bool ccc_buf_empty(ccc_buf const *);
void *ccc_buf_at(ccc_buf const *, size_t);
void *ccc_buf_back(ccc_buf const *);
void *ccc_buf_front(ccc_buf const *);
void *ccc_buf_alloc(ccc_buf *);
ccc_buf_result ccc_buf_pop_back(ccc_buf *);
ccc_buf_result ccc_buf_pop_back_n(ccc_buf *, size_t n);
void *ccc_buf_copy(ccc_buf *, size_t dst, size_t src);
ccc_buf_result ccc_buf_swap(ccc_buf *, uint8_t tmp[], size_t i, size_t j);
ccc_buf_result ccc_buf_erase(ccc_buf *, size_t i);

#endif /* BUF_H */
