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
struct CCC_Doubly_linked_list_node
{
    /** @private The next element. Non-null if elem is in list. */
    struct CCC_Doubly_linked_list_node *n;
    /** @private The previous element. Non-null if elem is in list. */
    struct CCC_Doubly_linked_list_node *p;
};

/** @private A doubly linked list with a single sentinel for both head and
tail. The list offers O(1) push, pop, insert, and erase at arbitrary positions
in the list. The sentinel (nil) operates as follows to ensure nodes in the list
never point to NULL.

An empty list.

```
    nil
  ┌─────┐
┌>│n=nil├──┐
└─┤p=nil│<─┘
  └─────┘
```

A list with one element.

```
    ┌───────────┐
    V           │
   nil      A   │
 ┌─────┐ ┌─────┐│
 │n=A  ├>│n=nil├┘
┌┤p=A  │<┤p=nil│
│└─────┘ └─────┘
│           ^
└───────────┘
```

A list with three elements.

```
    ┌───────────────────────────┐
    V                           │
   nil      A       B       C   │
 ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐│
 │n=A  ├>│n=B  ├>│n=C  ├>│n=nil├┘
┌┤p=C  │<┤p=nil│<┤p=A  │<┤p=B  │
│└─────┘ └─────┘ └─────┘ └─────┘
│                           ^
└───────────────────────────┘
```

The single nil allows us to use two pointers instead of the four it would
take with a head and tail nil. The only cost is slight care for certain
cutting and node clearing steps to ensure the nil addresses remain valid */
struct CCC_Doubly_linked_list
{
    /** @private The sentinel with storage in the actual list struct. */
    struct CCC_Doubly_linked_list_node nil;
    /** @private The number of elements constantly tracked for O(1) check. */
    size_t count;
    /** @private The size in bytes of the type which wraps this handle. */
    size_t sizeof_type;
    /** @private The offset in bytes of the intrusive element in user type. */
    size_t doubly_linked_list_node_offset;
    /** @private The user provided comparison callback for sorting. */
    CCC_Type_comparator_context *cmp;
    /** @private The user provided allocation function, if any. */
    CCC_Allocator *alloc;
    /** @private User provided context data, if any. */
    void *context;
};

/*=======================     Private Interface   ===========================*/

/** @private */
void
CCC_private_doubly_linked_list_push_back(struct CCC_Doubly_linked_list *,
                                         struct CCC_Doubly_linked_list_node *);
/** @private */
void
CCC_private_doubly_linked_list_push_front(struct CCC_Doubly_linked_list *,
                                          struct CCC_Doubly_linked_list_node *);
/** @private */
struct CCC_Doubly_linked_list_node *
CCC_private_doubly_linked_list_node_in(struct CCC_Doubly_linked_list const *,
                                       void const *any_struct);

/*=======================     Macro Implementations   =======================*/

/** @private Initialization at compile time is allowed in C due to the provided
name of the list being on the left hand side of the assignment operator. */
#define CCC_private_doubly_linked_list_initialize(                             \
    private_doubly_linked_list_name, private_struct_name,                      \
    private_doubly_linked_list_node_field, private_cmp_fn, private_alloc_fn,   \
    private_context_data)                                                      \
    {                                                                          \
        .nil.n = &(private_doubly_linked_list_name).nil,                       \
        .nil.p = &(private_doubly_linked_list_name).nil,                       \
        .sizeof_type = sizeof(private_struct_name),                            \
        .doubly_linked_list_node_offset = offsetof(                            \
            private_struct_name, private_doubly_linked_list_node_field),       \
        .count = 0,                                                            \
        .alloc = (private_alloc_fn),                                           \
        .cmp = (private_cmp_fn),                                               \
        .context = (private_context_data),                                     \
    }

/** @private */
#define CCC_private_doubly_linked_list_emplace_back(doubly_linked_list_ptr,    \
                                                    struct_initializer...)     \
    (__extension__({                                                           \
        typeof(struct_initializer) *private_doubly_linked_list_res = NULL;     \
        struct CCC_Doubly_linked_list *private_doubly_linked_list              \
            = (doubly_linked_list_ptr);                                        \
        if (private_doubly_linked_list)                                        \
        {                                                                      \
            if (private_doubly_linked_list->alloc)                             \
            {                                                                  \
                private_doubly_linked_list_res                                 \
                    = private_doubly_linked_list->alloc(                       \
                        NULL, private_doubly_linked_list->sizeof_type,         \
                        private_doubly_linked_list->context);                  \
                if (private_doubly_linked_list_res)                            \
                {                                                              \
                    *private_doubly_linked_list_res                            \
                        = (typeof(*private_doubly_linked_list_res))            \
                            struct_initializer;                                \
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

/** @private */
#define CCC_private_doubly_linked_list_emplace_front(doubly_linked_list_ptr,   \
                                                     struct_initializer...)    \
    (__extension__({                                                           \
        typeof(struct_initializer) *private_doubly_linked_list_res = NULL;     \
        struct CCC_Doubly_linked_list *private_doubly_linked_list              \
            = (doubly_linked_list_ptr);                                        \
        if (!private_doubly_linked_list->alloc)                                \
        {                                                                      \
            private_doubly_linked_list_res = NULL;                             \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            private_doubly_linked_list_res                                     \
                = private_doubly_linked_list->alloc(                           \
                    NULL, private_doubly_linked_list->sizeof_type,             \
                    private_doubly_linked_list->context);                      \
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

#endif /* CCC_IMPL_DOUBLY_LINKED_LIST_H */
