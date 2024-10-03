#ifndef CCC_FLAT_ORDERED_MAP_H
#define CCC_FLAT_ORDERED_MAP_H

#include "impl_flat_ordered_map.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct ccc_fom_ ccc_flat_ordered_map;

typedef struct ccc_fom_elem_ ccc_f_om_elem;

typedef union
{
    struct ccc_fom_entry_ impl_;
} ccc_f_om_entry;

#define ccc_fom_init(mem_ptr, capacity, struct_name, map_elem_field,           \
                     key_elem_field, alloc_fn, key_cmp, aux)                   \
    ccc_impl_fom_init(mem_ptr, capacity, struct_name, map_elem_field,          \
                      key_elem_field, alloc_fn, key_cmp, aux)

ccc_entry ccc_fom_insert(ccc_flat_ordered_map *, ccc_f_om_elem *out_handle,
                         void *tmp);

/*===========================   Iterators   =================================*/

void *ccc_fom_begin(ccc_flat_ordered_map const *);
void *ccc_fom_rbegin(ccc_flat_ordered_map const *);
void *ccc_fom_end(ccc_flat_ordered_map const *);
void *ccc_fom_rend(ccc_flat_ordered_map const *);
void *ccc_fom_next(ccc_flat_ordered_map const *, ccc_f_om_elem const *);
void *ccc_fom_rnext(ccc_flat_ordered_map const *, ccc_f_om_elem const *);

/*===========================    Getters    =================================*/

size_t ccc_fom_size(ccc_flat_ordered_map const *);
bool ccc_fom_empty(ccc_flat_ordered_map const *);
bool ccc_fom_validate(ccc_flat_ordered_map const *);

/*===========================    Namespace  =================================*/

#ifdef FLAT_ORDERED_MAP_USING_NAMESPACE_CCC
typedef ccc_flat_ordered_map flat_ordered_map;
typedef ccc_f_om_elem f_om_elem;
typedef ccc_f_om_entry f_om_entry;

#    define fom_init(args...) ccc_fom_init(args)
#    define fom_insert(args...) ccc_fom_insert(args)

#    define fom_begin(args...) ccc_fom_begin(args)
#    define fom_next(args...) ccc_fom_next(args)
#    define fom_rbegin(args...) ccc_fom_rbegin(args)
#    define fom_rnext(args...) ccc_fom_rnext(args)
#    define fom_end(args...) ccc_fom_end(args)
#    define fom_rend(args...) ccc_fom_rend(args)
#    define fom_size(args...) ccc_fom_size(args)
#    define fom_empty(args...) ccc_fom_empty(args)
#    define fom_validate(args...) ccc_fom_validate(args)
#endif /* FLAT_ORDERED_MAP_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_ORDERED_MAP_H */
