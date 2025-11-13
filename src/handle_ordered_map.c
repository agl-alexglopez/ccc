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
constant time queries for frequently accessed elements. */
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "handle_ordered_map.h"
#include "private/private_handle_ordered_map.h"
#include "private/private_types.h"
#include "types.h"

/*========================   Data Alignment Test   ==========================*/

/** @private A macro version of the runtime alignment operations we perform
for calculating bytes. This way we can use in static assert. The user data type
may not be the same alignment as the nodes and therefore the nodes array must
start at next aligned byte. */
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
CCC_handle_ordered_map_declare_fixed_map(fixed_map_test_type,
                                         struct Test_data_type, TCAP);
/** @private This is a static fixed size map exclusive to this translation unit
used to ensure assumptions about data layout are correct. The following static
asserts must be true in order to support the Struct of Array style layout we
use for the data and nodes. It is important that in our user code when we set
the positions of the node pointer relative to the data pointer the positions are
correct regardless of backing storage as a fixed map or heap allocation. */
static fixed_map_test_type data_nodes_layout_test;
/** Some assumptions in the code assume that nodes array is last so ensure that
is the case here. Also good to assume user data comes first. */
static_assert((char *)data_nodes_layout_test.data
                  < (char *)data_nodes_layout_test.nodes,
              "The order of the arrays in a Struct of Arrays map is data, then "
              "nodes.");
/** We don't care about the alignment or padding after the nodes array because
we never need to set or move any pointers to that position. The alignment is
important for the nodes pointer to be set to the correct aligned position and
so that we allocate enough bytes for our single allocation if the map is dynamic
and not a fixed type. */
static_assert(
    (char *)&data_nodes_layout_test.nodes[TCAP]
            - (char *)&data_nodes_layout_test.data[0]
        == roundup((sizeof(*data_nodes_layout_test.data) * TCAP),
                   alignof(*data_nodes_layout_test.nodes))
               + (sizeof(*data_nodes_layout_test.nodes) * TCAP),
    "The pointer difference in bytes between end of the nodes array and start "
    "of user data array must be the same as the total bytes we assume to be "
    "stored in that range. Alignment of user data must be considered.");
static_assert((char *)&data_nodes_layout_test.data
                      + roundup((sizeof(*data_nodes_layout_test.data) * TCAP),
                                alignof(*data_nodes_layout_test.nodes))
                  == (char *)&data_nodes_layout_test.nodes,
              "The start of the nodes array must begin at the next aligned "
              "byte given alignment of a node.");

/*==========================  Type Declarations   ===========================*/

/** @private */
enum
{
    LR = 2,
};

/** @private */
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
static size_t splay(struct CCC_Handle_ordered_map *t, size_t root,
                    void const *key, CCC_Key_comparator *order_fn);
static struct CCC_Handle_ordered_map_node *
node_at(struct CCC_Handle_ordered_map const *, size_t);
static void *data_at(struct CCC_Handle_ordered_map const *, size_t);
/* Returning the user struct type with stored offsets. */
static struct CCC_Handle_ordered_map_handle
handle(struct CCC_Handle_ordered_map *handle_ordered_map, void const *key);
static size_t erase(struct CCC_Handle_ordered_map *t, void const *key);
static size_t
maybe_allocate_insert(struct CCC_Handle_ordered_map *handle_ordered_map,
                      void const *user_type);
static CCC_Result resize(struct CCC_Handle_ordered_map *handle_ordered_map,
                         size_t new_capacity, CCC_Allocator *fn);
static void copy_soa(struct CCC_Handle_ordered_map const *src,
                     void *dst_data_base, size_t dst_capacity);
static size_t data_bytes(size_t sizeof_type, size_t capacity);
static size_t node_bytes(size_t capacity);
static struct CCC_Handle_ordered_map_node *
node_pos(size_t sizeof_type, void const *data, size_t capacity);
static size_t find(struct CCC_Handle_ordered_map *, void const *key);
static size_t connect_new_root(struct CCC_Handle_ordered_map *t,
                               size_t new_root, CCC_Order order_result);
static void insert(struct CCC_Handle_ordered_map *t, size_t n);
static void *key_in_slot(struct CCC_Handle_ordered_map const *t,
                         void const *user_struct);
static size_t allocate_slot(struct CCC_Handle_ordered_map *t);
static size_t total_bytes(size_t sizeof_type, size_t capacity);
static struct CCC_Range equal_range(struct CCC_Handle_ordered_map *t,
                                    void const *begin_key, void const *end_key,
                                    enum Branch traversal);
/* Returning the user key with stored offsets. */
static void *key_at(struct CCC_Handle_ordered_map const *t, size_t i);
/* Returning threeway comparison with user callback. */
static CCC_Order
order_nodes(struct CCC_Handle_ordered_map const *handle_ordered_map,
            void const *key, size_t node, CCC_Key_comparator *fn);
