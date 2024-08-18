#ifndef CCC_BUF_H
#define CCC_BUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    CCC_BUF_OK,
    CCC_BUF_FULL,
    CCC_BUF_ERR,
} ccc_buf_result;

/* This function must follow the API rules of the stdlib realloc function.
   Notably, a call to realloc on NULL input is the same as malloc for the
   specified size and is valid. */
typedef void *ccc_buf_realloc_fn(void *, size_t);

/* */
typedef void ccc_buf_free_fn(void *);

typedef struct
{
    void *mem;
    size_t elem_sz;
    size_t sz;
    size_t capacity;
    ccc_buf_realloc_fn *realloc_fn;
} ccc_buf;

#define CCC_BUF_INIT(mem, type, capacity, realloc_fn)                          \
    {                                                                          \
        (mem), sizeof(type), 0, (capacity), (realloc_fn)                       \
    }

#define CCC_BUF_EMPLACE(ccc_buf_ptr, index, struct_name,                       \
                        struct_initializer...)                                 \
    ({                                                                         \
        ccc_buf_result _macro_res_;                                            \
        {                                                                      \
            if (sizeof(struct_name) != ccc_buf_elem_size(ccc_buf_ptr))         \
            {                                                                  \
                _macro_res_ = CCC_BUF_ERR;                                     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                void *_macro_pos_ = ccc_buf_at(ccc_buf_ptr, i);                \
                if (!_macro_pos_)                                              \
                {                                                              \
                    _macro_res_ = CCC_BUF_ERR;                                 \
                }                                                              \
                else                                                           \
                {                                                              \
                    *((struct_name *)_macro_pos_)                              \
                        = (struct_name)struct_initializer;                     \
                    _macro_res_ = CCC_BUF_OK;                                  \
                }                                                              \
            }                                                                  \
        };                                                                     \
        _macro_res_;                                                           \
    })

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

ccc_buf_result ccc_buf_write(ccc_buf *, size_t i, void const *data);
ccc_buf_result ccc_buf_erase(ccc_buf *, size_t i);

ccc_buf_result ccc_buf_free(ccc_buf *, ccc_buf_free_fn *);

void *ccc_buf_begin(ccc_buf const *);
void *ccc_buf_next(ccc_buf const *, void const *);
void *ccc_buf_size_end(ccc_buf const *);
void *ccc_buf_capacity_end(ccc_buf const *);

#endif /* CCC_BUF_H */
