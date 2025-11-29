/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */
#include <stddef.h>
#include <string.h>

#include "priority_queue.h"
#include "private/private_priority_queue.h"
#include "types.h"

/*=========================  Function Prototypes   ==========================*/

static struct CCC_Priority_queue_node *
elem_in(struct CCC_Priority_queue const *, void const *);
static struct CCC_Priority_queue_node *merge(struct CCC_Priority_queue *,
                                             struct CCC_Priority_queue_node *,
                                             struct CCC_Priority_queue_node *);
static void link_child(struct CCC_Priority_queue_node *,
                       struct CCC_Priority_queue_node *);
static void init_node(struct CCC_Priority_queue_node *);
static size_t traversal_count(struct CCC_Priority_queue_node const *);
static CCC_Tribool has_valid_links(struct CCC_Priority_queue const *,
                                   struct CCC_Priority_queue_node const *,
                                   struct CCC_Priority_queue_node const *);
static struct CCC_Priority_queue_node *
delete_node(struct CCC_Priority_queue *, struct CCC_Priority_queue_node *);
static struct CCC_Priority_queue_node *
delete_min(struct CCC_Priority_queue *, struct CCC_Priority_queue_node *);
static void clear_node(struct CCC_Priority_queue_node *);
static void cut_child(struct CCC_Priority_queue_node *);
static void *struct_base(struct CCC_Priority_queue const *,
                         struct CCC_Priority_queue_node const *);
static CCC_Order order(struct CCC_Priority_queue const *,
                       struct CCC_Priority_queue_node const *,
                       struct CCC_Priority_queue_node const *);
static void update_fixup(struct CCC_Priority_queue *,
                         struct CCC_Priority_queue_node *, CCC_Type_modifier *,
                         void *);
static void increase_fixup(struct CCC_Priority_queue *,
                           struct CCC_Priority_queue_node *,
                           CCC_Type_modifier *, void *);
static void decrease_fixup(struct CCC_Priority_queue *,
                           struct CCC_Priority_queue_node *,
                           CCC_Type_modifier *, void *);

/*=========================  Interface Functions   ==========================*/

void *
CCC_priority_queue_front(CCC_Priority_queue const *const priority_queue)
{
    if (!priority_queue)
    {
        return NULL;
    }
    return priority_queue->root
             ? struct_base(priority_queue, priority_queue->root)
             : NULL;
}

void *
CCC_priority_queue_push(CCC_Priority_queue *const priority_queue,
                        CCC_Priority_queue_node *type_intruder)
{
    if (!type_intruder || !priority_queue)
    {
        return NULL;
    }
    void *ret = struct_base(priority_queue, type_intruder);
    if (priority_queue->allocate)
    {
        void *const node = priority_queue->allocate((CCC_Allocator_context){
            .input = NULL,
            .bytes = priority_queue->sizeof_type,
            .context = priority_queue->context,
        });
        if (!node)
        {
            return NULL;
        }
        (void)memcpy(node, ret, priority_queue->sizeof_type);
        ret = node;
        type_intruder = elem_in(priority_queue, ret);
    }
    init_node(type_intruder);
    priority_queue->root
        = merge(priority_queue, priority_queue->root, type_intruder);
    ++priority_queue->count;
    return ret;
}

CCC_Result
CCC_priority_queue_pop(CCC_Priority_queue *const priority_queue)
{
    if (!priority_queue || !priority_queue->root)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    struct CCC_Priority_queue_node *const popped = priority_queue->root;
    priority_queue->root = delete_min(priority_queue, priority_queue->root);
    priority_queue->count--;
    clear_node(popped);
    if (priority_queue->allocate)
    {
        (void)priority_queue->allocate((CCC_Allocator_context){
            .input = struct_base(priority_queue, popped),
            .bytes = 0,
            .context = priority_queue->context,
        });
    }
    return CCC_RESULT_OK;
}

void *
CCC_priority_queue_extract(CCC_Priority_queue *const priority_queue,
                           CCC_Priority_queue_node *const type_intruder)
{
    if (!priority_queue || !type_intruder || !priority_queue->root
        || !type_intruder->next || !type_intruder->prev)
    {
        return NULL;
    }
    priority_queue->root = delete_node(priority_queue, type_intruder);
    priority_queue->count--;
    clear_node(type_intruder);
    return struct_base(priority_queue, type_intruder);
}

