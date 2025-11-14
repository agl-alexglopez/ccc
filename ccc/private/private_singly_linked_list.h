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
#ifndef CCC_PRIVATE_SINGLY_LINKED_LIST_H
#define CCC_PRIVATE_SINGLY_LINKED_LIST_H

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
struct CCC_Singly_linked_list_node
{
    /** @private The next element. Non-null if elem is in list. */
    struct CCC_Singly_linked_list_node *n;
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
struct CCC_Singly_linked_list
{
    /** @private The sentinel with storage in the actual list struct. */
    struct CCC_Singly_linked_list_node nil;
    /** @private The number of elements constantly tracked for O(1) check. */
    size_t count;
    /** @private The size in bytes of the type which wraps this handle. */
    size_t sizeof_type;
    /** @private The offset in bytes of the intrusive element in user type. */
    size_t singly_linked_list_node_offset;
    /** @private The user provided comparison callback for sorting. */
    CCC_Type_comparator *compare;
    /** @private The user provided allocation function, if any. */
    CCC_Allocator *allocate;
    /** @private User provided context data, if any. */
    void *context;
};

/*=========================   Private Interface  ============================*/

/** @private */
void
CCC_private_singly_linked_list_push_front(struct CCC_Singly_linked_list *,
                                          struct CCC_Singly_linked_list_node *);

/*======================   Macro Implementations     ========================*/

/** @private */
#define CCC_private_singly_linked_list_initialize(                             \
    private_singly_linked_list_name, private_struct_name,                      \
    private_singly_linked_list_node_field, private_compare_fn,                 \
    private_allocate, private_context_data)                                    \
    {                                                                          \
        .nil.n = &(private_singly_linked_list_name).nil,                       \
        .sizeof_type = sizeof(private_struct_name),                            \
        .singly_linked_list_node_offset = offsetof(                            \
            private_struct_name, private_singly_linked_list_node_field),       \
        .count = 0,                                                            \
        .allocate = (private_allocate),                                        \
        .compare = (private_compare_fn),                                       \
        .context = (private_context_data),                                     \
    }

/** @private */
#define CCC_private_singly_linked_list_emplace_front(list_pointer,             \
                                                     struct_initializer...)    \
    (__extension__({                                                           \
        typeof(struct_initializer) *private_singly_linked_list_res = NULL;     \
        struct CCC_Singly_linked_list *private_singly_linked_list              \
            = (list_pointer);                                                  \
        if (private_singly_linked_list)                                        \
        {                                                                      \
            if (!private_singly_linked_list->allocate)                         \
            {                                                                  \
                private_singly_linked_list_res = NULL;                         \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_singly_linked_list_res                                 \
                    = private_singly_linked_list->allocate(                    \
                        (CCC_Allocator_context){                               \
                            .input = NULL,                                     \
                            .bytes = private_singly_linked_list->sizeof_type,  \
                            .context = private_singly_linked_list->context,    \
                        });                                                    \
                if (private_singly_linked_list_res)                            \
                {                                                              \
                    *private_singly_linked_list_res = struct_initializer;      \
                    CCC_private_singly_linked_list_push_front(                 \
                        private_singly_linked_list,                            \
                        CCC_singly_linked_list_node_in(                        \
                            private_singly_linked_list,                        \
                            private_singly_linked_list_res));                  \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_singly_linked_list_res;                                        \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_SINGLY_LINKED_LIST_H */
