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
#ifndef CCC_PRIVATE_PRIORITY_QUEUE_H
#define CCC_PRIVATE_PRIORITY_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @internal A node in the priority queue. The child is technically a left
child but the direction is not important. The next and prev pointers represent
sibling nodes in a circular doubly linked list in the child ring. When a node
loses a merge and is sent down to be another nodes child it joins this sibling
ring of nodes. The list is doubly linked and we have a parent pointer to keep
operations like delete min, erase, and update fast. */
struct CCC_Priority_queue_node
{
    /** @internal The left child of this node. */
    struct CCC_Priority_queue_node *child;
    /** @internal The next sibling in the sibling ring or self. */
    struct CCC_Priority_queue_node *next;
    /** @internal The previous sibling in the sibling ring or self. */
    struct CCC_Priority_queue_node *prev;
    /** @internal A parent or NULL if this is the root node. */
    struct CCC_Priority_queue_node *parent;
};

/** @internal A priority queue is a variation on a heap ordered tree aimed at
simple operations and run times that are very close to optimal. The root of
the entire heap never has a next, prev, or parent because only a single heap
root is allowed. Nodes can have a left child. The direction of the child does
not matter to the implementation because there is only one. Then, any node
besides the root of the entire heap can be in a ring of siblings with next
and previous pointers. Here is a sample heap.

```
< = next
> = prev

    ┌<0>┐
    └/──┘
  ┌<1>─<7>┐
  └/────/─┘
┌<9>┐┌<8>─<9>┐
└───┘└───────┘
```

The heap child rings are circular doubly linked lists to support fast update
and erase operations. This construction gives the following run times.

```
┌─────────┬─────────┬─────────┬─────────┐
│min      │delete   │increase │insert   │
│         │min      │decrease │         │
├─────────┼─────────┼─────────┼─────────┤
│O(1)     │O(log(N))│o(log(N))│O(1)     │
│         │amortized│amortized│         │
└─────────┴─────────┴─────────┴─────────┘
```

The proof for the increase/decrease runtime is complicated. The gist is that
updating the key is an `O(1)` operation. However, it restructures the tree in
such a way that the next delete min has more work to do. That is why update and
delete min have their amortized run times. In practice, the simplicity of the
implementation keeps the pairing heap fast and easy to understand. In fact, if
nodes are allocated ahead of time in a buffer, the pairing heap beats the
binary flat priority queue in the C Container Collection across many operations
with the trade-off being more memory consumed. */
struct CCC_Priority_queue
{
    /** @internal The node at the root of the heap. No parent. */
    struct CCC_Priority_queue_node *root;
    /** @internal Quantity of nodes stored in heap for O(1) reporting. */
    size_t count;
    /** @internal The byte offset of the intrusive node in user type. */
    size_t priority_queue_node_offset;
    /** @internal The size of the type we are intruding upon. */
    size_t sizeof_type;
    /** @internal The order of this heap, `CCC_ORDER_LESSER` (min) or
     * `CCC_ORDER_GREATER` (max).*/
    CCC_Order order;
    /** @internal The comparison function to enforce ordering. */
    CCC_Type_comparator *compare;
    /** @internal The allocation function, if any. */
    CCC_Allocator *allocate;
    /** @internal Auxiliary data, if any. */
    void *context;
};

/*=========================  Private Interface     ==========================*/

/** @internal */
void CCC_private_priority_queue_push(struct CCC_Priority_queue *,
                                     struct CCC_Priority_queue_node *);
/** @internal */
struct CCC_Priority_queue_node *
CCC_private_priority_queue_node_in(struct CCC_Priority_queue const *,
                                   void const *);
/** @internal */
CCC_Order
CCC_private_priority_queue_order(struct CCC_Priority_queue const *,
                                 struct CCC_Priority_queue_node const *,
                                 struct CCC_Priority_queue_node const *);
/** @internal */
struct CCC_Priority_queue_node *
CCC_private_priority_queue_merge(struct CCC_Priority_queue *,
                                 struct CCC_Priority_queue_node *old,
                                 struct CCC_Priority_queue_node *new);
/** @internal */
void CCC_private_priority_queue_cut_child(struct CCC_Priority_queue_node *);
/** @internal */
void CCC_private_priority_queue_init_node(struct CCC_Priority_queue_node *);
/** @internal */
struct CCC_Priority_queue_node *
CCC_private_priority_queue_delete_node(struct CCC_Priority_queue *,
                                       struct CCC_Priority_queue_node *);
/** @internal */
void *
CCC_private_priority_queue_struct_base(struct CCC_Priority_queue const *,
                                       struct CCC_Priority_queue_node const *);

/*=========================  Macro Implementations     ======================*/

/** @internal */
#define CCC_private_priority_queue_initialize(                                 \
    private_struct_name, private_priority_queue_node_field,                    \
    private_priority_queue_order, private_order_fn, private_allocate,          \
    private_context_data)                                                      \
    {                                                                          \
        .root = NULL,                                                          \
        .count = 0,                                                            \
        .priority_queue_node_offset                                            \
        = offsetof(private_struct_name, private_priority_queue_node_field),    \
        .sizeof_type = sizeof(private_struct_name),                            \
        .order = (private_priority_queue_order),                               \
        .compare = (private_order_fn),                                         \
        .allocate = (private_allocate),                                        \
        .context = (private_context_data),                                     \
    }

