/** @cond
Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
@endcond */
#ifndef CCC_IMPL_HANDLE_ORDERED_MAP_H
#define CCC_IMPL_HANDLE_ORDERED_MAP_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../types.h"
#include "impl_types.h" /* NOLINT */

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private Runs the top down splay tree algorithm with the addition of a free
list for providing new nodes within the buffer. The parent field normally
tracks parent when in the tree for iteration purposes. When a node is removed
from the tree it is added to the free singly linked list. The free list is a
LIFO push to front stack. */
struct ccc_homap_elem
{
    /** @private Child nodes in array to unify Left and Right. */
    size_t branch[2];
    union
    {
        /** @private Parent of splay tree node when allocated. */
        size_t parent;
        /** @private Points to next free when not allocated. */
        size_t next_free;
    };
};

/** @private A ordered map providing handle stability. Once elements are
inserted into the map they will not move slots even when the array resizes. The
free slots are tracked in a singly linked list that uses indices instead of
pointers so that it remains valid even when the table resizes. The 0th index of
the array is sacrificed for some coding simplicity and falsey 0. */
struct ccc_homap
{
    /** @private The contiguous array of user data. */
    void *data;
    /** @private The contiguous array of WAVL tree meta data. */
    struct ccc_homap_elem *nodes;
    /** @private The current capacity. */
    size_t capacity;
    /** @private The current size. */
    size_t count;
    /** @private The root node of the Splay Tree. */
    size_t root;
    /** @private The start of the free singly linked list. */
    size_t free_list;
    /** @private The size of the type stored in the map. */
    size_t sizeof_type;
    /** @private Where user key can be found in type. */
    size_t key_offset;
    /** @private The provided key comparison function. */
    ccc_any_key_cmp_fn *cmp;
    /** @private The provided allocation function, if any. */
    ccc_any_alloc_fn *alloc;
    /** @private The provided auxiliary data, if any. */
    void *aux;
};

/** @private A handle is like an entry but if the handle is Occupied, we can
guarantee the user that their element will not move from the provided index. */
struct ccc_htree_handle
{
    /** @private Map associated with this handle. */
    struct ccc_homap *hom;
    /** @private Current index of the handle. */
    size_t i;
    /** @private Saves last comparison direction. */
    ccc_threeway_cmp last_cmp;
    /** @private The entry status flag. */
    ccc_entry_status stats;
};

/** @private Enable return by compound literal reference on the stack. Think
of this method as return by value but with the additional ability to pass by
pointer in a functional style. `fnB(&(union ccc_homap_handle){fnA().impl});` */
union ccc_homap_handle
{
    /** @private The field containing the handle struct. */
    struct ccc_htree_handle impl;
};

/*===========================   Private Interface ===========================*/

/** @private */
void ccc_impl_hom_insert(struct ccc_homap *hom, size_t elem_i);
/** @private */
struct ccc_htree_handle ccc_impl_hom_handle(struct ccc_homap *hom,
                                            void const *key);
/** @private */
void *ccc_impl_hom_data_at(struct ccc_homap const *hom, size_t slot);
/** @private */
void *ccc_impl_hom_key_at(struct ccc_homap const *hom, size_t slot);
/** @private */
struct ccc_homap_elem *ccc_impl_homap_elem_at(struct ccc_homap const *hom,
                                              size_t slot);
/** @private */
size_t ccc_impl_hom_alloc_slot(struct ccc_homap *hom);

/*========================     Initialization       =========================*/

/** @private The user can declare a fixed size realtime ordered map with the
help of static asserts to ensure the layout is compatible with our internal
metadata. */
#define ccc_impl_hom_declare_fixed_map(impl_fixed_map_type_name,               \
                                       impl_key_val_type_name, impl_capacity)  \
    static_assert((impl_capacity) > 1,                                         \
                  "fixed size map must have capacity greater than 1");         \
    typedef struct                                                             \
    {                                                                          \
        impl_key_val_type_name data[(impl_capacity)];                          \
        struct ccc_homap_elem nodes[(impl_capacity)];                          \
    }(impl_fixed_map_type_name)

/** @private Taking the size of the array actually works here because the field
is of a known fixed size defined at compile time, not just a pointer. */
#define ccc_impl_hom_fixed_capacity(fixed_map_type_name)                       \
    (sizeof((fixed_map_type_name){}.nodes) / sizeof(struct ccc_homap_elem))

/** @private */
#define ccc_impl_hom_init(impl_memory_ptr, impl_type_name,                     \
                          impl_key_elem_field, impl_key_cmp_fn, impl_alloc_fn, \
                          impl_aux_data, impl_capacity)                        \
    {                                                                          \
        .data = (impl_memory_ptr),                                             \
        .nodes = NULL,                                                         \
        .capacity = (impl_capacity),                                           \
        .count = 0,                                                            \
        .root = 0,                                                             \
        .free_list = 0,                                                        \
        .sizeof_type = sizeof(impl_type_name),                                 \
        .key_offset = offsetof(impl_type_name, impl_key_elem_field),           \
        .cmp = (impl_key_cmp_fn),                                              \
        .alloc = (impl_alloc_fn),                                              \
        .aux = (impl_aux_data),                                                \
    }

