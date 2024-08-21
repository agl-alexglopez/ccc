#ifndef CCC_IMPL_FLAT_HASH_H
#define CCC_IMPL_FLAT_HASH_H

#include "buf.h"
#include "types.h"

#include <stdint.h>

struct ccc_impl_fhash_elem
{
    uint64_t hash;
};

struct ccc_impl_flat_hash
{
    ccc_buf *buf;
    ccc_free_fn *free_fn;
    ccc_hash_fn *hash_fn;
    ccc_eq_fn *eq_fn;
    void *aux;
    size_t hash_elem_offset;
};

#endif /* CCC_IMPL_FLAT_HASH_H */
