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
#ifndef CCC_IMPL_SINGLY_LINKED_LIST_H
#define CCC_IMPL_SINGLY_LINKED_LIST_H

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private A recursive structure for tracking a user element in a singly
linked list. Supports O(1) insert and delete at the front. Elements always have
a valid element to point to in the list due to the user of a sentinel so these
pointers are never NULL if an element is in the list. */
struct CCC_sll_elem
{
    /** @private The next element. Non-null if elem is in list. */
    struct CCC_sll_elem *n;
};

/** @private A singly linked list is a good stack based abstraction for push
and pop to front operations. If the user pre-allocates all the nodes they will
need in a buffer, and manages the slots, this can be an efficient data
structure that can avoid annoyances with maintaining contiguity if one were to
push from the front of a C++ style vector. For a flat container that does
support O(1) push and pop at the front see the flat double ended queue. However,
there are some specific abstractions that someone could hack on top of this
list, either on top of or by hacking on the provided implementation
(non-blocking linked list, concurrent hash table, etc). */
struct CCC_sll
{
    /** @private The sentinel with storage in the actual list struct. */
    struct CCC_sll_elem nil;
    /** @private The number of elements constantly tracked for O(1) check. */
    size_t count;
    /** @private The size in bytes of the type which wraps this handle. */
    size_t sizeof_type;
    /** @private The offset in bytes of the intrusive element in user type. */
    size_t sll_elem_offset;
    /** @private The user provided comparison callback for sorting. */
    CCC_Type_comparison_context_fn *cmp;
    /** @private The user provided allocation function, if any. */
    CCC_Allocator *alloc;
    /** @private User provided auxiliary data, if any. */
    void *aux;
};

/*=========================   Private Interface  ============================*/

/** @private */
void CCC_private_sll_push_front(struct CCC_sll *, struct CCC_sll_elem *);

/*======================   Macro Implementations     ========================*/

/** @private */
#define CCC_private_sll_initialize(private_sll_name, private_struct_name,      \
                                   private_sll_elem_field, private_cmp_fn,     \
                                   private_alloc_fn, private_aux_data)         \
    {                                                                          \
        .nil.n = &(private_sll_name).nil,                                      \
        .sizeof_type = sizeof(private_struct_name),                            \
        .sll_elem_offset                                                       \
        = offsetof(private_struct_name, private_sll_elem_field),               \
        .count = 0,                                                            \
        .alloc = (private_alloc_fn),                                           \
        .cmp = (private_cmp_fn),                                               \
        .aux = (private_aux_data),                                             \
    }

/** @private */
#define CCC_private_sll_emplace_front(list_ptr, struct_initializer...)         \
    (__extension__({                                                           \
        typeof(struct_initializer) *private_sll_res = NULL;                    \
        struct CCC_sll *private_sll = (list_ptr);                              \
        if (private_sll)                                                       \
        {                                                                      \
            if (!private_sll->alloc)                                           \
            {                                                                  \
                private_sll_res = NULL;                                        \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_sll_res                                                \
                    = private_sll->alloc(NULL, private_sll->sizeof_type);      \
                if (private_sll_res)                                           \
                {                                                              \
                    *private_sll_res = struct_initializer;                     \
                    CCC_private_sll_push_front(                                \
                        private_sll,                                           \
                        CCC_sll_elem_in(private_sll, private_sll_res));        \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_sll_res;                                                       \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_SINGLY_LINKED_LIST_H */