/* Returning read only indices for tree nodes. */
static size_t remove_from_tree(struct CCC_Handle_ordered_map *t, size_t ret);
static size_t min_max_from(struct CCC_Handle_ordered_map const *t, size_t start,
                           enum Branch dir);
static size_t next(struct CCC_Handle_ordered_map const *t, size_t n,
                   enum Branch traversal);
static size_t branch_i(struct CCC_Handle_ordered_map const *t, size_t parent,
                       enum Branch dir);
static size_t parent_i(struct CCC_Handle_ordered_map const *t, size_t child);
static size_t index_of(struct CCC_Handle_ordered_map const *t,
                       void const *key_val_type);
/* Returning references to index fields for tree nodes. */
static size_t *branch_ref(struct CCC_Handle_ordered_map const *t, size_t node,
                          enum Branch branch);
static size_t *parent_ref(struct CCC_Handle_ordered_map const *t, size_t node);

static CCC_Tribool
validate(struct CCC_Handle_ordered_map const *handle_ordered_map);

/* Returning void as miscellaneous helpers. */
static void init_node(struct CCC_Handle_ordered_map const *handle_ordered_map,
                      size_t node);
static void swap(char tmp[const], void *a, void *b, size_t sizeof_type);
static void link(struct CCC_Handle_ordered_map *t, size_t parent,
                 enum Branch dir, size_t subtree);
static size_t max(size_t, size_t);
static void delete_nodes(struct CCC_Handle_ordered_map *t,
                         CCC_Type_destructor *fn);

/*==============================  Interface    ==============================*/

void *
CCC_handle_ordered_map_at(CCC_Handle_ordered_map const *const h,
                          CCC_Handle_index const i)
{
    if (!h || !i)
    {
        return NULL;
    }
    return data_at(h, i);
}

CCC_Tribool
CCC_handle_ordered_map_contains(
    CCC_Handle_ordered_map *const handle_ordered_map, void const *const key)
{
    if (!handle_ordered_map || !key)
    {
        return CCC_TRIBOOL_ERROR;
    }
    handle_ordered_map->root
        = splay(handle_ordered_map, handle_ordered_map->root, key,
                handle_ordered_map->order);
    return order_nodes(handle_ordered_map, key, handle_ordered_map->root,
                       handle_ordered_map->order)
        == CCC_ORDER_EQUAL;
}

CCC_Handle_index
CCC_handle_ordered_map_get_key_val(
    CCC_Handle_ordered_map *const handle_ordered_map, void const *const key)
{
    if (!handle_ordered_map || !key)
    {
        return 0;
    }
    return find(handle_ordered_map, key);
}

