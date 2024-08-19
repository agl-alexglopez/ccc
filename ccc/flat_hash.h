#ifndef CCC_FLAT_HASH_H
#define CCC_FLAT_HASH_H

#include "buf.h"

#include <stdint.h>

typedef enum
{
    CCC_FHASH_OK = CCC_BUF_OK,
    CCC_FHASH_FULL = CCC_BUF_FULL,
    CCC_FHASH_ERR = CCC_BUF_ERR,
    CCC_FHASH_NOP,
} ccc_fhash_result;

typedef struct
{
    int64_t hash;
} ccc_fhash_elem;

typedef bool ccc_fhash_eq_fn(void const *, void const *, void *aux);

typedef void ccc_fhash_destructor_fn(void *);

typedef void ccc_fhash_print_fn(void const *);

typedef int64_t ccc_fhash_fn(void const *);

typedef struct
{
    ccc_buf *buf;
    ccc_buf_free_fn *free_fn;
    ccc_fhash_fn *hash_fn;
    ccc_fhash_eq_fn *eq_fn;
    void *aux;
    size_t hash_elem_offset;
} ccc_flat_hash;

ccc_fhash_result ccc_fhash_init(ccc_flat_hash *, ccc_buf *,
                                size_t hash_elem_offset, ccc_fhash_fn *,
                                ccc_fhash_eq_fn *, void *aux);
bool ccc_fhash_empty(ccc_flat_hash const *);
bool ccc_fhash_contains(ccc_flat_hash const *, void const *);
void const *ccc_fhash_find(ccc_flat_hash const *, void const *);
ccc_fhash_result ccc_fhash_insert(ccc_flat_hash *, void const *);
ccc_fhash_result ccc_fhash_erase(ccc_flat_hash *, void const *);
size_t ccc_fhash_size(ccc_flat_hash const *);

void const *ccc_fhash_begin(ccc_flat_hash const *);
void const *ccc_fhash_next(ccc_flat_hash const *, void const *iter);
void const *ccc_fhash_end(ccc_flat_hash const *);

#endif /* CCC_FLAT_HASH_H */
