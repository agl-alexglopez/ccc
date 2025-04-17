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
typedef struct ccc_dll_elem_
{
    /** @private The next element. Non-null if elem is in list. */
    struct ccc_dll_elem_ *n_;
    /** @private The previous element. Non-null if elem is in list. */
    struct ccc_dll_elem_ *p_;
} ccc_dll_elem_;

/** @private A doubly linked list with a single sentinel for both head and
tail. The list offers O(1) push, pop, insert, and erase at arbitrary positions
in the list. The sentinel operates as follows to ensure nodes in the list never
point to NULL.

An empty list.

      sentinel
    ┌──────────┐
  ┌>│n=sentinel├──┐
  └─┤p=sentinel│<─┘
    └──────────┘

A list with one element.

         ┌───────────────────┐
         V                   │
      sentinel        A      │
    ┌──────────┐ ┌──────────┐│
    │n=A       ├>│n=sentinel├┘
   ┌┤p=A       │<┤p=sentinel│
   │└──────────┘ └──────────┘
   │                  ^
   └──────────────────┘
A list with three elements.

         ┌─────────────────────────────────────────────┐
         V                                             │
      sentinel         A            B           C      │
    ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐│
    │n=A       ├>│n=B       ├>│n=C       ├>│n=sentinel├┘
   ┌┤p=C       │<┤p=sentinel│<┤p=A       │<┤p=B       │
   │└──────────┘ └──────────┘ └──────────┘ └──────────┘
   │                                            ^
   └────────────────────────────────────────────┘

The single sentinel allows us to use two pointers instead of the four it would
take with a head and tail sentinel. The only cost is slight care for certain
cutting and node clearing steps to ensure the sentinel addresses remain valid */
struct ccc_dll_
{
    /** @private The sentinel with storage in the actual list struct. */
    struct ccc_dll_elem_ sentinel_;
    /** @private The size in bytes of the type which wraps this handle. */
    size_t elem_sz_;
    /** @private The offset in bytes of the intrusive element in user type. */
    size_t dll_elem_offset_;
    /** @private The number of elements constantly tracked for O(1) check. */
    size_t sz_;
    /** @private The user provided comparison callback for sorting. */
    ccc_any_type_cmp_fn *cmp_;
    /** @private The user provided allocation function, if any. */
    ccc_any_alloc_fn *alloc_;
    /** @private User provided auxiliary data, if any. */
    void *aux_;
};

/*=======================     Private Interface   ===========================*/

/** @private */
void ccc_impl_dll_push_back(struct ccc_dll_ *, struct ccc_dll_elem_ *);
/** @private */
void ccc_impl_dll_push_front(struct ccc_dll_ *, struct ccc_dll_elem_ *);
/** @private */
struct ccc_dll_elem_ *ccc_impl_dll_elem_in(struct ccc_dll_ const *,
                                           void const *any_struct);

/*=======================     Macro Implementations   =======================*/

/** @private */
#define ccc_impl_dll_init(dll_name, struct_name, dll_elem_field, cmp_fn,       \
                          alloc_fn, aux_data)                                  \
    {                                                                          \
        .sentinel_.n_ = &(dll_name).sentinel_,                                 \
        .sentinel_.p_ = &(dll_name).sentinel_,                                 \
        .elem_sz_ = sizeof(struct_name),                                       \
        .dll_elem_offset_ = offsetof(struct_name, dll_elem_field),             \
        .sz_ = 0,                                                              \
        .alloc_ = (alloc_fn),                                                  \
        .cmp_ = (cmp_fn),                                                      \
        .aux_ = (aux_data),                                                    \
    }

/** @private */
#define ccc_impl_dll_emplace_back(dll_ptr, struct_initializer...)              \
    (__extension__({                                                           \
        typeof(struct_initializer) *dll_res_ = NULL;                           \
        struct ccc_dll_ *dll_ = (dll_ptr);                                     \
        if (dll_)                                                              \
        {                                                                      \
            if (dll_->alloc_)                                                  \
            {                                                                  \
                dll_res_ = dll_->alloc_(NULL, dll_->elem_sz_, dll_->aux_);     \
                if (dll_res_)                                                  \
                {                                                              \
                    *dll_res_ = (typeof(*dll_res_))struct_initializer;         \
                    ccc_impl_dll_push_back(                                    \
                        dll_, ccc_impl_dll_elem_in(dll_, dll_res_));           \
                }                                                              \
            }                                                                  \
        }                                                                      \
        dll_res_;                                                              \
    }))

/** @private */
#define ccc_impl_dll_emplace_front(dll_ptr, struct_initializer...)             \
    (__extension__({                                                           \
        typeof(struct_initializer) *dll_res_;                                  \
        struct ccc_dll_ *dll_ = (dll_ptr);                                     \
        assert(sizeof(*dll_res_) == dll_->elem_sz_);                           \
        if (!dll_->alloc_)                                                     \
        {                                                                      \
            dll_res_ = NULL;                                                   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            dll_res_ = dll_->alloc_(NULL, dll_->elem_sz_, dll_->aux_);         \
            if (dll_res_)                                                      \
            {                                                                  \
                *dll_res_ = struct_initializer;                                \
                ccc_impl_dll_push_front(dll_,                                  \
                                        ccc_impl_dll_elem_in(dll_, dll_res_)); \
            }                                                                  \
        }                                                                      \
        dll_res_;                                                              \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_DOUBLY_LINKED_LIST_H */