CCC_Handle_ordered_map_handle
CCC_handle_ordered_map_handle(CCC_Handle_ordered_map *const handle_ordered_map,
                              void const *const key)
{
    if (!handle_ordered_map || !key)
    {
        return (CCC_Handle_ordered_map_handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    return (CCC_Handle_ordered_map_handle){handle(handle_ordered_map, key)};
}

CCC_Handle_index
CCC_handle_ordered_map_insert_handle(
    CCC_Handle_ordered_map_handle const *const h,
    void const *const key_val_type)
{
    if (!h || !key_val_type)
    {
        return 0;
    }
    if (h->private.stats == CCC_ENTRY_OCCUPIED)
    {
        void *const ret = data_at(h->private.handle_ordered_map, h->private.i);
        if (key_val_type != ret)
        {
            (void)memcpy(ret, key_val_type,
                         h->private.handle_ordered_map->sizeof_type);
        }
        return h->private.i;
    }
    return maybe_allocate_insert(h->private.handle_ordered_map, key_val_type);
}

CCC_Handle_ordered_map_handle *
CCC_handle_ordered_map_and_modify(CCC_Handle_ordered_map_handle *const h,
                                  CCC_Type_updater *const fn)
{
    if (!h)
    {
        return NULL;
    }
    if (fn && h->private.stats & CCC_ENTRY_OCCUPIED)
    {
        fn((CCC_Type_context){
            .type = data_at(h->private.handle_ordered_map, h->private.i),
            .context = NULL,
        });
    }
    return h;
}

CCC_Handle_ordered_map_handle *
CCC_handle_ordered_map_and_modify_context(
    CCC_Handle_ordered_map_handle *const h, CCC_Type_updater *const fn,
    void *const context)
{
    if (!h)
    {
        return NULL;
    }
    if (fn && h->private.stats & CCC_ENTRY_OCCUPIED)
    {
        fn((CCC_Type_context){
            .type = data_at(h->private.handle_ordered_map, h->private.i),
            .context = context,
        });
    }
    return h;
}

CCC_Handle_index
CCC_handle_ordered_map_or_insert(CCC_Handle_ordered_map_handle const *const h,
                                 void const *const key_val_type)
{
    if (!h || !key_val_type)
    {
        return 0;
    }
    if (h->private.stats & CCC_ENTRY_OCCUPIED)
    {
        return h->private.i;
    }
    return maybe_allocate_insert(h->private.handle_ordered_map, key_val_type);
}

CCC_Handle
CCC_handle_ordered_map_swap_handle(
    CCC_Handle_ordered_map *const handle_ordered_map,
    void *const key_val_output)
{
    if (!handle_ordered_map || !key_val_output)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    size_t const found = find(handle_ordered_map,
                              key_in_slot(handle_ordered_map, key_val_output));
    if (found)
    {
        assert(handle_ordered_map->root);
        void *const ret = data_at(handle_ordered_map, handle_ordered_map->root);
        void *const tmp = data_at(handle_ordered_map, 0);
        swap(tmp, key_val_output, ret, handle_ordered_map->sizeof_type);
        return (CCC_Handle){{
            .i = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted
        = maybe_allocate_insert(handle_ordered_map, key_val_output);
    if (!inserted)
    {
        return (CCC_Handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .i = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_handle_ordered_map_try_insert(
    CCC_Handle_ordered_map *const handle_ordered_map,
    void const *const key_val_type)
{
    if (!handle_ordered_map || !key_val_type)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    size_t const found = find(handle_ordered_map,
                              key_in_slot(handle_ordered_map, key_val_type));
    if (found)
    {
        assert(handle_ordered_map->root);
        return (CCC_Handle){{
            .i = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted
        = maybe_allocate_insert(handle_ordered_map, key_val_type);
    if (!inserted)
    {
        return (CCC_Handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .i = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_handle_ordered_map_insert_or_assign(
    CCC_Handle_ordered_map *const handle_ordered_map,
    void const *const key_val_type)
{
    if (!handle_ordered_map || !key_val_type)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    size_t const found = find(handle_ordered_map,
                              key_in_slot(handle_ordered_map, key_val_type));
    if (found)
    {
        assert(handle_ordered_map->root);
        void *const f_base = data_at(handle_ordered_map, found);
        if (key_val_type != f_base)
        {
            memcpy(f_base, key_val_type, handle_ordered_map->sizeof_type);
        }
        return (CCC_Handle){{
            .i = found,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    size_t const inserted
        = maybe_allocate_insert(handle_ordered_map, key_val_type);
    if (!inserted)
    {
        return (CCC_Handle){{
            .i = 0,
            .stats = CCC_ENTRY_INSERT_ERROR,
        }};
    }
    return (CCC_Handle){{
        .i = inserted,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle
CCC_handle_ordered_map_remove(CCC_Handle_ordered_map *const handle_ordered_map,
                              void *const key_val_output)
{
    if (!handle_ordered_map || !key_val_output)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    size_t const removed = erase(
        handle_ordered_map, key_in_slot(handle_ordered_map, key_val_output));
    if (!removed)
    {
        return (CCC_Handle){{
            .i = 0,
            .stats = CCC_ENTRY_VACANT,
        }};
    }
    assert(removed);
    void const *const r = data_at(handle_ordered_map, removed);
    if (key_val_output != r)
    {
        (void)memcpy(key_val_output, r, handle_ordered_map->sizeof_type);
    }
    return (CCC_Handle){{
        .i = 0,
        .stats = CCC_ENTRY_OCCUPIED,
    }};
}

CCC_Handle
CCC_handle_ordered_map_remove_handle(CCC_Handle_ordered_map_handle *const h)
{
    if (!h)
    {
        return (CCC_Handle){{.stats = CCC_ENTRY_ARG_ERROR}};
    }
    if (h->private.stats == CCC_ENTRY_OCCUPIED)
    {
        size_t const erased
            = erase(h->private.handle_ordered_map,
                    key_at(h->private.handle_ordered_map, h->private.i));
        assert(erased);
        return (CCC_Handle){{
            .i = erased,
            .stats = CCC_ENTRY_OCCUPIED,
        }};
    }
    return (CCC_Handle){{
        .i = 0,
        .stats = CCC_ENTRY_VACANT,
    }};
}

CCC_Handle_index
CCC_handle_ordered_map_unwrap(CCC_Handle_ordered_map_handle const *const h)
{
    if (!h)
    {
        return 0;
    }
    return h->private.stats == CCC_ENTRY_OCCUPIED ? h->private.i : 0;
}

CCC_Tribool
CCC_handle_ordered_map_insert_error(
    CCC_Handle_ordered_map_handle const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (h->private.stats & CCC_ENTRY_INSERT_ERROR) != 0;
}

CCC_Tribool
CCC_handle_ordered_map_occupied(CCC_Handle_ordered_map_handle const *const h)
{
    if (!h)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return (h->private.stats & CCC_ENTRY_OCCUPIED) != 0;
}

CCC_Handle_status
CCC_handle_ordered_map_handle_status(
    CCC_Handle_ordered_map_handle const *const h)
{
    return h ? h->private.stats : CCC_ENTRY_ARG_ERROR;
}

CCC_Tribool
CCC_handle_ordered_map_is_empty(
    CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return !CCC_handle_ordered_map_count(handle_ordered_map).count;
}

CCC_Count
CCC_handle_ordered_map_count(
    CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){
        .count = handle_ordered_map->count ? handle_ordered_map->count - 1 : 0};
}

CCC_Count
CCC_handle_ordered_map_capacity(
    CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map)
    {
        return (CCC_Count){.error = CCC_RESULT_ARGUMENT_ERROR};
    }
    return (CCC_Count){.count = handle_ordered_map->capacity};
}

void *
CCC_handle_ordered_map_begin(
    CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map || !handle_ordered_map->capacity)
    {
        return NULL;
    }
    size_t const n
        = min_max_from(handle_ordered_map, handle_ordered_map->root, L);
    return data_at(handle_ordered_map, n);
}

void *
CCC_handle_ordered_map_rbegin(
    CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map || !handle_ordered_map->capacity)
    {
        return NULL;
    }
    size_t const n
        = min_max_from(handle_ordered_map, handle_ordered_map->root, R);
    return data_at(handle_ordered_map, n);
}

void *
CCC_handle_ordered_map_next(
    CCC_Handle_ordered_map const *const handle_ordered_map, void const *const e)
{
    if (!handle_ordered_map || !handle_ordered_map->capacity)
    {
        return NULL;
    }
    size_t const n
        = next(handle_ordered_map, index_of(handle_ordered_map, e), INORDER);
    return data_at(handle_ordered_map, n);
}

void *
CCC_handle_ordered_map_rnext(
    CCC_Handle_ordered_map const *const handle_ordered_map, void const *const e)
{
    if (!handle_ordered_map || !e || !handle_ordered_map->capacity)
    {
        return NULL;
    }
    size_t const n
        = next(handle_ordered_map, index_of(handle_ordered_map, e), R_INORDER);
    return data_at(handle_ordered_map, n);
}

void *
CCC_handle_ordered_map_end(
    CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map || !handle_ordered_map->capacity)
    {
        return NULL;
    }
    return data_at(handle_ordered_map, 0);
}

void *
CCC_handle_ordered_map_rend(
    CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map || !handle_ordered_map->capacity)
    {
        return NULL;
    }
    return data_at(handle_ordered_map, 0);
}

CCC_Range
CCC_handle_ordered_map_equal_range(
    CCC_Handle_ordered_map *const handle_ordered_map,
    void const *const begin_key, void const *const end_key)
{
    if (!handle_ordered_map || !begin_key || !end_key)
    {
        return (CCC_Range){};
    }
    return (CCC_Range){
        equal_range(handle_ordered_map, begin_key, end_key, INORDER)};
}

CCC_Reverse_range
CCC_handle_ordered_map_equal_rrange(
    CCC_Handle_ordered_map *const handle_ordered_map,
    void const *const rbegin_key, void const *const rend_key)

{
    if (!handle_ordered_map || !rbegin_key || !rend_key)
    {
        return (CCC_Reverse_range){};
    }
    return (CCC_Reverse_range){
        equal_range(handle_ordered_map, rbegin_key, rend_key, R_INORDER)};
}

CCC_Result
CCC_handle_ordered_map_reserve(CCC_Handle_ordered_map *const handle_ordered_map,
                               size_t const to_add, CCC_Allocator *const fn)
{
    if (!handle_ordered_map || !fn)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    /* Once initialized the Buffer always has a size of one for root node. */
    size_t const needed
        = handle_ordered_map->count + to_add + (handle_ordered_map->count == 0);
    if (needed <= handle_ordered_map->capacity)
    {
        return CCC_RESULT_OK;
    }
    size_t const old_count = handle_ordered_map->count;
    size_t old_cap = handle_ordered_map->capacity;
    CCC_Result const r = resize(handle_ordered_map, needed, fn);
    if (r != CCC_RESULT_OK)
    {
        return r;
    }
    if (!old_cap)
    {
        handle_ordered_map->count = 1;
    }
    old_cap = old_count ? old_cap : 0;
    size_t const new_cap = handle_ordered_map->capacity;
    size_t prev = 0;
    for (ptrdiff_t i = (ptrdiff_t)new_cap - 1; i > 0 && i >= (ptrdiff_t)old_cap;
         prev = i, --i)
    {
        node_at(handle_ordered_map, i)->next_free = prev;
    }
    if (!handle_ordered_map->free_list)
    {
        handle_ordered_map->free_list = prev;
    }
    return CCC_RESULT_OK;
}

CCC_Result
CCC_handle_ordered_map_copy(CCC_Handle_ordered_map *const dst,
                            CCC_Handle_ordered_map const *const src,
                            CCC_Allocator *const fn)
{
    if (!dst || !src || src == dst || (dst->capacity < src->capacity && !fn))
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    void *const dst_mem = dst->data;
    struct CCC_Handle_ordered_map_node *const dst_nodes = dst->nodes;
    size_t const dst_cap = dst->capacity;
    CCC_Allocator *const dst_allocate = dst->allocate;
    *dst = *src;
    dst->data = dst_mem;
    dst->nodes = dst_nodes;
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
    }
    if (!dst->data || !src->data)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    copy_soa(src, dst->data, dst->capacity);
    return CCC_RESULT_OK;
}

CCC_Result
CCC_handle_ordered_map_clear(CCC_Handle_ordered_map *const handle_ordered_map,
                             CCC_Type_destructor *const fn)
{
    if (!handle_ordered_map)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!fn)
    {
        handle_ordered_map->root = 0;
        handle_ordered_map->count = 1;
        return CCC_RESULT_OK;
    }
    delete_nodes(handle_ordered_map, fn);
    handle_ordered_map->count = 1;
    handle_ordered_map->root = 0;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_handle_ordered_map_clear_and_free(
    CCC_Handle_ordered_map *const handle_ordered_map,
    CCC_Type_destructor *const fn)
{
    if (!handle_ordered_map || !handle_ordered_map->allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!fn)
    {
        handle_ordered_map->root = 0;
        handle_ordered_map->count = 0;
        handle_ordered_map->capacity = 0;
        (void)handle_ordered_map->allocate((CCC_Allocator_context){
            .input = handle_ordered_map->data,
            .bytes = 0,
            .context = handle_ordered_map->context,
        });
        handle_ordered_map->data = NULL;
        handle_ordered_map->nodes = NULL;
        return CCC_RESULT_OK;
    }
    delete_nodes(handle_ordered_map, fn);
    handle_ordered_map->root = 0;
    handle_ordered_map->count = 0;
    handle_ordered_map->capacity = 0;
    (void)handle_ordered_map->allocate((CCC_Allocator_context){
        .input = handle_ordered_map->data,
        .bytes = 0,
        .context = handle_ordered_map->context,
    });
    handle_ordered_map->data = NULL;
    handle_ordered_map->nodes = NULL;
    return CCC_RESULT_OK;
}

CCC_Result
CCC_handle_ordered_map_clear_and_free_reserve(
    CCC_Handle_ordered_map *const handle_ordered_map,
    CCC_Type_destructor *const destructor, CCC_Allocator *const allocate)
{
    if (!handle_ordered_map || !allocate)
    {
        return CCC_RESULT_ARGUMENT_ERROR;
    }
    if (!destructor)
    {
        handle_ordered_map->root = 0;
        handle_ordered_map->count = 0;
        handle_ordered_map->capacity = 0;
        (void)allocate((CCC_Allocator_context){
            .input = handle_ordered_map->data,
            .bytes = 0,
            .context = handle_ordered_map->context,
        });
        handle_ordered_map->data = NULL;
        return CCC_RESULT_OK;
    }
    delete_nodes(handle_ordered_map, destructor);
    handle_ordered_map->root = 0;
    handle_ordered_map->count = 0;
    handle_ordered_map->capacity = 0;
    (void)allocate((CCC_Allocator_context){
        .input = handle_ordered_map->data,
        .bytes = 0,
        .context = handle_ordered_map->context,
    });
    handle_ordered_map->data = NULL;
    return CCC_RESULT_OK;
}

CCC_Tribool
CCC_handle_ordered_map_validate(
    CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map)
    {
        return CCC_TRIBOOL_ERROR;
    }
    return validate(handle_ordered_map);
}

/*===========================   Private Interface ===========================*/

void
CCC_private_handle_ordered_map_insert(
    struct CCC_Handle_ordered_map *const handle_ordered_map,
    size_t const elem_i)
{
    insert(handle_ordered_map, elem_i);
}

struct CCC_Handle_ordered_map_handle
CCC_private_handle_ordered_map_handle(
    struct CCC_Handle_ordered_map *const handle_ordered_map,
    void const *const key)
{
    return handle(handle_ordered_map, key);
}

void *
CCC_private_handle_ordered_map_key_at(
    struct CCC_Handle_ordered_map const *const handle_ordered_map,
    size_t const slot)
{
    return key_at(handle_ordered_map, slot);
}

void *
CCC_private_handle_ordered_map_data_at(
    struct CCC_Handle_ordered_map const *const handle_ordered_map,
    size_t const slot)
{
    return data_at(handle_ordered_map, slot);
}

size_t
CCC_private_handle_ordered_map_allocate_slot(
    struct CCC_Handle_ordered_map *const handle_ordered_map)
{
    return allocate_slot(handle_ordered_map);
}

/*===========================   Static Helpers    ===========================*/

static struct CCC_Range
equal_range(struct CCC_Handle_ordered_map *const t, void const *const begin_key,
            void const *const end_key, enum Branch const traversal)
{
    if (CCC_handle_ordered_map_is_empty(t))
    {
        return (struct CCC_Range){};
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
    return (struct CCC_Range){
        .begin = data_at(t, b),
        .end = data_at(t, e),
    };
}

static struct CCC_Handle_ordered_map_handle
handle(struct CCC_Handle_ordered_map *const handle_ordered_map,
       void const *const key)
{
    size_t const found = find(handle_ordered_map, key);
    if (found)
    {
        return (struct CCC_Handle_ordered_map_handle){
            .handle_ordered_map = handle_ordered_map,
            .i = found,
            .stats = CCC_ENTRY_OCCUPIED,
        };
    }
    return (struct CCC_Handle_ordered_map_handle){
        .handle_ordered_map = handle_ordered_map,
        .i = 0,
        .stats = CCC_ENTRY_VACANT,
    };
}

static size_t
maybe_allocate_insert(struct CCC_Handle_ordered_map *const handle_ordered_map,
                      void const *const user_type)
{
    /* The end sentinel node will always be at 0. This also means once
       initialized the internal size for implementer is always at least 1. */
    size_t const node = allocate_slot(handle_ordered_map);
    if (!node)
    {
        return 0;
    }
    (void)memcpy(data_at(handle_ordered_map, node), user_type,
                 handle_ordered_map->sizeof_type);
    insert(handle_ordered_map, node);
    return node;
}

static size_t
allocate_slot(struct CCC_Handle_ordered_map *const t)
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
            if (resize(t, max(old_cap * 2, 8), t->allocate) != CCC_RESULT_OK)
            {
                return 0;
            }
        }
        else
        {
            t->nodes = node_pos(t->sizeof_type, t->data, t->capacity);
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
resize(struct CCC_Handle_ordered_map *const handle_ordered_map,
       size_t const new_capacity, CCC_Allocator *const fn)
{
    if (handle_ordered_map->capacity
        && new_capacity <= handle_ordered_map->capacity - 1)
    {
        return CCC_RESULT_OK;
    }
    if (!fn)
    {
        return CCC_RESULT_NO_ALLOCATION_FUNCTION;
    }
    void *const new_data = fn((CCC_Allocator_context){
        .input = NULL,
        .bytes = total_bytes(handle_ordered_map->sizeof_type, new_capacity),
        .context = handle_ordered_map->context,
    });
    if (!new_data)
    {
        return CCC_RESULT_ALLOCATOR_ERROR;
    }
    copy_soa(handle_ordered_map, new_data, new_capacity);
    handle_ordered_map->nodes
        = node_pos(handle_ordered_map->sizeof_type, new_data, new_capacity);
    fn((CCC_Allocator_context){
        .input = handle_ordered_map->data,
        .bytes = 0,
        .context = handle_ordered_map->context,
    });
    handle_ordered_map->data = new_data;
    handle_ordered_map->capacity = new_capacity;
    return CCC_RESULT_OK;
}

static void
insert(struct CCC_Handle_ordered_map *const t, size_t const n)
{
    init_node(t, n);
    if (t->count == EMPTY_TREE)
    {
        t->root = n;
        return;
    }
    void const *const key = key_at(t, n);
    t->root = splay(t, t->root, key, t->order);
    CCC_Order const root_order = order_nodes(t, key, t->root, t->order);
    if (CCC_ORDER_EQUAL == root_order)
    {
        return;
    }
    (void)connect_new_root(t, n, root_order);
}

static size_t
erase(struct CCC_Handle_ordered_map *const t, void const *const key)
{
    if (CCC_handle_ordered_map_is_empty(t))
    {
        return 0;
    }
    size_t ret = splay(t, t->root, key, t->order);
    CCC_Order const found = order_nodes(t, key, ret, t->order);
    if (found != CCC_ORDER_EQUAL)
    {
        return 0;
    }
    ret = remove_from_tree(t, ret);
    return ret;
}

static size_t
remove_from_tree(struct CCC_Handle_ordered_map *const t, size_t const ret)
{
    if (!branch_i(t, ret, L))
    {
        t->root = branch_i(t, ret, R);
        link(t, 0, 0, t->root);
    }
    else
    {
        t->root = splay(t, branch_i(t, ret, L), key_at(t, ret), t->order);
        link(t, t->root, R, branch_i(t, ret, R));
    }
    node_at(t, ret)->next_free = t->free_list;
    t->free_list = ret;
    --t->count;
    return ret;
}

static size_t
connect_new_root(struct CCC_Handle_ordered_map *const t, size_t const new_root,
                 CCC_Order const order_result)
{
    enum Branch const dir = CCC_ORDER_GREATER == order_result;
    link(t, new_root, dir, branch_i(t, t->root, dir));
    link(t, new_root, !dir, t->root);
    *branch_ref(t, t->root, dir) = 0;
    t->root = new_root;
    /* The direction from end node is arbitrary. Need root to update parent. */
    link(t, 0, 0, t->root);
    return new_root;
}

static size_t
find(struct CCC_Handle_ordered_map *const t, void const *const key)
{
    if (!t->root)
    {
        return 0;
    }
    t->root = splay(t, t->root, key, t->order);
    return order_nodes(t, key, t->root, t->order) == CCC_ORDER_EQUAL ? t->root
                                                                     : 0;
}

static size_t
splay(struct CCC_Handle_ordered_map *const t, size_t root,
      void const *const key, CCC_Key_comparator *const order_fn)
{
    /* Pointers in an array and we can use the symmetric enum and flip it to
       choose the Left or Right subtree. Another benefit of our nil node: use it
       as our helper tree because we don't need its Left Right fields. */
    struct CCC_Handle_ordered_map_node *const nil = node_at(t, 0);
    nil->branch[L] = nil->branch[R] = nil->parent = 0;
    size_t l_r_subtrees[LR] = {0, 0};
    for (;;)
    {
        CCC_Order const key_order = order_nodes(t, key, root, order_fn);
        enum Branch const child_link = CCC_ORDER_GREATER == key_order;
        if (CCC_ORDER_EQUAL == key_order || !branch_i(t, root, child_link))
        {
            break;
        }
        CCC_Order const child_order
            = order_nodes(t, key, branch_i(t, root, child_link), order_fn);
        enum Branch const grandchild_link = CCC_ORDER_GREATER == child_order;
        /* A straight line has formed from root->child->grandchild. An
           opportunity to splay and heal the tree arises. */
        if (CCC_ORDER_EQUAL != child_order && child_link == grandchild_link)
        {
            size_t const child_node = branch_i(t, root, child_link);
            link(t, root, child_link, branch_i(t, child_node, !child_link));
            link(t, child_node, !child_link, root);
            root = child_node;
            if (!branch_i(t, root, child_link))
            {
                break;
            }
        }
        link(t, l_r_subtrees[!child_link], child_link, root);
        l_r_subtrees[!child_link] = root;
        root = branch_i(t, root, child_link);
    }
    link(t, l_r_subtrees[L], R, branch_i(t, root, L));
    link(t, l_r_subtrees[R], L, branch_i(t, root, R));
    link(t, root, L, nil->branch[R]);
    link(t, root, R, nil->branch[L]);
    t->root = root;
    link(t, 0, 0, t->root);
    return root;
}

/** Links the parent node to node starting at subtree root via direction dir.
updates the parent of the child being picked up by the new parent as well. */
static inline void
link(struct CCC_Handle_ordered_map *const t, size_t const parent,
     enum Branch const dir, size_t const subtree)
{
    *branch_ref(t, parent, dir) = subtree;
    *parent_ref(t, subtree) = parent;
}

static size_t
min_max_from(struct CCC_Handle_ordered_map const *const t, size_t start,
             enum Branch const dir)
{
    if (!start)
    {
        return 0;
    }
    for (; branch_i(t, start, dir); start = branch_i(t, start, dir))
    {}
    return start;
}

static size_t
next(struct CCC_Handle_ordered_map const *const t, size_t n,
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

/** Deletes all nodes in the tree by calling destructor function on them in
linear time and constant space. This function modifies nodes as it deletes the
tree elements. Assumes the destructor function is non-null.

This function does not update any count or capacity fields of the map, it
simply calls the destructor on each node and removes the nodes references to
other tree elements. */
static void
delete_nodes(struct CCC_Handle_ordered_map *const t,
             CCC_Type_destructor *const fn)
{
    assert(t);
    assert(fn);
    size_t node = t->root;
    while (node)
    {
        struct CCC_Handle_ordered_map_node *const e = node_at(t, node);
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
order_nodes(struct CCC_Handle_ordered_map const *const handle_ordered_map,
            void const *const key, size_t const node,
            CCC_Key_comparator *const fn)
{
    return fn((CCC_Key_comparator_context){
        .key_lhs = key,
        .type_rhs = data_at(handle_ordered_map, node),
        .context = handle_ordered_map->context,
    });
}

static inline void
init_node(struct CCC_Handle_ordered_map const *const handle_ordered_map,
          size_t const node)
{
    struct CCC_Handle_ordered_map_node *const e
        = node_at(handle_ordered_map, node);
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
            + alignof(*(struct CCC_Handle_ordered_map){}.nodes) - 1)
         & ~(alignof(*(struct CCC_Handle_ordered_map){}.nodes) - 1);
}

/** Calculates the number of bytes needed for the nodes array without any
consideration for end padding as no arrays follow. */
static inline size_t
node_bytes(size_t const capacity)
{
    return sizeof(*(struct CCC_Handle_ordered_map){}.nodes) * capacity;
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
static inline struct CCC_Handle_ordered_map_node *
node_pos(size_t const sizeof_type, void const *const data,
         size_t const capacity)
{
    return (struct CCC_Handle_ordered_map_node *)((char *)data
                                                  + data_bytes(sizeof_type,
                                                               capacity));
}

/** Copies over the Struct of Arrays contained within the one contiguous
allocation of the map to the new memory provided. Assumes the new_data pointer
points to the base of an allocation that has been allocated with sufficient
bytes to support the user data, nodes, and parity arrays for the provided new
capacity. */
static inline void
copy_soa(struct CCC_Handle_ordered_map const *const src,
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
}

static inline void
swap(char tmp[const], void *const a, void *const b, size_t const sizeof_type)
{
    if (a == b)
    {
        return;
    }
    (void)memcpy(tmp, a, sizeof_type);
    (void)memcpy(a, b, sizeof_type);
    (void)memcpy(b, tmp, sizeof_type);
}

static inline struct CCC_Handle_ordered_map_node *
node_at(struct CCC_Handle_ordered_map const *const t, size_t const i)
{
    return &t->nodes[i];
}

static inline void *
data_at(struct CCC_Handle_ordered_map const *const t, size_t const i)
{
    return (char *)t->data + (i * t->sizeof_type);
}

static inline size_t
branch_i(struct CCC_Handle_ordered_map const *const t, size_t const parent,
         enum Branch const dir)
{
    return node_at(t, parent)->branch[dir];
}

static inline size_t
parent_i(struct CCC_Handle_ordered_map const *const t, size_t const child)
{
    return node_at(t, child)->parent;
}

static inline size_t
index_of(struct CCC_Handle_ordered_map const *const t,
         void const *const key_val_type)
{
    assert(key_val_type >= t->data
           && (char *)key_val_type
                  < ((char *)t->data + (t->capacity * t->sizeof_type)));
    return ((char *)key_val_type - (char *)t->data) / t->sizeof_type;
}

static inline size_t *
branch_ref(struct CCC_Handle_ordered_map const *const t, size_t const node,
           enum Branch const branch)
{
    return &node_at(t, node)->branch[branch];
}

static inline size_t *
parent_ref(struct CCC_Handle_ordered_map const *const t, size_t const node)
{
    return &node_at(t, node)->parent;
}

static inline void *
key_at(struct CCC_Handle_ordered_map const *const t, size_t const i)
{
    return (char *)data_at(t, i) + t->key_offset;
}

static void *
key_in_slot(struct CCC_Handle_ordered_map const *t,
            void const *const user_struct)
{
    return (char *)user_struct + t->key_offset;
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
recursive_count(struct CCC_Handle_ordered_map const *const t, size_t const r)
{
    if (!r)
    {
        return 0;
    }
    return 1 + recursive_count(t, branch_i(t, r, R))
         + recursive_count(t, branch_i(t, r, L));
}

static CCC_Tribool
are_subtrees_valid(struct CCC_Handle_ordered_map const *t,
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
    return are_subtrees_valid(
               t, (struct Tree_range){.low = r.low,
                                      .root = branch_i(t, r.root, L),
                                      .high = r.root})
        && are_subtrees_valid(
               t, (struct Tree_range){.low = r.root,
                                      .root = branch_i(t, r.root, R),
                                      .high = r.high});
}

static CCC_Tribool
is_storing_parent(struct CCC_Handle_ordered_map const *const t, size_t const p,
                  size_t const root)
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
is_free_list_valid(struct CCC_Handle_ordered_map const *const t)
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

static CCC_Tribool
validate(struct CCC_Handle_ordered_map const *const handle_ordered_map)
{
    if (!handle_ordered_map->count)
    {
        return CCC_TRUE;
    }
    if (!are_subtrees_valid(
            handle_ordered_map,
            (struct Tree_range){.root = handle_ordered_map->root}))
    {
        return CCC_FALSE;
    }
    size_t const size
        = recursive_count(handle_ordered_map, handle_ordered_map->root);
    if (size && size != handle_ordered_map->count - 1)
    {
        return CCC_FALSE;
    }
    if (!is_storing_parent(handle_ordered_map, 0, handle_ordered_map->root))
    {
        return CCC_FALSE;
    }
    if (!is_free_list_valid(handle_ordered_map))
    {
        return CCC_FALSE;
    }
    return CCC_TRUE;
}

/* NOLINTEND(*misc-no-recursion) */