CCC_Result
CCC_priority_queue_erase(CCC_Priority_queue *const priority_queue,
                         CCC_Priority_queue_node *const type_intruder)
{
    if (!priority_queue || !type_intruder || !priority_queue->root
        || !type_intruder->next || !type_intruder->prev)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    priority_queue->root = delete_node(priority_queue, type_intruder);
    priority_queue->count--;
    if (priority_queue->allocate)
    {
        (void)priority_queue->allocate((CCC_Allocator_context){
            .input = struct_base(priority_queue, type_intruder),
            .bytes = 0,
            .context = priority_queue->context,
        });
    }
    return CCC_RESULT_OK;
}

/** Deletes all nodes in the heap in linear time and constant space. This is
achieved by continually bringing up any child lists and splicing them into the
current child list being considered. We are avoiding recursion or amortized
O(log(N)) pops with this method. */
CCC_Result
CCC_priority_queue_clear(CCC_Priority_queue *const priority_queue,
                         CCC_Type_destructor *const destroy)
{
    if (!priority_queue)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    {
        struct CCC_Priority_queue_node *node = priority_queue->root;
        while (node)
        {
            /* The child and its siblings cut to the front of the line and we
               start again as if the child is the first in this sibling list. */
            if (node->child)
            {
                struct CCC_Priority_queue_node *const child = node->child;
                struct CCC_Priority_queue_node *const node_end = node->next;
                /* Final element of e child list picks up child as head of list.
                 */
                node_end->prev = child;
                /* Now e picks up the last (wrapping) element of child list. */
                node->next = child->next;
                /* Child has a list so don't just set child's prev to e. */
                child->next->prev = node;
                /* Child list wrapping element is now end of e list. */
                child->next = node_end;
                /* Our traversal now jumps to start of list we spliced in. */
                node->child = NULL;
                node = child;
                continue;
            }
            /* No more child lists to splice in so this node is done.  */
            struct CCC_Priority_queue_node *const prev_node
                = node->prev == node ? NULL : node->prev;
            node->next->prev = node->prev;
            node->prev->next = node->next;
            node->parent = node->next = node->prev = node->child = NULL;
            void *const destroy_this = struct_base(priority_queue, node);
            if (destroy)
            {
                destroy((CCC_Type_context){
                    .type = destroy_this,
                    .context = priority_queue->context,
                });
            }
            if (priority_queue->allocate)
            {
                (void)priority_queue->allocate((CCC_Allocator_context){
                    .input = destroy_this,
                    .bytes = 0,
                    .context = priority_queue->context,
                });
            }
            node = prev_node;
        }
    }
    priority_queue->count = 0;
    priority_queue->root = NULL;
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_priority_queue_is_empty(CCC_Priority_queue const *const priority_queue)
{
    if (!priority_queue)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !priority_queue->count;
}

CCC_Count
CCC_priority_queue_count(CCC_Priority_queue const *const priority_queue)
{
    if (!priority_queue)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = priority_queue->count};
}

/* This is a difficult function. Without knowing if this new value is greater
   or less than the previous we must always perform a delete and reinsert if
   the value has not broken total order with the parent. It is not sufficient
   to check if the value has exceeded the value of the first left child as
   any sibling of that left child may be bigger than or smaller than that
   left child value. */
void *
CCC_priority_queue_update(CCC_Priority_queue *const priority_queue,
                          CCC_Priority_queue_node *const type_intruder,
                          CCC_Type_modifier *const modify, void *const context)
{
    if (!priority_queue || !type_intruder || !modify || !type_intruder->next
        || !type_intruder->prev)
    {
        return NULL;
    }
    update_fixup(priority_queue, type_intruder, modify, context);
    return struct_base(priority_queue, type_intruder);
}

/* Preferable to use this function if it is known the value is increasing.
   Much more efficient. */
void *
CCC_priority_queue_increase(CCC_Priority_queue *const priority_queue,
                            CCC_Priority_queue_node *const type_intruder,
                            CCC_Type_modifier *const modify,
                            void *const context)
{
    if (!priority_queue || !type_intruder || !modify || !type_intruder->next
        || !type_intruder->prev)
    {
        return NULL;
    }
    increase_fixup(priority_queue, type_intruder, modify, context);
    return struct_base(priority_queue, type_intruder);
}

/* Preferable to use this function if it is known the value is decreasing.
   Much more efficient. */
