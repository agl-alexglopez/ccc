#ifndef CCC_BUFFER_H
#define CCC_BUFFER_H

#include "impl_buffer.h"
#include "types.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct ccc_buf_ ccc_buffer;

#define ccc_buf_init(mem_ptr, capacity, alloc_fn)                              \
    ccc_impl_buf_init(mem_ptr, capacity, alloc_fn)

ccc_result ccc_buf_alloc(ccc_buffer *, size_t capacity, ccc_alloc_fn *);
void *ccc_buf_base(ccc_buffer const *);
size_t ccc_buf_size(ccc_buffer const *);

void ccc_buf_size_plus(ccc_buffer *, size_t n);
void ccc_buf_size_minus(ccc_buffer *, size_t n);
void ccc_buf_size_set(ccc_buffer *, size_t n);

size_t ccc_buf_capacity(ccc_buffer const *);
size_t ccc_buf_elem_size(ccc_buffer const *);
size_t ccc_buf_index_of(ccc_buffer const *, void const *slot);
bool ccc_buf_full(ccc_buffer const *);
bool ccc_buf_empty(ccc_buffer const *);
void *ccc_buf_at(ccc_buffer const *, size_t);
void *ccc_buf_back(ccc_buffer const *);
void *ccc_buf_front(ccc_buffer const *);
void *ccc_buf_alloc_back(ccc_buffer *);

void *ccc_buf_push_back(ccc_buffer *, void const *);
ccc_result ccc_buf_pop_back(ccc_buffer *);
ccc_result ccc_buf_pop_back_n(ccc_buffer *, size_t n);
void *ccc_buf_copy(ccc_buffer *, size_t dst, size_t src);
ccc_result ccc_buf_swap(ccc_buffer *, char tmp[], size_t i, size_t j);

ccc_result ccc_buf_write(ccc_buffer *, size_t i, void const *data);
ccc_result ccc_buf_erase(ccc_buffer *, size_t i);

void *ccc_buf_begin(ccc_buffer const *);
void *ccc_buf_next(ccc_buffer const *, void const *);
void *ccc_buf_end(ccc_buffer const *);

void *ccc_buf_rbegin(ccc_buffer const *);
void *ccc_buf_rnext(ccc_buffer const *, void const *);
void *ccc_buf_rend(ccc_buffer const *);

void *ccc_buf_capacity_end(ccc_buffer const *);

#ifndef BUFFER_USING_NAMESPACE_CCC
typedef ccc_buffer buffer;
#    define buf_init(args...) ccc_buf_init(args)
#    define buf_alloc(args...) ccc_buf_alloc(args)
#    define buf_base(args...) ccc_buf_base(args)
#    define buf_size(args...) ccc_buf_size(args)
#    define buf_size_plus(args...) ccc_buf_size_plus(args)
#    define buf_size_minus(args...) ccc_buf_size_minus(args)
#    define buf_size_set(args...) ccc_buf_size_set(args)
#    define buf_capacity(args...) ccc_buf_capacity(args)
#    define buf_elem_size(args...) ccc_buf_elem_size(args)
#    define buf_index_of(args...) ccc_buf_index_of(args)
#    define buf_full(args...) ccc_buf_full(args)
#    define buf_empty(args...) ccc_buf_empty(args)
#    define buf_at(args...) ccc_buf_at(args)
#    define buf_back(args...) ccc_buf_back(args)
#    define buf_front(args...) ccc_buf_front(args)
#    define buf_alloc_back(args...) ccc_buf_alloc_back(args)
#    define buf_push_back(args...) ccc_buf_push_back(args)
#    define buf_pop_back(args...) ccc_buf_pop_back(args)
#    define buf_pop_back_n(args...) ccc_buf_pop_back_n(args)
#    define buf_copy(args...) ccc_buf_copy(args)
#    define buf_swap(args...) ccc_buf_swap(args)
#    define buf_write(args...) ccc_buf_write(args)
#    define buf_erase(args...) ccc_buf_erase(args)
#    define buf_begin(args...) ccc_buf_begin(args)
#    define buf_next(args...) ccc_buf_next(args)
#    define buf_end(args...) ccc_buf_end(args)
#    define buf_rbegin(args...) ccc_buf_rbegin(args)
#    define buf_rnext(args...) ccc_buf_rnext(args)
#    define buf_rend(args...) ccc_buf_rend(args)
#    define buf_capacity_end(args...) ccc_buf_capacity_end(args)
#endif /* BUFFER_USING_NAMESPACE_CCC */

#endif /* CCC_BUFFER_H */