/** @private */
#define ccc_impl_hom_as(handle_ordered_map_ptr, type_name, handle...)          \
    ((type_name *)ccc_impl_hom_data_at((handle_ordered_map_ptr), (handle)))

/*==================     Core Macro Implementations     =====================*/

/** @private */
#define ccc_impl_hom_and_modify_w(handle_ordered_map_handle_ptr, type_name,    \
                                  closure_over_T...)                           \
    (__extension__({                                                           \
        __auto_type impl_hom_mod_hndl_ptr = (handle_ordered_map_handle_ptr);   \
        struct ccc_htree_handle impl_hom_mod_hndl                              \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_hom_mod_hndl_ptr)                                             \
        {                                                                      \
            impl_hom_mod_hndl = impl_hom_mod_hndl_ptr->impl;                   \
            if (impl_hom_mod_hndl.stats & CCC_ENTRY_OCCUPIED)                  \
            {                                                                  \
                type_name *const T = ccc_impl_hom_data_at(                     \
                    impl_hom_mod_hndl.hom, impl_hom_mod_hndl.i);               \
                if (T)                                                         \
                {                                                              \
                    closure_over_T                                             \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_hom_mod_hndl;                                                     \
    }))

/** @private */
#define ccc_impl_hom_or_insert_w(handle_ordered_map_handle_ptr,                \
                                 lazy_key_value...)                            \
    (__extension__({                                                           \
        __auto_type impl_hom_or_ins_hndl_ptr                                   \
            = (handle_ordered_map_handle_ptr);                                 \
        ccc_handle_i impl_hom_or_ins_ret = 0;                                  \
        if (impl_hom_or_ins_hndl_ptr)                                          \
        {                                                                      \
            struct ccc_htree_handle *impl_hom_or_ins_hndl                      \
                = &impl_hom_or_ins_hndl_ptr->impl;                             \
            if (impl_hom_or_ins_hndl->stats == CCC_ENTRY_OCCUPIED)             \
            {                                                                  \
                impl_hom_or_ins_ret = impl_hom_or_ins_hndl->i;                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_hom_or_ins_ret                                            \
                    = ccc_impl_hom_alloc_slot(impl_hom_or_ins_hndl->hom);      \
                if (impl_hom_or_ins_ret)                                       \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_impl_hom_data_at(          \
                        impl_hom_or_ins_hndl->hom, impl_hom_or_ins_ret))       \
                        = lazy_key_value;                                      \
                    ccc_impl_hom_insert(impl_hom_or_ins_hndl->hom,             \
                                        impl_hom_or_ins_ret);                  \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_hom_or_ins_ret;                                                   \
    }))

/** @private */
#define ccc_impl_hom_insert_handle_w(handle_ordered_map_handle_ptr,            \
                                     lazy_key_value...)                        \
    (__extension__({                                                           \
        __auto_type impl_hom_ins_hndl_ptr = (handle_ordered_map_handle_ptr);   \
        ccc_handle_i impl_hom_ins_hndl_ret = 0;                                \
        if (impl_hom_ins_hndl_ptr)                                             \
        {                                                                      \
            struct ccc_htree_handle *impl_hom_ins_hndl                         \
                = &impl_hom_ins_hndl_ptr->impl;                                \
            if (!(impl_hom_ins_hndl->stats & CCC_ENTRY_OCCUPIED))              \
            {                                                                  \
                impl_hom_ins_hndl_ret                                          \
                    = ccc_impl_hom_alloc_slot(impl_hom_ins_hndl->hom);         \
                if (impl_hom_ins_hndl_ret)                                     \
                {                                                              \
                    *((typeof(lazy_key_value) *)ccc_impl_hom_data_at(          \
                        impl_hom_ins_hndl->hom, impl_hom_ins_hndl_ret))        \
                        = lazy_key_value;                                      \
                    ccc_impl_hom_insert(impl_hom_ins_hndl->hom,                \
                                        impl_hom_ins_hndl_ret);                \
                }                                                              \
            }                                                                  \
            else if (impl_hom_ins_hndl->stats == CCC_ENTRY_OCCUPIED)           \
            {                                                                  \
                *((typeof(lazy_key_value) *)ccc_impl_hom_data_at(              \
                    impl_hom_ins_hndl->hom, impl_hom_ins_hndl->i))             \
                    = lazy_key_value;                                          \
            }                                                                  \
        }                                                                      \
        impl_hom_ins_hndl_ret;                                                 \
    }))

/** @private */
#define ccc_impl_hom_try_insert_w(handle_ordered_map_ptr, key, lazy_value...)  \
    (__extension__({                                                           \
        __auto_type impl_hom_try_ins_map_ptr = (handle_ordered_map_ptr);       \
        struct ccc_handl impl_hom_try_ins_hndl_ret                             \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_hom_try_ins_map_ptr)                                          \
        {                                                                      \
            __auto_type impl_hom_key = (key);                                  \
            struct ccc_htree_handle impl_hom_try_ins_hndl                      \
                = ccc_impl_hom_handle(impl_hom_try_ins_map_ptr,                \
                                      (void *)&impl_hom_key);                  \
            if (!(impl_hom_try_ins_hndl.stats & CCC_ENTRY_OCCUPIED))           \
            {                                                                  \
                impl_hom_try_ins_hndl_ret = (struct ccc_handl){                \
                    .i = ccc_impl_hom_alloc_slot(impl_hom_try_ins_hndl.hom),   \
                    .stats = CCC_ENTRY_INSERT_ERROR};                          \
                if (impl_hom_try_ins_hndl_ret.i)                               \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_impl_hom_data_at(              \
                        impl_hom_try_ins_map_ptr,                              \
                        impl_hom_try_ins_hndl_ret.i))                          \
                        = lazy_value;                                          \
                    *((typeof(impl_hom_key) *)ccc_impl_hom_key_at(             \
                        impl_hom_try_ins_hndl.hom,                             \
                        impl_hom_try_ins_hndl_ret.i))                          \
                        = impl_hom_key;                                        \
                    ccc_impl_hom_insert(impl_hom_try_ins_hndl.hom,             \
                                        impl_hom_try_ins_hndl_ret.i);          \
                    impl_hom_try_ins_hndl_ret.stats = CCC_ENTRY_VACANT;        \
                }                                                              \
            }                                                                  \
            else if (impl_hom_try_ins_hndl.stats == CCC_ENTRY_OCCUPIED)        \
            {                                                                  \
                impl_hom_try_ins_hndl_ret = (struct ccc_handl){                \
                    .i = impl_hom_try_ins_hndl.i,                              \
                    .stats = impl_hom_try_ins_hndl.stats};                     \
            }                                                                  \
        }                                                                      \
        impl_hom_try_ins_hndl_ret;                                             \
    }))

