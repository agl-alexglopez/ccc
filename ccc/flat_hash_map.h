#ifndef CCC_FLAT_HASH_MAP_H
#define CCC_FLAT_HASH_MAP_H

#include "impl/impl_flat_hash_map.h"
#include "types.h"

typedef union ccc_fhmap_entry_ ccc_fhmap_entry;
typedef struct ccc_fhmap_ ccc_flat_hash_map;

#define ccc_fhm_declare_fixed_map(fixed_map_type_name, key_val_type_name,      \
                                  capacity)                                    \
    ccc_impl_fhm_declare_fixed_map(fixed_map_type_name, key_val_type_name,     \
                                   capacity)

#define ccc_fhm_fixed_capacity(fixed_map_type_name)                            \
    ccc_impl_fhm_fixed_capacity(fixed_map_type_name)

#define ccc_fhm_init(data_ptr, tag_ptr, key_field, hash_fn, key_eq_fn,         \
                     alloc_fn, aux_data, capacity)                             \
    ccc_impl_fhm_init(data_ptr, tag_ptr, key_field, hash_fn, key_eq_fn,        \
                      alloc_fn, aux_data, capacity)

ccc_result ccc_fhm_copy(ccc_flat_hash_map *dst, ccc_flat_hash_map const *src,
                        ccc_alloc_fn *fn);

[[nodiscard]] ccc_tribool ccc_fhm_contains(ccc_flat_hash_map const *h,
                                           void const *key);
[[nodiscard]] void *ccc_fhm_get_key_val(ccc_flat_hash_map const *h,
                                        void const *key);
[[nodiscard]] ccc_fhmap_entry ccc_fhm_entry(ccc_flat_hash_map *h,
                                            void const *key);

#define ccc_fhm_entry_r(map_ptr, key_ptr)                                      \
    &(ccc_fhmap_entry)                                                         \
    {                                                                          \
        ccc_fhm_entry(map_ptr, key_ptr).impl_                                  \
    }

[[nodiscard]] void *ccc_fhm_or_insert(ccc_fhmap_entry const *e,
                                      void const *key_val_type);

#define ccc_fhm_or_insert_w(map_entry_ptr, lazy_key_value...)                  \
    ccc_impl_fhm_or_insert_w(map_entry_ptr, lazy_key_value)

[[nodiscard]] void *ccc_fhm_insert_entry(ccc_fhmap_entry const *e,
                                         void const *key_val_type);

#define ccc_fhm_insert_entry_w(map_entry_ptr, lazy_key_value...)               \
    ccc_impl_fhm_insert_entry_w(map_entry_ptr, lazy_key_value)

[[nodiscard]] ccc_entry ccc_fhm_remove_entry(ccc_fhmap_entry const *e);

#define ccc_fhm_remove_entry_r(map_entry_ptr)                                  \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_remove_entry(map_entry_ptr).impl_                              \
    }

[[nodiscard]] ccc_fhmap_entry *ccc_fhm_and_modify(ccc_fhmap_entry *e,
                                                  ccc_update_fn *fn);
[[nodiscard]] ccc_fhmap_entry *
ccc_fhm_and_modify_aux(ccc_fhmap_entry *e, ccc_update_fn *fn, void *aux);

#define ccc_fhm_and_modify_w(map_entry_ptr, type_name, closure_over_T...)      \
    &(ccc_fhmap_entry)                                                         \
    {                                                                          \
        ccc_impl_fhm_and_modify_w(map_entry_ptr, type_name, closure_over_T)    \
    }

[[nodiscard]]
ccc_entry ccc_fhm_swap_entry(ccc_flat_hash_map *h, void *key_val_type_output);

#define ccc_fhm_swap_entry_r(map_ptr, key_val_type_ptr)                        \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_swap_entry(map_ptr, key_val_type_ptr).impl_                    \
    }

[[nodiscard]]
ccc_entry ccc_fhm_try_insert(ccc_flat_hash_map *h, void *key_val_type);

#define ccc_fhm_try_insert_r(map_ptr, key_val_type_ptr)                        \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_try_insert(map_ptr, key_val_type_ptr).impl_                    \
    }

#define ccc_fhm_try_insert_w(map_ptr, key_val_type_ptr...)                     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_fhm_try_insert_w(map_ptr, key_val_type_ptr)                   \
    }

[[nodiscard]]
ccc_entry ccc_fhm_insert_or_assign(ccc_flat_hash_map *h, void *key_val_type);

#define ccc_fhm_insert_or_assign_r(map_ptr, key_val_type_ptr)                  \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_insert_or_assign(map_ptr, key_val_type_ptr).impl_              \
    }

#define ccc_fhm_insert_or_assign_w(map_ptr, key_val_type_ptr...)               \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_impl_fhm_insert_or_assign_w(map_ptr, key_val_type_ptr)             \
    }

[[nodiscard]] ccc_entry ccc_fhm_remove(ccc_flat_hash_map *h,
                                       void *key_val_type_output);

#define ccc_fhm_remove_r(map_ptr, key_val_type_output_ptr)                     \
    &(ccc_entry)                                                               \
    {                                                                          \
        ccc_fhm_remove(map_ptr, key_val_type_output_ptr).impl_                 \
    }

