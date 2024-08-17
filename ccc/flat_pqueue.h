#ifndef CCC_FLAT_PQUEUE_H
#define CCC_FLAT_PQUEUE_H

#include "buf.h"

#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

typedef enum
{
    CCC_FPQ_LES = -1,
    CCC_FPQ_EQL,
    CCC_FPQ_GRT,
} ccc_fpq_threeway_cmp;

typedef enum
{
    CCC_FPQ_OK = CCC_BUF_OK,
    CCC_FPQ_FULL = CCC_BUF_FULL,
    CCC_FPQ_ERR = CCC_BUF_ERR,
} ccc_fpq_result;

typedef ccc_fpq_threeway_cmp ccc_fpq_cmp_fn(void const *, void const *,
                                            void *aux);

typedef void ccc_fpq_destructor_fn(void *);

typedef void ccc_fpq_update_fn(void *, void *);

typedef void ccc_fpq_print_fn(void const *);

/* It does not make sense for a flat pqueue to be associated with any other
   buffer, comparison function, ordering, or auxiliarry data once it has been
   initialized. The CCC_FPQ_INIT macro allows for initialization at compile
   time for static/global data, or runtime for dynamic data so initialization
   via construction of const fields is always possible. There is no reason to
   access the fields directly or modify them. */
typedef struct
{
    ccc_buf *const buf;
    ccc_fpq_cmp_fn *const cmp;
    ccc_fpq_threeway_cmp const order;
    void *const aux;
} ccc_flat_pqueue;

#define CCC_FPQ_INIT(mem_buf, struct_name, fpq_elem_field, cmp_order, cmp_fn,  \
                     aux_data)                                                 \
    {                                                                          \
        .buf = (mem_buf), .cmp = (cmp_fn), .order = (cmp_order),               \
        .aux = (aux_data),                                                     \
    }

/* Given an initialized flat priority queue, a struct type, and its
   initializer, attempts to write an r-value of one's struct type into the
   backing buffer directly, returning the ccc_fpq_result according to the
   underlying buffer's allocation policy. If allocation fails because
   the underlying buffer does not define a reallocation policy and is full,
   CCC_FPQ_FULL is returned, otherwise CCC_FPQ_OK is returned. If the provided
   struct type does not match the size of the elements stored in the buffer
   CCC_FPQ_ERR is returned. Use as follows:

   struct val
   {
       int v;
       int id;
   };

   Various forms of designated initializers:

   ccc_fpq_result const res = CCC_FPQ_EMPLACE(&fpq, struct val, {.v = 10});
   ccc_fpq_result const res
       = CCC_FPQ_EMPLACE(&fpq, struct val, {.v = rand_value(), .id = 0});
   ccc_fpq_result const res
       = CCC_FPQ_EMPLACE(&fpq, struct val, {.v = 10, .id = 0});

   Older C notation requires all fields be specified on some compilers:

   ccc_fpq_result const res = CCC_FPQ_EMPLACE(&fpq, struct val, {10, 0});

   This macro avoids an additional copy if the struct values are constructed
   by hand or from input of other functions, requiring no intermediate storage.
   If generating any values within the struct occurs via expensive function
   calls or calls with side effects, note that such functions do not execute
   if allocation fails due to a full buffer and no reallocation policy. */
#define CCC_FPQ_EMPLACE(fpq, struct_name, struct_initializer...)               \
    ({                                                                         \
        ccc_fpq_result _macro_res_;                                            \
        {                                                                      \
            if (sizeof(struct_name) != ccc_buf_elem_size((fpq)->buf))          \
            {                                                                  \
                _macro_res_ = CCC_FPQ_ERR;                                     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                void *_macro_new_ = ccc_buf_alloc((fpq)->buf);                 \
                if (!_macro_new_)                                              \
                {                                                              \
                    _macro_res_ = CCC_FPQ_FULL;                                \
                }                                                              \
                else                                                           \
                {                                                              \
                    *((struct_name *)_macro_new_)                              \
                        = (struct_name)struct_initializer;                     \
                    if (ccc_buf_size((fpq)->buf) > 1)                          \
                    {                                                          \
                        uint8_t _macro_tmp_[ccc_buf_elem_size((fpq)->buf)];    \
                        bubble_up((fpq), _macro_tmp_,                          \
                                  ccc_buf_size((fpq)->buf) - 1);               \
                    }                                                          \
                    _macro_res_ = CCC_FPQ_OK;                                  \
                }                                                              \
            }                                                                  \
        };                                                                     \
        _macro_res_;                                                           \
    })

ccc_fpq_result ccc_fpq_realloc(ccc_flat_pqueue *, size_t new_capacity,
                               ccc_buf_realloc_fn *);
ccc_fpq_result ccc_fpq_push(ccc_flat_pqueue *, void const *);
void const *ccc_fpq_front(ccc_flat_pqueue const *);
void *ccc_fpq_pop(ccc_flat_pqueue *);
void *ccc_fpq_erase(ccc_flat_pqueue *, void *);
bool ccc_fpq_update(ccc_flat_pqueue *, void *, ccc_fpq_update_fn *, void *);

ccc_buf *ccc_fpq_buf(ccc_flat_pqueue const *);
void ccc_fpq_clear(ccc_flat_pqueue *, ccc_fpq_destructor_fn *);
bool ccc_fpq_empty(ccc_flat_pqueue const *);
size_t ccc_fpq_size(ccc_flat_pqueue const *);
bool ccc_fpq_validate(ccc_flat_pqueue const *);
ccc_fpq_threeway_cmp ccc_fpq_order(ccc_flat_pqueue const *);
void bubble_up(ccc_flat_pqueue *, uint8_t[], size_t);

void ccc_fpq_print(ccc_flat_pqueue const *, size_t, ccc_fpq_print_fn *);

#endif /* CCC_FLAT_PQUEUE_H */
