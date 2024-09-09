/*NOLINTBEGIN*/
#include "double_ended_priority_queue.h"
#include "flat_hash_map.h"
#include "ordered_map.h"
#include "realtime_ordered_map.h"
/*NOLINTEND*/

/* Substitution Failure is not an Error, except it is in C. So throw some more
   generics and macros at the problem until the compiler hushes up. The cast
   and dereference is to accomodate pointers to types and types themselves.
   For example, the Entry API takes entries by value in functions like
   OR_INSERT so we need to add the dereference of type punning (which is safe
   because it will never actually happen). */
#define SFINAE(type, x)                                                        \
    _Generic((x), type: (x), default: (*((type *)dumb_sfinae())))

void *dumb_sfinae(void);

#define CCC_IMPL_BEGIN(container_ptr)                                          \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_begin,                     \
        ccc_ordered_map *: ccc_om_begin,                                       \
        ccc_realtime_ordered_map *: ccc_rom_begin)(container_ptr)

#define CCC_IMPL_RBEGIN(container_ptr)                                         \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_rbegin,                    \
        ccc_ordered_map *: ccc_om_rbegin,                                      \
        ccc_realtime_ordered_map *: ccc_rom_rbegin)(container_ptr)

#define CCC_IMPL_NEXT(container_ptr, iter_elem_ptr)                            \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_next,                      \
        ccc_ordered_map *: ccc_om_next,                                        \
        ccc_realtime_ordered_map *: ccc_rom_next)((container_ptr),             \
                                                  (iter_elem_ptr))

#define CCC_IMPL_RNEXT(container_ptr, iter_elem_ptr)                           \
    _Generic((container_ptr),                                                  \
        ccc_double_ended_priority_queue *: ccc_depq_rnext,                     \
        ccc_ordered_map *: ccc_om_rnext,                                       \
        ccc_realtime_ordered_map *: ccc_rom_rnext)((container_ptr),            \
                                                   (iter_elem_ptr))

#define CCC_IMPL_ENTRY(container_ptr, key...)                                  \
    _Generic((container_ptr),                                                  \
        ccc_flat_hash_map                                                      \
            *: FHM_ENTRY(SFINAE(ccc_flat_hash_map *, (container_ptr)), key),   \
        ccc_ordered_map                                                        \
            *: OM_ENTRY(SFINAE(ccc_ordered_map *, (container_ptr)), key))

#define CCC_IMPL_OR_INSERT(entry, key_val...)                                  \
    _Generic((entry),                                                          \
        ccc_fh_map_entry: FHM_OR_INSERT(SFINAE(ccc_fh_map_entry, (entry)),     \
                                        key_val),                              \
        ccc_o_map_entry: OM_OR_INSERT(SFINAE(ccc_o_map_entry, (entry)),        \
                                      key_val),                                \
        ccc_rtom_entry: ROM_OR_INSERT(SFINAE(ccc_rtom_entry, (entry)),         \
                                      key_val))
