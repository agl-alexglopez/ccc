#ifndef CCC_EMPLACE_H
#define CCC_EMPLACE_H

#include "impl_flat_pqueue.h"
#include "impl_list.h"

#define EMPLACE(container_ptr, struct_initializer...)                          \
    _Generic(container_ptr,                                                    \
        ccc_flat_pqueue                                                        \
            *: CCC_IMPL_FPQ_EMPLACE(container_ptr, struct_initializer))

#define EMPLACE_BACK(container_ptr, struct_initializer...)                     \
    _Generic(container_ptr,                                                    \
        ccc_buf                                                                \
            *: CCC_IMPL_BUF_EMPLACE_BACK(container_ptr, struct_initializer),   \
        ccc_list                                                               \
            *: CCC_IMPL_L_EMPLACE_BACK(container_ptr, struct_initializer))

#define EMPLACE_FRONT(container_ptr, struct_initializer...)                    \
    _Generic(container_ptr,                                                    \
        ccc_list                                                               \
            *: CCC_IMPL_L_EMPLACE_FRONT(container_ptr, struct_initializer))

#endif /* CCC_EMPLACE_H */
