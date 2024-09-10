#ifndef CCC_BUFFER_H
#define CCC_BUFFER_H

#include "impl_buffer.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    struct ccc_buf_ impl_;
} ccc_buffer;

#define CCC_BUF_INIT(mem, type, capacity, alloc_fn)                            \
    (ccc_buffer) CCC_IMPL_BUF_INIT(mem, type, capacity, alloc_fn)

ccc_result ccc_buf_realloc(ccc_buffer *, size_t new_capacity, ccc_alloc_fn *);
void *ccc_buf_base(ccc_buffer const *);
size_t ccc_buf_size(ccc_buffer const *);
size_t ccc_buf_capacity(ccc_buffer const *);
size_t ccc_buf_elem_size(ccc_buffer const *);
size_t ccc_buf_index_of(ccc_buffer const *, void const *slot);
bool ccc_buf_full(ccc_buffer const *);
bool ccc_buf_empty(ccc_buffer const *);
void *ccc_buf_at(ccc_buffer const *, size_t);
void *ccc_buf_back(ccc_buffer const *);
void *ccc_buf_front(ccc_buffer const *);
void *ccc_buf_alloc(ccc_buffer *);
ccc_result ccc_buf_pop_back(ccc_buffer *);
ccc_result ccc_buf_pop_back_n(ccc_buffer *, size_t n);
void *ccc_buf_copy(ccc_buffer *, size_t dst, size_t src);
ccc_result ccc_buf_swap(ccc_buffer *, uint8_t tmp[], size_t i, size_t j);

ccc_result ccc_buf_write(ccc_buffer *, size_t i, void const *data);
ccc_result ccc_buf_erase(ccc_buffer *, size_t i);

ccc_result ccc_buf_free(ccc_buffer *, ccc_alloc_fn *);

void *ccc_buf_begin(ccc_buffer const *);
void *ccc_buf_next(ccc_buffer const *, void const *);
void *ccc_buf_size_end(ccc_buffer const *);
void *ccc_buf_capacity_end(ccc_buffer const *);

#endif /* CCC_BUFFER_H */
