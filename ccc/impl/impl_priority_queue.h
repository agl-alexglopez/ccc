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
#ifndef CCC_IMPL_PRIORITY_QUEUE_H
#define CCC_IMPL_PRIORITY_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "../types.h"

/* NOLINTBEGIN(readability-identifier-naming) */

/** @private A node in the priority queue. The child is technically a left
child but the direction is not important. The next and prev pointers represent
sibling nodes in a circular doubly linked list in the child ring. When a node
loses a merge and is sent down to be another nodes child it joins this sibling
ring of nodes. The list is doubly linked and we have a parent pointer to keep
operations like delete min, erase, and update fast. */
struct CCC_pq_elem
{
    /** @private The left child of this node. */
    struct CCC_pq_elem *child;
    /** @private The next sibling in the sibling ring or self. */
    struct CCC_pq_elem *next;
    /** @private The previous sibling in the sibling ring or self. */
    struct CCC_pq_elem *prev;
    /** @private A parent or NULL if this is the root node. */
    struct CCC_pq_elem *parent;
};

/** @private A priority queue is a variation on a heap ordered tree aimed at
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
struct CCC_pq
{
    /** @private The node at the root of the heap. No parent. */
    struct CCC_pq_elem *root;
    /** @private Quantity of nodes stored in heap for O(1) reporting. */
    size_t count;
    /** @private The byte offset of the intrusive node in user type. */
    size_t pq_elem_offset;
    /** @private The size of the type we are intruding upon. */
    size_t sizeof_type;
    /** @private The order of this heap, `CCC_LES` (min) or `CCC_GRT` (max).*/
    CCC_threeway_cmp order;
    /** @private The comparison function to enforce ordering. */
    CCC_any_type_cmp_fn *cmp;
    /** @private The allocation function, if any. */
    CCC_any_alloc_fn *alloc;
    /** @private Auxiliary data, if any. */
    void *aux;
};

/*=========================  Private Interface     ==========================*/

/** @private */
void CCC_impl_pq_push(struct CCC_pq *, struct CCC_pq_elem *);
/** @private */
struct CCC_pq_elem *CCC_impl_pq_elem_in(struct CCC_pq const *, void const *);
/** @private */
CCC_threeway_cmp CCC_impl_pq_cmp(struct CCC_pq const *,
                                 struct CCC_pq_elem const *,
                                 struct CCC_pq_elem const *);
/** @private */
struct CCC_pq_elem *CCC_impl_pq_merge(struct CCC_pq *pq,
                                      struct CCC_pq_elem *old,
                                      struct CCC_pq_elem *new);
/** @private */
void CCC_impl_pq_cut_child(struct CCC_pq_elem *);
/** @private */
void CCC_impl_pq_init_node(struct CCC_pq_elem *);
/** @private */
struct CCC_pq_elem *CCC_impl_pq_delete_node(struct CCC_pq *,
                                            struct CCC_pq_elem *);
/** @private */
void *CCC_impl_pq_struct_base(struct CCC_pq const *,
                              struct CCC_pq_elem const *);

/*=========================  Macro Implementations     ======================*/

/** @private */
#define CCC_impl_pq_init(impl_struct_name, impl_pq_elem_field, impl_pq_order,  \
                         impl_cmp_fn, impl_alloc_fn, impl_aux_data)            \
    {                                                                          \
        .root = NULL,                                                          \
        .count = 0,                                                            \
        .pq_elem_offset = offsetof(impl_struct_name, impl_pq_elem_field),      \
        .sizeof_type = sizeof(impl_struct_name),                               \
        .alloc = (impl_alloc_fn),                                              \
        .cmp = (impl_cmp_fn),                                                  \
        .order = (impl_pq_order),                                              \
        .aux = (impl_aux_data),                                                \
    }

/** @private */
#define CCC_impl_pq_emplace(pq_ptr, lazy_value...)                             \
    (__extension__({                                                           \
        typeof(lazy_value) *impl_pq_res = NULL;                                \
        struct CCC_pq *impl_pq = (pq_ptr);                                     \
        if (impl_pq)                                                           \
        {                                                                      \
            if (!impl_pq->alloc)                                               \
            {                                                                  \
                impl_pq_res = NULL;                                            \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_pq_res = impl_pq->alloc(NULL, impl_pq->sizeof_type,       \
                                             impl_pq->aux);                    \
                if (impl_pq_res)                                               \
                {                                                              \
                    *impl_pq_res = lazy_value;                                 \
                    CCC_impl_pq_push(                                          \
                        impl_pq, CCC_impl_pq_elem_in(impl_pq, impl_pq_res));   \
                }                                                              \
            }                                                                  \
        }                                                                      \
        impl_pq_res;                                                           \
    }))

