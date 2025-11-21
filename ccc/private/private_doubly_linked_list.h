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
#ifndef CCC_PRIVATE_DOUBLY_LINKED_LIST_H
#define CCC_PRIVATE_DOUBLY_LINKED_LIST_H

/** @cond */
#include <assert.h>
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @internal A recursive structure for tracking a user element in a doubly
linked list. Supports O(1) insert and delete at the front, back, or any
arbitrary position in the list. Elements always have a valid element to point
to in the list due to the user of a sentinel so these pointers are never NULL
if an element is in the list. */
struct CCC_Doubly_linked_list_node
{
    /** @internal The next element. Non-null if elem is in list. */
    struct CCC_Doubly_linked_list_node *next;
    /** @internal The previous element. Non-null if elem is in list. */
    struct CCC_Doubly_linked_list_node *previous;
};

/** @internal A doubly linked list storing head and tail pointers. The list
supports O(1) push, pop, insert, and erase at arbitrary positions.

Sentinel-based lists are common in C++ and other languages, but they are
problematic in standard C. A sentinel stored inside the list struct requires
a stable address. However, by default C copies struct by value, and such copies
would also copy the sentinel node. For example, if the user writes a simple
helper function that initializes the list with some extra constructor-like
actions and returns it to the caller a copy occurs. The sentinel next and
previous pointers would then point into the old stack frame, producing immediate
undefined behavior once the original struct goes out of scope.

Because C has no constructors, destructors, or move semantics, the API
cannot prevent users from copying the list structure. Therefore a sentinel
embedded in the list cannot be used safely. To ensure correct behavior under
normal C copy semantics, this implementation uses NULL head and tail pointers to
represent an empty list, and performs the required NULL checks when modifying
nodes.

This design also improves composability in multithreaded programs. Because the
list does not rely on a static global sentinel either, each thread may create
its own independent list (e.g., inside a per-thread memory arena or thread
stack frame) without hidden static state. The user can pass in context upon
initialization for these more advanced use cases.

Note that the list itself is not thread-safe; external synchronization is still
required if multiple threads access the same list instance. */
struct CCC_Doubly_linked_list
{
    /** @internal Pointer to the head element or NULL if list empty. */
    struct CCC_Doubly_linked_list_node *head;
    /** @internal Pointer to the tail element or NULL if list empty. */
    struct CCC_Doubly_linked_list_node *tail;
    /** @internal The number of elements constantly tracked for O(1) check. */
    size_t count;
    /** @internal The size in bytes of the type which wraps this handle. */
    size_t sizeof_type;
    /** @internal The offset in bytes of the intrusive element in user type. */
    size_t type_intruder_offset;
    /** @internal The user provided comparison callback for sorting. */
    CCC_Type_comparator *compare;
    /** @internal The user provided allocation function, if any. */
    CCC_Allocator *allocate;
    /** @internal User provided context data, if any. */
    void *context;
};

/*=======================     Private Interface   ===========================*/

/** @internal */
void
CCC_private_doubly_linked_list_push_back(struct CCC_Doubly_linked_list *,
                                         struct CCC_Doubly_linked_list_node *);
/** @internal */
void
CCC_private_doubly_linked_list_push_front(struct CCC_Doubly_linked_list *,
                                          struct CCC_Doubly_linked_list_node *);
/** @internal */
struct CCC_Doubly_linked_list_node *
CCC_private_doubly_linked_list_node_in(struct CCC_Doubly_linked_list const *,
                                       void const *any_struct);

/*=======================     Macro Implementations   =======================*/

/** @internal Initialization at compile time is allowed in C due to the provided
name of the list being on the left hand side of the assignment operator. */
#define CCC_private_doubly_linked_list_initialize(                             \
    private_doubly_linked_list_name, private_struct_name,                      \
    private_doubly_linked_list_node_field, private_compare_fn,                 \
    private_allocate, private_context_data)                                    \
    {                                                                          \
        .head = NULL,                                                          \
        .tail = NULL,                                                          \
        .sizeof_type = sizeof(private_struct_name),                            \
        .type_intruder_offset = offsetof(                                      \
            private_struct_name, private_doubly_linked_list_node_field),       \
        .count = 0,                                                            \
        .allocate = (private_allocate),                                        \
        .compare = (private_compare_fn),                                       \
        .context = (private_context_data),                                     \
    }

/** @internal */
#define CCC_private_doubly_linked_list_emplace_back(                           \
    doubly_linked_list_pointer, struct_initializer...)                         \
    (__extension__({                                                           \
        typeof(struct_initializer) *private_doubly_linked_list_res = NULL;     \
        struct CCC_Doubly_linked_list *private_doubly_linked_list              \
            = (doubly_linked_list_pointer);                                    \
        if (private_doubly_linked_list)                                        \
        {                                                                      \
            if (private_doubly_linked_list->allocate)                          \
            {                                                                  \
                private_doubly_linked_list_res                                 \
                    = private_doubly_linked_list->allocate(                    \
                        (CCC_Allocator_context){                               \
                            .input = NULL,                                     \
                            .bytes = private_doubly_linked_list->sizeof_type,  \
                            .context = private_doubly_linked_list->context,    \
                        });                                                    \
                if (private_doubly_linked_list_res)                            \
                {                                                              \
                    *private_doubly_linked_list_res = struct_initializer;      \
                    CCC_private_doubly_linked_list_push_back(                  \
                        private_doubly_linked_list,                            \
                        CCC_private_doubly_linked_list_node_in(                \
                            private_doubly_linked_list,                        \
                            private_doubly_linked_list_res));                  \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_doubly_linked_list_res;                                        \
    }))

/** @internal */
#define CCC_private_doubly_linked_list_emplace_front(                          \
    doubly_linked_list_pointer, struct_initializer...)                         \
    (__extension__({                                                           \
        typeof(struct_initializer) *private_doubly_linked_list_res = NULL;     \
        struct CCC_Doubly_linked_list *private_doubly_linked_list              \
            = (doubly_linked_list_pointer);                                    \
        if (!private_doubly_linked_list->allocate)                             \
        {                                                                      \
            private_doubly_linked_list_res = NULL;                             \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            private_doubly_linked_list_res                                     \
                = private_doubly_linked_list->allocate(                        \
                    (CCC_Allocator_context){                                   \
                        .input = NULL,                                         \
                        .bytes = private_doubly_linked_list->sizeof_type,      \
                        .context = private_doubly_linked_list->context,        \
                    });                                                        \
            if (private_doubly_linked_list_res)                                \
            {                                                                  \
                *private_doubly_linked_list_res = struct_initializer;          \
                CCC_private_doubly_linked_list_push_front(                     \
                    private_doubly_linked_list,                                \
                    CCC_private_doubly_linked_list_node_in(                    \
                        private_doubly_linked_list,                            \
                        private_doubly_linked_list_res));                      \
            }                                                                  \
        }                                                                      \
        private_doubly_linked_list_res;                                        \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_DOUBLY_LINKED_LIST_H */
