#ifndef CCC_FLAT_HASH_H
#define CCC_FLAT_HASH_H

#include "buf.h"

typedef enum
{
    CCC_HASH_OK = CCC_BUF_OK,
    CCC_HASH_FULL = CCC_BUF_FULL,
    CCC_HASH_ERR = CCC_BUF_ERR,
} ccc_hash_result;

typedef struct
{
    size_t distance;
} ccc_hash_elem;

typedef bool ccc_hash_eq_fn(void const *, void *const, void *aux);

typedef void ccc_hash_destructor_fn(void *);

typedef void ccc_hash_print_fn(void const *);

typedef void ccc_hash_fn(void const *, void *aux);

typedef struct
{
    ccc_buf *buf;
    ccc_hash_fn *hash;
    ccc_hash_eq_fn *eq;
} ccc_hash;

#define CCC_HASH_INIT(buf_ptr, hash_fn, eq_fn)                                 \
    {                                                                          \
        .buf = (buf_ptr), .hash = (hash_fn), .eq = (eq_fn)                     \
    }

bool ccc_hash_contains(ccc_hash *, void *);

#endif /* CCC_FLAT_HASH_H */
