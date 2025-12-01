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

This file implements a splay tree that does not support duplicates.
The code to support a splay tree that does not allow duplicates is much simpler
than the code to support a multimap implementation. This implementation is
based on the following source.

    1. Daniel Sleator, Carnegie Mellon University. Sleator's implementation of a
       topdown splay tree was instrumental in starting things off, but required
       extensive modification. I had to update parent and child tracking, and
       unite the left and right cases for fun. See the code for a generalizable
       strategy to eliminate symmetric left and right cases for any binary tree
       code. https://www.link.cs.cmu.edu/splay/

Because this is a self-optimizing data structure it may benefit from many
constant time queries for frequently accessed elements. It is also in a Struct
of Arrays layout to improve memory alignment and reduce wasted space. While
it is recommended that the user reserve space for the needed nodes ahead of
time, the amortized O(log(N)) run times of a Splay Tree remain the same in
the dynamic resizing case. */
#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <string.h>

#include "array_adaptive_map.h"
#include "private/private_array_adaptive_map.h"
#include "private/private_types.h"
#include "types.h"

/*========================   Data Alignment Test   ==========================*/

/** @internal A macro version of the runtime alignment operations we perform
for calculating bytes. This way we can use in static assert. The user data type
may not be the same alignment as the nodes and therefore the nodes array must
start at next aligned byte. */
#define roundup(bytes_to_round, alignment)                                     \
    (((bytes_to_round) + (alignment) - 1) & ~((alignment) - 1))

enum : size_t
{
    /* @internal Test capacity. */
    TCAP = 3,
};
/** @internal Use an int because that will force the nodes array to be wary of
where to start. The nodes are 8 byte aligned but an int is 4. This means the
nodes need to start after a 4 byte Buffer of padding at end of data array. */
struct Test_data_type
{
    int const i;
};
CCC_array_adaptive_map_declare_fixed_map(Fixed_map_test_type,
                                         struct Test_data_type, TCAP);
/** @internal This is a static fixed size map exclusive to this translation unit
used to ensure assumptions about data layout are correct. The following static
asserts must be true in order to support the Struct of Array style layout we
use for the data and nodes. It is important that in our user code when we set
the positions of the node pointer relative to the data pointer the positions are
correct regardless of backing storage as a fixed map or heap allocation. */
static Fixed_map_test_type const static_data_nodes_layout_test;
/** Some assumptions in the code assume that nodes array is last so ensure that
is the case here. Also good to assume user data comes first. */
static_assert((char const *)static_data_nodes_layout_test.data
                  < (char const *)static_data_nodes_layout_test.nodes,
              "The order of the arrays in a Struct of Arrays map is data, then "
              "nodes.");
/** We don't care about the alignment or padding after the nodes array because
we never need to set or move any pointers to that position. The alignment is
important for the nodes pointer to be set to the correct aligned position and
so that we allocate enough bytes for our single allocation if the map is dynamic
and not a fixed type. */
static_assert(
    (char const *)&static_data_nodes_layout_test.nodes[TCAP]
            - (char const *)&static_data_nodes_layout_test.data[0]
        == roundup((sizeof(*static_data_nodes_layout_test.data) * TCAP),
                   alignof(*static_data_nodes_layout_test.nodes))
               + (sizeof(*static_data_nodes_layout_test.nodes) * TCAP),
    "The pointer difference in bytes between end of the nodes array and start "
    "of user data array must be the same as the total bytes we assume to be "
    "stored in that range. Alignment of user data must be considered.");
static_assert((char const *)&static_data_nodes_layout_test.data
                      + roundup((sizeof(*static_data_nodes_layout_test.data)
                                 * TCAP),
                                alignof(*static_data_nodes_layout_test.nodes))
                  == (char const *)&static_data_nodes_layout_test.nodes,
              "The start of the nodes array must begin at the next aligned "
              "byte given alignment of a node.");

/*==========================  Type Declarations   ===========================*/

/** @internal */
enum
{
    LR = 2,
};

/** @internal */
enum Branch
{
    L = 0,
    R,
};

#define INORDER R
#define R_INORDER L

enum
{
    EMPTY_TREE = 2,
};

/* Buffer allocates before insert. "Empty" has nil 0th slot and one more. */

/*==============================  Prototypes   ==============================*/

/* Returning the internal elem type with stored offsets. */
static size_t splay(struct CCC_Array_adaptive_map *, size_t, void const *,
                    CCC_Key_comparator *);
static struct CCC_Array_adaptive_map_node *
node_at(struct CCC_Array_adaptive_map const *, size_t);
static void *data_at(struct CCC_Array_adaptive_map const *, size_t);
/* Returning the user struct type with stored offsets. */
static struct CCC_Array_adaptive_map_handle
handle(struct CCC_Array_adaptive_map *, void const *key);
static size_t erase(struct CCC_Array_adaptive_map *, void const *key);
static size_t maybe_allocate_insert(struct CCC_Array_adaptive_map *,
                                    void const *user_type);
static CCC_Result resize(struct CCC_Array_adaptive_map *, size_t new_capacity,
                         CCC_Allocator *);
