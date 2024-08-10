#ifndef FLAT_PQUEUE
#define FLAT_PQUEUE

#include "attrib.h"
#include "buf.h"

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
    size_t heap_index;
    size_t buf_key;
} ccc_fpq_elem;

typedef ccc_fpq_threeway_cmp ccc_fpq_cmp_fn(ccc_fpq_elem const *,
                                            ccc_fpq_elem const *, void *);

typedef void fpq_destructor_fn(ccc_fpq_elem *);

typedef void fpq_update_fn(ccc_fpq_elem *, void *);

typedef void fpq_print_fn(ccc_fpq_elem const *);

typedef struct ccc_flat_pqueue
{
    ccc_buf *buf;
    size_t fpq_elem_offset;
    ccc_fpq_cmp_fn *cmp;
    ccc_fpq_threeway_cmp order;
    void *aux;
} ccc_flat_pqueue;

#define CCC_FPQ_OF(struct, member, fpq_elem)                                   \
    ((struct *)((uint8_t *)&(fpq_elem)->buf_key                                \
                - offsetof(struct, member.buf_key))) /* NOLINT */

#define CCC_FPQ_INIT(mem_buf, elem_offset, cmp_order, cmp_fn, aux_data)        \
    {                                                                          \
        .buf = (mem_buf), .fpq_elem_offset = (elem_offset), .cmp = (cmp_fn),   \
        .order = (cmp_order), .aux = (aux_data)                                \
    }

ccc_fpq_elem const *ccc_fpq_front(ccc_flat_pqueue const[static 1])
    ATTRIB_NONNULL(1);
ccc_buf *ccc_fpq_buf(ccc_flat_pqueue const[static 1]) ATTRIB_NONNULL(1);
ccc_buf_result ccc_fpq_push(ccc_flat_pqueue[static 1], ccc_fpq_elem[static 1])
    ATTRIB_NONNULL(1, 2);
ccc_fpq_elem *ccc_fpq_pop(ccc_flat_pqueue[static 1]) ATTRIB_NONNULL(1);
ccc_fpq_elem *ccc_fpq_erase(ccc_flat_pqueue[static 1], ccc_fpq_elem[static 1])
    ATTRIB_NONNULL(1, 2);
void ccc_fpq_clear(ccc_flat_pqueue[static 1], fpq_destructor_fn *)
    ATTRIB_NONNULL(1);
bool ccc_fpq_empty(ccc_flat_pqueue const[static 1]) ATTRIB_NONNULL(1);
size_t ccc_fpq_size(ccc_flat_pqueue const[static 1]) ATTRIB_NONNULL(1);
bool ccc_fpq_update(ccc_flat_pqueue[static 1], ccc_fpq_elem[static 1],
                    fpq_update_fn *, void *) ATTRIB_NONNULL(1);
bool ccc_fpq_validate(ccc_flat_pqueue const[static 1]) ATTRIB_NONNULL(1);
ccc_fpq_threeway_cmp ccc_fpq_order(ccc_flat_pqueue const[static 1])
    ATTRIB_NONNULL(1);

void ccc_fpq_print(ccc_flat_pqueue const[static 1], size_t, fpq_print_fn *)
    ATTRIB_NONNULL(1);

#endif /* FLAT_PQUEUE */
