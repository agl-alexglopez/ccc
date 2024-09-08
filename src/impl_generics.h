/*NOLINTBEGIN*/
#include "double_ended_priority_queue.h"
#include "flat_hash_map.h"
#include "ordered_map.h"
#include "realtime_ordered_map.h"
/*NOLINTEND*/

#define C_IMPL_BEGIN(container_ptr)                                            \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_begin,                     \
        ccc_ordered_map *: ccc_om_begin,                                       \
        ccc_realtime_ordered_map *: ccc_rom_begin)(container_ptr)

#define C_IMPL_RBEGIN(container_ptr)                                           \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_rbegin,                    \
        ccc_ordered_map *: ccc_om_rbegin,                                      \
        ccc_realtime_ordered_map *: ccc_rom_rbegin)(container_ptr)

#define C_IMPL_NEXT(container_ptr, iter_elem_ptr)                              \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_next,                      \
        ccc_ordered_map *: ccc_om_next,                                        \
        ccc_realtime_ordered_map *: ccc_rom_next)((container_ptr),             \
                                                  (iter_elem_ptr))

#define C_IMPL_RNEXT(container_ptr, iter_elem_ptr)                             \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_rnext,                     \
        ccc_ordered_map *: ccc_om_rnext,                                       \
        ccc_realtime_ordered_map *: ccc_rom_rnext)((container_ptr),            \
                                                   (iter_elem_ptr))

#define C_IMPL_ENTRY(container_ptr, key...)                                    \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map                                                      \
            *: FHM_ENTRY((ccc_flat_hash_map *)(container_ptr), key),           \
        ccc_ordered_map *: OM_ENTRY((ccc_ordered_map *)(container_ptr), key))