static void copy_soa(struct CCC_Array_adaptive_map const *source,
                     void *destination_data_base, size_t destination_capacity);
static size_t data_bytes(size_t sizeof_type, size_t capacity);
static size_t node_bytes(size_t capacity);
static struct CCC_Array_adaptive_map_node *
node_pos(size_t sizeof_type, void const *data, size_t capacity);
static size_t find(struct CCC_Array_adaptive_map *, void const *key);
static void connect_new_root(struct CCC_Array_adaptive_map *, size_t new_root,
                             CCC_Order order_result);
static void insert(struct CCC_Array_adaptive_map *, size_t n);
static void *key_in_slot(struct CCC_Array_adaptive_map const *,
                         void const *user_struct);
static size_t allocate_slot(struct CCC_Array_adaptive_map *);
static size_t total_bytes(size_t sizeof_type, size_t capacity);
static struct CCC_Handle_range equal_range(struct CCC_Array_adaptive_map *,
                                           void const *begin_key,
                                           void const *end_key,
                                           enum Branch traversal);
/* Returning the user key with stored offsets. */
static void *key_at(struct CCC_Array_adaptive_map const *, size_t i);
/* Returning threeway comparison with user callback. */
static CCC_Order order_nodes(struct CCC_Array_adaptive_map const *,
                             void const *key, size_t node,
                             CCC_Key_comparator *);
/* Returning read only indices for tree nodes. */
static size_t remove_from_tree(struct CCC_Array_adaptive_map *, size_t ret);
static size_t min_max_from(struct CCC_Array_adaptive_map const *, size_t start,
                           enum Branch dir);
static size_t next(struct CCC_Array_adaptive_map const *, size_t n,
                   enum Branch traversal);
static size_t branch_index(struct CCC_Array_adaptive_map const *, size_t parent,
                           enum Branch dir);
static size_t parent_index(struct CCC_Array_adaptive_map const *, size_t child);
/* Returning references to index fields for tree nodes. */
static size_t *branch_pointer(struct CCC_Array_adaptive_map const *,
                              size_t node, enum Branch branch);
static size_t *parent_pointer(struct CCC_Array_adaptive_map const *,
                              size_t node);

static CCC_Tribool validate(struct CCC_Array_adaptive_map const *);

/* Returning void as miscellaneous helpers. */
static void init_node(struct CCC_Array_adaptive_map const *, size_t node);
static void swap(char temp[const], void *a, void *b, size_t sizeof_type);
static void link(struct CCC_Array_adaptive_map *, size_t parent,
                 enum Branch dir, size_t subtree);
static size_t max(size_t, size_t);
static void delete_nodes(struct CCC_Array_adaptive_map *,
                         CCC_Type_destructor *);

/*==============================  Interface    ==============================*/

void *
CCC_array_adaptive_map_at(CCC_Array_adaptive_map const *const map,
                          CCC_Handle_index const index)
{
    if (!map || !index)
    {
        return NULL;
    }
    return data_at(map, index);
}

