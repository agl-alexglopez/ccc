/** Copyright 2025 Alexander G. Lopez

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file contains my implementation of a handle realtime ordered map. The added
realtime prefix is to indicate that this map meets specific run time bounds
that can be relied upon consistently. This is may not be the case if a map
is implemented with some self-optimizing data structure like a Splay Tree.

This map, however, promises O(lg N) search, insert, and remove as a true
upper bound, inclusive. This is achieved through a Weak AVL (WAVL) tree
that is derived from the following two sources.

[1] Bernhard Haeupler, Siddhartha Sen, and Robert E. Tarjan, 2014.
Rank-Balanced Trees, J.ACM Transactions on Algorithms 11, 4, Article 0
(June 2015), 24 pages.
https://sidsen.azurewebsites.net//papers/rb-trees-talg.pdf

[2] Phil Vachon (pvachon) https://github.com/pvachon/wavl_tree
This implementation is heavily influential throughout. However there have
been some major adjustments and simplifications. Namely, the allocation has
been adjusted to accommodate this library's ability to be an allocating or
non-allocating container. All left-right symmetric cases have been united
into one and I chose to tackle rotations and deletions slightly differently,
shortening the code significantly. A few other changes and improvements
suggested by the authors of the original paper are implemented. Finally, the
data structure has been placed into a Buffer with relative indices rather
than pointers. See the required license at the bottom of the file for
BSD-2-Clause compliance.

Overall a WAVL tree is quite impressive for it's simplicity and purported
improvements over AVL and Red-Black trees. The rank framework is intuitive
and flexible in how it can be implemented. */
#include <assert.h>
#include <limits.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "handle_realtime_ordered_map.h"
#include "private/private_handle_realtime_ordered_map.h"
#include "private/private_types.h"
#include "types.h"

/*========================   Data Alignment Test   ==========================*/

/** @private A macro version of the runtime alignment operations we perform
for calculating bytes. This way we can use in static assert. The user data type
may not be the same alignment as the nodes and therefore the nodes array must
start at next aligned byte. Similarly the parity array may not be on an aligned
byte after the nodes array, though in the current implementation it is.
Regardless we always ensure the position is correct with respect to power of two
alignments in C. */
#define roundup(bytes_to_round, alignment)                                     \
    (((bytes_to_round) + (alignment) - 1) & ~((alignment) - 1))

enum : size_t
{
    /* @private Test capacity. */
    TCAP = 3,
};
/** @private Use an int because that will force the nodes array to be wary of
where to start. The nodes are 8 byte aligned but an int is 4. This means the
nodes need to start after a 4 byte Buffer of padding at end of data array. */
struct Test_data_type
{
    int i;
};
CCC_handle_realtime_ordered_map_declare_fixed_map(fixed_map_test_type,
                                                  struct Test_data_type, TCAP);
/** @private This is a static fixed size map exclusive to this translation unit
used to ensure assumptions about data layout are correct. The following static
asserts must be true in order to support the Struct of Array style layout we
use for the data, nodes, and parity arrays. It is important that in our user
code when we set the positions of the nodes and parity pointers relative to the
data pointer the positions are correct regardless of if our backing storage is
a fixed map or heap allocation. */
static fixed_map_test_type data_nodes_parity_layout_test;
/** Some assumptions in the code assume that parity array is last so ensure that
is the case here. Also good to assume user data comes first. */
static_assert(((char *)data_nodes_parity_layout_test.data
               < (char *)data_nodes_parity_layout_test.nodes)
                  && ((char *)data_nodes_parity_layout_test.nodes
                      < (char *)data_nodes_parity_layout_test.parity)
                  && ((char *)data_nodes_parity_layout_test.data
                      < (char *)data_nodes_parity_layout_test.parity),
              "The order of the arrays in a Struct of Arrays map is data, then "
              "nodes, then parity.");
/** We don't care about the alignment or padding after the parity array because
we never need to set or move any pointers to that position. The alignment is
important for the nodes and parity pointer to be set to the correct aligned
positions and so that we allocate enough bytes for our single allocation if
the map is dynamic and not a fixed type. */
static_assert(
    (char *)&data_nodes_parity_layout_test
                .parity[CCC_private_handle_realtime_ordered_map_blocks(TCAP)]
            - (char *)&data_nodes_parity_layout_test.data[0]
        == roundup((sizeof(*data_nodes_parity_layout_test.data) * TCAP),
                   alignof(*data_nodes_parity_layout_test.nodes))
               + roundup((sizeof(*data_nodes_parity_layout_test.nodes) * TCAP),
                         alignof(*data_nodes_parity_layout_test.parity))
               + (sizeof(*data_nodes_parity_layout_test.parity)
                  * CCC_private_handle_realtime_ordered_map_blocks(TCAP)),
    "The pointer difference in bytes between end of parity bit array and start "
    "of user data array must be the same as the total bytes we assume to be "
    "stored in that range. Alignment of user data must be considered.");
static_assert((char *)&data_nodes_parity_layout_test.data
                      + roundup((sizeof(*data_nodes_parity_layout_test.data)
                                 * TCAP),
                                alignof(*data_nodes_parity_layout_test.nodes))
                  == (char *)&data_nodes_parity_layout_test.nodes,
              "The start of the nodes array must begin at the next aligned "
              "byte given alignment of a node.");
static_assert(
    (char *)&data_nodes_parity_layout_test.parity
        == ((char *)&data_nodes_parity_layout_test.data
            + roundup((sizeof(*data_nodes_parity_layout_test.data) * TCAP),
                      alignof(*data_nodes_parity_layout_test.nodes))
            + roundup((sizeof(*data_nodes_parity_layout_test.nodes) * TCAP),
                      alignof(*data_nodes_parity_layout_test.parity))),
    "The start of the parity array must begin at the next aligned byte given "
    "alignment of both the data and nodes array.");

/*==========================  Type Declarations   ===========================*/

/** @private */
enum Branch
{
    L = 0,
    R,
};

/** @private To make insertions and removals more efficient we can remember the
last node encountered on the search for the requested node. It will either be
the correct node or the parent of the missing node if it is not found. This
means insertions will not need a second search of the tree and we can insert
immediately by adding the child. */
struct Query
{
    /** The last branch direction we took to the found or missing node. */
    CCC_Order last_order;
    union
    {
        /** The node was found so here is its index in the array. */
        size_t found;
        /** The node was not found so here is its direct parent. */
        size_t parent;
    };
};

#define INORDER R
#define RINORDER L
#define MINDIR L
#define MAXDIR R

enum
{
    SINGLE_TREE_NODE = 2,
};

/** @private A block of parity bits. */
typedef typeof(*(struct CCC_Handle_realtime_ordered_map){}.parity) Parity_block;

enum : size_t
{
    /** @private The number of bits in a block of parity bits. */
    PARITY_BLOCK_BITS = sizeof(Parity_block) * CHAR_BIT,
};

/*==============================  Prototypes   ==============================*/

/* Returning the user struct type with stored offsets. */
static void
insert(struct CCC_Handle_realtime_ordered_map *handle_realtime_ordered_map,
       size_t parent_i, CCC_Order last_order, size_t elem_i);
static CCC_Result
resize(struct CCC_Handle_realtime_ordered_map *handle_realtime_ordered_map,
       size_t new_capacity, CCC_Allocator *fn);
static void copy_soa(struct CCC_Handle_realtime_ordered_map const *src,
                     void *dst_data_base, size_t dst_capacity);
static size_t data_bytes(size_t sizeof_type, size_t capacity);
static size_t node_bytes(size_t capacity);
static size_t parity_bytes(size_t capacity);
static struct CCC_Handle_realtime_ordered_map_node *
node_pos(size_t sizeof_type, void const *data, size_t capacity);
static Parity_block *parity_pos(size_t sizeof_type, void const *data,
                                size_t capacity);
static size_t maybe_allocate_insert(
    struct CCC_Handle_realtime_ordered_map *handle_realtime_ordered_map,
    size_t parent, CCC_Order last_order, void const *user_type);
static size_t remove_fixup(struct CCC_Handle_realtime_ordered_map *t,
                           size_t remove);
static size_t allocate_slot(struct CCC_Handle_realtime_ordered_map *t);
static void delete_nodes(struct CCC_Handle_realtime_ordered_map *t,
                         CCC_Type_destructor *fn);
