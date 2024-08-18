#ifndef CCC_FLAT_HASH_H
#define CCC_FLAT_HASH_H

#include "buf.h"

typedef enum
{
    CCC_HASH_OK = CCC_BUF_OK,
    CCC_HASH_FULL = CCC_BUF_FULL,
    CCC_HASH_ERR = CCC_BUF_ERR,
    CCC_HASH_NOP,
} ccc_hash_result;

typedef struct
{
    size_t hash;
} ccc_hash_elem;

typedef bool ccc_hash_eq_fn(void const *, void const *, void *aux);

typedef void ccc_hash_destructor_fn(void *);

typedef void ccc_hash_print_fn(void const *);

typedef size_t ccc_hash_fn(void const *);

typedef struct
{
    ccc_buf *buf;
    ccc_buf_free_fn *free_fn;
    ccc_hash_fn *hash_fn;
    ccc_hash_eq_fn *eq_fn;
    void *aux;
    size_t hash_elem_offset;
} ccc_hash;

#define CCC_HASH_INIT(buf_ptr, struct_type, hash_elem_name, hash_fn, eq_fn,    \
                      aux_data)                                                \
    {                                                                          \
        .buf = (buf_ptr), offsetof(struct_type, hash_elem_name),               \
        .hash = (hash_fn), .eq = (eq_fn), .aux = (aux_data)                    \
    }

ccc_hash_result ccc_hash_init(ccc_hash *, ccc_buf *, size_t hash_elem_offset,
                              ccc_hash_fn *, ccc_hash_eq_fn *, void *aux);
bool ccc_hash_contains(ccc_hash const *, void const *);
void const *ccc_hash_find(ccc_hash const *, void const *);
ccc_hash_result ccc_hash_insert(ccc_hash *, void const *);
ccc_hash_result ccc_hash_erase(ccc_hash *, void const *);

#endif /* CCC_FLAT_HASH_H */
