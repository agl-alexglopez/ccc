#ifndef CCC_BUF_H
#define CCC_BUF_H

#include "impl_buf.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    struct ccc_impl_buf impl;
} ccc_buf;

#define CCC_BUF_INIT(mem, type, capacity, realloc_fn)                          \
    CCC_IMPL_BUF_INIT(mem, type, capacity, realloc_fn)

#define CCC_BUF_EMPLACE(ccc_buf_ptr, index, struct_name,                       \
                        struct_initializer...)                                 \
    CCC_IMPL_BUF_EMPLACE(ccc_buf_ptr, index, struct_name, struct_initializer)

ccc_result ccc_buf_realloc(ccc_buf *, size_t new_capacity, ccc_realloc_fn *);
void *ccc_buf_base(ccc_buf const *);
size_t ccc_buf_size(ccc_buf const *);
size_t ccc_buf_capacity(ccc_buf const *);
size_t ccc_buf_elem_size(ccc_buf const *);
size_t ccc_buf_index_of(ccc_buf const *, void const *slot);
bool ccc_buf_full(ccc_buf const *);
bool ccc_buf_empty(ccc_buf const *);
void *ccc_buf_at(ccc_buf const *, size_t);
void *ccc_buf_back(ccc_buf const *);
void *ccc_buf_front(ccc_buf const *);
void *ccc_buf_alloc(ccc_buf *);
ccc_result ccc_buf_pop_back(ccc_buf *);
ccc_result ccc_buf_pop_back_n(ccc_buf *, size_t n);
void *ccc_buf_copy(ccc_buf *, size_t dst, size_t src);
ccc_result ccc_buf_swap(ccc_buf *, uint8_t tmp[], size_t i, size_t j);

ccc_result ccc_buf_write(ccc_buf *, size_t i, void const *data);
ccc_result ccc_buf_erase(ccc_buf *, size_t i);

ccc_result ccc_buf_free(ccc_buf *, ccc_realloc_fn *);

void *ccc_buf_begin(ccc_buf const *);
void *ccc_buf_next(ccc_buf const *, void const *);
void *ccc_buf_size_end(ccc_buf const *);
void *ccc_buf_capacity_end(ccc_buf const *);

#endif /* CCC_BUF_H */