/* Returning the user key with stored offsets. */
static void *key_at(struct CCC_Handle_realtime_ordered_map const *t, size_t i);
static void *key_in_slot(struct CCC_Handle_realtime_ordered_map const *t,
                         void const *user_struct);
/* Returning the internal elem type with stored offsets. */
static struct CCC_Handle_realtime_ordered_map_node *
node_at(struct CCC_Handle_realtime_ordered_map const *, size_t);
static void *data_at(struct CCC_Handle_realtime_ordered_map const *, size_t);
/* Returning the internal query helper to aid in handle handling. */
static struct Query
find(struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map,
     void const *key);
/* Returning the handle core to the Handle Interface. */
static inline struct CCC_Handle_realtime_ordered_map_handle handle(
    struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map,
    void const *key);
/* Returning a generic range that can be use for range or rrange. */
static struct CCC_Range
equal_range(struct CCC_Handle_realtime_ordered_map const *, void const *,
            void const *, enum Branch);
/* Returning threeway comparison with user callback. */
static CCC_Order order_nodes(
    struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map,
    void const *key, size_t node, CCC_Key_comparator *fn);
/* Returning read only indices for tree nodes. */
static size_t sibling_of(struct CCC_Handle_realtime_ordered_map const *t,
                         size_t x);
static size_t next(struct CCC_Handle_realtime_ordered_map const *t, size_t n,
                   enum Branch traversal);
static size_t min_max_from(struct CCC_Handle_realtime_ordered_map const *t,
                           size_t start, enum Branch dir);
static size_t branch_i(struct CCC_Handle_realtime_ordered_map const *t,
                       size_t parent, enum Branch dir);
static size_t parent_i(struct CCC_Handle_realtime_ordered_map const *t,
                       size_t child);
static size_t index_of(struct CCC_Handle_realtime_ordered_map const *t,
                       void const *key_val_type);
/* Returning references to index fields for tree nodes. */
static size_t *branch_r(struct CCC_Handle_realtime_ordered_map const *t,
                        size_t node, enum Branch branch);
static size_t *parent_r(struct CCC_Handle_realtime_ordered_map const *t,
                        size_t node);
/* Returning WAVL tree status. */
static CCC_Tribool is_0_child(struct CCC_Handle_realtime_ordered_map const *,
                              size_t p, size_t x);
static CCC_Tribool is_1_child(struct CCC_Handle_realtime_ordered_map const *,
                              size_t p, size_t x);
static CCC_Tribool is_2_child(struct CCC_Handle_realtime_ordered_map const *,
                              size_t p, size_t x);
static CCC_Tribool is_3_child(struct CCC_Handle_realtime_ordered_map const *,
                              size_t p, size_t x);
static CCC_Tribool is_01_parent(struct CCC_Handle_realtime_ordered_map const *,
                                size_t x, size_t p, size_t y);
static CCC_Tribool is_11_parent(struct CCC_Handle_realtime_ordered_map const *,
                                size_t x, size_t p, size_t y);
static CCC_Tribool is_02_parent(struct CCC_Handle_realtime_ordered_map const *,
                                size_t x, size_t p, size_t y);
static CCC_Tribool is_22_parent(struct CCC_Handle_realtime_ordered_map const *,
                                size_t x, size_t p, size_t y);
static CCC_Tribool is_leaf(struct CCC_Handle_realtime_ordered_map const *t,
                           size_t x);
static CCC_Tribool parity(struct CCC_Handle_realtime_ordered_map const *t,
                          size_t node);
static void set_parity(struct CCC_Handle_realtime_ordered_map const *t,
                       size_t node, CCC_Tribool status);
static size_t total_bytes(size_t sizeof_type, size_t capacity);
static size_t block_count(size_t node_count);
static CCC_Tribool validate(
    struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map);
/* Returning void and maintaining the WAVL tree. */
static void init_node(struct CCC_Handle_realtime_ordered_map const *t,
                      size_t node);
static void insert_fixup(struct CCC_Handle_realtime_ordered_map *t, size_t z,
                         size_t x);
static void rebalance_3_child(struct CCC_Handle_realtime_ordered_map *t,
                              size_t z, size_t x);
static void transplant(struct CCC_Handle_realtime_ordered_map *t, size_t remove,
                       size_t replacement);
static void promote(struct CCC_Handle_realtime_ordered_map const *t, size_t x);
static void demote(struct CCC_Handle_realtime_ordered_map const *t, size_t x);
static void double_promote(struct CCC_Handle_realtime_ordered_map const *t,
                           size_t x);
static void double_demote(struct CCC_Handle_realtime_ordered_map const *t,
                          size_t x);

static void rotate(struct CCC_Handle_realtime_ordered_map *t, size_t z,
                   size_t x, size_t y, enum Branch dir);
static void double_rotate(struct CCC_Handle_realtime_ordered_map *t, size_t z,
                          size_t x, size_t y, enum Branch dir);
/* Returning void as miscellaneous helpers. */
static void swap(char tmp[const], void *a, void *b, size_t sizeof_type);
static size_t max(size_t, size_t);

/*==============================  Interface    ==============================*/

void *
CCC_handle_realtime_ordered_map_at(
    CCC_Handle_realtime_ordered_map const *const h, CCC_Handle_index const i)
{
    if (!h || !i)
    {
        return NULL;
    }
    return data_at(h, i);
}