/** @private */
#define CCC_impl_pq_update_w(pq_ptr, any_type_ptr, update_closure_over_T...)   \
    (__extension__({                                                           \
        struct CCC_pq *const impl_pq = (pq_ptr);                               \
        typeof(*any_type_ptr) *T = (any_type_ptr);                             \
        if (impl_pq && T)                                                      \
        {                                                                      \
            struct CCC_pq_elem *const impl_pq_elem_ptr                         \
                = CCC_impl_pq_elem_in(impl_pq, T);                             \
            if (impl_pq_elem_ptr->parent                                       \
                && CCC_impl_pq_cmp(impl_pq, impl_pq_elem_ptr,                  \
                                   impl_pq_elem_ptr->parent)                   \
                       == impl_pq->order)                                      \
            {                                                                  \
                CCC_impl_pq_cut_child(impl_pq_elem_ptr);                       \
                {update_closure_over_T} impl_pq->root = CCC_impl_pq_merge(     \
                    impl_pq, impl_pq->root, impl_pq_elem_ptr);                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_pq->root                                                  \
                    = CCC_impl_pq_delete_node(impl_pq, impl_pq_elem_ptr);      \
                CCC_impl_pq_init_node(impl_pq_elem_ptr);                       \
                {update_closure_over_T} impl_pq->root = CCC_impl_pq_merge(     \
                    impl_pq, impl_pq->root, impl_pq_elem_ptr);                 \
            }                                                                  \
        }                                                                      \
        T;                                                                     \
    }))

/** @private */
#define CCC_impl_pq_increase_w(pq_ptr, any_type_ptr,                           \
                               increase_closure_over_T...)                     \
    (__extension__({                                                           \
        struct CCC_pq *const impl_pq = (pq_ptr);                               \
        typeof(*any_type_ptr) *T = (any_type_ptr);                             \
        if (impl_pq && T)                                                      \
        {                                                                      \
            struct CCC_pq_elem *const impl_pq_elem_ptr                         \
                = CCC_impl_pq_elem_in(impl_pq, T);                             \
            if (impl_pq->order == CCC_GRT)                                     \
            {                                                                  \
                CCC_impl_pq_cut_child(impl_pq_elem_ptr);                       \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_pq->root                                                  \
                    = CCC_impl_pq_delete_node(impl_pq, impl_pq_elem_ptr);      \
                CCC_impl_pq_init_node(impl_pq_elem_ptr);                       \
            }                                                                  \
            {increase_closure_over_T} impl_pq->root                            \
                = CCC_impl_pq_merge(impl_pq, impl_pq->root, impl_pq_elem_ptr); \
        }                                                                      \
        T;                                                                     \
    }))

/** @private */
#define CCC_impl_pq_decrease_w(pq_ptr, any_type_ptr,                           \
                               decrease_closure_over_T...)                     \
    (__extension__({                                                           \
        struct CCC_pq *const impl_pq = (pq_ptr);                               \
        typeof(*any_type_ptr) *T = (any_type_ptr);                             \
        if (impl_pq && T)                                                      \
        {                                                                      \
            struct CCC_pq_elem *const impl_pq_elem_ptr                         \
                = CCC_impl_pq_elem_in(impl_pq, T);                             \
            if (impl_pq->order == CCC_LES)                                     \
            {                                                                  \
                CCC_impl_pq_cut_child(impl_pq_elem_ptr);                       \
            }                                                                  \
            else                                                               \
            {                                                                  \
                impl_pq->root                                                  \
                    = CCC_impl_pq_delete_node(impl_pq, impl_pq_elem_ptr);      \
                CCC_impl_pq_init_node(impl_pq_elem_ptr);                       \
            }                                                                  \
            {decrease_closure_over_T} impl_pq->root                            \
                = CCC_impl_pq_merge(impl_pq, impl_pq->root, impl_pq_elem_ptr); \
        }                                                                      \
        T;                                                                     \
    }))

/* NOLINTEND(readability-identifier-naming) */

#endif /* CCC_IMPL_PRIORITY_QUEUE_H */
