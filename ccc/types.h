#ifndef CCC_TYPES_H
#define CCC_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    CCC_OK = 0,
    CCC_NO_REALLOC,
    CCC_MEM_ERR,
    CCC_INPUT_ERR,
    CCC_NOP,
} ccc_result;

typedef enum
{
    CCC_LES = -1,
    CCC_EQL,
    CCC_GRT,
} ccc_threeway_cmp;

typedef struct
{
    void *const begin;
    void *const end;
} ccc_range;

typedef struct
{
    void *const rbegin;
    void *const end;
} ccc_rrange;

typedef struct
{
    bool found;
    void *entry;
} ccc_entry;

typedef void *ccc_realloc_fn(void *, size_t);

typedef ccc_threeway_cmp ccc_cmp_fn(void const *a, void const *b, void *aux);

typedef void ccc_print_fn(void const *);

typedef void ccc_update_fn(void *, void *aux);

typedef void ccc_destructor_fn(void *);

typedef bool ccc_eq_fn(void const *, void const *, void *aux);

typedef uint64_t ccc_hash_fn(void const *);

#endif /* CCC_TYPES_H */
