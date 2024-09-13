#ifndef CCC_TYPES_H
#define CCC_TYPES_H

#include "impl_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    struct ccc_range_ impl_;
} ccc_range;

typedef struct
{
    struct ccc_range_ impl_;
} ccc_rrange;

typedef struct
{
    struct ccc_entry_ impl_;
} ccc_entry;

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
    CCC_CMP_ERR,
} ccc_threeway_cmp;

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
    void *container;
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

bool ccc_entry_occupied(ccc_entry const *e);
bool ccc_entry_insert_error(ccc_entry const *e);
void *ccc_entry_unwrap(ccc_entry const *e);

void *ccc_begin_range(ccc_range const *r);
void *ccc_end_range(ccc_range const *r);
void *ccc_rbegin_rrange(ccc_rrange const *r);
void *ccc_rend_rrange(ccc_rrange const *r);

#ifdef TYPES_USING_NAMESPACE_CCC

#    define entry_occupied(entry_ptr) ccc_entry_occupied(entry_ptr)
#    define entry_insert_error(entry_ptr) ccc_entry_insert_error(entry_ptr)
#    define entry_unwrap(entry_ptr) ccc_entry_unwrap(entry_ptr)
#    define begin_range(range_ptr) ccc_begin_range(range_ptr)
#    define end_range(range_ptr) ccc_end_range(range_ptr)
#    define rbegin_rrange(range_ptr) ccc_rbegin_rrange(range_ptr)
#    define rend_rrange(range_ptr) ccc_rend_rrange(range_ptr)

#endif /* TYPES_USING_NAMESPACE_CCC */

#endif /* CCC_TYPES_H */