[[nodiscard]] void *ccc_fhm_begin(ccc_flat_hash_map const *h);
[[nodiscard]] void *ccc_fhm_next(ccc_flat_hash_map const *h,
                                 void *key_val_type_iter);
[[nodiscard]] void *ccc_fhm_end(ccc_flat_hash_map const *h);

ccc_result ccc_fhm_clear(ccc_flat_hash_map *h, ccc_destructor_fn *fn);
ccc_result ccc_fhm_clear_and_free(ccc_flat_hash_map *h, ccc_destructor_fn *fn);
[[nodiscard]] ccc_tribool ccc_fhm_validate(ccc_flat_hash_map const *h);

[[nodiscard]] void *ccc_fhm_unwrap(ccc_fhmap_entry const *e);
[[nodiscard]] ccc_tribool ccc_fhm_occupied(ccc_fhmap_entry const *e);
[[nodiscard]] ccc_tribool ccc_fhm_insert_error(ccc_fhmap_entry const *e);
[[nodiscard]] ccc_handle_status ccc_fhm_handle_status(ccc_fhmap_entry const *e);

[[nodiscard]] ccc_tribool ccc_fhm_is_empty(ccc_flat_hash_map const *h);
[[nodiscard]] ccc_ucount ccc_fhm_size(ccc_flat_hash_map const *h);
[[nodiscard]] ccc_ucount ccc_fhm_capacity(ccc_flat_hash_map const *h);
[[nodiscard]] void *ccc_fhm_data(ccc_flat_hash_map const *h);

/** Define this preprocessor directive if shorter names are helpful. Ensure
 no namespace clashes occur before shortening. */
#ifdef FLAT_HASH_MAP_USING_NAMESPACE_CCC
typedef ccc_flat_hash_map flat_hash_map;
typedef ccc_fhmap_entry fhmap_entry;
#    define fhm_declare_fixed_map(args...) ccc_fhm_declare_fixed_map(args)
#    define fhm_fixed_capacity(args...) ccc_fhm_fixed_capacity(args)
#    define fhm_init(args...) ccc_fhm_init(args)
#    define fhm_copy(args...) ccc_fhm_copy(args)
#    define fhm_and_modify_w(args...) ccc_fhm_and_modify_w(args)
#    define fhm_or_insert_w(args...) ccc_fhm_or_insert_w(args)
#    define fhm_insert_entry_w(args...) ccc_fhm_insert_entry_w(args)
#    define fhm_try_insert_w(args...) ccc_fhm_try_insert_w(args)
#    define fhm_insert_or_assign_w(args...) ccc_fhm_insert_or_assign_w(args)
#    define fhm_contains(args...) ccc_fhm_contains(args)
#    define fhm_get_key_val(args...) ccc_fhm_get_key_val(args)
#    define fhm_remove_r(args...) ccc_fhm_remove_r(args)
#    define fhm_swap_entry_r(args...) ccc_fhm_swap_entry_r(args)
#    define fhm_try_insert_r(args...) ccc_fhm_try_insert_r(args)
#    define fhm_insert_or_assign_r(args...) ccc_fhm_insert_or_assign_r(args)
#    define fhm_remove_entry_r(args...) ccc_fhm_remove_entry_r(args)
#    define fhm_remove(args...) ccc_fhm_remove(args)
#    define fhm_swap_entry(args...) ccc_fhm_swap_entry(args)
#    define fhm_try_insert(args...) ccc_fhm_try_insert(args)
#    define fhm_insert_or_assign(args...) ccc_fhm_insert_or_assign(args)
#    define fhm_remove_entry(args...) ccc_fhm_remove_entry(args)
#    define fhm_entry_r(args...) ccc_fhm_entry_r(args)
#    define fhm_entry(args...) ccc_fhm_entry(args)
#    define fhm_and_modify(args...) ccc_fhm_and_modify(args)
#    define fhm_and_modify_aux(args...) ccc_fhm_and_modify_aux(args)
#    define fhm_or_insert(args...) ccc_fhm_or_insert(args)
#    define fhm_insert_entry(args...) ccc_fhm_insert_entry(args)
#    define fhm_unwrap(args...) ccc_fhm_unwrap(args)
#    define fhm_occupied(args...) ccc_fhm_occupied(args)
#    define fhm_insert_error(args...) ccc_fhm_insert_error(args)
#    define fhm_begin(args...) ccc_fhm_begin(args)
#    define fhm_next(args...) ccc_fhm_next(args)
#    define fhm_end(args...) ccc_fhm_end(args)
#    define fhm_data(args...) ccc_fhm_data(args)
#    define fhm_is_empty(args...) ccc_fhm_is_empty(args)
#    define fhm_size(args...) ccc_fhm_size(args)
#    define fhm_clear(args...) ccc_fhm_clear(args)
#    define fhm_clear_and_free(args...) ccc_fhm_clear_and_free(args)
#    define fhm_next_prime(args...) ccc_fhm_next_prime(args)
#    define fhm_capacity(args...) ccc_fhm_capacity(args)
#    define fhm_validate(args...) ccc_fhm_validate(args)
#endif

#endif /* CCC_FLAT_HASH_MAP_H */