CCC_Tribool
CCC_handle_realtime_ordered_map_contains(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    void const *const key)
{
    if (!handle_realtime_ordered_map || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return CCC_ORDER_EQUAL == find(handle_realtime_ordered_map, key).last_order;
}

CCC_Handle_index
CCC_handle_realtime_ordered_map_get_key_val(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    void const *const key)
{
    if (!handle_realtime_ordered_map || !key)
    {
        return 0;
    }
    struct Query const q = find(handle_realtime_ordered_map, key);
    return (CCC_ORDER_EQUAL == q.last_order) ? q.found : 0;
}

CCC_Handle
CCC_handle_realtime_ordered_map_swap_handle(
    CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    void *const key_val_type_output)
{
    if (!handle_realtime_ordered_map || !key_val_type_output)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct Query const q
        = find(handle_realtime_ordered_map,
               key_in_slot(handle_realtime_ordered_map, key_val_type_output));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        void *const slot = data_at(handle_realtime_ordered_map, q.found);
        void *const tmp = data_at(handle_realtime_ordered_map, 0);
        swap(tmp, key_val_type_output, slot,
             handle_realtime_ordered_map->sizeof_type);
        return (CCC_Handle){{
            .i = q.found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const i
        = maybe_allocate_insert(handle_realtime_ordered_map, q.parent,
                                q.last_order, key_val_type_output);
    if (!i)
    {
        return (CCC_Handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .i = i,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_handle_realtime_ordered_map_try_insert(
    CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    void const *const key_val_type)
{
    if (!handle_realtime_ordered_map || !key_val_type)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct Query const q
        = find(handle_realtime_ordered_map,
               key_in_slot(handle_realtime_ordered_map, key_val_type));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        return (CCC_Handle){{
            .i = q.found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const i = maybe_allocate_insert(
        handle_realtime_ordered_map, q.parent, q.last_order, key_val_type);
    if (!i)
    {
        return (CCC_Handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .i = i,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_handle_realtime_ordered_map_insert_or_assign(
    CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    void const *const key_val_type)
{
    if (!handle_realtime_ordered_map || !key_val_type)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct Query const q
        = find(handle_realtime_ordered_map,
               key_in_slot(handle_realtime_ordered_map, key_val_type));
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        void *const found = data_at(handle_realtime_ordered_map, q.found);
        (void)memcpy(found, key_val_type,
                     handle_realtime_ordered_map->sizeof_type);
        return (CCC_Handle){{
            .i = q.found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const i = maybe_allocate_insert(
        handle_realtime_ordered_map, q.parent, q.last_order, key_val_type);
    if (!i)
    {
        return (CCC_Handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .i = i,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle_realtime_ordered_map_handle *
CCC_handle_realtime_ordered_map_and_modify(
    CCC_Handle_realtime_ordered_map_handle *const h, CCC_Type_updater *const fn)
{
    if (h && fn && h->private.stats & CCC_ENTRY_OCCUPIED && h->private.i > 0)
    {
        fn((CCC_Type_context){
            .type
            = data_at(h->private.handle_realtime_ordered_map, h->private.i),
            NULL,
        });
    }
    return h;
}

CCC_Handle_realtime_ordered_map_handle *
CCC_handle_realtime_ordered_map_and_modify_context(
    CCC_Handle_realtime_ordered_map_handle *const h, CCC_Type_updater *const fn,
    void *const context)
{
    if (h && fn && h->private.stats & CCC_ENTRY_OCCUPIED
        && h->private.stats > 0)
    {
        fn((CCC_Type_context){
            .type
            = data_at(h->private.handle_realtime_ordered_map, h->private.i),
            context,
        });
    }
    return h;
}

CCC_Handle_index
CCC_handle_realtime_ordered_map_or_insert(
    CCC_Handle_realtime_ordered_map_handle const *const h,
    void const *const key_val_type)
{
    if (!h || !key_val_type)
    {
        return 0;
    }
    if (h->private.stats == CCC_ENTRY_OCCUPIED)
    {
        return h->private.i;
    }
    return maybe_allocate_insert(h->private.handle_realtime_ordered_map,
                                 h->private.i, h->private.last_order,
                                 key_val_type);
}

CCC_Handle_index
CCC_handle_realtime_ordered_map_insert_handle(
    CCC_Handle_realtime_ordered_map_handle const *const h,
    void const *const key_val_type)
{
    if (!h || !key_val_type)
    {
        return 0;
    }
    if (h->private.stats == CCC_ENTRY_OCCUPIED)
    {
        void *const slot
            = data_at(h->private.handle_realtime_ordered_map, h->private.i);
        if (slot != key_val_type)
        {
            (void)memcpy(slot, key_val_type,
                         h->private.handle_realtime_ordered_map->sizeof_type);
        }
        return h->private.i;
    }
    return maybe_allocate_insert(h->private.handle_realtime_ordered_map,
                                 h->private.i, h->private.last_order,
                                 key_val_type);
}

CCC_Handle_realtime_ordered_map_handle
CCC_handle_realtime_ordered_map_handle(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    void const *const key)
{
    if (!handle_realtime_ordered_map || !key)
    {
        return (CCC_Handle_realtime_ordered_map_handle){
            {.stats = CCC_ENTRY_ARG_ERROR}};
    }
    return (CCC_Handle_realtime_ordered_map_handle){
        handle(handle_realtime_ordered_map, key)};
}

CCC_Handle
CCC_handle_realtime_ordered_map_remove_handle(
    CCC_Handle_realtime_ordered_map_handle const *const h)
{
    if (!h)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (h->private.stats == CCC_ENTRY_OCCUPIED)
    {
        size_t const ret = remove_fixup(h->private.handle_realtime_ordered_map,
                                        h->private.i);
        return (CCC_Handle){{
            .i = ret,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Handle){{
        .i = 0,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_handle_realtime_ordered_map_remove(
    CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    void *const key_val_type_output)
{
    if (!handle_realtime_ordered_map || !key_val_type_output)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    struct Query const q
        = find(handle_realtime_ordered_map,
               key_in_slot(handle_realtime_ordered_map, key_val_type_output));
    if (q.last_order != CCC_ORDER_EQUAL)
    {
        return (CCC_Handle){{
            .i = 0,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    size_t const removed = remove_fixup(handle_realtime_ordered_map, q.found);
    assert(removed);
    void const *const r = data_at(handle_realtime_ordered_map, removed);
    if (key_val_type_output != r)
    {
        (void)memcpy(key_val_type_output, r,
                     handle_realtime_ordered_map->sizeof_type);
    }
    return (CCC_Handle){{
        .i = 0,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

CCC_Range
CCC_handle_realtime_ordered_map_equal_range(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    void const *const begin_key, void const *const end_key)
{
    if (!handle_realtime_ordered_map || !begin_key || !end_key)
    {
        return (CCC_Range){};
    }
    return (CCC_Range){
        equal_range(handle_realtime_ordered_map, begin_key, end_key, INORDER)};
}

CCC_Reverse_range
CCC_handle_realtime_ordered_map_equal_rrange(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    void const *const rbegin_key, void const *const rend_key)
{
    if (!handle_realtime_ordered_map || !rbegin_key || !rend_key)
    {
        return (CCC_Reverse_range){};
    }
    return (CCC_Reverse_range){equal_range(handle_realtime_ordered_map,
                                           rbegin_key, rend_key, RINORDER)};
}

CCC_Handle_index
CCC_handle_realtime_ordered_map_unwrap(
    CCC_Handle_realtime_ordered_map_handle const *const h)
{
    if (h && h->private.stats & CCC_ENTRY_OCCUPIED && h->private.i > 0)
    {
        return h->private.i;
    }
    return 0;
}

CCC_Tribool
CCC_handle_realtime_ordered_map_insert_error(
    CCC_Handle_realtime_ordered_map_handle const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (h->private.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_handle_realtime_ordered_map_occupied(
    CCC_Handle_realtime_ordered_map_handle const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (h->private.stats & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Handle_status
CCC_handle_realtime_ordered_map_handle_status(
    CCC_Handle_realtime_ordered_map_handle const *const h)
{
    return h ? h->private.stats : CCC_ENTRY_ARG_ERROR;
}

CCC_Tribool
CCC_handle_realtime_ordered_map_is_empty(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map)
{
    if (!handle_realtime_ordered_map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !CCC_handle_realtime_ordered_map_count(handle_realtime_ordered_map)
                .count;
}

CCC_Count
CCC_handle_realtime_ordered_map_count(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map)
{
    if (!handle_realtime_ordered_map)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    if (!handle_realtime_ordered_map->count)
    {
        return (CCC_Count){.count = 0};
    }
    /* The root slot is occupied at 0 but don't don't tell user. */
    return (CCC_Count){.count = handle_realtime_ordered_map->count - 1};
}

CCC_Count
CCC_handle_realtime_ordered_map_capacity(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map)
{
    if (!handle_realtime_ordered_map)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = handle_realtime_ordered_map->capacity};
}

void *
CCC_handle_realtime_ordered_map_begin(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map)
{
    if (!handle_realtime_ordered_map || !handle_realtime_ordered_map->capacity)
    {
        return NULL;
    }
    size_t const n = min_max_from(handle_realtime_ordered_map,
                                  handle_realtime_ordered_map->root, MINDIR);
    return data_at(handle_realtime_ordered_map, n);
}

void *
CCC_handle_realtime_ordered_map_rbegin(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map)
{
    if (!handle_realtime_ordered_map || !handle_realtime_ordered_map->capacity)
    {
        return NULL;
    }
    size_t const n = min_max_from(handle_realtime_ordered_map,
                                  handle_realtime_ordered_map->root, MAXDIR);
    return data_at(handle_realtime_ordered_map, n);
}

void *
CCC_handle_realtime_ordered_map_next(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    void const *const key_val_type_iter)
{
    if (!handle_realtime_ordered_map || !key_val_type_iter
        || !handle_realtime_ordered_map->capacity)
    {
        return NULL;
    }
    size_t const n = next(
        handle_realtime_ordered_map,
        index_of(handle_realtime_ordered_map, key_val_type_iter), INORDER);
    return data_at(handle_realtime_ordered_map, n);
}

void *
CCC_handle_realtime_ordered_map_rnext(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map,
    void const *const key_val_type_iter)
{
    if (!handle_realtime_ordered_map || !key_val_type_iter
        || !handle_realtime_ordered_map->capacity)
    {
        return NULL;
    }
    size_t const n = next(
        handle_realtime_ordered_map,
        index_of(handle_realtime_ordered_map, key_val_type_iter), RINORDER);
    return data_at(handle_realtime_ordered_map, n);
}

void *
CCC_handle_realtime_ordered_map_end(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map)
{
    if (!handle_realtime_ordered_map || !handle_realtime_ordered_map->capacity)
    {
        return NULL;
    }
    return data_at(handle_realtime_ordered_map, 0);
}

void *
CCC_handle_realtime_ordered_map_rend(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map)
{
    if (!handle_realtime_ordered_map || !handle_realtime_ordered_map->capacity)
    {
        return NULL;
    }
    return data_at(handle_realtime_ordered_map, 0);
}

CCC_Result
CCC_handle_realtime_ordered_map_reserve(
    CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    size_t const to_add, CCC_Allocator *const fn)
{
    if (!handle_realtime_ordered_map || !fn)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Once initialized the Buffer always has a size of one for root node. */
    size_t const needed = handle_realtime_ordered_map->count + to_add
                        + (handle_realtime_ordered_map->count == 0);
    if (needed <= handle_realtime_ordered_map->capacity)
    {
        return CCC_RESULT_OK;
    }
    size_t const old_count = handle_realtime_ordered_map->count;
    size_t old_cap = handle_realtime_ordered_map->capacity;
    CCC_Result const r = resize(handle_realtime_ordered_map, needed, fn);
    if (r != CCC_RESULT_OK)
    {
        return r;
    }
    set_parity(handle_realtime_ordered_map, 0, CCC_TRUE);
    if (!old_cap)
    {
        handle_realtime_ordered_map->count = 1;
    }
    old_cap = old_count ? old_cap : 0;
    size_t const new_cap = handle_realtime_ordered_map->capacity;
    size_t prev = 0;
    for (ptrdiff_t i = (ptrdiff_t)new_cap - 1; i > 0 && i >= (ptrdiff_t)old_cap;
         prev = i, --i)
    {
        node_at(handle_realtime_ordered_map, i)->next_free = prev;
    }
    if (!handle_realtime_ordered_map->free_list)
    {
        handle_realtime_ordered_map->free_list = prev;
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_handle_realtime_ordered_map_copy(
    CCC_Handle_realtime_ordered_map *const dst,
    CCC_Handle_realtime_ordered_map const *const src, CCC_Allocator *const fn)
{
    if (!dst || !src || src == dst || (dst->capacity < src->capacity && !fn))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    void *const dst_mem = dst->data;
    struct CCC_Handle_realtime_ordered_map_node *const dst_nodes = dst->nodes;
    Parity_block *const dst_parity = dst->parity;
    size_t const dst_cap = dst->capacity;
    CCC_Allocator *const dst_allocate = dst->allocate;
    *dst = *src;
    dst->data = dst_mem;
    dst->nodes = dst_nodes;
    dst->parity = dst_parity;
    dst->capacity = dst_cap;
    dst->allocate = dst_allocate;
    if (!src->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (dst->capacity < src->capacity)
    {
        CCC_Result const r = resize(dst, src->capacity, fn);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
    }
    else
    {
        /* Might not be necessary but not worth finding out. Do every time. */
        dst->nodes = node_pos(dst->sizeof_type, dst->data, dst->capacity);
        dst->parity = parity_pos(dst->sizeof_type, dst->data, dst->capacity);
    }
    if (!dst->data || !src->data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    copy_soa(src, dst->data, dst->capacity);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_handle_realtime_ordered_map_clear(
    CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    CCC_Type_destructor *const fn)
{
    if (!handle_realtime_ordered_map)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!fn)
    {
        handle_realtime_ordered_map->root = 0;
        handle_realtime_ordered_map->count = 1;
        return CCC_RESULT_OK;
    }
    delete_nodes(handle_realtime_ordered_map, fn);
    handle_realtime_ordered_map->count = 1;
    handle_realtime_ordered_map->root = 0;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_handle_realtime_ordered_map_clear_and_free(
    CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    CCC_Type_destructor *const fn)
{
    if (!handle_realtime_ordered_map || !handle_realtime_ordered_map->allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!fn)
    {
        handle_realtime_ordered_map->root = 0;
        handle_realtime_ordered_map->count = 0;
        handle_realtime_ordered_map->capacity = 0;
        (void)handle_realtime_ordered_map->allocate((CCC_Allocator_context){
            .input = handle_realtime_ordered_map->data,
            .bytes = 0,
            .context = handle_realtime_ordered_map->context,
        });
        handle_realtime_ordered_map->data = NULL;
        handle_realtime_ordered_map->nodes = NULL;
        handle_realtime_ordered_map->parity = NULL;
        return CCC_RESULT_OK;
    }
    delete_nodes(handle_realtime_ordered_map, fn);
    handle_realtime_ordered_map->root = 0;
    handle_realtime_ordered_map->capacity = 0;
    (void)handle_realtime_ordered_map->allocate((CCC_Allocator_context){
        .input = handle_realtime_ordered_map->data,
        .bytes = 0,
        .context = handle_realtime_ordered_map->context});
    handle_realtime_ordered_map->data = NULL;
    handle_realtime_ordered_map->nodes = NULL;
    handle_realtime_ordered_map->parity = NULL;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_handle_realtime_ordered_map_clear_and_free_reserve(
    CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    CCC_Type_destructor *const destructor, CCC_Allocator *const allocate)
{
    if (!handle_realtime_ordered_map || !allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        handle_realtime_ordered_map->root = 0;
        handle_realtime_ordered_map->count = 0;
        handle_realtime_ordered_map->capacity = 0;
        (void)allocate((CCC_Allocator_context){
            .input = handle_realtime_ordered_map->data,
            .bytes = 0,
            .context = handle_realtime_ordered_map->context,
        });
        handle_realtime_ordered_map->data = NULL;
        return CCC_RESULT_OK;
    }
    delete_nodes(handle_realtime_ordered_map, destructor);
    handle_realtime_ordered_map->root = 0;
    handle_realtime_ordered_map->capacity = 0;
    (void)allocate((CCC_Allocator_context){
        .input = handle_realtime_ordered_map->data,
        .bytes = 0,
        .context = handle_realtime_ordered_map->context,
    });
    handle_realtime_ordered_map->data = NULL;
    handle_realtime_ordered_map->nodes = NULL;
    handle_realtime_ordered_map->parity = NULL;
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_handle_realtime_ordered_map_validate(
    CCC_Handle_realtime_ordered_map const *const handle_realtime_ordered_map)
{
    if (!handle_realtime_ordered_map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(handle_realtime_ordered_map);
}

/*========================  Private Interface  ==============================*/

void
CCC_private_handle_realtime_ordered_map_insert(
    struct CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    size_t const parent_i, CCC_Order const last_order, size_t const elem_i)
{
    insert(handle_realtime_ordered_map, parent_i, last_order, elem_i);
}

struct CCC_Handle_realtime_ordered_map_handle
CCC_private_handle_realtime_ordered_map_handle(
    struct CCC_Handle_realtime_ordered_map const *const
        handle_realtime_ordered_map,
    void const *const key)
{
    return handle(handle_realtime_ordered_map, key);
}

void *
CCC_private_handle_realtime_ordered_map_data_at(
    struct CCC_Handle_realtime_ordered_map const *const
        handle_realtime_ordered_map,
    size_t const slot)
{
    return data_at(handle_realtime_ordered_map, slot);
}

void *
CCC_private_handle_realtime_ordered_map_key_at(
    struct CCC_Handle_realtime_ordered_map const *const
        handle_realtime_ordered_map,
    size_t const slot)
{
    return key_at(handle_realtime_ordered_map, slot);
}

struct CCC_Handle_realtime_ordered_map_node *
CCC_private_handle_realtime_ordered_map_node_at(
    struct CCC_Handle_realtime_ordered_map const *handle_realtime_ordered_map,
    size_t const i)
{
    return node_at(handle_realtime_ordered_map, i);
}

size_t
CCC_private_handle_realtime_ordered_map_allocate_slot(
    struct CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map)
{
    return allocate_slot(handle_realtime_ordered_map);
}

/*==========================  Static Helpers   ==============================*/

static size_t
maybe_allocate_insert(
    struct CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    size_t const parent, CCC_Order const last_order,
    void const *const user_type)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const node = allocate_slot(handle_realtime_ordered_map);
    if (!node)
    {
        return 0;
    }
    (void)memcpy(data_at(handle_realtime_ordered_map, node), user_type,
                 handle_realtime_ordered_map->sizeof_type);
    insert(handle_realtime_ordered_map, parent, last_order, node);
    return node;
}

static size_t
allocate_slot(struct CCC_Handle_realtime_ordered_map *const t)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const old_count = t->count;
    size_t old_cap = t->capacity;
    if (!old_count || old_count == old_cap)
    {
        assert(!t->free_list);
        if (old_count == old_cap)
        {
            if (resize(t, max(old_cap * 2, PARITY_BLOCK_BITS), t->allocate)
                != CCC_RESULT_OK)
            {
                return 0;
            }
        }
        else
        {
            t->nodes = node_pos(t->sizeof_type, t->data, t->capacity);
            t->parity = parity_pos(t->sizeof_type, t->data, t->capacity);
        }
        old_cap = old_count ? old_cap : 0;
        size_t const new_cap = t->capacity;
        size_t prev = 0;
        for (size_t i = new_cap - 1; i > 0 && i >= old_cap; prev = i, --i)
        {
            node_at(t, i)->next_free = prev;
        }
        t->free_list = prev;
        t->count = max(old_count, 1);
        set_parity(t, 0, CCC_TRUE);
    }
    if (!t->free_list)
    {
        return 0;
    }
    ++t->count;
    size_t const slot = t->free_list;
    t->free_list = node_at(t, slot)->next_free;
    return slot;
}

static CCC_Result
resize(
    struct CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    size_t const new_capacity, CCC_Allocator *const fn)
{
    if (handle_realtime_ordered_map->capacity
        && new_capacity <= handle_realtime_ordered_map->capacity - 1)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    void *const new_data = fn((CCC_Allocator_context){
        .input = NULL,
        .bytes
        = total_bytes(handle_realtime_ordered_map->sizeof_type, new_capacity),
        .context = handle_realtime_ordered_map->context,
    });
    if (!new_data)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    copy_soa(handle_realtime_ordered_map, new_data, new_capacity);
    handle_realtime_ordered_map->nodes = node_pos(
        handle_realtime_ordered_map->sizeof_type, new_data, new_capacity);
    handle_realtime_ordered_map->parity = parity_pos(
        handle_realtime_ordered_map->sizeof_type, new_data, new_capacity);
    fn((CCC_Allocator_context){
        .input = handle_realtime_ordered_map->data,
        .bytes = 0,
        .context = handle_realtime_ordered_map->context,
    });
    handle_realtime_ordered_map->data = new_data;
    handle_realtime_ordered_map->capacity = new_capacity;
    return CCC_RESULT_OK;
}

static void
insert(
    struct CCC_Handle_realtime_ordered_map *const handle_realtime_ordered_map,
    size_t const parent_i, CCC_Order const last_order, size_t const elem_i)
{
    struct CCC_Handle_realtime_ordered_map_node *elem
        = node_at(handle_realtime_ordered_map, elem_i);
    init_node(handle_realtime_ordered_map, elem_i);
    if (handle_realtime_ordered_map->count == SINGLE_TREE_NODE)
    {
        handle_realtime_ordered_map->root = elem_i;
        return;
    }
    assert(last_order == CCC_ORDER_GREATER || last_order == CCC_ORDER_LESSER);
    struct CCC_Handle_realtime_ordered_map_node *parent
        = node_at(handle_realtime_ordered_map, parent_i);
    CCC_Tribool const rank_rule_break
        = !parent->branch[L] && !parent->branch[R];
    parent->branch[CCC_ORDER_GREATER == last_order] = elem_i;
    elem->parent = parent_i;
    if (rank_rule_break)
    {
        insert_fixup(handle_realtime_ordered_map, parent_i, elem_i);
    }
}

static struct CCC_Handle_realtime_ordered_map_handle
handle(struct CCC_Handle_realtime_ordered_map const *const
           handle_realtime_ordered_map,
       void const *const key)
{
    struct Query const q = find(handle_realtime_ordered_map, key);
    if (CCC_ORDER_EQUAL == q.last_order)
    {
        return (struct CCC_Handle_realtime_ordered_map_handle){
            .handle_realtime_ordered_map
            = (struct CCC_Handle_realtime_ordered_map *)
                handle_realtime_ordered_map,
            .last_order = q.last_order,
            .i = q.found,
            .stats = CCC_ENTRY_OCCUPIED,
        };
    }
    return (struct CCC_Handle_realtime_ordered_map_handle){
        .handle_realtime_ordered_map
        = (struct CCC_Handle_realtime_ordered_map *)handle_realtime_ordered_map,
        .last_order = q.last_order,
        .i = q.parent,
        .stats = CCC_ENTRY_NO_UNWRAP | CCC_ENTRY_VACANT,
    };
}

static struct Query
find(struct CCC_Handle_realtime_ordered_map const *const
         handle_realtime_ordered_map,
     void const *const key)
{
    size_t parent = 0;
    struct Query q = {.last_order = CCC_ORDER_ERROR,
                      .found = handle_realtime_ordered_map->root};
    while (q.found)
    {
        q.last_order = order_nodes(handle_realtime_ordered_map, key, q.found,
                                   handle_realtime_ordered_map->order);
        if (CCC_ORDER_EQUAL == q.last_order)
        {
            return q;
        }
        parent = q.found;
        q.found = branch_i(handle_realtime_ordered_map, q.found,
                           CCC_ORDER_GREATER == q.last_order);
    }
    /* Type punning here OK as both union members have same type and size. */
    q.parent = parent;
    return q;
}

static size_t
next(struct CCC_Handle_realtime_ordered_map const *const t, size_t n,
     enum Branch const traversal)
{
    if (!n)
    {
        return 0;
    }
    assert(!parent_i(t, t->root));
    /* The node is an internal one that has a sub-tree to explore first. */
    if (branch_i(t, n, traversal))
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = branch_i(t, n, traversal); branch_i(t, n, !traversal);
             n = branch_i(t, n, !traversal))
        {}
        return n;
    }
    /* This is how to return internal nodes on the way back up from a leaf. */
    size_t p = parent_i(t, n);
    for (; p && branch_i(t, p, !traversal) != n; n = p, p = parent_i(t, p))
    {}
    return p;
}

static struct CCC_Range
equal_range(struct CCC_Handle_realtime_ordered_map const *const t,
            void const *const begin_key, void const *const end_key,
            enum Branch const traversal)
{
    if (CCC_handle_realtime_ordered_map_is_empty(t))
    {
        return (struct CCC_Range){};
    }
    CCC_Order const les_or_grt[2] = {CCC_ORDER_LESSER, CCC_ORDER_GREATER};
    struct Query b = find(t, begin_key);
    if (b.last_order == les_or_grt[traversal])
    {
        b.found = next(t, b.found, traversal);
    }
    struct Query e = find(t, end_key);
    if (e.last_order != les_or_grt[!traversal])
    {
        e.found = next(t, e.found, traversal);
    }
    return (struct CCC_Range){
        .begin = data_at(t, b.found),
        .end = data_at(t, e.found),
    };
}

static size_t
min_max_from(struct CCC_Handle_realtime_ordered_map const *const t,
             size_t start, enum Branch const dir)
{
    if (!start)
    {
        return 0;
    }
    for (; branch_i(t, start, dir); start = branch_i(t, start, dir))
    {}
    return start;
}

/** Deletes all nodes in the tree by calling destructor function on them in
linear time and constant space. This function modifies nodes as it deletes the
tree elements. Assumes the destructor function is non-null.

This function does not update any count or capacity fields of the map, it
simply calls the destructor on each node and removes the nodes references to
other tree elements. */
static void
delete_nodes(struct CCC_Handle_realtime_ordered_map *const t,
             CCC_Type_destructor *const fn)
{
    assert(t);
    assert(fn);
    size_t node = t->root;
    while (node)
    {
        struct CCC_Handle_realtime_ordered_map_node *const e = node_at(t, node);
        if (e->branch[L])
        {
            size_t const left = e->branch[L];
            e->branch[L] = node_at(t, left)->branch[R];
            node_at(t, left)->branch[R] = node;
            node = left;
            continue;
        }
        size_t const next = e->branch[R];
        e->branch[L] = e->branch[R] = 0;
        e->parent = 0;
        fn((CCC_Type_context){
            .type = data_at(t, node),
            .context = t->context,
        });
        node = next;
    }
}

static inline CCC_Order
order_nodes(struct CCC_Handle_realtime_ordered_map const *const
                handle_realtime_ordered_map,
            void const *const key, size_t const node,
            CCC_Key_comparator *const fn)
{
    return fn((CCC_Key_comparator_context){
        .key_lhs = key,
        .type_rhs = data_at(handle_realtime_ordered_map, node),
        .context = handle_realtime_ordered_map->context,
    });
}

/** Calculates the number of bytes needed for user data INCLUDING any bytes we
need to add to the end of the array such that the following nodes array starts
on an aligned byte boundary given the alignment requirements of a node. This
means the value returned from this function may or may not be slightly larger
then the raw size of just user elements if rounding up must occur. */
static inline size_t
data_bytes(size_t const sizeof_type, size_t const capacity)
{
    return ((sizeof_type * capacity)
            + alignof(*(struct CCC_Handle_realtime_ordered_map){}.nodes) - 1)
         & ~(alignof(*(struct CCC_Handle_realtime_ordered_map){}.nodes) - 1);
}

/** Calculates the number of bytes needed for the nodes array INCLUDING any
bytes we need to add to the end of the array such that the following parity bit
array starts on an aligned byte boundary given the alignment requirements of
a parity block. This means the value returned from this function may or may not
be slightly larger then the raw size of just the nodes array if rounding up must
occur. */
static inline size_t
node_bytes(size_t const capacity)
{
    return ((sizeof(*(struct CCC_Handle_realtime_ordered_map){}.nodes)
             * capacity)
            + alignof(*(struct CCC_Handle_realtime_ordered_map){}.parity) - 1)
         & ~(alignof(*(struct CCC_Handle_realtime_ordered_map){}.parity) - 1);
}

/** Calculates the number of bytes needed for the parity block bit array. No
rounding up or alignment concerns need apply because this is the last array
in the allocation. */
static inline size_t
parity_bytes(size_t capacity)
{
    return sizeof(Parity_block) * block_count(capacity);
}

/** Calculates the number of bytes needed for all arrays in the Struct of Arrays
map design INCLUDING any extra padding bytes that need to be added between the
data and node arrays and the node and parity arrays. Padding might be needed if
the alignment of the type in next array that follows a preceding array is
different from the preceding array. In that case it is the preceding array's
responsibility to add padding bytes to its end such that the next array begins
on an aligned byte boundary for its own type. This means that the bytes returned
by this function may be greater than summing the (sizeof(type) * capacity) for
each array in the conceptual struct. */
static inline size_t
total_bytes(size_t sizeof_type, size_t const capacity)
{
    return data_bytes(sizeof_type, capacity) + node_bytes(capacity)
         + parity_bytes(capacity);
}

/** Returns the base of the node array relative to the data base pointer. This
positions is guaranteed to be the first aligned byte given the alignment of the
node type after the data array. The data array has added any necessary padding
after it to ensure that the base of the node array is aligned for its type. */
static inline struct CCC_Handle_realtime_ordered_map_node *
node_pos(size_t const sizeof_type, void const *const data,
         size_t const capacity)
{
    return (
        struct CCC_Handle_realtime_ordered_map_node *)((char *)data
                                                       + data_bytes(sizeof_type,
                                                                    capacity));
}

/** Returns the base of the parity array relative to the data base pointer. This
positions is guaranteed to be the first aligned byte given the alignment of the
parity block type after the data and node arrays. The node array has added any
necessary padding after it to ensure that the base of the parity block array is
aligned for its type. */
static inline Parity_block *
parity_pos(size_t const sizeof_type, void const *const data,
           size_t const capacity)
{
    return (Parity_block *)((char *)data + data_bytes(sizeof_type, capacity)
                            + node_bytes(capacity));
}

/** Copies over the Struct of Arrays contained within the one contiguous
allocation of the map to the new memory provided. Assumes the new_data pointer
points to the base of an allocation that has been allocated with sufficient
bytes to support the user data, nodes, and parity arrays for the provided new
capacity. */
static inline void
copy_soa(struct CCC_Handle_realtime_ordered_map const *const src,
         void *const dst_data_base, size_t const dst_capacity)
{
    if (!src->data)
    {
        return;
    }
    assert(dst_capacity >= src->capacity);
    size_t const sizeof_type = src->sizeof_type;
    /* Each section of the allocation "grows" when we re-size so one copy would
       not work. Instead each component is copied over allowing each to grow. */
    (void)memcpy(dst_data_base, src->data,
                 data_bytes(sizeof_type, src->capacity));
    (void)memcpy(node_pos(sizeof_type, dst_data_base, dst_capacity),
                 node_pos(sizeof_type, src->data, src->capacity),
                 node_bytes(src->capacity));
    (void)memcpy(parity_pos(sizeof_type, dst_data_base, dst_capacity),
                 parity_pos(sizeof_type, src->data, src->capacity),
                 parity_bytes(src->capacity));
}

static inline void
init_node(struct CCC_Handle_realtime_ordered_map const *const t,
          size_t const node)
{
    set_parity(t, node, CCC_FALSE);
    struct CCC_Handle_realtime_ordered_map_node *const e = node_at(t, node);
    e->branch[L] = e->branch[R] = e->parent = 0;
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const sizeof_type)
{
    if (a == b || !a || !b)
    {
        return;
    }
    (void)memcpy(tmp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, tmp, sizeof_type);
}

static inline struct CCC_Handle_realtime_ordered_map_node *
node_at(struct CCC_Handle_realtime_ordered_map const *const t, size_t const i)
{
    return &t->nodes[i];
}

static inline void *
data_at(struct CCC_Handle_realtime_ordered_map const *const t, size_t const i)
{
    return (char *)t->data + (t->sizeof_type * i);
}

static inline Parity_block *
block_at(struct CCC_Handle_realtime_ordered_map const *const t, size_t const i)
{
    return &t->parity[i / PARITY_BLOCK_BITS];
}

static inline Parity_block
bit_on(size_t const i)
{
    static_assert((PARITY_BLOCK_BITS & (PARITY_BLOCK_BITS - 1)) == 0,
                  "the number of bits in a block is always a power of two, "
                  "avoiding modulo operations.");
    return ((Parity_block)1) << (i & (PARITY_BLOCK_BITS - 1));
}

static inline size_t
branch_i(struct CCC_Handle_realtime_ordered_map const *const t,
         size_t const parent, enum Branch const dir)
{
    return node_at(t, parent)->branch[dir];
}

static inline size_t
parent_i(struct CCC_Handle_realtime_ordered_map const *const t,
         size_t const child)
{
    return node_at(t, child)->parent;
}

static inline size_t
index_of(struct CCC_Handle_realtime_ordered_map const *const t,
         void const *const key_val_type)
{
    assert(key_val_type >= t->data
           && (char *)key_val_type
                  < ((char *)t->data + (t->capacity * t->sizeof_type)));
    return ((char *)key_val_type - (char *)t->data) / t->sizeof_type;
}

static inline CCC_Tribool
parity(struct CCC_Handle_realtime_ordered_map const *const t, size_t const node)
{
    return (*block_at(t, node) & bit_on(node)) != 0;
}

static inline void
set_parity(struct CCC_Handle_realtime_ordered_map const *const t,
           size_t const node, CCC_Tribool const status)
{
    if (status)
    {
        *block_at(t, node) |= bit_on(node);
    }
    else
    {
        *block_at(t, node) &= ~bit_on(node);
    }
}

static inline size_t
block_count(size_t const node_count)
{
    return (node_count + (PARITY_BLOCK_BITS - 1)) / PARITY_BLOCK_BITS;
}

static inline size_t *
branch_r(struct CCC_Handle_realtime_ordered_map const *t, size_t const node,
         enum Branch const branch)
{
    return &node_at(t, node)->branch[branch];
}

static inline size_t *
parent_r(struct CCC_Handle_realtime_ordered_map const *t, size_t const node)
{

    return &node_at(t, node)->parent;
}

static inline void *
key_at(struct CCC_Handle_realtime_ordered_map const *const t, size_t const i)
{
    return (char *)data_at(t, i) + t->key_offset;
}

static void *
key_in_slot(struct CCC_Handle_realtime_ordered_map const *t,
            void const *const user_struct)
{
    return (char *)user_struct + t->key_offset;
}

/*=======================   WAVL Tree Maintenance   =========================*/

static void
insert_fixup(struct CCC_Handle_realtime_ordered_map *const t, size_t z,
             size_t x)
{
    do
    {
        promote(t, z);
        x = z;
        z = parent_i(t, z);
        if (!z)
        {
            return;
        }
    }
    while (is_01_parent(t, x, z, sibling_of(t, x)));

    if (!is_02_parent(t, x, z, sibling_of(t, x)))
    {
        return;
    }
    assert(x);
    assert(is_0_child(t, z, x));
    enum Branch const p_to_x_dir = branch_i(t, z, R) == x;
    size_t const y = branch_i(t, x, !p_to_x_dir);
    if (!y || is_2_child(t, z, y))
    {
        rotate(t, z, x, y, !p_to_x_dir);
        demote(t, z);
    }
    else
    {
        assert(is_1_child(t, z, y));
        double_rotate(t, z, x, y, p_to_x_dir);
        promote(t, y);
        demote(t, x);
        demote(t, z);
    }
}

static size_t
remove_fixup(struct CCC_Handle_realtime_ordered_map *const t,
             size_t const remove)
{
    size_t y = 0;
    size_t x = 0;
    size_t p = 0;
    CCC_Tribool two_child = CCC_FALSE;
    if (!branch_i(t, remove, R) || !branch_i(t, remove, L))
    {
        y = remove;
        p = parent_i(t, y);
        x = branch_i(t, y, !branch_i(t, y, L));
        *parent_r(t, x) = parent_i(t, y);
        if (!p)
        {
            t->root = x;
        }
        two_child = is_2_child(t, p, y);
        *branch_r(t, p, branch_i(t, p, R) == y) = x;
    }
    else
    {
        y = min_max_from(t, branch_i(t, remove, R), MINDIR);
        p = parent_i(t, y);
        x = branch_i(t, y, !branch_i(t, y, L));
        *parent_r(t, x) = parent_i(t, y);

        /* Save if check and improve readability by assuming this is true. */
        assert(p);

        two_child = is_2_child(t, p, y);
        *branch_r(t, p, branch_i(t, p, R) == y) = x;
        transplant(t, remove, y);
        if (remove == p)
        {
            p = y;
        }
    }

    if (p)
    {
        if (two_child)
        {
            assert(p);
            rebalance_3_child(t, p, x);
        }
        else if (!x && branch_i(t, p, L) == branch_i(t, p, R))
        {
            assert(p);
            CCC_Tribool const demote_makes_3_child
                = is_2_child(t, parent_i(t, p), p);
            demote(t, p);
            if (demote_makes_3_child)
            {
                rebalance_3_child(t, parent_i(t, p), p);
            }
        }
        assert(!is_leaf(t, p) || !parity(t, p));
    }
    node_at(t, remove)->next_free = t->free_list;
    t->free_list = remove;
    --t->count;
    return remove;
}

static void
transplant(struct CCC_Handle_realtime_ordered_map *const t, size_t const remove,
           size_t const replacement)
{
    assert(remove);
    assert(replacement);
    *parent_r(t, replacement) = parent_i(t, remove);
    if (!parent_i(t, remove))
    {
        t->root = replacement;
    }
    else
    {
        size_t const p = parent_i(t, remove);
        *branch_r(t, p, branch_i(t, p, R) == remove) = replacement;
    }
    struct CCC_Handle_realtime_ordered_map_node *const remove_r
        = node_at(t, remove);
    struct CCC_Handle_realtime_ordered_map_node *const replace_r
        = node_at(t, replacement);
    *parent_r(t, remove_r->branch[R]) = replacement;
    *parent_r(t, remove_r->branch[L]) = replacement;
    replace_r->branch[R] = remove_r->branch[R];
    replace_r->branch[L] = remove_r->branch[L];
    set_parity(t, replacement, parity(t, remove));
}

static void
rebalance_3_child(struct CCC_Handle_realtime_ordered_map *const t, size_t z,
                  size_t x)
{
    assert(z);
    CCC_Tribool made_3_child = CCC_FALSE;
    do
    {
        size_t const g = parent_i(t, z);
        size_t const y = branch_i(t, z, branch_i(t, z, L) == x);
        made_3_child = is_2_child(t, g, z);
        if (is_2_child(t, z, y))
        {
            demote(t, z);
        }
        else if (is_22_parent(t, branch_i(t, y, L), y, branch_i(t, y, R)))
        {
            demote(t, z);
            demote(t, y);
        }
        else /* p(x) is 1,3, y is not a 2,2 parent, and x is 3-child.*/
        {
            assert(is_3_child(t, z, x));
            enum Branch const z_to_x_dir = branch_i(t, z, R) == x;
            size_t const w = branch_i(t, y, !z_to_x_dir);
            if (is_1_child(t, y, w))
            {
                rotate(t, z, y, branch_i(t, y, z_to_x_dir), z_to_x_dir);
                promote(t, y);
                demote(t, z);
                if (is_leaf(t, z))
                {
                    demote(t, z);
                }
            }
            else /* w is a 2-child and v will be a 1-child. */
            {
                size_t const v = branch_i(t, y, z_to_x_dir);
                assert(is_2_child(t, y, w));
                assert(is_1_child(t, y, v));
                double_rotate(t, z, y, v, !z_to_x_dir);
                double_promote(t, v);
                demote(t, y);
                double_demote(t, z);
                /* Optional "Rebalancing with Promotion," defined as follows:
                       if node z is a non-leaf 1,1 node, we promote it;
                       otherwise, if y is a non-leaf 1,1 node, we promote it.
                       (See Figure 4.) (Haeupler et. al. 2014, 17).
                   This reduces constants in some of theorems mentioned in the
                   paper but may not be worth doing. Rotations stay at 2 worst
                   case. Should revisit after more performance testing. */
                if (!is_leaf(t, z)
                    && is_11_parent(t, branch_i(t, z, L), z, branch_i(t, z, R)))
                {
                    promote(t, z);
                }
                else if (!is_leaf(t, y)
                         && is_11_parent(t, branch_i(t, y, L), y,
                                         branch_i(t, y, R)))
                {
                    promote(t, y);
                }
            }
            return;
        }
        x = z;
        z = g;
    }
    while (z && made_3_child);
}

/** A single rotation is symmetric. Here is the right case. Lowercase are nodes
and uppercase are arbitrary subtrees.
        z            x
     ╭──┴──╮      ╭──┴──╮
     x     C      A     z
   ╭─┴─╮      ->      ╭─┴─╮
   A   y              y   C
       │              │
       B              B */
static void
rotate(struct CCC_Handle_realtime_ordered_map *const t, size_t const z,
       size_t const x, size_t const y, enum Branch const dir)
{
    assert(z);
    struct CCC_Handle_realtime_ordered_map_node *const z_r = node_at(t, z);
    struct CCC_Handle_realtime_ordered_map_node *const x_r = node_at(t, x);
    size_t const g = parent_i(t, z);
    x_r->parent = g;
    if (!g)
    {
        t->root = x;
    }
    else
    {
        struct CCC_Handle_realtime_ordered_map_node *const g_r = node_at(t, g);
        g_r->branch[g_r->branch[R] == z] = x;
    }
    x_r->branch[dir] = z;
    z_r->parent = x;
    z_r->branch[!dir] = y;
    *parent_r(t, y) = z;
}

/** A double rotation shouldn't actually be two calls to rotate because that
would invoke pointless memory writes. Here is an example of double right.
Lowercase are nodes and uppercase are arbitrary subtrees.

        z            y
     ╭──┴──╮      ╭──┴──╮
     x     D      x     z
   ╭─┴─╮     -> ╭─┴─╮ ╭─┴─╮
   A   y        A   B C   D
     ╭─┴─╮
     B   C */
static void
double_rotate(struct CCC_Handle_realtime_ordered_map *const t, size_t const z,
              size_t const x, size_t const y, enum Branch const dir)
{
    assert(z && x && y);
    struct CCC_Handle_realtime_ordered_map_node *const z_r = node_at(t, z);
    struct CCC_Handle_realtime_ordered_map_node *const x_r = node_at(t, x);
    struct CCC_Handle_realtime_ordered_map_node *const y_r = node_at(t, y);
    size_t const g = z_r->parent;
    y_r->parent = g;
    if (!g)
    {
        t->root = y;
    }
    else
    {
        struct CCC_Handle_realtime_ordered_map_node *const g_r = node_at(t, g);
        g_r->branch[g_r->branch[R] == z] = y;
    }
    x_r->branch[!dir] = y_r->branch[dir];
    *parent_r(t, y_r->branch[dir]) = x;
    y_r->branch[dir] = x;
    x_r->parent = y;

    z_r->branch[dir] = y_r->branch[!dir];
    *parent_r(t, y_r->branch[!dir]) = z;
    y_r->branch[!dir] = z;
    z_r->parent = y;
}

/* Returns true for rank difference 0 (rule break) between the parent and node.
         p
      0╭─╯
       x */
[[maybe_unused]] static inline CCC_Tribool
is_0_child(struct CCC_Handle_realtime_ordered_map const *const t,
           size_t const p, size_t const x)
{
    return p && parity(t, p) == parity(t, x);
}

/* Returns true for rank difference 1 between the parent and node.
         p
      1╭─╯
       x */
static inline CCC_Tribool
is_1_child(struct CCC_Handle_realtime_ordered_map const *const t,
           size_t const p, size_t const x)
{
    return p && parity(t, p) != parity(t, x);
}

/* Returns true for rank difference 2 between the parent and node.
         p
      2╭─╯
       x */
static inline CCC_Tribool
is_2_child(struct CCC_Handle_realtime_ordered_map const *const t,
           size_t const p, size_t const x)
{
    return p && parity(t, p) == parity(t, x);
}

/* Returns true for rank difference 3 between the parent and node.
         p
      3╭─╯
       x */
[[maybe_unused]] static inline CCC_Tribool
is_3_child(struct CCC_Handle_realtime_ordered_map const *const t,
           size_t const p, size_t const x)
{
    return p && parity(t, p) != parity(t, x);
}

/* Returns true if a parent is a 0,1 or 1,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮1
       x   y */
static inline CCC_Tribool
is_01_parent(struct CCC_Handle_realtime_ordered_map const *const t,
             size_t const x, size_t const p, size_t const y)
{
    assert(p);
    return (!parity(t, x) && !parity(t, p) && parity(t, y))
        || (parity(t, x) && parity(t, p) && !parity(t, y));
}

/* Returns true if a parent is a 1,1 node. Either child may be the sentinel
   node which has a parity of 1 and rank -1.
         p
      1╭─┴─╮1
       x   y */
static inline CCC_Tribool
is_11_parent(struct CCC_Handle_realtime_ordered_map const *const t,
             size_t const x, size_t const p, size_t const y)
{
    assert(p);
    return (!parity(t, x) && parity(t, p) && !parity(t, y))
        || (parity(t, x) && !parity(t, p) && parity(t, y));
}

/* Returns true if a parent is a 0,2 or 2,0 node, which is not allowed. Either
   child may be the sentinel node which has a parity of 1 and rank -1.
         p
      0╭─┴─╮2
       x   y */
static inline CCC_Tribool
is_02_parent(struct CCC_Handle_realtime_ordered_map const *const t,
             size_t const x, size_t const p, size_t const y)
{
    assert(p);
    return (parity(t, x) == parity(t, p)) && (parity(t, p) == parity(t, y));
}

/* Returns true if a parent is a 2,2 or 2,2 node, which is allowed. 2,2 nodes
   are allowed in a WAVL tree but the absence of any 2,2 nodes is the exact
   equivalent of a normal AVL tree which can occur if only insertions occur
   for a WAVL tree. Either child may be the sentinel node which has a parity of
   1 and rank -1.
         p
      2╭─┴─╮2
       x   y */
static inline CCC_Tribool
is_22_parent(struct CCC_Handle_realtime_ordered_map const *const t,
             size_t const x, size_t const p, size_t const y)
{
    assert(p);
    return (parity(t, x) == parity(t, p)) && (parity(t, p) == parity(t, y));
}

static inline void
promote(struct CCC_Handle_realtime_ordered_map const *const t, size_t const x)
{
    if (x)
    {
        *block_at(t, x) ^= bit_on(x);
    }
}

static inline void
demote(struct CCC_Handle_realtime_ordered_map const *const t, size_t const x)
{
    promote(t, x);
}

/* Parity based ranks mean this is no-op but leave in case implementation ever
   changes. Also, makes clear what sections of code are trying to do. */
static inline void
double_promote(struct CCC_Handle_realtime_ordered_map const *const,
               size_t const)
{}

/* Parity based ranks mean this is no-op but leave in case implementation ever
   changes. Also, makes clear what sections of code are trying to do. */
static inline void
double_demote(struct CCC_Handle_realtime_ordered_map const *const, size_t const)
{}

static inline CCC_Tribool
is_leaf(struct CCC_Handle_realtime_ordered_map const *const t, size_t const x)
{
    return !branch_i(t, x, L) && !branch_i(t, x, R);
}

static inline size_t
sibling_of(struct CCC_Handle_realtime_ordered_map const *const t,
           size_t const x)
{
    size_t const p = parent_i(t, x);
    assert(p);
    /* We want the sibling so we need the truthy value to be opposite of x. */
    return node_at(t, p)->branch[branch_i(t, p, L) == x];
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

/** @private */
struct Tree_range
{
    size_t low;
    size_t root;
    size_t high;
};

static size_t
recursive_count(struct CCC_Handle_realtime_ordered_map const *const t,
                size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_count(t, branch_i(t, r, R))
         + recursive_count(t, branch_i(t, r, L));
}

static CCC_Tribool
are_subtrees_valid(struct CCC_Handle_realtime_ordered_map const *t,
                   struct Tree_range const r)
{
    if (!r.root)
    {
        return CCC_TRUE;
    }
    if (r.low
        && order_nodes(t, key_at(t, r.low), r.root, t->order)
               != CCC_ORDER_LESSER)
    {
        return CCC_FALSE;
    }
    if (r.high
        && order_nodes(t, key_at(t, r.high), r.root, t->order)
               != CCC_ORDER_GREATER)
    {
        return CCC_FALSE;
    }
    return are_subtrees_valid(t,
                              (struct Tree_range){
                                  .low = r.low,
                                  .root = branch_i(t, r.root, L),
                                  .high = r.root,
                              })
        && are_subtrees_valid(t, (struct Tree_range){
                                     .low = r.root,
                                     .root = branch_i(t, r.root, R),
                                     .high = r.high,
                                 });
}

static CCC_Tribool
is_storing_parent(struct CCC_Handle_realtime_ordered_map const *const t,
                  size_t const p, size_t const root)
{
    if (!root)
    {
        return CCC_TRUE;
    }
    if (parent_i(t, root) != p)
    {
        return CCC_FALSE;
    }
    return is_storing_parent(t, root, branch_i(t, root, L))
        && is_storing_parent(t, root, branch_i(t, root, R));
}

static CCC_Tribool
is_free_list_valid(struct CCC_Handle_realtime_ordered_map const *const t)
{
    if (!t->count)
    {
        return CCC_TRUE;
    }
    size_t list_check = 0;
    for (size_t cur = t->free_list; cur && list_check < t->capacity;
         cur = node_at(t, cur)->next_free, ++list_check)
    {}
    return (list_check + t->count == t->capacity);
}

static inline CCC_Tribool
validate(struct CCC_Handle_realtime_ordered_map const *const
             handle_realtime_ordered_map)
{
    /* If we haven't lazily initialized we should not check anything. */
    if (handle_realtime_ordered_map->data
        && (!handle_realtime_ordered_map->nodes
            || !handle_realtime_ordered_map->parity))
    {
        return CCC_TRUE;
    }
    if (!handle_realtime_ordered_map->count
        && !parity(handle_realtime_ordered_map, 0))
    {
        return CCC_FALSE;
    }
    if (!are_subtrees_valid(
            handle_realtime_ordered_map,
            (struct Tree_range){.root = handle_realtime_ordered_map->root}))
    {
        return CCC_FALSE;
    }
    size_t const size = recursive_count(handle_realtime_ordered_map,
                                        handle_realtime_ordered_map->root);
    if (size && size != handle_realtime_ordered_map->count - 1)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(handle_realtime_ordered_map, 0,
                           handle_realtime_ordered_map->root))
    {
        return CCC_FALSE;
    }
    if (!is_free_list_valid(handle_realtime_ordered_map))
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */

/* Below you will find the required license for code that inspired the
implementation of a WAVL tree in this repository for some map containers.

The original repository can be found here:

https://github.com/pvachon/wavl_tree

The original implementation has be changed to eliminate left and right cases,
simplify deletion, and work within the C Container Collection memory framework.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
