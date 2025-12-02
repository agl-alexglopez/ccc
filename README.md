# C Container Collection (CCC)

The C Container Collection offers a variety of containers for C programmers who want fine-grained control of memory in their programs. All containers offer both allocating and non-allocating interfaces. For the motivations of why such a library is helpful in C read on.

## Installation

The following are required for install:

- GCC or Clang supporting `C23`.
    - 100% coverage of `C23` is not required. For example, at the time of writing Clang 19.1.1 and GCC 14.2 have all features used in this collection covered, but older versions of each compiler may work as well.
- CMake >= 3.23.
- Read [INSTALL.md](INSTALL.md).

Currently, this library supports a `FetchContent` or manual installation via CMake. The [INSTALL.md](INSTALL.md) file is included in all [Releases](https://github.com/agl-alexglopez/ccc/releases).

## Quick Start

- Read the [DOCS](https://agl-alexglopez.github.io/ccc).
- Read [types.h](https://agl-alexglopez.github.io/ccc/types_8h.html) to understand the `CCC_Allocator` interface.
- Read the [header](https://agl-alexglopez.github.io/ccc/files.html) for the desired container to understand its functionality.
- Read about generic [traits.h](https://agl-alexglopez.github.io/ccc/traits_8h.html) shared across containers to make code more succinct.
- Read [CONTRIBUTING.md](CONTRIBUTING.md) if interested in project structure, tools, and todos.

## Containers

<details>
<summary>bitset.h (dropdown)</summary>
A fixed or dynamic contiguous array of bits for set operations.

```c
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#define BITSET_USING_NAMESPACE_CCC
#include "ccc/Bitset.h"
#include "ccc/types.h"

#define DIGITS 9L
#define ROWS DIGITS
#define COLS DIGITS
#define BOX_SIZE 3L

/* clang-format off */
static int const valid_board[9][9] =
{{5,3,0, 0,7,0, 0,0,0}
,{6,0,0, 1,9,5, 0,0,0}
,{0,9,8, 0,0,0, 0,6,0}

,{8,0,0, 0,6,0, 0,0,3}
,{4,0,0, 8,0,3, 0,0,1}
,{7,0,0, 0,2,0, 0,0,6}

,{0,6,0, 0,0,0, 2,8,0}
,{0,0,0, 4,1,9, 0,0,5}
,{0,0,0, 0,8,0, 0,7,9}};

static int const invalid_board[9][9] =
{{8,3,0, 0,7,0, 0,0,0} /* 8 in first box top left. */
,{6,0,0, 1,9,5, 0,0,0}
,{0,9,8, 0,0,0, 0,6,0} /* 8 in first box bottom right. */

,{8,0,0, 0,6,0, 0,0,3} /* 8 also overlaps with 8 in top left by row. */
,{4,0,0, 8,0,3, 0,0,1}
,{7,0,0, 0,2,0, 0,0,6}

,{0,6,0, 0,0,0, 2,8,0}
,{0,0,0, 4,1,9, 0,0,5}
,{0,0,0, 0,8,0, 0,7,9}};
/* clang-format on */

/* Returns if the box is valid (CCC_TRUE if valid CCC_FALSE if not). */
static CCC_Tribool
validate_sudoku_box(int const board[9][9], Bitset *const row_check,
                    Bitset *const col_check, size_t const row_start,
                    size_t const col_start)
{
    Bitset box_check = bitset_initialize(bitset_blocks(DIGITS), NULL, NULL, DIGITS);
    CCC_Tribool was_on = CCC_FALSE;
    for (size_t r = row_start; r < row_start + BOX_SIZE; ++r)
    {
        for (size_t c = col_start; c < col_start + BOX_SIZE; ++c)
        {
            if (!board[r][c])
            {
                continue;
            }
            /* Need the zero based digit. */
            size_t const digit = board[r][c] - 1;
            was_on = bitset_set(&box_check, digit, CCC_TRUE);
            if (was_on)
            {
                return CCC_FALSE;
            }
            was_on = bitset_set(row_check, (r * DIGITS) + digit, CCC_TRUE);
            if (was_on)
            {
                return CCC_FALSE;
            }
            was_on = bitset_set(col_check, (c * DIGITS) + digit, CCC_TRUE);
            if (was_on)
            {
                return CCC_FALSE;
            }
        }
    }
    return CCC_TRUE;
}

/* A small problem like this is a perfect use case for a stack based bit set.
   All sizes are known at compile time meaning we get memory management for
   free and the optimal space and time complexity for this problem. */

static CCC_Tribool
is_valid_sudoku(int const board[9][9])
{
    Bitset row_check
        = bitset_initialize(bitset_blocks(ROWS * DIGITS), NULL, NULL, ROWS * DIGITS);
    Bitset col_check
        = bitset_initialize(bitset_blocks(ROWS * DIGITS), NULL, NULL, COLS * DIGITS);
    for (size_t row = 0; row < ROWS; row += BOX_SIZE)
    {
        for (size_t col = 0; col < COLS; col += BOX_SIZE)
        {
            if (!validate_sudoku_box(board, &row_check, &col_check, row, col))
            {
                return CCC_FALSE;
            }
        }
    }
    return CCC_TRUE;
}

int
main(void)
{
    CCC_Tribool result = is_valid_sudoku(valid_board);
    assert(result == CCC_TRUE);
    result = is_valid_sudoku(invalid_board);
    assert(result == CCC_FALSE);
    return 0;
}
```
</details>

<details>
<summary>buffer.h (dropdown)</summary>
A fixed or dynamic contiguous array of a single user defined type.

```c
#include <assert.h>
#define BUFFER_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/buffer.h"
#include "ccc/traits.h"

enum : size_t
{
    HCAP = 12,
};

static inline int
maxint(int const a, int const b)
{
    return a > b ? a : b;
}

/* Trapping rainwater Leetcode problem with iterators */
int
main(void)
{
    Buffer const heights
        = buffer_initialize(((int[HCAP]){0, 1, 0, 2, 1, 0, 1, 3, 2, 1, 2, 1}), int, NULL,
                   NULL, HCAP, HCAP);
    int const correct_trapped = 6;
    int trapped = 0;
    int lpeak = *buffer_front_as(&heights, int);
    int rpeak = *buffer_back_as(&heights, int);
    /* Easy way to have a "skip first" iterator because the iterator is
       returned from each iterator function. */
    int const *l = next(&heights, begin(&heights));
    int const *r = reverse_next(&heights, reverse_begin(&heights));
    while (l <= r)
    {
        if (lpeak < rpeak)
        {
            lpeak = maxint(lpeak, *l);
            trapped += (lpeak - *l);
            l = next(&heights, l);
        }
        else
        {
            rpeak = maxint(rpeak, *r);
            trapped += (rpeak - *r);
            r = reverse_next(&heights, r);
        }
    }
    assert(trapped == correct_trapped);
    return 0;
}
```
</details>

<details>
<summary>doubly_linked_list.h (dropdown)</summary>
A dynamic container for efficient insertion and removal at any position.

```c
#include <assert.h>
#define DOUBLY_LINKED_LIST_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/doubly_linked_list.h"
#include "ccc/traits.h"

struct Int_node
{
    int i;
    Doubly_linked_list_node e;
};

static CCC_Order
int_cmp(CCC_Type_comparator_context const cmp)
{
    struct Int_node const *const left = cmp.type_left;
    struct Int_node const *const right = cmp.type_right;
    return (left->i > right->i) - (left->i < right->i);
}

int
main(void)
{
    /* doubly linked list l, list elem field e, no allocation permission,
       comparing integers, no context data. */
    Doubly_linked_list l = doubly_linked_list_initialize(l, struct Int_node, e, int_cmp, NULL, NULL);
    struct Int_node elems[3] = {{.i = 3}, {.i = 2}, {.i = 1}};
    (void)push_back(&l, &elems[0].e);
    (void)push_front(&l, &elems[1].e);
    (void)push_back(&l, &elems[2].e);
    (void)pop_back(&l);
    struct Int_node *e = back(&l);
    assert(e->i == 3);
    return 0;
}
```

</details>

<details>
<summary>flat_doubled_ended_queue.h (dropdown)</summary>
A dynamic or fixed size double ended queue offering contiguously stored elements. When fixed size, its behavior is that of a ring buffer.

```c
#include <assert.h>
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/flat_double_ended_queue.h"
#include "ccc/traits.h"

int
main(void)
{
    /* stack array, no allocation permission, no context data, capacity 2 */
    Flat_doubled_ended_queue q = flat_doubled_ended_queue_initialize((int[2]){}, int, NULL, NULL, 2);
    (void)push_back(&q, &(int){3});
    (void)push_front(&q, &(int){2});
    (void)push_back(&q, &(int){1}); /* Overwrite 2. */
    int *i = front(&q);
    assert(*i == 3);
    i = back(&q);
    assert(*i == 1);
    (void)pop_back(&q);
    i = back(&q);
    assert(*i == 3);
    i = front(&q);
    assert(*i == 3);
    return 0;
}
```

</details>

<details>
<summary>flat_hash_map.h (dropdown)</summary>
Amortized O(1) access to elements stored in a flat array by key. Not pointer stable.

```c
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/flat_hash_map.h"
#include "ccc/traits.h"

struct Key_val
{
    int key;
    int val;
};

static uint64_t
flat_hash_map_int_to_u64(CCC_Key_context const k)
{
    int const key_int = *((int *)k.key);
    uint64_t x = key_int;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

CCC_Order
flat_hash_map_id_cmp(CCC_Key_comparator_context const cmp)
{
    struct Key_val const *const right = cmp.type_right;
    int const left = *((int *)cmp.key_left;
    return (left > right->key) - (left < right->key);
}

flat_hash_map_declare_fixed_map(Standard_fixed_map, struct Key_val, 1024);

enum : size_t
{
    STANDARD_FIXED_CAP = flat_hash_map_fixed_capacity(Standard_fixed_map),
};

/* Longest Consecutive Sequence Leetcode Problem */
int
main(void)
{
    CCC_Flat_hash_map fh = flat_hash_map_initialize(
        &(Standard_fixed_map){},
        struct Key_val,
        key,
        flat_hash_map_int_to_u64,
        flat_hash_map_id_cmp,
        NULL,
        NULL,
        STANDARD_FIXED_CAP
    );
    /* Longest sequence is 1,2,3,4,5,6,7,8,9,10 of length 10. */
    int const nums[] = {
        99, 54, 1, 4, 9,  2, 3,   4,  8,  271, 32, 45, 86, 44, 7,  777, 6,  20,
        19, 5,  9, 1, 10, 4, 101, 15, 16, 17,  18, 19, 20, 10, 21, 22,  23,
    };
    static_assert(sizeof(nums) / sizeof(nums[0]) < STANDARD_FIXED_CAP / 2);
    int const correct_max_run = 10;
    size_t const nums_size = sizeof(nums) / sizeof(nums[0]);
    int max_run = 0;
    for (size_t i = 0; i < nums_size; ++i)
    {
        int const n = nums[i];
        CCC_Entry const *const seen_n
            = try_insert_wrap(&fh, &(struct Key_val){.key = n, .val = 1});
        /* We have already connected this run as much as possible. */
        if (occupied(seen_n))
        {
            continue;
        }
        assert(!insert_error(seen_n));

        /* There may or may not be runs already existing to left and right. */
        struct Key_val const *const connect_left
            = get_key_value(&fh, &(int){n - 1});
        struct Key_val const *const connect_right
            = get_key_value(&fh, &(int){n + 1});
        int const left_run = connect_left ? connect_left->val : 0;
        int const right_run = connect_right ? connect_right->val : 0;
        int const full_run = left_run + 1 + right_run;

        /* Track solution to problem. */
        max_run = full_run > max_run ? full_run : max_run;

        /* Update the boundaries of the full run range. */
        ((struct Key_val *)unwrap(seen_n))->val = full_run;
        CCC_Entry const *const run_min = insert_or_assign_wrap(
            &fh, &(struct Key_val){.key = n - left_run, .val = full_run});
        CCC_Entry const *const run_max = insert_or_assign_wrap(
            &fh, &(struct Key_val){.key = n + right_run, .val = full_run});
        assert(occupied(run_min));
        assert(occupied(run_max));
        assert(!insert_error(run_min));
        assert(!insert_error(run_max));
    }
    assert(max_run == correct_max_run);
    return 0;
}
```

</details>

<details>
<summary>flat_priority_queue.h (dropdown)</summary>

```c
#include <assert.h>
#define BUFFER_USING_NAMESPACE_CCC
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/buffer.h"
#include "ccc/flat_priority_queue.h"
#include "ccc/traits.h"

CCC_Order
int_cmp(CCC_Type_comparator_context const ints)
{
    int const left = *(int *)ints.type_left;
    int const right = *(int *)ints.type_right;
    return (left > right) - (left < right);
}

/* In place O(N * log(N)) time O(1) space sort. */
int
main(void)
{
    int heap[20] = {12, 61, -39, 76, 48, -93, -77, -81, 35, 21,
                    -3, 90, 20,  27, 97, -22, -20, -19, 70, 76};
    enum : size_t
    {
        HCAP = sizeof(heap) / sizeof(*heap),
    };
    Flat_priority_queue priority_queue = flat_priority_queue_heapify_initialize(heap, int, CCC_LES, int_cmp, NULL,
                                              NULL, HCAP, HCAP);
    Buffer const b = flat_priority_queue_heapsort(&priority_queue, &(int){0});
    int const *prev = begin(&b);
    assert(prev != NULL);
    assert(buffer_count(&b).count == HCAP);
    size_t count = 1;
    for (int const *cur = next(&b, prev); cur != end(&b); cur = next(&b, cur))
    {
        assert(*prev >= *cur);
        prev = cur;
        ++count;
    }
    assert(count == HCAP);
    return 0;
}
```

</details>

<details>
<summary>array_adaptive_map.h (dropdown)</summary>
An ordered map implemented in array with an index based self-optimizing tree. Offers handle stability. Handles remain valid until an element is removed from a table regardless of other insertions, other deletions, or resizing of the array.

```c
#include <assert.h>
#include <stdbool.h>
#define ARRAY_ADAPTIVE_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#include "ccc/array_adaptive_map.h"
#include "ccc/traits.h"

struct Key_val
{
    int key;
    int val;
};

array_adaptive_map_declare_fixed_map(Key_val_fixed_map, struct Key_val, 26);

static CCC_Order
Key_val_cmp(CCC_Key_comparator_context const cmp)
{
    struct Key_val const *const right = cmp.type_right;
    int const key_left = *((int *)cmp.key_left);
    return (key_left > right->key) - (key_left < right->key);
}

int
main(void)
{
    /* stack array of 25 elements with one slot for sentinel, intrusive field
       named elem, key field named key, no allocation permission, key comparison
       function, no context data. */
    Array_adaptive_map s = array_adaptive_map_initialize(
        &(Key_val_fixed_map){},
        struct Key_val,
        key,
        Key_val_cmp,
        NULL,
        NULL,
        array_adaptive_map_fixed_capacity(Key_val_fixed_map)
    );
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct Key_val){.key = id, .val = i}.elem);
    }
    /* This should be the following Range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int range_keys[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    Handle_range r = equal_range(&s, &(int){6}, &(int){44});
    int index = 0;
    for (Handle_index i = range_begin(&r); i != range_end(&r); i = next(&s, i))
    {
        struct Key_val const *const kv = array_adaptive_map_at(&s, i);
        assert(kv->key == range_keys[index]);
        ++index;
    }
    /* This should be the following Range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int range_reverse_keys[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    Handle_range_reverse rr = equal_range_reverse(&s, &(int){119}, &(int){84});
    index = 0;
    for (Handle_index i = range_reverse_begin(&rr); i != range_reverse_end(&rr);
         i = reverse_next(&s, i))
    {
        struct Key_val const *const kv = array_adaptive_map_at(&s, i);
        assert(kv->key == range_reverse_keys[index]);
        ++index;
    }
    return 0;
}
```

</details>

<details>
<summary>array_tree_map.h (dropdown)</summary>
An ordered map with strict runtime bounds implemented in an array with indices tracking the tree structure. Offers handle stability. Handles remain valid until an element is removed from a table regardless of other insertions, other deletions, or resizing of the array.

```c
#include <assert.h>
#include <stdbool.h>
#define ARRAY_TREE_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#include "ccc/array_tree_map.h"
#include "ccc/traits.h"

struct Key_val
{
    int key;
    int val;
};
CCC_array_tree_map_declare_fixed_map(Key_val_fixed_map, struct Val, 64);

static CCC_Order
Key_val_cmp(CCC_Key_comparator_context const cmp)
{
    struct Key_val const *const right = cmp.type_right;
    int const key_left = *((int *)cmp.key_left);
    return (key_left > right->key) - (key_left < right->key);
}

int
main(void)
{
    /* stack array, user defined type, key field named key, no allocation
       permission, key comparison function, no context data. */
    Array_tree_map s = array_tree_map_initialize(
        &(Key_val_fixed_map){},
        struct Val,
        key,
        hrmap_key_cmp,
        NULL,
        NULL,
        array_tree_map_fixed_capacity(Key_val_fixed_map)
    );
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct Key_val){.key = id, .val = i}.elem);
    }
    /* This should be the following Range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int range_keys[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    Handle_range r = equal_range(&s, &(int){6}, &(int){44});
    int index = 0;
    for (Handle_index i = range_begin(&r); i != range_end(&r);
         i = next(&s, &i->elem))
    {
        struct Key_val const *const kv = array_tree_map_at(&s, i);
        assert(kv->key == range_keys[index]);
        ++index;
    }
    /* This should be the following Range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int range_reverse_keys[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    Handle_range_reverse rr = equal_range_reverse(&s, &(int){119}, &(int){84});
    index = 0;
    for (Handle_index i = range_reverse_begin(&rr); i != range_reverse_end(&rr);
         i = reverse_next(&s, &i->elem))
    {
        struct Key_val const *const kv = array_tree_map_at(&s, i);
        assert(kv->key == range_reverse_keys[index]);
        ++index;
    }
    return 0;
}
```

</details>

<details>
<summary>adaptive_map.h (dropdown)</summary>
A pointer stable ordered map that stores unique keys, implemented with a self-optimizing tree structure.

```c
#include <assert.h>
#include <string.h>
#define ADAPTIVE_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/adaptive_map.h"
#include "ccc/traits.h"

struct name
{
    Adaptive_map_node e;
    char const *name;
};

CCC_Order
Key_val_cmp(CCC_Key_comparator_context cmp)
{
    char const *const key = *(char **)cmp.key_left;
    struct name const *const right = cmp.type_right;
    int const res = strcmp(key, right->name);
    if (res == 0)
    {
        return CCC_EQL;
    }
    if (res < 0)
    {
        return CCC_LES;
    }
    return CCC_GRT;
}

int
main(void)
{
    struct name nodes[5];
    /* adaptive_map named om, stores struct name, intrusive field e, key field
       name, no allocation permission, comparison fn, no context */
    adaptive_map om = adaptive_map_initialize(om, struct name, e, name, Key_val_cmp, NULL, NULL);
    char const *const sorted_names[5]
        = {"Ferris", "Glenda", "Rocky", "Tux", "Ziggy"};
    size_t const size = sizeof(sorted_names) / sizeof(sorted_names[0]);
    size_t j = 7 % size;
    for (size_t i = 0; i < size; ++i, j = (j + 7) % size)
    {
        nodes[count(&om).count].name = sorted_names[j];
        CCC_Entry e = insert_or_assign(&om, &nodes[count(&om).count].e);
        assert(!insert_error(&e) && !occupied(&e));
    }
    j = 0;
    for (struct name const *n = begin(&om); n != end(&om); n = next(&om, &n->e))
    {
        assert(n->name == sorted_names[j]);
        assert(strcmp(n->name, sorted_names[j]) == 0);
        ++j;
    }
    assert(count(&om).count == size);
    CCC_Entry e = try_insert(&om, &(struct name){.name = "Ferris"}.e);
    assert(count(&om).count == size);
    assert(occupied(&e));
    return 0;
}
```

</details>

<details>
<summary>priority_queue.h (dropdown)</summary>
A pointer stable priority queue offering O(1) push and efficient decrease, increase, erase, and extract operations.

```c
#include <assert.h>
#include <stdbool.h>
#define PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#include "ccc/priority_queue.h"
#include "ccc/traits.h"

struct Val
{
    Priority_queue_node elem;
    int val;
};

static CCC_Order
val_cmp(CCC_Type_comparator_context const cmp)
{
    struct Val const *const left = cmp.type_left;
    struct Val const *const right = cmp.type_right;
    return (left->val > right->val) - (left->val < right->val);
}

int
main(void)
{
    struct Val elems[5]
        = {{.val = 3}, {.val = 3}, {.val = 7}, {.val = -1}, {.val = 5}};
    Priority_queue priority_queue = priority_queue_initialize(struct Val, elem, CCC_LES, val_cmp, NULL, NULL);
    for (size_t i = 0; i < (sizeof(elems) / sizeof(elems[0])); ++i)
    {
        struct Val const *const v = push(&priority_queue, &elems[i].elem);
        assert(v && v->val == elems[i].val);
    }
    bool const decreased = priority_queue_decrease_with(&priority_queue, &elems[4].elem,
                                         { elems[4].val = -99; });
    assert(decreased);
    struct Val const *const v = front(&priority_queue);
    assert(v->val == -99);
    return 0;
}
```

</details>

<details>
<summary>tree_map.h (dropdown)</summary>
A pointer stable ordered map meeting strict O(lg N) runtime bounds for realtime applications.

```c
#include <assert.h>
#define TREE_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#include "ccc/tree_map.h"
#include "ccc/traits.h"

struct Key_val
{
    Tree_map_node elem;
    int key;
    int val;
};

static CCC_Order
Key_val_cmp(CCC_Key_comparator_context const cmp)
{
    struct Key_val const *const right = cmp.type_right;
    int const key_left = *((int *)cmp.key_left);
    return (key_left > right->key) - (key_left < right->key);
}

int
main(void)
{
    struct Key_val elems[25];
    /* stack array of 25 elements with one slot for sentinel, intrusive field
       named elem, key field named key, no allocation permission, key comparison
       function, no context data. */
    Tree_map s
        = tree_map_initialize(s, struct Key_val, elem, key, Key_val_cmp, NULL, NULL);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        elems[i].key = id;
        elems[i].val = i;
        (void)insert_or_assign(&s, &elems[i].elem);
    }
    /* This should be the following Range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int range_keys[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    Range r = equal_range(&s, &(int){6}, &(int){44});
    int index = 0;
    for (struct Key_val *i = range_begin(&r); i != range_end(&r);
         i = next(&s, &i->elem))
    {
        assert(i->key == range_keys[index]);
        ++index;
    }
    /* This should be the following Range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int range_reverse_keys[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    Range_reverse rr = equal_range_reverse(&s, &(int){119}, &(int){84});
    index = 0;
    for (struct Key_val *i = range_reverse_begin(&rr); i != range_reverse_end(&rr);
         i = reverse_next(&s, &i->elem))
    {
        assert(i->key == range_reverse_keys[index]);
        ++index;
    }
    return 0;
}
```

</details>

<details>
<summary>singly_linked_list.h (dropdown)</summary>
A low overhead push-to-front container. When contiguity is not possible and the access pattern resembles a stack this is more-efficient than a doubly-linked list.

```c
#include <assert.h>
#define SINGLY_LINKED_LIST_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/singly_linked_list.h"
#include "ccc/traits.h"

struct Int_node
{
    int i;
    Singly_linked_list_node e;
};

static CCC_Order
int_cmp(CCC_Type_comparator_context const cmp)
{
    struct Int_node const *const left = cmp.type_left;
    struct Int_node const *const right = cmp.type_right;
    return (left->i > right->i) - (left->i < right->i);
}

int
main(void)
{
    /* singly linked list l, list elem field e, no allocation permission,
       comparing integers, no context data. */
    Singly_linked_list l = singly_linked_list_initialize(l, struct Int_node, e, int_cmp, NULL, NULL);
    struct Int_node elems[3] = {{.i = 3}, {.i = 2}, {.i = 1}};
    (void)push_front(&l, &elems[0].e);
    (void)push_front(&l, &elems[1].e);
    (void)push_front(&l, &elems[2].e);
    struct Int_node const *i = front(&l);
    assert(i->i == 1);
    pop_front(&l);
    i = front(&l);
    assert(i->i == 2);
    return 0;
}
```

</details>

## Features

- [Intrusive and non-intrusive containers](#intrusive-and-non-intrusive-containers).
- [Non-allocating container options](#non-allocating-containers).
- [Compile time initialization](#compile-time-initialization).
- [No `container_of` macro required of the user to get to their type after a function call](#no-container-of-macros).
- [Rust's Entry API for associative containers with C and C++ influences](#rusts-entry-interface).
    - Opt-in macros for more succinct insertion and in place modifications (see "closures" in the [and_modify_wth](https://agl-alexglopez.github.io/ccc/flat__hash__map_8h.html) interface for associative containers).
- [Container Traits implemented with C `_Generic` capabilities](#traits).

### Intrusive and Non-Intrusive Containers

Currently, many associative containers ask the user to store an element in their type. This means wrapping an element in a struct such as this type found in `samples/graph.c` for the priority queue.

```c
struct Cost
{
    CCC_Priority_queue_node node;
    int cost;
    char name;
    char from;
};
```

The interface may then ask for a handle to this type for certain operations. For example, a priority queue has the following interface for pushing an element.

```c
void *CCC_priority_queue_push(CCC_Priority_queue *priority_queue, CCC_Priority_queue_node *elem);
```

Non-Intrusive containers exist when a flat container can operate without such help from the user. The `Flat_priority_queue` is a good example of this. When initializing we give it the following information.

```c
#define CCC_flat_priority_queue_initialize(data_pointer, cmp_order, cmp_fn, allocate, context_data, capacity) \
    CCC_impl_flat_priority_queue_initialize(data_pointer, cmp_order, cmp_fn, allocate, context_data, capacity)

/* For example: */

CCC_Flat_priority_queue flat_priority_queue
    = CCC_flat_priority_queue_initialize((int[40]){}, int, CCC_LESSER, int_cmp, NULL, NULL, 40);

```

Here a small min priority queue of integers with a maximum capacity of 40 has been allocated on the stack with no allocation permission and no context data needed. As long as the flat priority queue knows the type upon initialization no intrusive elements are needed. We could have also initialized this container as empty if we provide an allocation function (see [allocation](#allocation) for more on allocation permission).

```c
CCC_Flat_priority_queue flat_priority_queue
    = CCC_flat_priority_queue_initialize(NULL, int, CCC_LESSER, int_cmp, std_allocate, NULL, 0);
```

The interface then looks like this.

```c
void *CCC_flat_priority_queue_push(CCC_Flat_priority_queue *flat_priority_queue, void const *e, void *temp);
```

The element `e` here is just a generic reference to whatever type the user stores in the container and `temp` is a swap slot provided by the user.

### Non-Allocating Containers

As was mentioned in the previous section, all containers can be forbidden from allocating memory. In the flat priority queue example we had this initialization.

```c
CCC_Flat_priority_queue flat_priority_queue
    = CCC_flat_priority_queue_initialize((int[40]){}, int, CCC_LES, int_cmp, NULL, NULL, 40);
```

For flat containers, fixed capacity is straightforward. Once space runs out, further insertion functions will fail and report that failure in different ways depending on the function used. If other behavior occurs when space runs out, such as ring Buffer behavior for the flat double ended queue, it will be documented in the header of that container.

For non-flat containers that can't assume they are stored contiguously in memory, the initialization looks like this when allocation is prohibited.

```c
struct Id_val
{
    CCC_Doubly_linked_list_node e;
    int id;
    int val;
};

CCC_Doubly_linked_list dll
    = CCC_doubly_linked_list_initialize(dll, struct Id_val, e, val_cmp, NULL, NULL);
```

All interface functions now expect the memory containing the intrusive elements to exist with the appropriate scope and lifetime for the programmer's needs. Consider the following classic problem with scoping in C.

```c
/* !WARNING: THIS IS A BAD IDEA FOR DEMONSTRATION PURPOSES! */
void push_three(CCC_Doubly_linked_list *const dll)
{
    struct Id_val v0 = {};
    struct Id_val *v = push_back(dll, &v0.e);
    assert(v == &v0);
    struct Id_val v1 = {.id = 1, .val = 1};
    v = push_back(dll, &v1.e);
    assert(v == &v1);
    struct Id_val v2 = {.id = 2, .val = 2};
    v = push_back(dll, &v2.e);
    assert(v == &v2);
    /* WHOOPS! FUNCTION IS ABOUT TO RETURN! */
}
```

Here, the container pushes stack allocated structs directly into the list. The container has not been given allocation permission so it assumes the memory it is given has the appropriate lifetime for the programmer's needs. When this function ends, that memory is invalid because its scope and lifetime has ended. Using `malloc` in this case would be the traditional approach, but there are a variety of ways a programmer can control scope and lifetime. This library does not prescribe any specific strategy to managing memory when allocation is prohibited. For example compositions of allocating and non-allocating containers, see the `samples/`.

### Compile Time Initialization

Because the user may choose the source of memory for a container, initialization at compile time is possible for all containers.

A flat hash map may be initialized at compile time if the maximum size is fixed and no allocation permission is needed.

```c
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
flat_hash_map_declare_fixed_map(Val_fixed_map, struct Val, 64);
static flat_hash_map static_fh = flat_hash_map_initialize(
    &(static Val_fixed_map){},
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_id_cmp,
    NULL,
    NULL,
    flat_hash_map_fixed_capacity(val_fixed_map)
);
```

A flat hash map can also be initialized in preparation for dynamic allocation at compile time if an allocation function is provided (see [allocation](#allocation) for more on `std_alloc`).

```c
#define FLAT_HASH_MAP_USING_NAMESPACE_CCC
struct Val
{
    int key;
    int val;
};
static flat_hash_map static_fh = flat_hash_map_initialize(
    NULL,
    struct Val,
    key,
    flat_hash_map_int_to_u64,
    flat_hash_map_id_cmp,
    std_allocate,
    NULL,
    0
);
```

All other containers provide default initialization macros that can be used at compile time or runtime. For example, initializing a ring Buffer at compile time is simple.

```c
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
static Flat_doubled_ended_queue ring_buffer
    = flat_doubled_ended_queue_initialize((static int[4096]){}, int, NULL, NULL, 4096);
```

In all the preceding examples initializing at compile time simplifies the code, eliminates the need for initialization functions, and ensures that all containers are ready to operate when execution begins. Using compound literal initialization also helps create better ownership of memory for each container, eliminating named references to a container's memory that could be accessed by mistake.


### No Container of Macros

Traditionally, intrusive containers provide the following macro.

```c
/** list_entry - get the struct for this entry
@pointer the &struct list_node pointer.
@type the type of the struct this is embedded in.
@member	the name of the list_node within the struct. */
#define list_entry(pointer, type, member) \
	container_of(pointer, type, member)

/* A provided function by the container. */
struct List_node *list_front(list *l);
```

Then, the user code looks like this.

```c
struct Id
{
    int id;
    struct List_node id_node;
};
/* Or when writing a comparison callback. */
bool
is_id_a_less(struct List_node const *const a,
             struct List_node const *const b, void *const context)
{
    struct Id const *const a_ = list_entry(a, struct Id, id_node);
    struct Id const *const b_ = list_entry(b, struct Id, id_node);
    return a_->id < b_->id;
}

int
main(void)
{
    static struct List id_list = list_initialize(id_list);
    /* ...fill list... */
    struct Id *front = list_entry(list_front(&id_list), struct Id, id_node);
}
```

Here, it may seem manageable to use such a macro, but it is required at every location in the code where the user type is needed. The opportunity for bugs in entering the type or field name grows the more the macro is used. It is better to take care of this step for the user and present a cleaner interface.

Here is the same list example in the C Container Collection.

```c
struct Id
{
    int id;
    CCC_Doubly_linked_list_node id_node;
};
/* Or when writing a comparison callback. */
CCC_Order
id_cmp(CCC_Type_comparator_context const cmp)
{
    struct Id const *const left = cmp.type_left;
    struct Id const *const right = cmp.type_right;
    return (left->id > right->id) - (left->id < right->id);
}

int
main (void)
{
    static CCC_Doubly_linked_list id_list
        = CCC_doubly_linked_list_initialize(id_list, struct Id, id_node, id_cmp, NULL, NULL);
    /* ...fill list... */
    struct Id *front = CCC_doubly_linked_list_front(&id_list);
    struct Id *new_id = generate_id();
    struct Id *new_front = CCC_doubly_linked_list_push_front(&id_list, &new_id->id_node);
}
```

Internally the containers will remember the offsets of the provided elements within the user struct wrapping the intruder. Then, the contract of the interface is simpler: provide a handle to the container and receive your type in return. The user takes on less complexity overall by providing a slightly more detailed initialization. If the container does not require intrusive elements than this is not a concern to begin with. However, this ensures consistency across return values to access user types for intrusive and non-intrusive containers: a reference to the user type is always returned.

### Rust's Entry Interface

Rust has solid interfaces for associative containers, largely due to the Entry Interface. In the C Container Collection the core of all associative containers is inspired by the Entry Interface (these versions are found in `ccc/traits.h` but specific names, behaviors, and parameters can be read in each container's header).

- `CCC_Entry(container_pointer, key_pointer...)` - Obtains an entry, a view into an Occupied or Vacant user type stored in the container.
- `CCC_and_modify(entry_pointer, mod_fn)` - Modify an occupied entry with a callback.
- `CCC_and_modify_context(entry_pointer, mod_fn, context_args)` - Modify an Occupied entry with a callback that requires context data.
- `CCC_or_insert(entry_pointer, or_insert_args)` - Insert a default key value if Vacant or return the Occupied entry.
- `CCC_insert_entry(entry_pointer, insert_entry_args)` - Invariantly insert a new key value, overwriting an Occupied entry if needed.
- `CCC_remove_entry(entry_pointer)` - Remove an Occupied entry from the container or do nothing.

Other Rust Interface functions like `get_key_value`, `insert`, and `remove` are included and can provide information about previous values stored in the container.

Each container offers it's own C version of "closures" for the `and_modify_with`. Here is an example from the `samples/words.c` program.

- `and_modify_with(array_adaptive_map_entry_pointer, type_name, closure_over_T...)` - Run code in `closure_over_T` on the stored user type `T`.

```c
typedef struct
{
    String_offset ofs;
    int cnt;
} Word;
/* Increment a found Word or insert a default count of 1. */
CCC_Handle_index const h =
array_adaptive_map_or_insert_with(
    array_adaptive_map_and_modify_with(handle_wrap(&hom, &key_ofs), Word, { T->cnt++; }),
    (Word){.ofs = ofs, .cnt = 1}
);
```

This is possible because of the details discussed in the previous section. Containers can always provide the user type stored in the container directly. However, there are other options to achieve the same result.

Some C++ associative container interfaces have also been adapted to the Entry Interface.

- `CCC_try_insert(container_pointer, try_insert_args)` - Inserts a new element if none was present and reports if a previous entry existed.
- `CCC_insert_or_assign(container_pointer, insert_or_assign_args)` - Inserts a new element invariantly and reports if a previous entry existed.

Many other containers fall back to C++ style interfaces when it makes sense to do so.

#### Lazy Evaluation

Many of the above functions come with an optional macro variant. For example, the `or_insert` function for associative containers will come with an `or_insert_w` variant, short for or insert "with." The word "with" in this context means a direct r-value.

Here is an example for generating a maze with Prim's algorithm in the `samples/maze.c` sample.

The functional version.

```c
struct Point const next = {
    .r = c->cell.r + dir_offsets[i].r,
    .c = c->cell.c + dir_offsets[i].c,
};
struct Prim_cell new = (struct Prim_cell){
    .cell = next,
    .cost = rand_range(0, 100),
};
struct Prim_cell *const cell = or_insert(entry_wrap(&cost_map, &next), &new);
```

The lazily evaluated macro version.

```c
struct Point const next = {
    .r = c->cell.r + dir_offsets[i].r,
    .c = c->cell.c + dir_offsets[i].c,
};
struct Prim_cell const *const cell = flat_hash_map_or_insert_with(
    entry_wrap(&cost_map, &next),
    (struct Prim_cell){
        .cell = next,
        .cost = rand_range(0, 100),
    }
);
```

The second example is slightly more convenient and efficient. The compound literal is provided to be directly assigned to a Vacant memory location allowing the compiler to decide how the copy should be performed; it is only constructed if there is no entry present. This also means the random generation function is only called if a Vacant entry requires the insertion of a new value. So, expensive function calls can be lazily evaluated only when needed.

Here is another example illustrating the difference between the two.

```c
struct Val
{
    Adaptive_map_node e;
    int key;
    int val;
};

CCC_Entry e = adaptive_map_try_insert(&om, &(struct Val){.key = 3, .val = 1}.e);
```

The same insertion with the "with" variant.

```c
static inline struct Val
val(int val_arg)
{
    return (struct Val){.val = val_args};
}

CCC_Entry *e = adaptive_map_try_insert_with(&om, 3, val(1));
```

This second version illustrates a few key points. R-values are provided directly as keys and values, not references to keys and values. Also, a function call to generate a value to be inserted is completely acceptable; the function is only called if insertion is required. Finally, the functions `try_insert_w` and `insert_or_assign_w` will ensure the key in the newly inserted value matches the key searched, saving the user some typing and ensuring they don't make a mistake in this regard.

The lazy evaluation of the `_w` family of functions offer an expressive way to write C code when needed. See each container's header for more.

### Traits

Traits, found in `ccc/traits.h`, offer a more succinct way to use shared functionality across containers. Instead of calling `CCC_flat_hash_map_entry` when trying to obtain an entry from a flat hash map, one can simply call `entry`. Traits utilize `_Generic` in C to choose the correct container function based on parameters provided.

Traits cost nothing at runtime but may slightly increase compilation memory use.

```c
#define TRAITS_USING_NAMESPACE_CCC
typedef struct
{
    str_ofs ofs;
    int cnt;
} Word;
/* ... Elsewhere generate offset ofs as key. */
Word default = {.ofs = ofs, .cnt = 1};
CCC_Handle_index const h =
    or_insert(and_modify(handle_wrap(&hom, &ofs), increment), &default);
```

Or the following.

```c
#define TRAITS_USING_NAMESPACE_CCC
typedef struct
{
    str_ofs ofs;
    int cnt;
} Word;
/* ... Elsewhere generate offset ofs as key. */
Word default = {.ofs = ofs, .cnt = 1};
Array_adaptive_map_handle *h = handle_wrap(&hom, &ofs);
h = and_modify(h, increment)
Word *w = array_adaptive_map_at(&hom, or_insert(h, &default));
```

Traits are completely opt-in by including the `traits.h` header.

## Allocation

When allocation is required, this collection offers the following interface. The user provides this function to containers upon initialization.

```c
typedef struct
{
    void *input;
    size_t bytes;
    void *context;
} CCC_Allocator_context;
typedef void *CCC_Allocator(CCC_Allocator_context);
```

An allocation function implements the following behavior, where `input` is pointer to memory, `bytes` is number of bytes to allocate, and `context` is a reference to any supplementary information required for allocation, deallocation, or reallocation. The `context` parameter is passed to a container upon its initialization and the programmer may choose how to best utilize this reference (read on for more on `context`).

- If NULL is provided with a `bytes` of 0, NULL is returned.
- If NULL is provided with a non-zero `bytes`, new memory is allocated/returned.
- If `input` is non-NULL it has been previously allocated by the allocate function.
- If `input` is non-NULL with non-zero `bytes`, `input` is resized to at least `bytes`
  bytes. The pointer returned is NULL if resizing fails. Upon success, the
  pointer returned might not be equal to the pointer provided.
- If `input` is non-NULL and `bytes` is 0, `input` is freed and NULL is returned.

One may be tempted to use realloc to check all of these boxes but realloc is implementation defined on some of these points. The `context` parameter also discourages users from providing realloc. For example, one solution using the standard library allocator might be implemented as follows (`context` is not needed):

```c
void *
std_allocator(CCC_Allocator_context const context)
{
    if (!context.input && !context.bytes)
    {
        return NULL;
    }
    if (!context.input)
    {
        return malloc(context.bytes);
    }
    if (!context.bytes)
    {
        free(context.input);
        return NULL;
    }
    return realloc(context.input, context.bytes);
}
```

However, the above example is only useful if the standard library allocator is used. Any allocator that implements the required behavior is sufficient. For ideas of how to utilize the `context` parameter, see the sample programs. Using custom arena allocators or container compositions are cases when `context` is helpful in taming lifetimes and simplifying allocation. See the tests for extensive use of a minimal stack allocator that helps speed up tests by avoiding calls into the standard library heap allocator.

### Constructors

Another concern for the programmer related to allocation may be constructors and destructors, a C++ shaped peg for a C shaped hole. In general, this library has some limited support for destruction but does not provide an interface for direct constructors as C++ would define them; though this may change.

Consider a constructor. If the container is allowed to allocate, and the user wants to insert a new element, they may see an interface like this (pseudocode as all containers are slightly different).

```c
void *insert_or_assign(Container *c, Container_node *e);
```

Because the user has wrapped the intrusive container element in their type, the entire user type will be written to the new allocation. All interfaces can also confirm when insertion succeeds if global state needs to be set in this case. So, if some action beyond setting values needs to be performed, there are multiple opportunities to do so.

### Destructors

For destructors, the argument is similar but the container does offer more help. If an action other than freeing the memory of a user type is needed upon removal, there are options in an interface to obtain the element to be removed. Associative containers offer functions that can obtain entries (similar to Rust's Entry API). This reference can then be examined and complex destructor actions can occur before removal. Other containers like lists or priority queues offer references to an element of interest such as front, back, max, min, etc. These can all allow destructor-like actions before removal. One exception is the following interfaces.

The clear function works for pointer stable containers and flat containers.

```c
Result clear(Container *c, Destructor *fn);
```

The clear and free function works for flat containers.

```c
Result clear_and_free(Container *c, Destructor *fn);
```

The above functions free the resources of the container. Because there is no way to access each element before it is freed when this function is called, a destructor callback can be passed to operate on each element before deallocation.

## Samples

For examples of what code that uses these ideas looks like, read and use the sample programs in the `samples/`. I try to only add non-trivial samples that do something mildly interesting to give a good idea of how to take advantage of this flexible memory philosophy.

The samples are not included in the release. To build them, clone the repository. Usage instructions should be available with the `-h` flag to any program or at the top of the file.

Clang.

```zsh
make all-clang-rel
./build/bin/[SAMPLE] [SAMPLE CLI ARGS]
```

GCC.

```zsh
make all-gcc-rel
./build/bin/[SAMPLE] [SAMPLE CLI ARGS]
```

## Tests

The tests also include various use cases that may be of interest. Tests are not included in the release. Clone the repository.

Clang.

```zsh
make all-clang-rel
make rtest
```

GCC.

```zsh
make all-gcc-rel
make rtest
```

## Miscellaneous Why?

- Why are non-allocating containers needed? They are quite common in Operating Systems development. Kernel code may manage a process or thread that is part of many OS subsystems: the CPU scheduler, the virtual memory paging system, the child and parent process spawn and wait mechanisms. All of these systems require that the process use or be a part of some data structures. It is easiest to separate participation in these data structures from memory allocation. The process holds handles to intrusive elements to participate in these data structures because the thread/task/process will live until it has finished executing meaning its lifetime is greater than or equal to the longest lifetime data structure of which it is a part. Embedded developers also often seem interested in non-allocating containers when an entire program's memory use is known before execution begins. However, this library explores if non-allocating containers can have more general applications for creative C programming.
- Why is initialization so ugly? Yes, this is a valid concern. Upfront complexity helps eliminate the need for a `container_of` macro for the user see the [no container_of macros](#no-container-of-macros) section for more. Later code becomes much cleaner and simpler.
- Why callbacks? Freedom for more varied comparisons and allocations. Learn to love context data. Also you have the chance to craft the optimal function for your application; for example writing a perfectly tailored hash function for your data set.
- Why not header only? Readability, maintainability, and update ability, for changing implementations in the source files. If the user wants to explore the implementation everything should be easily understandable. This can also be helpful if the user wants to fork and change a data structure to fit their needs. Smaller object size and easier modular compilation is also nice.
- Why not opaque pointers and true implementation hiding? This is not possible in C if the user is in charge of memory. The container types must be complete if the user wishes to store them on the stack or data segment. I try to present a clean interface.
- Why `Array_` and `Flat_` maps? Mostly experimenting. A flat hash map offers the ability to pack user data and fingerprint meta data in separate contiguous arrays within the same allocation. The handle variants of containers offer the same benefit along with handle stability. Many also follow the example of Zig with its Struct of Arrays approach to many data structures. Struct of Array (SOA) style data structures focus on space optimizations due to perfect alignment of user types, container metadata, and any supplementary data in separate arrays but within the same allocation.
- Why `C23`? It is a great standard that helps with some initialization and macro ideas implemented in the library. Clang covers all of the features used on many platforms. Newer GCC versions also have them covered.

## Related

If these containers do not fit your needs, here are some excellent data structure libraries I have found for C. They are clever, fast, and elegant. They lean into creating a C++ template-like system for C that offers C's version of type safety. These are good if you are transitioning from higher level languages like C++ and Rust and want a similar data structure experience.

- [STC - Smart Template Containers](https://github.com/stclib/STC)
- [C Template Library (CTL)](https://github.com/glouw/ctl)
- [CC: Convenient Containers](https://github.com/JacksonAllan/CC)

## Citations

A few containers are based off of important work by other data structure developers and researchers. Here are the data structures that inspired some containers in this collection. Full citations and notices of changes can be read in the respective implementations in `source/` directory.

- Rust's hashbrown hash table was the basis for the `flat_hash_map.h` container.
    - https://github.com/rust-lang/hashbrown
- Phil Vachon's implementation of a WAVL tree was the inspiration for the `tree_map.h` containers.
    - https://github.com/pvachon/wavl_tree
- Research by Daniel Sleator in implementations of Splay Trees helped shape the `adaptive_map.h` containers.
    - https://www.link.cs.cmu.edu/splay/
