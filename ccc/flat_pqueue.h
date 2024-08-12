#ifndef FLAT_PQUEUE
#define FLAT_PQUEUE

#include "buf.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

typedef enum ccc_fpq_threeway_cmp
{
    CCC_FPQ_LES = -1,
    CCC_FPQ_EQL,
    CCC_FPQ_GRT,
} ccc_fpq_threeway_cmp;

typedef struct ccc_fpq_elem
{
    uint8_t handle;
} ccc_fpq_elem;

typedef ccc_fpq_threeway_cmp ccc_fpq_cmp_fn(ccc_fpq_elem const *,
                                            ccc_fpq_elem const *, void *);

typedef void fpq_destructor_fn(ccc_fpq_elem *);

typedef void fpq_update_fn(ccc_fpq_elem *, void *);

typedef void fpq_print_fn(ccc_fpq_elem const *);

typedef struct ccc_flat_pqueue
{
    ccc_buf *const buf;
    size_t const fpq_elem_offset;
    ccc_fpq_cmp_fn *const cmp;
    ccc_fpq_threeway_cmp const order;
    void *const aux;
} ccc_flat_pqueue;

#define CCC_FPQ_OF(struct, member, fpq_elem)                                   \
    ((struct *)((uint8_t *)&(fpq_elem)->handle                                 \
                - offsetof(struct, member.handle))) /* NOLINT */

#define CCC_FPQ_INIT(mem_buf, struct_name, fpq_elem_field, cmp_order, cmp_fn,  \
                     aux_data)                                                 \
    {                                                                          \
        .buf = (mem_buf),                                                      \
        .fpq_elem_offset = offsetof(struct_name, fpq_elem_field),              \
        .cmp = (cmp_fn), .order = (cmp_order), .aux = (aux_data)               \
    }

/* Given an initialized flat priority queue, a struct type, and its
   initializer, attempts to push an r-value of one's struct type into the
   backing buffer directly, returning the ccc_buf_result according to the
   underlying buffer's allocation policy. If allocation fails because
   the underlying buffer does not define a reallocation policy and is full,
   CCC_BUF_FULL is returned, otherwise CCC_BUF_OK is returned. If the provided
   struct type does not match the size of the elements stored in the buffer
   CCC_BUF_ERR is returned. Use as follows:

   struct val
   {
       int v;
       int id;
       ccc_fpq_elem e;
   };
   static struct val memory[100];
   static ccc_buf buf = CCC_BUF_INIT(&memory, struct val, 100, NULL);
   static ccc_flat_pqueue fpq =
       CCC_FPQ_INIT(&buf, offsetof(struct val, e), CCC_FPQ_LES, val_cmp, NULL);
   int main()
   {
       ccc_buf_result const res = CCC_FPQ_PUSH(&fpq, struct val, {.v = 10});
       return 0;
   }

   Most importantly, the struct name shall be provided followed by a brace
   initializer with fields of the struct type specified. Here are further
   examples of types of initialization possible.

   Various forms of designated initializers:

   ccc_buf_result const res = CCC_FPQ_PUSH(&fpq, struct val, {.v = 10});
   ccc_buf_result const res
       = CCC_FPQ_PUSH(&fpq, struct val, {.v = 10, .id = 0});
   ccc_buf_result const res
       = CCC_FPQ_PUSH(&fpq, struct val, {.v = 10, .id = 0, .e = {0}});

   Older C notation requires all fields be specified on some compilers:

   ccc_buf_result const res = CCC_FPQ_PUSH(&fpq, struct val, {10, 0, {0}});

   This macro provides a more convenient succinct notation that avoids an
   additional copy if the struct values are constructed by hand or
   from input of other functions, requiring no intermediate storage. */
#define CCC_FPQ_PUSH(fpq, struct_name, struct_brace_initializer...)            \
    ({                                                                         \
        ccc_buf_result _macro_res_;                                            \
        {                                                                      \
            if (sizeof(struct_name) != ccc_buf_elem_size((fpq)->buf))          \
            {                                                                  \
                _macro_res_ = CCC_BUF_ERR;                                     \
            }                                                                  \
            else                                                               \
            {                                                                  \
                void *_macro_new_ = ccc_buf_alloc((fpq)->buf);                 \
                if (_macro_new_)                                               \
                {                                                              \
                    *((struct_name *)_macro_new_)                              \
                        = (struct_name)struct_brace_initializer;               \
                    if (ccc_buf_size((fpq)->buf) > 1)                          \
                    {                                                          \
                        uint8_t _macro_tmp_[ccc_buf_elem_size((fpq)->buf)];    \
                        bubble_up((fpq), (_macro_tmp_),                        \
                                  ccc_buf_size((fpq)->buf) - 1);               \
                    }                                                          \
                    _macro_res_ = CCC_BUF_OK;                                  \
                }                                                              \
                else                                                           \
                {                                                              \
                    _macro_res_ = CCC_BUF_FULL;                                \
                }                                                              \
            }                                                                  \
        };                                                                     \
        _macro_res_;                                                           \
    })

ccc_buf_result ccc_fpq_push(ccc_flat_pqueue *, void const *);
ccc_fpq_elem const *ccc_fpq_front(ccc_flat_pqueue const *);
ccc_buf *ccc_fpq_buf(ccc_flat_pqueue const *);
ccc_fpq_elem *ccc_fpq_pop(ccc_flat_pqueue *);
ccc_fpq_elem *ccc_fpq_erase(ccc_flat_pqueue *, ccc_fpq_elem *);
void ccc_fpq_clear(ccc_flat_pqueue *, fpq_destructor_fn *);
bool ccc_fpq_empty(ccc_flat_pqueue const *);
size_t ccc_fpq_size(ccc_flat_pqueue const *);
bool ccc_fpq_update(ccc_flat_pqueue *, ccc_fpq_elem *, fpq_update_fn *, void *);
bool ccc_fpq_validate(ccc_flat_pqueue const *);
ccc_fpq_threeway_cmp ccc_fpq_order(ccc_flat_pqueue const *);
void bubble_up(ccc_flat_pqueue *, uint8_t[], size_t);

void ccc_fpq_print(ccc_flat_pqueue const *, size_t, fpq_print_fn *);

#endif /* FLAT_PQUEUE */
