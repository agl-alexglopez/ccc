# C Container Collection (CCC)

The C Container Collection offers a variety of containers for C programmers who want fine-grained control of memory in their programs. All containers offer both allocating and non-allocating interfaces. For the motivations of why such a library is helpful in C read on.

## Installation

The following are required for install:

- GCC or Clang supporting C23.
    - 100% coverage of C23 is not required. For example, at the time of writing Clang 19.1.1 and GCC 14.2 have all features used in this collection covered, but older versions of each compiler may work as well.
- CMake >= 3.23.

Currently, this library supports a manual installation via CMake. See the [INSTALL.md](INSTALL.md) file for more details. This file is also included when you download a simplified release from the [Releases](https://github.com/agl-alexglopez/ccc/releases) page.

## Quick Start

- Read the [DOCS](https://agl-alexglopez.github.io/ccc).
- Read [types.h](https://agl-alexglopez.github.io/ccc/types_8h.html) to understand the `ccc_alloc_fn` interface.
- Read the [header](https://agl-alexglopez.github.io/ccc/files.html) for the desired container to understand its functionality.
- Read about generic [traits.h](https://agl-alexglopez.github.io/ccc/traits_8h.html) shared across containers to make code more succinct.
- Read [CONTRIBUTING.md](CONTRIBUTING.md) if interested in project structure, tools, and todos.

## Containers

<details>
<summary>buffer.h (dropdown)</summary>
A fixed or dynamic contiguous array of a single user defined type.

```c
#include <assert.h>
#define BUFFER_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/buffer.h"
#include "ccc/traits.h"

int
main(void)
{
    /* stack array, no allocation permission, no aux data, capacity 5 */
    buffer b = buf_init((int[5]){}, NULL, NULL, 5);
    (void)push_back(&b, &(int){3});
    (void)push_back(&b, &(int){2});
    (void)push_back(&b, &(int){1});
    (void)pop_back(&b);
    int *i = back(&b);
    assert(*i == 2);
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

struct int_elem
{
    int i;
    dll_elem e;
};

static ccc_threeway_cmp
int_cmp(ccc_cmp const cmp)
{
    struct int_elem const *const lhs = cmp.user_type_lhs;
    struct int_elem const *const rhs = cmp.user_type_rhs;
    return (lhs->i > rhs->i) - (lhs->i < rhs->i);
}

int
main(void)
{
    /* doubly linked list l, list elem field e, no allocation permission,
       comparing integers, no auxiliary data. */
    doubly_linked_list l = dll_init(l, struct int_elem, e, NULL, int_cmp, NULL);
    struct int_elem elems[3] = {{.i = 3}, {.i = 2}, {.i = 1}};
    (void)push_back(&l, &elems[0].e);
    (void)push_front(&l, &elems[1].e);
    (void)push_back(&l, &elems[2].e);
    (void)pop_back(&l);
    struct int_elem *e = back(&l);
    assert(e->i == 3);
    return 0;
}
```

</details>

<details>
<summary>flat_double_ended_queue.h (dropdown)</summary>
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
    /* stack array, no allocation permission, no aux data, capacity 2 */
    flat_double_ended_queue q = fdeq_init((int[2]){}, NULL, NULL, 2);
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

struct key_val
{
    fhmap_elem e;
    int key;
    int val;
};

static uint64_t
fhmap_int_to_u64(ccc_user_key const k)
{
    int const key_int = *((int *)k.user_key);
    uint64_t x = key_int;
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

bool
fhmap_id_eq(ccc_key_cmp const cmp)
{
    struct key_val const *const va = cmp.user_type_rhs;
    return va->key == *((int *)cmp.key_lhs);
}

/* Two Sum */
int
main(void)
{
    struct key_val vals[20];
    /* stack array backed, key field named key, intrusive field e, no
       allocation permission, a hash function, an equality function, no aux. */
    ccc_flat_hash_map fh;
    ccc_result const res
        = fhm_init(&fh, vals, sizeof(vals) / sizeof(vals[0]), key, e, NULL,
                   fhmap_int_to_u64, fhmap_id_eq, NULL);
    assert(res == CCC_OK);
    int const addends[10] = {1, 3, -980, 6, 7, 13, 44, 32, 995, -1};
    int const target = 15;
    int solution_indices[2] = {-1, -1};
    for (size_t i = 0; i < (sizeof(addends) / sizeof(addends[0])); ++i)
    {
        /* Functions take keys and structs by reference. */
        struct key_val const *const other_addend
            = get_key_val(&fh, &(int){target - addends[i]});
        if (other_addend)
        {
            solution_indices[0] = (int)i;
            solution_indices[1] = other_addend->val;
            break;
        }
        /* Macros take keys and structs by value. */
        (void)fhm_insert_or_assign_w(&fh, addends[i],
                                     (struct key_val){.val = i});
    }
    assert(solution_indices[0] == 8);
    assert(solution_indices[1] == 2);
    assert(addends[solution_indices[0]] + addends[solution_indices[1]]
           == target);
    return 0;
}
```

</details>

<details>
<summary>flat_ordered_map.h (dropdown)</summary>
An ordered map implemented in array with an index based self-optimizing tree.

```c
#include <assert.h>
#include <stdbool.h>
#define FLAT_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#include "ccc/flat_ordered_map.h"
#include "ccc/traits.h"

struct kval
{
    fomap_elem elem;
    int key;
    int val;
};

static ccc_threeway_cmp
kval_cmp(ccc_key_cmp const cmp)
{
    struct kval const *const rhs = cmp.user_type_rhs;
    int const key_lhs = *((int *)cmp.key_lhs);
    return (key_lhs > rhs->key) - (key_lhs < rhs->key);
}

int
main(void)
{
    /* stack array of 25 elements with one slot for sentinel, intrusive field
       named elem, key field named key, no allocation permission, key comparison
       function, no aux data. */
    flat_ordered_map s
        = fom_init((struct kval[26]){}, 26, elem, key, NULL, kval_cmp, NULL);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct kval){.key = id, .val = i}.elem);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int range_keys[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    range r = equal_range(&s, &(int){6}, &(int){44});
    int index = 0;
    for (struct kval *i = begin_range(&r); i != end_range(&r);
         i = next(&s, &i->elem))
    {
        assert(i->key == range_keys[index]);
        ++index;
    }
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int rrange_keys[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    rrange rr = equal_rrange(&s, &(int){119}, &(int){84});
    index = 0;
    for (struct kval *i = rbegin_rrange(&rr); i != rend_rrange(&rr);
         i = rnext(&s, &i->elem))
    {
        assert(i->key == rrange_keys[index]);
        ++index;
    }
    return 0;
}
```

</details>

<details>
<summary>flat_priority_queue.h (dropdown)</summary>

```c
#include <assert.h>
#define FLAT_PRIORITY_QUEUE_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/flat_priority_queue.h"
#include "ccc/traits.h"

ccc_threeway_cmp
int_cmp(ccc_cmp const ints)
{
    int const lhs = *(int *)ints.user_type_lhs;
    int const rhs = *(int *)ints.user_type_rhs;
    return (lhs > rhs) - (lhs < rhs);
}

/* In place O(n) time O(1) space partial sort. */
int
main(void)
{
    int heap[20] = {12, 61, -39, 76, 48, -93, -77, -81, 35, 21,
                    -3, 90, 20,  27, 97, -22, -20, -19, 70, 76};
    /* Heapify existing array of values, with capacity, size one less than
       capacity for swap space, min priority queue, no allocation, no aux. */
    flat_priority_queue pq = fpq_heapify_init(
        heap, (sizeof(heap) / sizeof(int)), 19, CCC_LES, NULL, int_cmp, NULL);
    (void)fpq_update_w(&pq, &heap[5], { heap[5] -= 4; });
    int prev = *((int *)front(&pq));
    (void)pop(&pq);
    while (!is_empty(&pq))
    {
        int cur = *((int *)front(&pq));
        (void)pop(&pq);
        assert(cur >= prev);
        prev = cur;
    }
    return 0;
}
```

</details>

<details>
<summary>flat_realtime_ordered_map.h (dropdown)</summary>
An ordered map with strict runtime bounds implemented in an array with indices tracking the tree structure.

```c
#include <assert.h>
#include <stdbool.h>
#define FLAT_REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#include "ccc/flat_realtime_ordered_map.h"
#include "ccc/traits.h"

struct kval
{
    fromap_elem elem;
    int key;
    int val;
};

static ccc_threeway_cmp
kval_cmp(ccc_key_cmp const cmp)
{
    struct kval const *const rhs = cmp.user_type_rhs;
    int const key_lhs = *((int *)cmp.key_lhs);
    return (key_lhs > rhs->key) - (key_lhs < rhs->key);
}

int
main(void)
{
    /* stack array of 25 elements with one slot for sentinel, intrusive field
       named elem, key field named key, no allocation permission, key comparison
       function, no aux data. */
    flat_realtime_ordered_map s
        = frm_init((struct kval[26]){}, 26, elem, key, NULL, kval_cmp, NULL);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        (void)insert_or_assign(&s, &(struct kval){.key = id, .val = i}.elem);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int range_keys[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    range r = equal_range(&s, &(int){6}, &(int){44});
    int index = 0;
    for (struct kval *i = begin_range(&r); i != end_range(&r);
         i = next(&s, &i->elem))
    {
        assert(i->key == range_keys[index]);
        ++index;
    }
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int rrange_keys[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    rrange rr = equal_rrange(&s, &(int){119}, &(int){84});
    index = 0;
    for (struct kval *i = rbegin_rrange(&rr); i != rend_rrange(&rr);
         i = rnext(&s, &i->elem))
    {
        assert(i->key == rrange_keys[index]);
        ++index;
    }
    return 0;
}
```

</details>

<details>
<summary>ordered_map.h (dropdown)</summary>
A pointer stable ordered map that stores unique keys, implemented with a self-optimizing tree structure.

```c
#include <assert.h>
#include <string.h>
#define ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/ordered_map.h"
#include "ccc/traits.h"

struct name
{
    omap_elem e;
    char const *name;
};

ccc_threeway_cmp
kval_cmp(ccc_key_cmp cmp)
{
    char const *const key = *(char **)cmp.key_lhs;
    struct name const *const rhs = cmp.user_type_rhs;
    int const res = strcmp(key, rhs->name);
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
    /* ordered_map named om, stores struct name, intrusive field e, key field
       name, no allocation permission, comparison fn, no aux */
    ordered_map om = om_init(om, struct name, e, name, NULL, kval_cmp, NULL);
    char const *const sorted_names[5]
        = {"Ferris", "Glenda", "Rocky", "Tux", "Ziggy"};
    size_t const size = sizeof(sorted_names) / sizeof(sorted_names[0]);
    size_t j = 7 % size;
    for (size_t i = 0; i < size; ++i, j = (j + 7) % size)
    {
        nodes[size(&om)].name = sorted_names[j];
        ccc_entry e = insert_or_assign(&om, &nodes[size(&om)].e);
        assert(!insert_error(&e) && !occupied(&e));
    }
    j = 0;
    for (struct name const *n = begin(&om); n != end(&om); n = next(&om, &n->e))
    {
        assert(n->name == sorted_names[j]);
        assert(strcmp(n->name, sorted_names[j]) == 0);
        ++j;
    }
    assert(size(&om) == size);
    ccc_entry e = try_insert(&om, &(struct name){.name = "Ferris"}.e);
    assert(size(&om) == size);
    assert(occupied(&e));
    return 0;
}
```

</details>

<details>
<summary>ordered_multimap.h (dropdown)</summary>
A pointer stable ordered map allowing storage of duplicate keys; searches and removals of duplicates will yield the oldest duplicate. An ordered multimap uses a self optimizing tree structures and is suitable for a priority queue if round robin fairness among duplicates is needed.

```c
#include <assert.h>
#include <string.h>
#define ORDERED_MULTIMAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#include "ccc/ordered_multimap.h"
#include "ccc/traits.h"

struct name
{
    ommap_elem e;
    char const *name;
};

ccc_threeway_cmp
kval_cmp(ccc_key_cmp cmp)
{
    char const *const key = *(char **)cmp.key_lhs;
    struct name const *const rhs = cmp.user_type_rhs;
    int const res = strcmp(key, rhs->name);
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
    struct name nodes[10];
    /* ordered_map named om, stores struct name, intrusive field e, key field
       name, no allocation permission, comparison fn, no aux */
    ordered_multimap om
        = omm_init(om, struct name, e, name, NULL, kval_cmp, NULL);
    char const *const sorted_repeat_names[10]
        = {"Ferris", "Ferris", "Glenda", "Glenda", "Rocky",
           "Rocky",  "Tux",    "Tux",    "Ziggy",  "Ziggy"};
    size_t const size
        = sizeof(sorted_repeat_names) / sizeof(sorted_repeat_names[0]);
    size_t j = 11 % size;
    for (size_t i = 0; i < size; ++i, j = (j + 11) % size)
    {
        nodes[size(&om)].name = sorted_repeat_names[j];
        ccc_entry e = insert(&om, &nodes[size(&om)].e);
        assert(!insert_error(&e));
    };
    j = 1;
    for (struct name *prev = rbegin(&om), *n = rnext(&om, &prev->e);
         n != rend(&om); n = rnext(&om, &n->e), n = rnext(&om, &n->e),
                     prev = rnext(&om, &prev->e), prev = rnext(&om, &prev->e))
    {
        assert(strcmp(n->name, sorted_repeat_names[j]) == 0);
        assert(strcmp(n->name, prev->name) == 0);
        j += 2;
    }
    assert(size(&om) == size);
    ccc_entry e = insert(&om, &(struct name){.name = "Ferris"}.e);
    assert(size(&om) == size + 1);
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

struct val
{
    pq_elem elem;
    int val;
};

static ccc_threeway_cmp
val_cmp(ccc_cmp const cmp)
{
    struct val const *const lhs = cmp.user_type_lhs;
    struct val const *const rhs = cmp.user_type_rhs;
    return (lhs->val > rhs->val) - (lhs->val < rhs->val);
}

int
main(void)
{
    struct val elems[5]
        = {{.val = 3}, {.val = 3}, {.val = 7}, {.val = -1}, {.val = 5}};
    priority_queue pq = pq_init(struct val, elem, CCC_LES, NULL, val_cmp, NULL);
    for (size_t i = 0; i < sizeof(elems) / sizeof(elems[0]); ++i)
    {
        struct val const *const v = push(&pq, &elems[i].elem);
        assert(v && v->val == elems[i].val);
    }
    bool const decreased = pq_decrease_w(&pq, &elems[4].elem,
                                         { elems[4].val = -99; });
    assert(decreased);
    struct val const *const v = front(&pq);
    assert(v->val == -99);
    return 0;
}
```

</details>

<details>
<summary>realtime_ordered_map.h (dropdown)</summary>
A pointer stable ordered map meeting strict O(lg N) runtime bounds for realtime applications.

```c
#include <assert.h>
#define REALTIME_ORDERED_MAP_USING_NAMESPACE_CCC
#define TRAITS_USING_NAMESPACE_CCC
#define TYPES_USING_NAMESPACE_CCC
#include "ccc/realtime_ordered_map.h"
#include "ccc/traits.h"

struct kval
{
    romap_elem elem;
    int key;
    int val;
};

static ccc_threeway_cmp
kval_cmp(ccc_key_cmp const cmp)
{
    struct kval const *const rhs = cmp.user_type_rhs;
    int const key_lhs = *((int *)cmp.key_lhs);
    return (key_lhs > rhs->key) - (key_lhs < rhs->key);
}

int
main(void)
{
    struct kval elems[25];
    /* stack array of 25 elements with one slot for sentinel, intrusive field
       named elem, key field named key, no allocation permission, key comparison
       function, no aux data. */
    realtime_ordered_map s
        = rom_init(s, struct kval, elem, key, NULL, kval_cmp, NULL);
    int const num_nodes = 25;
    /* 0, 5, 10, 15, 20, 25, 30, 35,... 120 */
    for (int i = 0, id = 0; i < num_nodes; ++i, id += 5)
    {
        elems[i].key = id;
        elems[i].val = i;
        (void)insert_or_assign(&s, &elems[i].elem);
    }
    /* This should be the following range [6,44). 6 should raise to
       next value not less than 6, 10 and 44 should be the first
       value greater than 44, 45. */
    int range_keys[8] = {10, 15, 20, 25, 30, 35, 40, 45};
    range r = equal_range(&s, &(int){6}, &(int){44});
    int index = 0;
    for (struct kval *i = begin_range(&r); i != end_range(&r);
         i = next(&s, &i->elem))
    {
        assert(i->key == range_keys[index]);
        ++index;
    }
    /* This should be the following range [119,84). 119 should be
       dropped to first value not greater than 119 and last should
       be dropped to first value less than 84. */
    int rrange_keys[8] = {115, 110, 105, 100, 95, 90, 85, 80};
    rrange rr = equal_rrange(&s, &(int){119}, &(int){84});
    index = 0;
    for (struct kval *i = rbegin_rrange(&rr); i != rend_rrange(&rr);
         i = rnext(&s, &i->elem))
    {
        assert(i->key == rrange_keys[index]);
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

struct int_elem
{
    int i;
    sll_elem e;
};

static ccc_threeway_cmp
int_cmp(ccc_cmp const cmp)
{
    struct int_elem const *const lhs = cmp.user_type_lhs;
    struct int_elem const *const rhs = cmp.user_type_rhs;
    return (lhs->i > rhs->i) - (lhs->i < rhs->i);
}

int
main(void)
{
    /* singly linked list l, list elem field e, no allocation permission,
       comparing integers, no auxiliary data. */
    singly_linked_list l = sll_init(l, struct int_elem, e, NULL, int_cmp, NULL);
    struct int_elem elems[3] = {{.i = 3}, {.i = 2}, {.i = 1}};
    (void)push_front(&l, &elems[0].e);
    (void)push_front(&l, &elems[1].e);
    (void)push_front(&l, &elems[2].e);
    struct int_elem const *i = front(&l);
    assert(i->i == 1);
    pop_front(&l);
    i = front(&l);
    assert(i->i == 2);
    return 0;
}
```

</details>

## Features

- Intrusive and non-intrusive containers.
- Non-allocating container options.
- No `container_of` macro required of the user to get to their type after a function call.
- Rust's Entry API for associative containers with C and C++ influences.
- Opt-in macros for more succinct insertion and in place modifications (see "closures" in the [and_modify_w](https://agl-alexglopez.github.io/ccc/flat__hash__map_8h.html) interface for associative containers).
- Container Traits implemented with C `_Generic` capabilities.

### Intrusive and Non-Intrusive Containers

Currently, all associative containers ask the user to store an element in their type. This means wrapping an element in a struct such as this type found in `samples/graph.c` for the flat hash map.

```c
struct path_backtrack_cell
{
    ccc_fhmap_elem elem;
    struct point current;
    struct point parent;
};
```

The interface may then ask for a handle to this type for certain operations. For example, a flat hash map we have the following interface for `try_insert`.

```c
ccc_entry ccc_fhm_try_insert(ccc_flat_hash_map *h,
                             ccc_fhmap_elem *key_val_handle);
```

Here, the user is trying to insert a new key and value into the hash map which in the above example would be a `struct path_backtrack_cell` with the `current` and `parent` fields set appropriately.

Non-Intrusive containers exist when a flat container can operate without such help from the user. The `flat_priority_queue` is a good example of this. When initializing we give it the following information.

```c
#define ccc_fpq_init(mem_ptr, capacity, cmp_order, alloc_fn, cmp_fn, aux_data) \
    ccc_impl_fpq_init(mem_ptr, capacity, cmp_order, alloc_fn, cmp_fn, aux_data)

/* For example: */

ccc_flat_priority_queue fpq
    = ccc_fpq_init((int[40]){}, 40, CCC_LES, NULL, int_cmp, NULL);

```

Here a small min priority queue of integers with a maximum capacity of 40 has been allocated on the stack with no allocation permission and no auxiliary data needed. As long as the flat priority queue knows the type upon initialization no intrusive elements are needed. We could have also initialized this container as empty if we provide an allocation function (see [allocation](#allocation) for more on allocation permission).

```c
ccc_flat_priority_queue fpq
    = ccc_fpq_init((int *)NULL, 0, CCC_LES, std_alloc, int_cmp, NULL);
```

Notice that we need to help the container by casting to the type we are storing. The interface then looks like this.

```c
void *ccc_fpq_push(ccc_flat_priority_queue *fpq, void const *e);
```

The element `e` here is just a generic reference to whatever type the user stores in the container.

### Non-Allocating Containers

As was mentioned in the previous section, all containers can be forbidden from allocating memory. In the flat priority queue example we had this initialization.

```c
ccc_flat_priority_queue fpq
    = ccc_fpq_init((int[40]){}, 40, CCC_LES, NULL, int_cmp, NULL);
```

For flat containers, fixed capacity is straightforward. Once space runs out, further insertion functions will fail and report that failure in different ways depending on the function used.

For non-flat containers that can't assume they are stored contiguously in memory, the initialization looks like this when allocation is prohibited.

```c
struct id_val
{
    ccc_dll_elem e;
    int id;
    int val;
};

ccc_doubly_linked_list dll
    = ccc_dll_init(dll, struct id_val, e, NULL, val_cmp, NULL);
```

All interface functions now expect the memory containing the intrusive elements to exist with the appropriate scope and lifetime for the programmer's needs.

```c
/* !WARNING: THIS IS A BAD IDEA FOR DEMONSTRATION PURPOSES! */
void push_three(ccc_doubly_linked_list *const dll)
{
    struct id_val v0 = {};
    struct id_val *v = push_back(dll, &v0.e);
    assert(v == &v0);
    struct id_val v1 = {.id = 1, .val = 1};
    v = push_back(dll, &v1.e);
    assert(v == &v1);
    struct id_val v2 = {.id = 2, .val = 2};
    v = push_back(dll, &v2.e);
    assert(v == &v2);
}
```

Here, the container pushes stack allocated structs directly into the list. The container has not been given allocation permission so it assumes the memory it is given has the appropriate lifetime for the programmer's needs. When this function ends, that memory is invalid because its scope and lifetime has ended. Using `malloc` in this case would be the traditional approach, but there are a variety of ways a programmer can control scope and lifetime. This library does not prescribe any specific strategy to managing memory when allocation is prohibited. For example compositions of allocating and non-allocating containers, see the `samples/`.

### No `container_of` Macros

Traditionally, intrusive containers provide the following macro.

```c
/** list_entry - get the struct for this entry
@ptr the &struct list_elem pointer.
@type the type of the struct this is embedded in.
@member	the name of the list_elem within the struct. */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/* A provided function by the container. */
struct list_elem *list_front(list *l);
```

Then, the user code looks like this.

```c
struct id
{
    int id;
    struct list_elem id_elem;
};
/* ...  */
static struct list id_list = LIST_INIT(id_list);
/* ...  */
struct id *front = list_entry(list_front(&id_list), struct id, id_elem);
/* Or when writing a comparison callback. */
bool
is_id_a_less(struct list_elem const *const a,
             struct list_elem const *const b, void *const aux)
{
    struct id const *const a_ = list_entry(a, struct id, id_elem);
    struct id const *const b_ = list_entry(b, struct id, id_elem);
    return a_->id < b_->id;
}
```

Here, it may seem manageable to use such a macro, but it is required at every location in the code where the user type is needed. The opportunity for bugs in entering the type or field name grows the more the macro is used. It is better to take care of this step for the user and present a cleaner interface.

Here is the same list example in the C Container Collection.

```c
struct id
{
    int id;
    ccc_dll_elem id_elem;
};
/* ... */
static ccc_doubly_linked_list id_list
    = ccc_dll_init(id_list, struct id, id_elem, NULL, id_cmp, NULL);
/* ... */
struct id *front = ccc_dll_front(&id_list);
struct id *new_id = generate_id();
struct id *new_front = ccc_dll_push_front(&id_list, &new_id->id_elem);
/* Or when writing a comparison callback. */
ccc_threeway_cmp
id_cmp(ccc_cmp const cmp)
{
    struct id const *const lhs = cmp.user_type_lhs;
    struct id const *const rhs = cmp.user_type_rhs;
    return (lhs->id > rhs->id) - (lhs->id < rhs->id);
}
```

Internally the containers will remember the offsets of the provided elements within the user struct wrapping the intruder. Then, the contract of the interface is simpler: provide a handle to the container and receive your type in return. The user takes on less complexity overall by providing a slightly more detailed initialization.

Composing multiple containers with this approach is also possible. Consider the following struct from `samples/graph.c` used to run Dijkstra's algorithm.

```c
struct dijkstra_vertex
{
    romap_elem path_elem;
    pq_elem pq_elem;
    int dist;
    char cur_name;
    char prev_name;
};
/* ... Later Initialization After Memory is Prepared ... */
realtime_ordered_map path_map = rom_init(path_map, struct dijkstra_vertex,
    path_elem, cur_name, arena_alloc, cmp_prev_vertices, &bump_arena);
priority_queue costs_pq = pq_init(struct dijkstra_vertex, pq_elem, CCC_LES,
    NULL, cmp_pq_costs, NULL);
/*... Steps to Prepare to Run the Algorithm ...*/
while (!is_empty(&costs_pq))
{
    struct dijkstra_vertex *cur = front(&costs_pq);
    (void)pop(&costs_pq);
    /* ... Check for Stopping Condition Then Continue ... */
    struct node const *const edges = vertex_at(graph, cur->cur_name)->edges;
    for (int i = 0; i < MAX_DEGREE && edges[i].name; ++i)
    {
        struct dijkstra_vertex *next
            = get_key_val(&path_map, &edges[i].name);
        int alt = cur->dist + edges[i].cost;
        if (alt < next->dist)
        {
            pq_decrease_w(&costs_pq, &next->pq_elem, {
                next->prev_name = cur->cur_name;
                next->dist = alt;
            });
        }
    }
}
/* ... Rebuild Shortest Path etc... */
```

One conceptual element, a `dijkstra_vertex`, is part of two containers, a map and a priority queue. The priority queue piggy backs of the memory controlled by the map so that we always have access to all vertices while the algorithm runs. An already complex algorithm is not cluttered by further steps to accommodate macros.

### Rust's Entry Interface

Rust has solid interfaces for associative containers, largely due to the Entry API/Interface. In the C Container Collection the core of all associative containers is inspired by the Entry Interface (these versions are found in `ccc/traits.h` but specific names, behaviors, and parameters can be read in each container's header).

- `ccc_entry(container_ptr, key_ptr...)` - Obtains an entry, a view into an Occupied or Vacant user type stored in the container.
- `ccc_and_modify(entry_ptr, mod_fn)` - Modify an occupied entry with a callback.
- `ccc_and_modify_aux(entry_ptr, mod_fn, aux_args)` - Modify an Occupied entry with a callback that requires auxiliary data.
- `ccc_or_insert(entry_ptr, or_insert_args)` - Insert a default key value if Vacant or return the Occupied entry.
- `ccc_insert_entry(entry_ptr, insert_entry_args)` - Invariantly insert a new key value, overwriting an Occupied entry if needed.
- `ccc_remove_entry(entry_ptr)` - Remove an Occupied entry from the container or do nothing.

Other Rust Interface functions like `get_key_val`, `insert`, and `remove` are included and can provide information about previous values stored in the container.

Each container offers it's own C version of "closures" for the `and_modify_w` macro, short for and modify "with". Here is an example from the `samples/words.c` program.

- `and_modify_w(flat_ordered_map_entry_ptr, type_name, closure_over_T...)` - Run code in `closure_over_T` on the stored user type `T`.

```c
typedef struct
{
    str_ofs str_arena_offset;
    int cnt;
    fomap_elem e;
} word;
/* Increment a found word or insert a default count of 1. */
word *w =
fom_or_insert_w(
    fom_and_modify_w(entry_r(&fom, &ofs), word, { T->cnt++; }),
    (word){.str_arena_offset = ofs, .cnt = 1}
);
```

This is possible because of the details discussed in the previous section. Containers can always provide the user type stored in the container directly. However, there are other options to achieve the same result.

Some C++ associative container interfaces have also been adapted to the Entry Interface.

- `ccc_try_insert(container_ptr, try_insert_args)` - Inserts a new element if none was present and reports if a previous entry existed.
- `ccc_insert_or_assign(container_ptr, insert_or_assign_args)` - Inserts a new element invariantly and reports if a previous entry existed.

Many other containers fall back to C++ style interfaces when it makes sense to do so.

#### Lazy Evaluation

Many of the above functions come with an optional macro variant. For example, the `or_insert` function for associative containers will come with an `or_insert_w` variant, short for or insert "with." The word "with" in this context means a direct r-value.

Here is an example for generating a maze with Prim's algorithm in the `samples/maze.c` sample.

The functional version.

```c
struct point const next = {.r = c->cell.r + dir_offsets[i].r,
                           .c = c->cell.c + dir_offsets[i].c};
struct prim_cell new = (struct prim_cell){.cell = next,
                                          .cost = rand_range(0, 100)};
struct prim_cell *const cell = or_insert(entry_r(&costs, &next), &new.map_elem);
```

The lazily evaluated macro version.

```c
struct point const next = {.r = c->cell.r + dir_offsets[i].r,
                           .c = c->cell.c + dir_offsets[i].c};
struct prim_cell *const cell = om_or_insert_w(
    entry_r(&costs, &next),
    (struct prim_cell){.cell = next, .cost = rand_range(0, 100)});
```

The second example is slightly more convenient and efficient. The compound literal is provided to be directly assigned to a Vacant memory location; it is only constructed if there is no entry present. This also means the random generation function is only called if a Vacant entry requires the insertion of a new value. So, expensive function calls can be lazily evaluated only when needed.

Here is another example illustrating the difference between the two.

```c
struct val
{
    omap_elem e;
    int key;
    int val;
};

ccc_entry e = om_try_insert(&om, &(struct val){.key = 3, .val = 1}.e);
```

The same insertion with the "with" variant.

```c
static inline struct val
val(int val_arg)
{
    return (struct val){.val = val_args};
}

ccc_entry *e = om_try_insert_w(&om, 3, val(1));
```

This second version illustrates a few key points. R-values are provided directly as keys and values, not references to keys and values. Also, a function call to generate a value to be inserted is completely acceptable; the function is only called if insertion is required. Finally, the functions `try_insert_w` and `insert_or_assign_w` will ensure the key in the newly inserted value matches the key searched, saving the user some typing and ensuring they don't make a mistake in this regard.

The lazy evaluation of the `_w` family of functions offer an expressive way to write C code when needed. See each container's header for more.

### Traits

Traits, found in `ccc/traits.h`, offer a more succinct way to use shared functionality across containers. Instead of calling `ccc_fhm_entry` when trying to obtain an entry from a flat hash map, one can simply call `entry`. Traits utilize `_Generic` in C to choose the correct container function based on parameters provided.

Traits cost nothing at runtime but may increase compilation resources and time, though I have not been able to definitively measure a human noticeable difference in this regard. For example, consider two ways to use the entry interface.

```c
typedef struct
{
    str_ofs str_arena_offset;
    int cnt;
    fomap_elem e;
} word;
/* ... Elsewhere generate offset ofs as key. */
word default = {.str_arena_offset = ofs, .cnt = 1};
word *w = or_insert(and_modify(entry_r(&fom, &ofs), increment), &default.e);
```

Or the following.

```c
typedef struct
{
    str_ofs str_arena_offset;
    int cnt;
    fomap_elem e;
} word;
/* ... Elsewhere generate offset ofs as key. */
word default = {.str_arena_offset = ofs, .cnt = 1};
fomap_entry *e = entry_r(&fom, &ofs);
e = and_modify(e, increment)
word *w = or_insert(e, &default.e);
```

Using the first method in your code may expand the code evaluated in different `_Generic` cases greatly increasing compilation memory use and time (I have not yet measured the validity of these concerns). Such nesting concerns are not relevant if the container specific versions of these functions are used. Traits are completely opt-in by including the `traits.h` header.

## Allocation

When allocation is required, this collection offers the following interface. The user provides this function to containers upon initialization.

```c
typedef void *ccc_alloc_fn(void *ptr, size_t size, void *aux);
```

An allocation function implements the following behavior, where ptr is pointer to memory, size is number of bytes to allocate, and aux is a reference to any supplementary information required for allocation, deallocation, or reallocation. The aux parameter is passed to a container upon its initialization and the programmer may choose how to best utilize this reference (read on for more on aux).

- If NULL is provided with a size of 0, NULL is returned.
- If NULL is provided with a non-zero size, new memory is allocated/returned.
- If ptr is non-NULL it has been previously allocated by the alloc function.
- If ptr is non-NULL with non-zero size, ptr is resized to at least size
  size. The pointer returned is NULL if resizing fails. Upon success, the
  pointer returned might not be equal to the pointer provided.
- If ptr is non-NULL and size is 0, ptr is freed and NULL is returned.

One may be tempted to use realloc to check all of these boxes but realloc is implementation defined on some of these points. The aux parameter also discourages users from providing realloc. For example, one solution using the standard library allocator might be implemented as follows (aux is not needed):

```c
void *
std_alloc(void *const ptr, size_t const size, void *)
{
    if (!ptr && !size)
    {
        return NULL;
    }
    if (!ptr)
    {
        return malloc(size);
    }
    if (!size)
    {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}
```

However, the above example is only useful if the standard library allocator is used. Any allocator that implements the required behavior is sufficient. For ideas of how to utilize the aux parameter, see the sample programs. Using custom arena allocators or container compositions are cases when aux is helpful in taming lifetimes and simplifying allocation.

### Constructors

Another concern for the programmer related to allocation may be constructors and destructors, a C++ shaped peg for a C shaped hole. In general, this library has some limited support for destruction but does not provide an interface for direct constructors as C++ would define them; though this may change.

Consider a constructor. If the container is allowed to allocate, and the user wants to insert a new element, they may see an interface like this (pseudocode as all containers are slightly different).

```c
void *insert(container *c, container_elem *e);
```

Because the user has wrapped the intrusive container element in their type, the entire user type will be written to the new allocation. All interfaces can also confirm when insertion succeeds if global state needs to be set in this case. So, if some action beyond setting values needs to be performed, there are multiple opportunities to do so.

### Destructors

For destructors, the argument is similar but the container does offer more help. If an action other than freeing the memory of a user type is needed upon removal, there are options in an interface to obtain the element to be removed. Associative containers offer functions that can obtain entries (similar to Rust's Entry API). This reference can then be examined and complex destructor actions can occur before removal. Other containers like lists or priority queues offer references to an element of interest such as front, back, max, min, etc. These can all allow destructor-like actions before removal. One exception is the following interfaces.

The clear function works for pointer stable containers and flat containers.

```c
result clear(container *c, destructor_fn *fn);
```

The clear and free function works for flat containers.

```c
result clear_and_free(container *c, destructor_fn *fn);
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
- Why is initialization so ugly? Yes, this is a valid concern. Upfront complexity helps eliminate the need for a `container_of` macro for the user see the [no container_of macros](#no-container_of-macros) section for more.
- Why callbacks? Freedom for more varied comparisons and allocations. Learn to love auxiliary data. Also you have the chance to craft the optimal function for your application; for example writing a perfectly tailored hash function for your data set.
- Why not header only? Readability, maintainability, and update ability, for changing implementations in the source files. If the user wants to explore the implementation everything should be easily understandable. Smaller object size and easier modular compilation is also nice.
- Why not opaque pointers and true implementation hiding? This is not possible in C if the user is in charge of memory. The container types must be complete if the user wishes to store them on the stack or data segment. I try to present a clean interface.
- Why flat maps? Mostly experimenting. Flat maps track the tree structure through indices not pointers. This makes the data structure copyable, relocatable, serializable, or writable to disk at the cost of pointer stability in most cases. This can also make logging and recording program state easier.
- Why not a better hash map? Haven't gotten to it yet. This container has the most room for improvement. There are many exciting recent developments in hash tables that can be built upon to improve this container.
- Why C23? It is a great standard that helps with some initialization and macro ideas implemented in the library. Clang covers all of the features used on many platforms. Newer gcc versions also have them covered.

## Related

If these containers do not fit your needs, here are some excellent data structure libraries I have found for C. They are clever, fast, and elegant, taking care of all memory management for you.

- [STC - Smart Template Containers](https://github.com/stclib/STC)
- [C Template Library (CTL)](https://github.com/glouw/ctl)
- [CC: Convenient Containers](https://github.com/JacksonAllan/CC)

