#ifndef HEAP_PQUEUE
#define HEAP_PQUEUE

#include "attrib.h"

#include <stdbool.h>
#include <stddef.h>
/* NOLINTNEXTLINE */
#include <stdint.h>

typedef enum ccc_fpq_threeway_cmp
{
    HPQLES = -1,
    HPQEQL,
    HPQGRT,
} ccc_fpq_threeway_cmp;

typedef struct ccc_fpq_elem
{
    size_t handle;
} ccc_fpq_elem;

typedef ccc_fpq_threeway_cmp ccc_fpq_cmp_fn(ccc_fpq_elem const *,
                                            ccc_fpq_elem const *, void *);

typedef void fpq_destructor_fn(ccc_fpq_elem *);

typedef void fpq_update_fn(ccc_fpq_elem *, void *);

typedef void fpq_print_fn(ccc_fpq_elem const *);

typedef struct ccc_flat_pqueue
{
    ccc_fpq_elem **heap ATTRIB_PRIVATE;
    size_t sz ATTRIB_PRIVATE;
    size_t capacity ATTRIB_PRIVATE;
    ccc_fpq_cmp_fn *cmp ATTRIB_PRIVATE;
    ccc_fpq_threeway_cmp order ATTRIB_PRIVATE;
    void *aux ATTRIB_PRIVATE;
} ccc_flat_pqueue;

#define HPQ_ENTRY(FPQ_ELEM, STRUCT, MEMBER)                                    \
    ((STRUCT *)((uint8_t *)&(FPQ_ELEM)->handle                                 \
                - offsetof(STRUCT, MEMBER.handle))) /* NOLINT */

void ccc_fpq_init(ccc_flat_pqueue *, ccc_fpq_threeway_cmp fpq_ordering,
                  ccc_fpq_cmp_fn *, void *);
ccc_fpq_elem const *ccc_fpq_front(ccc_flat_pqueue const *);
void ccc_fpq_push(ccc_flat_pqueue *, ccc_fpq_elem *);
ccc_fpq_elem *ccc_fpq_pop(ccc_flat_pqueue *);
ccc_fpq_elem *ccc_fpq_erase(ccc_flat_pqueue *, ccc_fpq_elem *);
void ccc_fpq_clear(ccc_flat_pqueue *, fpq_destructor_fn *);
bool ccc_fpq_empty(ccc_flat_pqueue const *);
size_t ccc_fpq_size(ccc_flat_pqueue const *);
bool ccc_fpq_update(ccc_flat_pqueue *, ccc_fpq_elem *, fpq_update_fn *, void *);
bool ccc_fpq_validate(ccc_flat_pqueue const *);
ccc_fpq_threeway_cmp ccc_fpq_order(ccc_flat_pqueue const *);

void ccc_fpq_print(ccc_flat_pqueue const *, size_t, fpq_print_fn *);

#endif