/** @private */
#define ccc_impl_hom_insert_or_assign_w(handle_ordered_map_ptr, key,           \
                                        lazy_value...)                         \
    (__extension__({                                                           \
        __auto_type impl_hom_ins_or_assign_map_ptr = (handle_ordered_map_ptr); \
        struct ccc_handl impl_hom_ins_or_assign_hndl_ret                       \
            = {.stats = CCC_ENTRY_ARG_ERROR};                                  \
        if (impl_hom_ins_or_assign_map_ptr)                                    \
        {                                                                      \
            __auto_type impl_hom_key = (key);                                  \
            struct ccc_htree_handle impl_hom_ins_or_assign_hndl                \
                = ccc_impl_hom_handle(impl_hom_ins_or_assign_map_ptr,          \
                                      (void *)&impl_hom_key);                  \
            if (!(impl_hom_ins_or_assign_hndl.stats & CCC_ENTRY_OCCUPIED))     \
            {                                                                  \
                impl_hom_ins_or_assign_hndl_ret                                \
                    = (struct ccc_handl){.i = ccc_impl_hom_alloc_slot(         \
                                             impl_hom_ins_or_assign_hndl.hom), \
                                         .stats = CCC_ENTRY_INSERT_ERROR};     \
                if (impl_hom_ins_or_assign_hndl_ret.i)                         \
                {                                                              \
                    *((typeof(lazy_value) *)ccc_impl_hom_data_at(              \
                        impl_hom_ins_or_assign_map_ptr,                        \
                        impl_hom_ins_or_assign_hndl_ret.i))                    \
                        = lazy_value;                                          \
                    *((typeof(impl_hom_key) *)ccc_impl_hom_key_at(             \
                        impl_hom_ins_or_assign_hndl.hom,                       \
                        impl_hom_ins_or_assign_hndl_ret.i))                    \
                        = impl_hom_key;                                        \
                    ccc_impl_hom_insert(impl_hom_ins_or_assign_hndl.hom,       \
                                        impl_hom_ins_or_assign_hndl_ret.i);    \
                    impl_hom_ins_or_assign_hndl_ret.stats = CCC_ENTRY_VACANT;  \
                }                                                              \
            }                                                                  \
            else if (impl_hom_ins_or_assign_hndl.stats == CCC_ENTRY_OCCUPIED)  \
            {                                                                  \
                *((typeof(lazy_value) *)ccc_impl_hom_data_at(                  \
                    impl_hom_ins_or_assign_hndl.hom,                           \
                    impl_hom_ins_or_assign_hndl.i))                            \
                    = lazy_value;                                              \
                impl_hom_ins_or_assign_hndl_ret = (struct ccc_handl){          \
                    .i = impl_hom_ins_or_assign_hndl.i,                        \
                    .stats = impl_hom_ins_or_assign_hndl.stats};               \
                *((typeof(impl_hom_key) *)ccc_impl_hom_key_at(                 \
                    impl_hom_ins_or_assign_hndl.hom,                           \
                    impl_hom_ins_or_assign_hndl.i))                            \
                    = impl_hom_key;                                            \
            }                                                                  \
        }                                                                      \
        impl_hom_ins_or_assign_hndl_ret;                                       \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_HANDLE_ORDERED_MAP_H */