/** @internal */
#define CCC_private_priority_queue_emplace(priority_queue_pointer,             \
                                           type_compound_literal...)           \
    (__extension__({                                                           \
        typeof(type_compound_literal) *private_priority_queue_res = NULL;      \
        struct CCC_Priority_queue *private_priority_queue                      \
            = (priority_queue_pointer);                                        \
        if (private_priority_queue)                                            \
        {                                                                      \
            if (!private_priority_queue->allocate)                             \
            {                                                                  \
                private_priority_queue_res = NULL;                             \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_priority_queue_res = private_priority_queue->allocate( \
                    (CCC_Allocator_context){                                   \
                        .input = NULL,                                         \
                        .bytes = private_priority_queue->sizeof_type,          \
                        .context = private_priority_queue->context,            \
                    });                                                        \
                if (private_priority_queue_res)                                \
                {                                                              \
                    *private_priority_queue_res = type_compound_literal;       \
                    CCC_private_priority_queue_push(                           \
                        private_priority_queue,                                \
                        CCC_private_priority_queue_node_in(                    \
                            private_priority_queue,                            \
                            private_priority_queue_res));                      \
                }                                                              \
            }                                                                  \
        }                                                                      \
        private_priority_queue_res;                                            \
    }))

/** @internal */
#define CCC_private_priority_queue_update_w(                                   \
    priority_queue_pointer, type_pointer, update_closure_over_T...)            \
    (__extension__({                                                           \
        struct CCC_Priority_queue *const private_priority_queue                \
            = (priority_queue_pointer);                                        \
        typeof(*type_pointer) *T = (type_pointer);                             \
        if (private_priority_queue && T)                                       \
        {                                                                      \
            struct CCC_Priority_queue_node *const                              \
                private_priority_queue_node_pointer                            \
                = CCC_private_priority_queue_node_in(private_priority_queue,   \
                                                     T);                       \
            if (private_priority_queue_node_pointer->parent                    \
                && CCC_private_priority_queue_order(                           \
                       private_priority_queue,                                 \
                       private_priority_queue_node_pointer,                    \
                       private_priority_queue_node_pointer->parent)            \
                       == private_priority_queue->order)                       \
            {                                                                  \
                CCC_private_priority_queue_cut_child(                          \
                    private_priority_queue_node_pointer);                      \
                {update_closure_over_T} private_priority_queue->root           \
                    = CCC_private_priority_queue_merge(                        \
                        private_priority_queue, private_priority_queue->root,  \
                        private_priority_queue_node_pointer);                  \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_priority_queue->root                                   \
                    = CCC_private_priority_queue_delete_node(                  \
                        private_priority_queue,                                \
                        private_priority_queue_node_pointer);                  \
                CCC_private_priority_queue_init_node(                          \
                    private_priority_queue_node_pointer);                      \
                {update_closure_over_T} private_priority_queue->root           \
                    = CCC_private_priority_queue_merge(                        \
                        private_priority_queue, private_priority_queue->root,  \
                        private_priority_queue_node_pointer);                  \
            }                                                                  \
        }                                                                      \
        T;                                                                     \
    }))

/** @internal */
#define CCC_private_priority_queue_increase_w(                                 \
    priority_queue_pointer, type_pointer, increase_closure_over_T...)          \
    (__extension__({                                                           \
        struct CCC_Priority_queue *const private_priority_queue                \
            = (priority_queue_pointer);                                        \
        typeof(*type_pointer) *T = (type_pointer);                             \
        if (private_priority_queue && T)                                       \
        {                                                                      \
            struct CCC_Priority_queue_node *const                              \
                private_priority_queue_node_pointer                            \
                = CCC_private_priority_queue_node_in(private_priority_queue,   \
                                                     T);                       \
            if (private_priority_queue->order == CCC_ORDER_GREATER)            \
            {                                                                  \
                CCC_private_priority_queue_cut_child(                          \
                    private_priority_queue_node_pointer);                      \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_priority_queue->root                                   \
                    = CCC_private_priority_queue_delete_node(                  \
                        private_priority_queue,                                \
                        private_priority_queue_node_pointer);                  \
                CCC_private_priority_queue_init_node(                          \
                    private_priority_queue_node_pointer);                      \
            }                                                                  \
            {increase_closure_over_T} private_priority_queue->root             \
                = CCC_private_priority_queue_merge(                            \
                    private_priority_queue, private_priority_queue->root,      \
                    private_priority_queue_node_pointer);                      \
        }                                                                      \
        T;                                                                     \
    }))

/** @internal */
#define CCC_private_priority_queue_decrease_w(                                 \
    priority_queue_pointer, type_pointer, decrease_closure_over_T...)          \
    (__extension__({                                                           \
        struct CCC_Priority_queue *const private_priority_queue                \
            = (priority_queue_pointer);                                        \
        typeof(*type_pointer) *T = (type_pointer);                             \
        if (private_priority_queue && T)                                       \
        {                                                                      \
            struct CCC_Priority_queue_node *const                              \
                private_priority_queue_node_pointer                            \
                = CCC_private_priority_queue_node_in(private_priority_queue,   \
                                                     T);                       \
            if (private_priority_queue->order == CCC_ORDER_LESSER)             \
            {                                                                  \
                CCC_private_priority_queue_cut_child(                          \
                    private_priority_queue_node_pointer);                      \
            }                                                                  \
            else                                                               \
            {                                                                  \
                private_priority_queue->root                                   \
                    = CCC_private_priority_queue_delete_node(                  \
                        private_priority_queue,                                \
                        private_priority_queue_node_pointer);                  \
                CCC_private_priority_queue_init_node(                          \
                    private_priority_queue_node_pointer);                      \
            }                                                                  \
            {decrease_closure_over_T} private_priority_queue->root             \
                = CCC_private_priority_queue_merge(                            \
                    private_priority_queue, private_priority_queue->root,      \
                    private_priority_queue_node_pointer);                      \
        }                                                                      \
        T;                                                                     \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_PRIVATE_PRIORITY_QUEUE_H */
