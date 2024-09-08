#ifndef CCC_GENERICS_H
#define CCC_GENERICS_H

#include "impl_generics.h"
#include "types.h"

#define C_BEGIN(container_ptr) C_IMPL_BEGIN(container_ptr)

#define C_RBEGIN(container_ptr) C_IMPL_RBEGIN(container_ptr)

#define C_NEXT(container_ptr, void_iter_ptr)                                   \
    C_IMPL_NEXT(container_ptr, void_iter_ptr)

#define C_RNEXT(container_ptr, void_iter_ptr)                                  \
    C_IMPL_RNEXT(container_ptr, void_iter_ptr)

#define C_ENTRY(container_ptr, key...) C_IMPL_ENTRY(container_ptr, key)

bool ccc_entry_occupied(ccc_entry);
bool ccc_entry_error(ccc_entry);
void *ccc_entry_unwrap(ccc_entry);

void *ccc_begin_range(ccc_range const *);
void *ccc_end_range(ccc_range const *);

void *ccc_begin_rrange(ccc_rrange const *);
void *ccc_end_rrange(ccc_rrange const *);

#endif /* CCC_GENERICS_H */
