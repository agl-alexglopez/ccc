#ifndef CCC_TYPES_H
#define CCC_TYPES_H

#include "impl_types.h"

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

typedef struct
{
    struct ccc_range_ impl;
} ccc_range;

typedef struct
{
    struct ccc_rrange_ impl;
} ccc_rrange;

#define CCC_ENTRY_OCCUPIED 0x1
#define CCC_ENTRY_VACANT 0
#define CCC_ENTRY_ERROR 0x2

typedef struct
{
    struct ccc_entry_ impl;
} ccc_entry;

typedef void *ccc_alloc_fn(void *, size_t);

typedef ccc_threeway_cmp ccc_cmp_fn(ccc_cmp);

typedef void ccc_print_fn(void const *container);

typedef void ccc_update_fn(ccc_update);

typedef void ccc_destructor_fn(void *container);

typedef bool ccc_key_eq_fn(ccc_key_cmp);

typedef ccc_threeway_cmp ccc_key_cmp_fn(ccc_key_cmp);

typedef uint64_t ccc_hash_fn(void const *to_hash);

bool ccc_entry_occupied(ccc_entry const *e);
bool ccc_entry_error(ccc_entry const *e);
void *ccc_entry_unwrap(ccc_entry const *e);
void *ccc_begin_range(ccc_range const *r);
void *ccc_end_range(ccc_range const *r);
void *ccc_begin_rrange(ccc_rrange const *r);
void *ccc_end_rrange(ccc_rrange const *r);

#ifdef TYPES_USING_NAMESPACE_CCC
typedef ccc_result result;
typedef ccc_threeway_cmp threeway_cmp;
typedef ccc_cmp cmp;
typedef ccc_key_cmp key_cmp;
typedef ccc_update update;
typedef ccc_range range;
typedef ccc_rrange rrange;
typedef ccc_entry entry;
typedef ccc_alloc_fn alloc_fn;
typedef ccc_cmp_fn cmp_fn;
typedef ccc_print_fn print_fn;
typedef ccc_update_fn update_fn;
typedef ccc_destructor_fn destructor_fn;
typedef ccc_key_eq_fn key_eq_fn;
typedef ccc_key_cmp_fn key_cmp_fn;
typedef ccc_hash_fn hash_fn;
#    define ENTRY_VACANT CCC_ENTRY_VACANT
#    define ENTRY_ERROR CCC_ENTRY_ERROR
#    define entry_occupied ccc_entry_occupied
#    define entry_error ccc_entry_error
#    define entry_unwrap ccc_entry_unwrap
#    define begin_range ccc_begin_range
#    define end_range ccc_end_range
#    define begin_rrange ccc_begin_rrange
#    define end_rrange ccc_end_rrange
#endif

#endif /* CCC_TYPES_H */