CCC_Tribool
CCC_array_adaptive_map_contains(CCC_Array_adaptive_map *const map,
                                void const *const key)
{
    if (!map || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    map->root = splay(map, map->root, key, map->order);
    return order_nodes(map, key, map->root, map->order) == CCC_ORDER_EQUAL;
}

CCC_Handle_index
CCC_array_adaptive_map_get_key_value(CCC_Array_adaptive_map *const map,
                                     void const *const key)
{
    if (!map || !key)
    {
        return 0;
    }
    return find(map, key);
}

CCC_Array_adaptive_map_handle
CCC_array_adaptive_map_handle(CCC_Array_adaptive_map *const map,
                              void const *const key)
{
    if (!map || !key)
    {
        return (CCC_Array_adaptive_map_handle){
            {.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    return (CCC_Array_adaptive_map_handle){handle(map, key)};
}

CCC_Handle_index
CCC_array_adaptive_map_insert_handle(
    CCC_Array_adaptive_map_handle const *const handle,
    void const *const key_val_type)
{
    if (!handle || !key_val_type)
    {
        return 0;
    }
    if (handle->private.status == CCC_ENTRY_OCCUPIED)
    {
        void *const ret = data_at(handle->private.map, handle->private.index);
        if (key_val_type != ret)
        {
            (void)memcpy(ret, key_val_type, handle->private.map->sizeof_type);
        }
        return handle->private.index;
    }
    return maybe_allocate_insert(handle->private.map, key_val_type);
}

CCC_Array_adaptive_map_handle *
CCC_array_adaptive_map_and_modify(CCC_Array_adaptive_map_handle *const handle,
                                  CCC_Type_modifier *const modify)
{
    if (!handle)
    {
        return NULL;
    }
    if (modify && handle->private.status & CCC_ENTRY_OCCUPIED)
    {
        modify((CCC_Type_context){
            .type = data_at(handle->private.map, handle->private.index),
            .context = NULL,
        });
    }
    return handle;
}

CCC_Array_adaptive_map_handle *
CCC_array_adaptive_map_and_modify_context(
    CCC_Array_adaptive_map_handle *const handle,
    CCC_Type_modifier *const modify, void *const context)
{
    if (!handle)
    {
        return NULL;
    }
    if (modify && handle->private.status & CCC_ENTRY_OCCUPIED)
    {
        modify((CCC_Type_context){
            .type = data_at(handle->private.map, handle->private.index),
            .context = context,
        });
    }
    return handle;
}

CCC_Handle_index
CCC_array_adaptive_map_or_insert(
    CCC_Array_adaptive_map_handle const *const handle,
    void const *const key_val_type)
{
    if (!handle || !key_val_type)
    {
        return 0;
    }
    if (handle->private.status & CCC_ENTRY_OCCUPIED)
    {
        return handle->private.index;
    }
    return maybe_allocate_insert(handle->private.map, key_val_type);
}

CCC_Handle
CCC_array_adaptive_map_swap_handle(CCC_Array_adaptive_map *const map,
                                   void *const type_output)
{
    if (!map || !type_output)
    {
        return (CCC_Handle){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    size_t const found = find(map, key_in_slot(map, type_output));
    if (found)
    {
        assert(map->root);
        void *const ret = data_at(map, map->root);
        void *const temp = data_at(map, 0);
        swap(temp, type_output, ret, map->sizeof_type);
        return (CCC_Handle){{
            .index = found,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted = maybe_allocate_insert(map, type_output);
    if (!inserted)
    {
        return (CCC_Handle){{
            .index = 0,
            .status = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .index = inserted,
        .status = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_array_adaptive_map_try_insert(CCC_Array_adaptive_map *const map,
                                  void const *const key_val_type)
{
    if (!map || !key_val_type)
    {
        return (CCC_Handle){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    size_t const found = find(map, key_in_slot(map, key_val_type));
    if (found)
    {
        assert(map->root);
        return (CCC_Handle){{
            .index = found,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted = maybe_allocate_insert(map, key_val_type);
    if (!inserted)
    {
        return (CCC_Handle){{
            .index = 0,
            .status = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .index = inserted,
        .status = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_array_adaptive_map_insert_or_assign(CCC_Array_adaptive_map *const map,
                                        void const *const key_val_type)
{
    if (!map || !key_val_type)
    {
        return (CCC_Handle){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    size_t const found = find(map, key_in_slot(map, key_val_type));
    if (found)
    {
        assert(map->root);
        void *const f_base = data_at(map, found);
        if (key_val_type != f_base)
        {
            memcpy(f_base, key_val_type, map->sizeof_type);
        }
        return (CCC_Handle){{
            .index = found,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted = maybe_allocate_insert(map, key_val_type);
    if (!inserted)
    {
        return (CCC_Handle){{
            .index = 0,
            .status = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .index = inserted,
        .status = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_array_adaptive_map_remove(CCC_Array_adaptive_map *const map,
                              void *const type_output)
{
    if (!map || !type_output)
    {
        return (CCC_Handle){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    size_t const removed = erase(map, key_in_slot(map, type_output));
    if (!removed)
    {
        return (CCC_Handle){{
            .index = 0,
            .status = CCC_ENTRY_VACANT,
        }};
    }
    assert(removed);
    void const *const r = data_at(map, removed);
    if (type_output != r)
    {
        (void)memcpy(type_output, r, map->sizeof_type);
    }
    return (CCC_Handle){{
        .index = 0,
        .status = CCC_ENTRY_OCCUPIED,
    }};
}

CCC_Handle
CCC_array_adaptive_map_remove_handle(
    CCC_Array_adaptive_map_handle *const handle)
{
    if (!handle)
    {
        return (CCC_Handle){{.status = CCC_ENTRY_ARGUMENT_ERROR}};
    }
    if (handle->private.status == CCC_ENTRY_OCCUPIED)
    {
        size_t const erased
            = erase(handle->private.map,
                    key_at(handle->private.map, handle->private.index));
        assert(erased);
        return (CCC_Handle){{
            .index = erased,
            .status = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Handle){{
        .index = 0,
        .status = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle_index
CCC_array_adaptive_map_unwrap(CCC_Array_adaptive_map_handle const *const handle)
{
    if (!handle)
    {
        return 0;
    }
    return handle->private.status == CCC_ENTRY_OCCUPIED ? handle->private.index
                                                        : 0;
}

CCC_Tribool
CCC_array_adaptive_map_insert_error(
    CCC_Array_adaptive_map_handle const *const handle)
{
    if (!handle)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (handle->private.status & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_array_adaptive_map_occupied(
    CCC_Array_adaptive_map_handle const *const handle)
{
    if (!handle)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (handle->private.status & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Handle_status
CCC_array_adaptive_map_handle_status(
    CCC_Array_adaptive_map_handle const *const handle)
{
    return handle ? handle->private.status : CCC_ENTRY_ARGUMENT_ERROR;
}

CCC_Tribool
CCC_array_adaptive_map_is_empty(CCC_Array_adaptive_map const *const map)
{
    if (!map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !CCC_array_adaptive_map_count(map).count;
}

CCC_Count
CCC_array_adaptive_map_count(CCC_Array_adaptive_map const *const map)
{
    if (!map)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = map->count ? map->count - 1 : 0};
}

CCC_Count
CCC_array_adaptive_map_capacity(CCC_Array_adaptive_map const *const map)
{
    if (!map)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = map->capacity};
}

CCC_Handle_index
CCC_array_adaptive_map_begin(CCC_Array_adaptive_map const *const map)
{
    if (!map || !map->capacity)
    {
        return 0;
    }
    size_t const n = min_max_from(map, map->root, L);
    return n;
}

CCC_Handle_index
CCC_array_adaptive_map_reverse_begin(CCC_Array_adaptive_map const *const map)
{
    if (!map || !map->capacity)
    {
        return 0;
    }
    size_t const n = min_max_from(map, map->root, R);
    return n;
}

CCC_Handle_index
CCC_array_adaptive_map_next(CCC_Array_adaptive_map const *const map,
                            CCC_Handle_index const iterator)
{
    if (!map || !map->capacity)
    {
        return 0;
    }
    size_t const n = next(map, iterator, INORDER);
    return n;
}

CCC_Handle_index
CCC_array_adaptive_map_reverse_next(CCC_Array_adaptive_map const *const map,
                                    CCC_Handle_index const iterator)
{
    if (!map || !iterator || !map->capacity)
    {
        return 0;
    }
    size_t const n = next(map, iterator, R_INORDER);
    return n;
}

CCC_Handle_index
CCC_array_adaptive_map_end(CCC_Array_adaptive_map const *const)
{
    return 0;
}

CCC_Handle_index
CCC_array_adaptive_map_reverse_end(CCC_Array_adaptive_map const *const)
{
    return 0;
}

CCC_Handle_range
CCC_array_adaptive_map_equal_range(CCC_Array_adaptive_map *const map,
                                   void const *const begin_key,
                                   void const *const end_key)
{
    if (!map || !begin_key || !end_key)
    {
        return (CCC_Handle_range){};
    }
    return (CCC_Handle_range){equal_range(map, begin_key, end_key, INORDER)};
}

CCC_Handle_range_reverse
CCC_array_adaptive_map_equal_range_reverse(CCC_Array_adaptive_map *const map,
                                           void const *const reverse_begin_key,
                                           void const *const reverse_end_key)

{
    if (!map || !reverse_begin_key || !reverse_end_key)
    {
        return (CCC_Handle_range_reverse){};
    }
    return (CCC_Handle_range_reverse){
        equal_range(map, reverse_begin_key, reverse_end_key, R_INORDER)};
}

CCC_Result
CCC_array_adaptive_map_reserve(CCC_Array_adaptive_map *const map,
                               size_t const to_add,
                               CCC_Allocator *const allocate)
{
    if (!map || !to_add || !allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Once initialized the Buffer always has a size of one for root node. */
    size_t const needed = map->count + to_add + (map->count == 0);
    if (needed <= map->capacity)
    {
        return CCC_RESULT_OK;
    }
    size_t const old_count = map->count;
    size_t old_cap = map->capacity;
    CCC_Result const r = resize(map, needed, allocate);
    if (r != CCC_RESULT_OK)
    {
        return r;
    }
    if (!old_cap)
    {
        map->count = 1;
    }
    old_cap = old_count ? old_cap : 0;
    size_t const new_cap = map->capacity;
    size_t prev = 0;
    for (ptrdiff_t i = (ptrdiff_t)new_cap - 1; i > 0 && i >= (ptrdiff_t)old_cap;
         prev = i, --i)
    {
        node_at(map, i)->next_free = prev;
    }
    if (!map->free_list)
    {
        map->free_list = prev;
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_array_adaptive_map_copy(CCC_Array_adaptive_map *const destination,
                            CCC_Array_adaptive_map const *const source,
                            CCC_Allocator *const allocate)
{
    if (!destination || !source || source == destination
        || (destination->capacity < source->capacity && !allocate))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    void *const destination_data = destination->data;
    struct CCC_Array_adaptive_map_node *const destination_nodes
        = destination->nodes;
    size_t const destination_cap = destination->capacity;
    CCC_Allocator *const destination_allocate = destination->allocate;
    *destination = *source;
    destination->data = destination_data;
    destination->nodes = destination_nodes;
    destination->capacity = destination_cap;
    destination->allocate = destination_allocate;
    if (!source->capacity)
    {
        return CCC_RESULT_OK;
    }
    if (destination->capacity < source->capacity)
    {
        CCC_Result const r = resize(destination, source->capacity, allocate);
        if (r != CCC_RESULT_OK)
        {
            return r;
        }
    }
    else
    {
        /* Might not be necessary but not worth finding out. Do every time. */
        destination->nodes = node_pos(destination->sizeof_type,
                                      destination->data, destination->capacity);
    }
    if (!destination->data || !source->data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    copy_soa(source, destination->data, destination->capacity);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_array_adaptive_map_clear(CCC_Array_adaptive_map *const map,
                             CCC_Type_destructor *const destroy)
{
    if (!map)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (destroy)
    {
        delete_nodes(map, destroy);
    }
    map->count = 1;
    map->root = 0;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_array_adaptive_map_clear_and_free(CCC_Array_adaptive_map *const map,
                                      CCC_Type_destructor *const destroy)
{
    if (!map || !map->allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (destroy)
    {
        delete_nodes(map, destroy);
    }
    map->root = 0;
    map->count = 0;
    map->capacity = 0;
    (void)map->allocate((CCC_Allocator_context){
        .input = map->data,
        .bytes = 0,
        .context = map->context,
    });
    map->data = NULL;
    map->nodes = NULL;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_array_adaptive_map_clear_and_free_reserve(
    CCC_Array_adaptive_map *const map, CCC_Type_destructor *const destroy,
    CCC_Allocator *const allocate)
{
    if (!map || !allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (destroy)
    {
        delete_nodes(map, destroy);
    }
    map->root = 0;
    map->count = 0;
    map->capacity = 0;
    (void)allocate((CCC_Allocator_context){
        .input = map->data,
        .bytes = 0,
        .context = map->context,
    });
    map->data = NULL;
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_array_adaptive_map_validate(CCC_Array_adaptive_map const *const map)
{
    if (!map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(map);
}

/*===========================   Private Interface ===========================*/

void
CCC_private_array_adaptive_map_insert(struct CCC_Array_adaptive_map *const map,
                                      size_t const elem_i)
{
    insert(map, elem_i);
}

struct CCC_Array_adaptive_map_handle
CCC_private_array_adaptive_map_handle(struct CCC_Array_adaptive_map *const map,
                                      void const *const key)
{
    return handle(map, key);
}

void *
CCC_private_array_adaptive_map_key_at(
    struct CCC_Array_adaptive_map const *const map, size_t const slot)
{
    return key_at(map, slot);
}

void *
CCC_private_array_adaptive_map_data_at(
    struct CCC_Array_adaptive_map const *const map, size_t const slot)
{
    return data_at(map, slot);
}

size_t
CCC_private_array_adaptive_map_allocate_slot(
    struct CCC_Array_adaptive_map *const map)
{
    return allocate_slot(map);
}

/*===========================   Static Helpers    ===========================*/

static struct CCC_Handle_range
equal_range(struct CCC_Array_adaptive_map *const t, void const *const begin_key,
            void const *const end_key, enum Branch const traversal)
{
    if (CCC_array_adaptive_map_is_empty(t))
    {
        return (struct CCC_Handle_range){};
    }
    /* As with most BST code the cases are perfectly symmetrical. If we
       are seeking an increasing or decreasing range we need to make sure
       we follow the [inclusive, exclusive) range rule. This means double
       checking we don't need to progress to the next greatest or next
       lesser element depending on the direction we are traversing. */
    CCC_Order const les_or_grt[2] = {CCC_ORDER_LESSER, CCC_ORDER_GREATER};
    size_t b = splay(t, t->root, begin_key, t->order);
    if (order_nodes(t, begin_key, b, t->order) == les_or_grt[traversal])
    {
        b = next(t, b, traversal);
    }
    size_t e = splay(t, t->root, end_key, t->order);
    if (order_nodes(t, end_key, e, t->order) != les_or_grt[!traversal])
    {
        e = next(t, e, traversal);
    }
    return (struct CCC_Handle_range){
        .begin = b,
        .end = e,
    };
}

static struct CCC_Array_adaptive_map_handle
handle(struct CCC_Array_adaptive_map *const map, void const *const key)
{
    size_t const found = find(map, key);
    if (found)
    {
        return (struct CCC_Array_adaptive_map_handle){
            .map = map,
            .index = found,
            .status = CCC_ENTRY_OCCUPIED,
        };
    }
    return (struct CCC_Array_adaptive_map_handle){
        .map = map,
        .index = 0,
        .status = CCC_ENTRY_VACANT,
    };
}

static size_t
maybe_allocate_insert(struct CCC_Array_adaptive_map *const map,
                      void const *const user_type)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const node = allocate_slot(map);
    if (!node)
    {
        return 0;
    }
    (void)memcpy(data_at(map, node), user_type, map->sizeof_type);
    insert(map, node);
    return node;
}

static size_t
allocate_slot(struct CCC_Array_adaptive_map *const map)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const old_count = map->count;
    size_t old_cap = map->capacity;
    if (!old_count || old_count == old_cap)
    {
        assert(!map->free_list);
        if (old_count == old_cap)
        {
            if (resize(map, max(old_cap * 2, 8), map->allocate)
                != CCC_RESULT_OK)
            {
                return 0;
            }
        }
        else
        {
            map->nodes = node_pos(map->sizeof_type, map->data, map->capacity);
        }
        old_cap = old_count ? old_cap : 0;
        size_t const new_cap = map->capacity;
        size_t prev = 0;
        for (size_t i = new_cap - 1; i > 0 && i >= old_cap; prev = i, --i)
        {
            node_at(map, i)->next_free = prev;
        }
        map->free_list = prev;
        map->count = max(old_count, 1);
    }
    if (!map->free_list)
    {
        return 0;
    }
    ++map->count;
    size_t const slot = map->free_list;
    map->free_list = node_at(map, slot)->next_free;
    return slot;
}

static CCC_Result
resize(struct CCC_Array_adaptive_map *const map, size_t const new_capacity,
       CCC_Allocator *const allocate)
{
    if (map->capacity && new_capacity <= map->capacity - 1)
    {
        return CCC_RESULT_OK;
    }
    if (!allocate)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    void *const new_data = allocate((CCC_Allocator_context){
        .input = NULL,
        .bytes = total_bytes(map->sizeof_type, new_capacity),
        .context = map->context,
    });
    if (!new_data)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    copy_soa(map, new_data, new_capacity);
    map->nodes = node_pos(map->sizeof_type, new_data, new_capacity);
    allocate((CCC_Allocator_context){
        .input = map->data,
        .bytes = 0,
        .context = map->context,
    });
    map->data = new_data;
    map->capacity = new_capacity;
    return CCC_RESULT_OK;
}

static void
insert(struct CCC_Array_adaptive_map *const map, size_t const n)
{
    init_node(map, n);
    if (map->count == EMPTY_TREE)
    {
        map->root = n;
        return;
    }
    void const *const key = key_at(map, n);
    map->root = splay(map, map->root, key, map->order);
    CCC_Order const root_order = order_nodes(map, key, map->root, map->order);
    if (CCC_ORDER_EQUAL == root_order)
    {
        return;
    }
    connect_new_root(map, n, root_order);
}

static void
connect_new_root(struct CCC_Array_adaptive_map *const map,
                 size_t const new_root, CCC_Order const order_result)
{
    enum Branch const dir = CCC_ORDER_GREATER == order_result;
    link(map, new_root, dir, branch_index(map, map->root, dir));
    link(map, new_root, !dir, map->root);
    *branch_pointer(map, map->root, dir) = 0;
    map->root = new_root;
    *parent_pointer(map, map->root) = 0;
}

static size_t
erase(struct CCC_Array_adaptive_map *const map, void const *const key)
{
    if (CCC_array_adaptive_map_is_empty(map))
    {
        return 0;
    }
    size_t const ret = splay(map, map->root, key, map->order);
    CCC_Order const found = order_nodes(map, key, ret, map->order);
    if (found != CCC_ORDER_EQUAL)
    {
        return 0;
    }
    return remove_from_tree(map, ret);
}

static size_t
remove_from_tree(struct CCC_Array_adaptive_map *const map, size_t const ret)
{
    if (!branch_index(map, ret, L))
    {
        map->root = branch_index(map, ret, R);
        *parent_pointer(map, map->root) = 0;
    }
    else
    {
        map->root = splay(map, branch_index(map, ret, L), key_at(map, ret),
                          map->order);
        link(map, map->root, R, branch_index(map, ret, R));
    }
    node_at(map, ret)->next_free = map->free_list;
    map->free_list = ret;
    --map->count;
    return ret;
}

static size_t
find(struct CCC_Array_adaptive_map *const map, void const *const key)
{
    if (!map->root)
    {
        return 0;
    }
    map->root = splay(map, map->root, key, map->order);
    return order_nodes(map, key, map->root, map->order) == CCC_ORDER_EQUAL
             ? map->root
             : 0;
}

/** Adopts D. Sleator technique for splaying. Notable to this method is the
general improvement to the tree that occurs because we always splay the key
to the root, OR the next closest value to the key to the root. This has
interesting performance implications for real data sets.

This implementation has been modified to unite the left and right symmetries
and manage the parent pointers. Parent pointers are not usual for splay trees
but are necessary for a clean iteration API. */
static size_t
splay(struct CCC_Array_adaptive_map *const map, size_t root,
      void const *const key, CCC_Key_comparator *const compare)
{
    assert(root);
    /* Splaying brings the key element up to the root. The zigzag fixes of
       splaying repair the tree and we remember the roots of these changes in
       this helper tree. At the end, make the root pick up these modified left
       and right helpers. The nil node should NULL initialized to start. */
    struct CCC_Array_adaptive_map_node *const nil = node_at(map, 0);
    nil->branch[L] = nil->branch[R] = nil->parent = 0;
    size_t left_right_subtrees[LR] = {0, 0};
    for (;;)
    {
        CCC_Order const root_order = order_nodes(map, key, root, compare);
        enum Branch const order_link = CCC_ORDER_GREATER == root_order;
        size_t const child = branch_index(map, root, order_link);
        if (CCC_ORDER_EQUAL == root_order || !child)
        {
            break;
        }
        CCC_Order const child_order = order_nodes(
            map, key, branch_index(map, root, order_link), compare);
        enum Branch const child_order_link = CCC_ORDER_GREATER == child_order;
        /* A straight line has formed from root->child->grandchild. An
           opportunity to splay and heal the tree arises. */
        if (CCC_ORDER_EQUAL != child_order && order_link == child_order_link)
        {
            link(map, root, order_link, branch_index(map, child, !order_link));
            link(map, child, !order_link, root);
            root = child;
            if (!branch_index(map, root, order_link))
            {
                break;
            }
        }
        link(map, left_right_subtrees[!order_link], order_link, root);
        left_right_subtrees[!order_link] = root;
        root = branch_index(map, root, order_link);
    }
    link(map, left_right_subtrees[L], R, branch_index(map, root, L));
    link(map, left_right_subtrees[R], L, branch_index(map, root, R));
    link(map, root, L, nil->branch[R]);
    link(map, root, R, nil->branch[L]);
    map->root = root;
    *parent_pointer(map, map->root) = 0;
    return root;
}

/** Links the parent node to node starting at subtree root via direction dir.
updates the parent of the child being picked up by the new parent as well. */
static inline void
link(struct CCC_Array_adaptive_map *const map, size_t const parent,
     enum Branch const dir, size_t const subtree)
{
    *branch_pointer(map, parent, dir) = subtree;
    *parent_pointer(map, subtree) = parent;
}

static size_t
min_max_from(struct CCC_Array_adaptive_map const *const map, size_t start,
             enum Branch const dir)
{
    if (!start)
    {
        return 0;
    }
    for (; branch_index(map, start, dir); start = branch_index(map, start, dir))
    {}
    return start;
}

static size_t
next(struct CCC_Array_adaptive_map const *const map, size_t n,
     enum Branch const traversal)
{
    if (!n)
    {
        return 0;
    }
    assert(!parent_index(map, map->root));
    /* The node is an internal one that has a sub-tree to explore first. */
    if (branch_index(map, n, traversal))
    {
        /* The goal is to get far left/right ASAP in any traversal. */
        for (n = branch_index(map, n, traversal);
             branch_index(map, n, !traversal);
             n = branch_index(map, n, !traversal))
        {}
        return n;
    }
    /* This is how to return internal nodes on the way back up from a leaf. */
    size_t p = parent_index(map, n);
    for (; p && branch_index(map, p, !traversal) != n;
         n = p, p = parent_index(map, p))
    {}
    return p;
}

/** Deletes all nodes in the tree by calling destructor function on them in
linear time and constant space. This function modifies nodes as it deletes the
tree elements. Assumes the destructor function is non-null.

This function does not update any count or capacity fields of the map, it
simply calls the destructor on each node and removes the nodes references to
other tree elements. */
static void
delete_nodes(struct CCC_Array_adaptive_map *const map,
             CCC_Type_destructor *const destroy)
{
    assert(map);
    assert(destroy);
    size_t node = map->root;
    while (node)
    {
        struct CCC_Array_adaptive_map_node *const e = node_at(map, node);
        if (e->branch[L])
        {
            size_t const left = e->branch[L];
            e->branch[L] = node_at(map, left)->branch[R];
            node_at(map, left)->branch[R] = node;
            node = left;
            continue;
        }
        size_t const next = e->branch[R];
        e->branch[L] = e->branch[R] = 0;
        e->parent = 0;
        destroy((CCC_Type_context){
            .type = data_at(map, node),
            .context = map->context,
        });
        node = next;
    }
}

static inline CCC_Order
order_nodes(struct CCC_Array_adaptive_map const *const map,
            void const *const key, size_t const node,
            CCC_Key_comparator *const compare)
{
    return compare((CCC_Key_comparator_context){
        .key_left = key,
        .type_right = data_at(map, node),
        .context = map->context,
    });
}

static inline void
init_node(struct CCC_Array_adaptive_map const *const map, size_t const node)
{
    struct CCC_Array_adaptive_map_node *const e = node_at(map, node);
    e->branch[L] = e->branch[R] = e->parent = 0;
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
            + alignof(*(struct CCC_Array_adaptive_map){}.nodes) - 1)
         & ~(alignof(*(struct CCC_Array_adaptive_map){}.nodes) - 1);
}

/** Calculates the number of bytes needed for the nodes array without any
consideration for end padding as no arrays follow. */
static inline size_t
node_bytes(size_t const capacity)
{
    return sizeof(*(struct CCC_Array_adaptive_map){}.nodes) * capacity;
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
    return data_bytes(sizeof_type, capacity) + node_bytes(capacity);
}

/** Returns the base of the node array relative to the data base pointer. This
positions is guaranteed to be the first aligned byte given the alignment of the
node type after the data array. The data array has added any necessary padding
after it to ensure that the base of the node array is aligned for its type. */
static inline struct CCC_Array_adaptive_map_node *
node_pos(size_t const sizeof_type, void const *const data,
         size_t const capacity)
{
    return (struct CCC_Array_adaptive_map_node *)((char *)data
                                                  + data_bytes(sizeof_type,
                                                               capacity));
}

/** Copies over the Struct of Arrays contained within the one contiguous
allocation of the map to the new memory provided. Assumes the new_data pointer
points to the base of an allocation that has been allocated with sufficient
bytes to support the user data, nodes, and parity arrays for the provided new
capacity. */
static inline void
copy_soa(struct CCC_Array_adaptive_map const *const source,
         void *const destination_data_base, size_t const destination_capacity)
{
    if (!source->data)
    {
        return;
    }
    assert(destination_capacity >= source->capacity);
    size_t const sizeof_type = source->sizeof_type;
    /* Each section of the allocation "grows" when we re-size so one copy would
       not work. Instead each component is copied over allowing each to grow. */
    (void)memcpy(destination_data_base, source->data,
                 data_bytes(sizeof_type, source->capacity));
    (void)memcpy(
        node_pos(sizeof_type, destination_data_base, destination_capacity),
        node_pos(sizeof_type, source->data, source->capacity),
        node_bytes(source->capacity));
}

static inline void
swap(char temp[const], void *const a, void *const b, size_t const sizeof_type)
{
    if (a == b)
    {
        return;
    }
    (void)memcpy(temp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, temp, sizeof_type);
}

static inline struct CCC_Array_adaptive_map_node *
node_at(struct CCC_Array_adaptive_map const *const map, size_t const i)
{
    return &map->nodes[i];
}

static inline void *
data_at(struct CCC_Array_adaptive_map const *const map, size_t const i)
{
    return (char *)map->data + (i * map->sizeof_type);
}

static inline size_t
branch_index(struct CCC_Array_adaptive_map const *const map,
             size_t const parent, enum Branch const dir)
{
    return node_at(map, parent)->branch[dir];
}

static inline size_t
parent_index(struct CCC_Array_adaptive_map const *const map, size_t const child)
{
    return node_at(map, child)->parent;
}

static inline size_t *
branch_pointer(struct CCC_Array_adaptive_map const *const map,
               size_t const node, enum Branch const branch)
{
    return &node_at(map, node)->branch[branch];
}

static inline size_t *
parent_pointer(struct CCC_Array_adaptive_map const *const map,
               size_t const node)
{
    return &node_at(map, node)->parent;
}

static inline void *
key_at(struct CCC_Array_adaptive_map const *const map, size_t const i)
{
    return (char *)data_at(map, i) + map->key_offset;
}

static void *
key_in_slot(struct CCC_Array_adaptive_map const *map,
            void const *const user_struct)
{
    return (char *)user_struct + map->key_offset;
}

static inline size_t
max(size_t const a, size_t const b)
{
    return a > b ? a : b;
}

/*===========================   Validation   ===============================*/

/* NOLINTBEGIN(*misc-no-recursion) */

/** @internal */
struct Tree_range
{
    size_t low;
    size_t root;
    size_t high;
};

static size_t
recursive_count(struct CCC_Array_adaptive_map const *const map, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_count(map, branch_index(map, r, R))
         + recursive_count(map, branch_index(map, r, L));
}

static CCC_Tribool
are_subtrees_valid(struct CCC_Array_adaptive_map const *map,
                   struct Tree_range const r)
{
    if (!r.root)
    {
        return CCC_TRUE;
    }
    if (r.low
        && order_nodes(map, key_at(map, r.low), r.root, map->order)
               != CCC_ORDER_LESSER)
    {
        return CCC_FALSE;
    }
    if (r.high
        && order_nodes(map, key_at(map, r.high), r.root, map->order)
               != CCC_ORDER_GREATER)
    {
        return CCC_FALSE;
    }
    return are_subtrees_valid(
               map, (struct Tree_range){.low = r.low,
                                        .root = branch_index(map, r.root, L),
                                        .high = r.root})
        && are_subtrees_valid(
               map, (struct Tree_range){.low = r.root,
                                        .root = branch_index(map, r.root, R),
                                        .high = r.high});
}

static CCC_Tribool
is_storing_parent(struct CCC_Array_adaptive_map const *const map,
                  size_t const p, size_t const root)
{
    if (!root)
    {
        return CCC_TRUE;
    }
    if (parent_index(map, root) != p)
    {
        return CCC_FALSE;
    }
    return is_storing_parent(map, root, branch_index(map, root, L))
        && is_storing_parent(map, root, branch_index(map, root, R));
}

static CCC_Tribool
is_free_list_valid(struct CCC_Array_adaptive_map const *const map)
{
    if (!map->count)
    {
        return CCC_TRUE;
    }
    size_t cur_free_index = map->free_list;
    size_t list_count = 0;
    while (cur_free_index && list_count < map->capacity)
    {
        cur_free_index = node_at(map, cur_free_index)->next_free;
        ++list_count;
    }
    if (cur_free_index)
    {
        return CCC_FALSE;
    }
    if (list_count + map->count != map->capacity)
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

static CCC_Tribool
validate(struct CCC_Array_adaptive_map const *const map)
{
    if (!map->count)
    {
        return CCC_TRUE;
    }
    if (!are_subtrees_valid(map, (struct Tree_range){.root = map->root}))
    {
        return CCC_FALSE;
    }
    size_t const size = recursive_count(map, map->root);
    if (size && size != map->count - 1)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(map, 0, map->root))
    {
        return CCC_FALSE;
    }
    if (!is_free_list_valid(map))
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
