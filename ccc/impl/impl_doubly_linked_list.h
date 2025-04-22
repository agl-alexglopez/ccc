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
#ifndef CCC_IMPL_DOUBLY_LINKED_LIST_H
#define CCC_IMPL_DOUBLY_LINKED_LIST_H

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private A recursive structure for tracking a user element in a doubly
linked list. Supports O(1) insert and delete at the front, back, or any
arbitrary position in the list. Elements always have a valid element to point
to in the list due to the user of a sentinel so these pointers are never NULL
if an element is in the list. */
struct ccc_dll_elem
{
    /** @private The next element. Non-null if elem is in list. */
    struct ccc_dll_elem *n;
    /** @private The previous element. Non-null if elem is in list. */
    struct ccc_dll_elem *p;
};

/** @private A doubly linked list with a single sentinel for both head and
tail. The list offers O(1) push, pop, insert, and erase at arbitrary positions
in the list. The sentinel (nil) operates as follows to ensure nodes in the list
never point to NULL.

An empty list.

      nil
    ┌─────┐
  ┌>│n=nil├──┐
  └─┤p=nil│<─┘
    └─────┘

A list with one element.

         ┌─────────┐
         V         │
      nil      A   │
    ┌─────┐ ┌─────┐│
    │n=A  ├>│n=nil├┘
   ┌┤p=A  │<┤p=nil│
   │└─────┘ └─────┘
   │           ^
   └───────────┘
A list with three elements.

       ┌───────────────────────────┐
       V                           │
      nil      A       B       C   │
    ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐│
    │n=A  ├>│n=B  ├>│n=C  ├>│n=nil├┘
   ┌┤p=C  │<┤p=nil│<┤p=A  │<┤p=B  │
   │└─────┘ └─────┘ └─────┘ └─────┘
   │                           ^
   └───────────────────────────┘

The single nil allows us to use two pointers instead of the four it would
take with a head and tail nil. The only cost is slight care for certain
cutting and node clearing steps to ensure the nil addresses remain valid */
struct ccc_dll
{
    /** @private The sentinel with storage in the actual list struct. */
    struct ccc_dll_elem nil;
    /** @private The number of elements constantly tracked for O(1) check. */
    size_t count;
    /** @private The size in bytes of the type which wraps this handle. */
    size_t sizeof_type;
    /** @private The offset in bytes of the intrusive element in user type. */
    size_t dll_elem_offset;
    /** @private The user provided comparison callback for sorting. */
    ccc_any_type_cmp_fn *cmp;
    /** @private The user provided allocation function, if any. */
    ccc_any_alloc_fn *alloc;
    /** @private User provided auxiliary data, if any. */
    void *aux;
};

/*=======================     Private Interface   ===========================*/

/** @private */
void ccc_impl_dll_push_back(struct ccc_dll *, struct ccc_dll_elem *);
/** @private */
void ccc_impl_dll_push_front(struct ccc_dll *, struct ccc_dll_elem *);
/** @private */
struct ccc_dll_elem *ccc_impl_dll_elem_in(struct ccc_dll const *,
                                          void const *any_struct);

/*=======================     Macro Implementations   =======================*/

/** @private */
#define ccc_impl_dll_init(impl_dll_name, impl_struct_name,                     \
                          impl_dll_elem_field, impl_cmp_fn, impl_alloc_fn,     \
                          impl_aux_data)                                       \
    {                                                                          \
        .nil.n = &(impl_dll_name).nil,                                         \
        .nil.p = &(impl_dll_name).nil,                                         \
        .sizeof_type = sizeof(impl_struct_name),                               \
        .dll_elem_offset = offsetof(impl_struct_name, impl_dll_elem_field),    \
        .count = 0,                                                            \
        .alloc = (impl_alloc_fn),                                              \
        .cmp = (impl_cmp_fn),                                                  \
        .aux = (impl_aux_data),                                                \
    }

/** @private */
#define ccc_impl_dll_emplace_back(dll_ptr, struct_initializer...)              \
    (__extension__({                                                           \
        typeof(struct_initializer) *impl_dll_res = NULL;                       \
        struct ccc_dll *impl_dll = (dll_ptr);                                  \
        if (impl_dll)                                                          \
        {                                                                      \
            if (impl_dll->alloc)                                               \
            {                                                                  \
                impl_dll_res = impl_dll->alloc(NULL, impl_dll->sizeof_type,    \
                                               impl_dll->aux);                 \
                if (impl_dll_res)                                              \
                {                                                              \
                    *impl_dll_res = (typeof(*impl_dll_res))struct_initializer; \
                    ccc_impl_dll_push_back(                                    \
                        impl_dll,                                              \
                        ccc_impl_dll_elem_in(impl_dll, impl_dll_res));         \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_dll_res;                                                          \
    }))

/** @private */
#define ccc_impl_dll_emplace_front(dll_ptr, struct_initializer...)             \
    (__extension__({                                                           \
        typeof(struct_initializer) *impl_dll_res = NULL;                       \
        struct ccc_dll *impl_dll = (dll_ptr);                                  \
        if (!impl_dll->alloc)                                                  \
        {                                                                      \
            impl_dll_res = NULL;                                               \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            impl_dll_res                                                       \
                = impl_dll->alloc(NULL, impl_dll->sizeof_type, impl_dll->aux); \
            if (impl_dll_res)                                                  \
            {                                                                  \
                *impl_dll_res = struct_initializer;                            \
                ccc_impl_dll_push_front(                                       \
                    impl_dll, ccc_impl_dll_elem_in(impl_dll, impl_dll_res));   \
            }                                                                  \
        }                                                                      \
        impl_dll_res;                                                          \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_DOUBLY_LINKED_LIST_H */
