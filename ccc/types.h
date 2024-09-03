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
    void const *entry;
    uint8_t status;
} ccc_entry;

typedef struct
{
    void const *const container_a;
    void const *const container_b;
    void *aux;
} ccc_cmp;

typedef struct
{
    void const *const container;
    void const *const key;
    void *aux;
} ccc_key_cmp;

typedef struct
{
    void *const container;
    void *aux;
} ccc_update;

typedef void *ccc_alloc_fn(void *, size_t);

typedef ccc_threeway_cmp ccc_cmp_fn(ccc_cmp);

typedef void ccc_print_fn(void const *container);

typedef void ccc_update_fn(ccc_update);

typedef void ccc_destructor_fn(void *container);

typedef bool ccc_key_eq_fn(ccc_key_cmp);

typedef ccc_threeway_cmp ccc_key_cmp_fn(ccc_key_cmp);

typedef uint64_t ccc_hash_fn(void const *to_hash);

#endif /* CCC_TYPES_H */
