#ifndef CCC_GENERICS_H
#define CCC_GENERICS_H

#include "impl_generics.h"
#include "types.h"

#define CCC_BEGIN(container_ptr) CCC_IMPL_BEGIN(container_ptr)

#define CCC_RBEGIN(container_ptr) CCC_IMPL_RBEGIN(container_ptr)

#define CCC_NEXT(container_ptr, void_iter_ptr)                                 \
    CCC_IMPL_NEXT(container_ptr, void_iter_ptr)

#define CCC_RNEXT(container_ptr, void_iter_ptr)                                \
    CCC_IMPL_RNEXT(container_ptr, void_iter_ptr)

#define CCC_ENTRY(container_ptr, key...) CCC_IMPL_ENTRY(container_ptr, key)

#define CCC_OR_INSERT(entry, key_val...) CCC_IMPL_OR_INSERT(entry, key_val)

bool ccc_entry_occupied(ccc_entry);
bool ccc_entry_error(ccc_entry);
void *ccc_entry_unwrap(ccc_entry);

void *ccc_begin_range(ccc_range const *);
void *ccc_end_range(ccc_range const *);

void *ccc_begin_rrange(ccc_rrange const *);
void *ccc_end_rrange(ccc_rrange const *);

#ifdef CCC_GENERICS_SHORT_NAMES
#    define BEGIN CCC_BEGIN
#    define RBEGIN CCC_RBEGIN
#    define NEXT CCC_NEXT
#    define RNEXT CCC_RNEXT
#    define ENTRY CCC_ENTRY
#    define OR_INSERT CCC_OR_INSERT
#    define entry_occupied ccc_entry_occupied
#    define entry_error ccc_entry_error
#    define entry_unwrap ccc_entry_unwrap
#    define begin_range ccc_begin_range
#    define end_range ccc_end_range
#    define begin_rrange ccc_begin_rrange
#    define end_rrange ccc_end_rrange
#endif

#endif /* CCC_GENERICS_H */