void *
CCC_priority_queue_decrease(CCC_Priority_queue *const priority_queue,
                            CCC_Priority_queue_node *const type_intruder,
                            CCC_Type_modifier *const modify,
                            void *const context)
{
    if (!priority_queue || !type_intruder || !modify || !type_intruder->next
        || !type_intruder->prev)
    {
        return NULL;
    }
    decrease_fixup(priority_queue, type_intruder, modify, context);
    return struct_base(priority_queue, type_intruder);
}

CCC_Tribool
CCC_priority_queue_validate(CCC_Priority_queue const *const priority_queue)
{
    if (!priority_queue
        || (priority_queue->root && priority_queue->root->parent))
    {
        return CCC_FALSE;
    }
    if (!has_valid_links(priority_queue, NULL, priority_queue->root))
    {
        return CCC_FALSE;
    }
    if (traversal_count(priority_queue->root) != priority_queue->count)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

CCC_Order
CCC_priority_queue_order(CCC_Priority_queue const *const priority_queue)
{
    return priority_queue ? priority_queue->order : CCC_ORDER_ERROR;
}

/*=========================  Private Interface     ==========================*/

void
CCC_private_priority_queue_push(struct CCC_Priority_queue *const priority_queue,
                                struct CCC_Priority_queue_node *const node)
{
    init_node(node);
    priority_queue->root = merge(priority_queue, priority_queue->root, node);
    ++priority_queue->count;
}

struct CCC_Priority_queue_node *
CCC_private_priority_queue_node_in(
    struct CCC_Priority_queue const *const priority_queue,
    void const *const any_struct)
{
    return elem_in(priority_queue, any_struct);
}

CCC_Order
CCC_private_priority_queue_order(
    struct CCC_Priority_queue const *const priority_queue,
    struct CCC_Priority_queue_node const *const left,
    struct CCC_Priority_queue_node const *const right)
{
    return order(priority_queue, left, right);
}

struct CCC_Priority_queue_node *
CCC_private_priority_queue_merge(
    struct CCC_Priority_queue *const priority_queue,
    struct CCC_Priority_queue_node *const old,
    struct CCC_Priority_queue_node *const new)
{
    return merge(priority_queue, old, new);
}

void
CCC_private_priority_queue_cut_child(
    struct CCC_Priority_queue_node *const child)
{
    cut_child(child);
}

void
CCC_private_priority_queue_init_node(
    struct CCC_Priority_queue_node *const child)
{
    init_node(child);
}

struct CCC_Priority_queue_node *
CCC_private_priority_queue_delete_node(
    struct CCC_Priority_queue *const priority_queue,
    struct CCC_Priority_queue_node *const root)
{
    return delete_node(priority_queue, root);
}

void *
CCC_private_priority_queue_struct_base(
    struct CCC_Priority_queue const *const priority_queue,
    struct CCC_Priority_queue_node const *const node)
{
    return struct_base(priority_queue, node);
}

/*========================   Static Helpers  ================================*/

static void
update_fixup(struct CCC_Priority_queue *const priority_queue,
             struct CCC_Priority_queue_node *const node,
             CCC_Type_modifier *const modify, void *context)
{
    if (node->parent
        && order(priority_queue, node, node->parent) == priority_queue->order)
    {
        cut_child(node);
        modify((CCC_Type_context){
            .type = struct_base(priority_queue, node),
            .context = context,
        });
        priority_queue->root
            = merge(priority_queue, priority_queue->root, node);
        return;
    }
    priority_queue->root = delete_node(priority_queue, node);
    init_node(node);
    modify((CCC_Type_context){
        .type = struct_base(priority_queue, node),
        .context = context,
    });
    priority_queue->root = merge(priority_queue, priority_queue->root, node);
}

static void
increase_fixup(struct CCC_Priority_queue *const priority_queue,
               struct CCC_Priority_queue_node *const node,
               CCC_Type_modifier *const modify, void *context)
{
    if (priority_queue->order == CCC_ORDER_GREATER)
    {
        cut_child(node);
    }
    else
    {
        priority_queue->root = delete_node(priority_queue, node);
        init_node(node);
    }
    modify((CCC_Type_context){
        .type = struct_base(priority_queue, node),
        .context = context,
    });
    priority_queue->root = merge(priority_queue, priority_queue->root, node);
}

static void
decrease_fixup(struct CCC_Priority_queue *const priority_queue,
               struct CCC_Priority_queue_node *const e,
               CCC_Type_modifier *const modify, void *const context)
{
    if (priority_queue->order == CCC_ORDER_LESSER)
    {
        cut_child(e);
    }
    else
    {
        priority_queue->root = delete_node(priority_queue, e);
        init_node(e);
    }
    modify((CCC_Type_context){
        .type = struct_base(priority_queue, e),
        .context = context,
    });
    priority_queue->root = merge(priority_queue, priority_queue->root, e);
}

/** Cuts the child out of its current sibling list and redirects parent if
this child is directly pointed to by parent. The child is then made into its
own circular sibling list. The left child of this child, if one exists, is
still pointed to and not modified by this function. */
static void
cut_child(struct CCC_Priority_queue_node *const child)
{
    child->next->prev = child->prev;
    child->prev->next = child->next;
    if (child->parent && child == child->parent->child)
    {
        /* To preserve the shuffle down properties the prev child should
           become the new child as that is the next youngest node. */
        child->parent->child = child->prev == child ? NULL : child->prev;
    }
    child->parent = NULL;
    child->next = child->prev = child;
}

static struct CCC_Priority_queue_node *
delete_node(struct CCC_Priority_queue *const priority_queue,
            struct CCC_Priority_queue_node *const root)
{
    if (priority_queue->root == root)
    {
        return delete_min(priority_queue, root);
    }
    cut_child(root);
    return merge(priority_queue, priority_queue->root,
                 delete_min(priority_queue, root));
}

/* Uses Fredman et al. oldest to youngest pairing method mentioned on pg 124
of the paper to pair nodes in one pass. Of all the variants for pairing given
in the paper this one is the back-to-front variant and the only one for which
the runtime analysis holds identically to the two-pass standard variant. A
non-trivial example for min heap.

```
< = next_sibling
> = prev_sibling

  ┌<1>┐
  └/──┘
┌<9>─<1>─<9>─<7>─<8>┐
└───────────────────┘
        |
        v
┌<9>─<1>─<7>─<8>┐
└────────/──────┘
      ┌<9>┐
      └───┘
        |
        v
┌<9>─<1>─<7>┐
└────────/──┘
      ┌<8>─<9>┐
      └───────┘
        |
        v
  ┌<1>─<7>┐
  └/────/─┘
┌<9>┐┌<8>─<9>┐
└───┘└───────┘
        |
        v
    ┌<1>┐
    └/──┘
  ┌<7>─<9>┐
  └/──────┘
┌<8>─<9>┐
└───────┘
```

Delete min is the slowest operation offered by the priority queue and in
part contributes to the amortized `o(log(N))` runtime of the decrease key
operation. */
static struct CCC_Priority_queue_node *
delete_min(struct CCC_Priority_queue *const priority_queue,
           struct CCC_Priority_queue_node *root)
{
    if (!root->child)
    {
        return NULL;
    }
    struct CCC_Priority_queue_node *const eldest = root->child->next;
    struct CCC_Priority_queue_node *accumulator = root->child->next;
    struct CCC_Priority_queue_node *cur = root->child->next->next;
    while (cur != eldest && cur->next != eldest)
    {
        struct CCC_Priority_queue_node *const next = cur->next;
        struct CCC_Priority_queue_node *const next_cur = cur->next->next;
        next->next = next->prev = NULL;
        cur->next = cur->prev = NULL;
        /* Double merge ensures `O(log(N))` steps rather than O(N). */
        accumulator = merge(priority_queue, accumulator,
                            merge(priority_queue, cur, next));
        cur = next_cur;
    }
    /* This covers the odd or even case for number of pairings. */
    root
        = cur == eldest ? accumulator : merge(priority_queue, accumulator, cur);
    /* The root is always alone in its circular list at the end of merges. */
    root->next = root->prev = root;
    root->parent = NULL;
    return root;
}

/** Merges two priority queues, making the winner by ordering the root and
pushing the loser to the left child ring. Old should be the element that has
been in the queue longer and new, newer. This algorithm will still work if this
argument ordering is not respected and it does not change runtime, but it is how
to comply with the strategy outlined in the Fredman et. al. paper. */
static struct CCC_Priority_queue_node *
merge(struct CCC_Priority_queue *const priority_queue,
      struct CCC_Priority_queue_node *const old,
      struct CCC_Priority_queue_node *const new)
{
    if (!old || !new || old == new)
    {
        return old ? old : new;
    }
    if (order(priority_queue, new, old) == priority_queue->order)
    {
        link_child(new, old);
        return new;
    }
    link_child(old, new);
    return old;
}

/* Oldest nodes shuffle down, new drops in to replace. This supports the
ring representation from Fredman et al., pg 125, fig 14 where the left
child's next pointer wraps to the last element in the list. The previous
pointer is to support faster deletes and decrease key operations.

< = next_sibling
> = prev_sibling

         A        A            A
        ╱        ╱            ╱
    ┌─<B>─┐  ┌─<C>──<B>─┐ ┌─<D>──<C>──<B>─┐
    └─────┘  └──────────┘ └───────────────┘

Pairing in the delete min phase would then start at B in this example and work
towards D. That is the oldest to youngest order mentioned in the paper and
helps set up the one-pass back-to-front variant mentioned in the paper allowing
the same runtime guarantees as the two pass standard pairing heap. */
static void
link_child(struct CCC_Priority_queue_node *const parent,
           struct CCC_Priority_queue_node *const child)
{
    if (parent->child)
    {
        child->next = parent->child->next;
        child->prev = parent->child;
        parent->child->next->prev = child;
        parent->child->next = child;
    }
    else
    {
        child->next = child->prev = child;
    }
    parent->child = child;
    child->parent = parent;
}

static inline CCC_Order
order(struct CCC_Priority_queue const *const priority_queue,
      struct CCC_Priority_queue_node const *const left,
      struct CCC_Priority_queue_node const *const right)
{
    return priority_queue->compare((CCC_Type_comparator_context){
        .type_left = struct_base(priority_queue, left),
        .type_right = struct_base(priority_queue, right),
        .context = priority_queue->context,
    });
}

static inline void *
struct_base(struct CCC_Priority_queue const *const priority_queue,
            struct CCC_Priority_queue_node const *const node)
{
    return ((char *)&(node->child)) - priority_queue->type_intruder_offset;
}

static inline struct CCC_Priority_queue_node *
elem_in(struct CCC_Priority_queue const *const priority_queue,
        void const *const any_struct)
{
    return (struct CCC_Priority_queue_node
                *)((char *)any_struct + priority_queue->type_intruder_offset);
}

static inline void
init_node(struct CCC_Priority_queue_node *const node)
{
    node->child = node->parent = NULL;
    node->next = node->prev = node;
}

static inline void
clear_node(struct CCC_Priority_queue_node *const node)
{
    node->child = node->next = node->prev = node->parent = NULL;
}

/*========================     Validation ================================*/

/* NOLINTBEGIN(*misc-no-recursion) */

static size_t
traversal_count(struct CCC_Priority_queue_node const *const root)
{
    if (!root)
    {
        return 0;
    }
    size_t count = 0;
    struct CCC_Priority_queue_node const *cur = root;
    do
    {
        count += 1 + traversal_count(cur->child);
    }
    while ((cur = cur->next) != root);
    return count;
}

static CCC_Tribool
has_valid_links(struct CCC_Priority_queue const *const priority_queue,
                struct CCC_Priority_queue_node const *const parent,
                struct CCC_Priority_queue_node const *const child)
{
    if (!child)
    {
        return CCC_TRUE;
    }
    struct CCC_Priority_queue_node const *current = child;
    CCC_Order const wrong_order = priority_queue->order == CCC_ORDER_LESSER
                                    ? CCC_ORDER_GREATER
                                    : CCC_ORDER_LESSER;
    do
    {
        /* Reminder: Don't combine these if checks into one. Separating them
           makes it easier to find the problem when stepping through gdb. */
        if (!current)
        {
            return CCC_FALSE;
        }
        if (parent && child->parent != parent)
        {
            return CCC_FALSE;
        }
        if (parent && parent->child != child->parent->child)
        {
            return CCC_FALSE;
        }
        if (child->next->prev != child || child->prev->next != child)
        {
            return CCC_FALSE;
        }
        if (parent && (order(priority_queue, parent, current) == wrong_order))
        {
            return CCC_FALSE;
        }
        if (!has_valid_links(priority_queue, current,
                             current->child)) /* ! RECURSE ! */
        {
            return CCC_FALSE;
        }
    }
    while ((current = current->next) != child);
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
