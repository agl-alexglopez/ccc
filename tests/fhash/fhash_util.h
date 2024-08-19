#ifndef FHASH_UTIL_H
#define FHASH_UTIL_H

#include "flat_hash.h"

#include <stdint.h>

struct val
{
    int id;
    int val;
    ccc_fhash_elem e;
};

int64_t fhash_val_zero(void const *);
int64_t fhash_val_last_digit(void const *);
bool fhash_val_eq(void const *, void const *, void *aux);
bool fhash_id_and_val_eq(void const *, void const *, void *aux);

#endif /* FHASH_UTIL_H */
