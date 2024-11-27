/** @file
@brief The Flat Double Ended Queue Interface

An FDEQ offers contiguous storage and random access, push, and pop in constant
time. The contiguous nature of the buffer makes it well-suited to dynamic or
fixed size contexts where a double ended queue is needed.

If the container is initialized with allocation permission it will resize when
needed but support constant time push and pop to the front and back when
resizing is not required, resulting in amortized O(1) operations.

If the FDEQ is initialized without allocation permission its behavior is
equivalent to a Ring Buffer. This is somewhat unique in that it does not fail
to insert elements when size is equal to capacity. This means that push front,
push back, pop front, and pop back are O(1) operations. However, if any push
exceeds capacity an element where the push should occur is overwritten.

To shorten names in the interface, define the following preprocessor directive
at the top of your file.

```
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
```

All types and functions can then be written without the `ccc_` prefix. */
#ifndef CCC_FLAT_DOUBLE_ENDED_QUEUE_H
#define CCC_FLAT_DOUBLE_ENDED_QUEUE_H

/** @cond */
#include <stddef.h>
/** @endcond */

#include "impl/impl_flat_double_ended_queue.h"
#include "types.h"

/** @brief A contiguous buffer for O(1) push and pop from front and back.
@warning it is undefined behavior to use an uninitialized flat double ended
queue.

A flat double ended queue can be initialized on the stack, heap, or data
segment at compile time or runtime. */
typedef struct ccc_fdeq_ ccc_flat_double_ended_queue;

/** @name Initialization Interface
Initialize and create containers with memory, callbacks, and permissions. */
/**@{*/

/** @brief Initialize the fdeq with memory and allocation permission.
@param [in] mem_ptr a pointer to existing memory or ((T *)NULL).
@param [in] alloc_fn the allocator function, if allocation is allowed.
@param [in] aux_data any auxiliary data needed for element destruction.
@param [in] capacity the number of contiguous elements at mem_ptr
@param [in] optional_size an optional initial size between 1 and capacity.
@return the fdeq on the right hand side of an equality operator at runtime or
compiletime (e.g. ccc_flat_double_ended_queue q = ccc_fdeq_init(...);) */
#define ccc_fdeq_init(mem_ptr, alloc_fn, aux_data, capacity, optional_size...) \
    ccc_impl_fdeq_init(mem_ptr, alloc_fn, aux_data, capacity, optional_size)

/** @brief Copy the fdeq from src to newly initialized dst.
@param [in] dst the destination that will copy the source fdeq.
@param [in] src the source of the fdeq.
@param [in] fn the allocation function in case resizing of dst is needed.
@return the result of the copy operation. If the destination capacity is less
than the source capacity and no allocation function is provided an input error
is returned. If resizing is required and resizing of dst fails a memory error
is returned.
@note dst must have capacity greater than or equal to src. If dst capacity is
less than src, an allocation function must be provided with the fn argument.

Note that there are two ways to copy data from source to destination: provide
sufficient memory and pass NULL as fn, or allow the copy function to take care
of allocation for the copy.

Manual memory management with no allocation function provided.

```
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
flat_double_ended_queue src = fdeq_init((int[10]){}, NULL, NULL, 10);
int *new_mem = malloc(sizeof(int) * fdeq_capacity(&src));
flat_double_ended_queue dst
    = fdeq_init(new_mem, NULL, NULL, fdeq_capacity(&src));
ccc_result res = fdeq_copy(&dst, &src, NULL);
```

The above requires dst capacity be greater than or equal to src capacity. Here
is memory management handed over to the copy function.

```
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
flat_double_ended_queue src = fdeq_init((int *)NULL, std_alloc, NULL, 0);
(void)ccc_fdeq_push_back_range(&src, 5, (int[5]){0,1,2,3,4});
flat_double_ended_queue dst = fdeq_init((int *)NULL, std_alloc, NULL, 0);
ccc_result res = fdeq_copy(&dst, &src, std_alloc);
```

The above allows dst to have a capacity less than that of the src as long as
copy has been provided an allocation function to resize dst. Note that this
would still work if copying to a destination that the user wants as a fixed
size fdeq (ring buffer).

```
#define FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
flat_double_ended_queue src = fdeq_init((int *)NULL, std_alloc, NULL, 0);
(void)ccc_fdeq_push_back_range(&src, 5, (int[5]){0,1,2,3,4});
flat_double_ended_queue dst = fdeq_init((int *)NULL, NULL, NULL, 0);
ccc_result res = fdeq_copy(&dst, &src, std_alloc);
```

The above sets up dst as a ring buffer while src is a dynamic fdeq. Because an
allocation function is provided, the dst is resized once for the copy and
retains its fixed size after the copy is complete. This would require the user
to manually free the underlying buffer at dst eventually if this method is used.
Usually it is better to allocate the memory explicitly before the copy if
copying between ring buffers.

These optional allow users to stay consistent across containers with their
memory management strategies. */
ccc_result ccc_fdeq_copy(ccc_flat_double_ended_queue *dst,
                         ccc_flat_double_ended_queue const *src,
                         ccc_alloc_fn *fn);

/**@}*/

/** @name Insert and Remove Interface
Add or remove elements from the FDEQ. */
/**@{*/

/** @brief Write an element directly to the back slot of the fdeq. O(1) if no
allocation permission amortized O(1) if allocation permission is given and a
resize is required.
@param [in] fdeq_ptr a pointer to the fdeq.
@param [in] value for integral types, the direct value. For structs and
unions use compound literal syntax.
@return a reference to the inserted element. If allocation is permitted and a
resizing is required to insert the element but fails, NULL is returned. */
#define ccc_fdeq_emplace_back(fdeq_ptr, value...)                              \
    ccc_impl_fdeq_emplace_back(fdeq_ptr, value)

/** @brief Write an element directly to the front slot of the fdeq. O(1) if no
allocation permission amortized O(1) if allocation permission is given and a
resize is required.
@param [in] fdeq_ptr a pointer to the fdeq.
@param [in] value for integral types, the direct value. For structs and
unions use compound literal syntax.
@return a reference to the inserted element. If allocation is permitted and a
resizing is required to insert the element but fails, NULL is returned. */
#define ccc_fdeq_emplace_front(fdeq_ptr, value...)                             \
    ccc_impl_fdeq_emplace_front(fdeq_ptr, value)

/** @brief Push the user type to the back of the fdeq. O(1) if no allocation
permission amortized O(1) if allocation permission is given and a resize is
required.
@param [in] fdeq a pointer to the fdeq.
@param [in] elem a pointer to the user type to insert into the fdeq.
@return a reference to the inserted element. */
[[nodiscard]] void *ccc_fdeq_push_back(ccc_flat_double_ended_queue *fdeq,
                                       void const *elem);

/** @brief Push the range of user types to the back of the fdeq. O(N).
@param [in] fdeq pointer to the fdeq.
@param [in] n the number of user types in the elems range.
@param [in] elems a pointer to the array of user types.
@return ok if insertion was successful. If allocation is permitted and a resize
is needed but fails an error is returned. If bad input is provided an input
error is returned.

Note that if no allocation is permitted the fdeq behaves as a ring buffer.
Therefore, pushing a range that will exceed capacity will overwrite elements
at the beginning of the fdeq. */
ccc_result ccc_fdeq_push_back_range(ccc_flat_double_ended_queue *fdeq, size_t n,
                                    void const *elems);

/** @brief Push the user type to the front of the fdeq. O(1) if no
allocation permission amortized O(1) if allocation permission is given and a
resize is required.
@param [in] fdeq a pointer to the fdeq.
@param [in] elem a pointer to the user type to insert into the fdeq.
@return a reference to the inserted element. */
[[nodiscard]] void *ccc_fdeq_push_front(ccc_flat_double_ended_queue *fdeq,
                                        void const *elem);

/** @brief Push the range of user types to the front of the fdeq. O(N).
@param [in] fdeq a pointer to the fdeq.
@param [in] n the number of user types in the elems range.
@param [in] elems a pointer to the array of user types.
@return ok if insertion was successful. If allocation is permitted and a resize
is needed but fails an error is returned. If bad input is provided an input
error is returned.

Note that if no allocation is permitted the fdeq behaves as a ring buffer.
Therefore, pushing a range that will exceed capacity will overwrite elements
at the back of the fdeq. */
ccc_result ccc_fdeq_push_front_range(ccc_flat_double_ended_queue *fdeq,
                                     size_t n, void const *elems);

/** @brief Push the range of user types before pos of the fdeq. O(N).
@param [in] fdeq a pointer to the fdeq.
@param [in] pos the position in the fdeq before which to push the range.
@param [in] n the number of user types in the elems range.
@param [in] elems a pointer to the array of user types.
@return a pointer to the start of the inserted range or NULL if a resize was
required and could not complete.

Note that if no allocation is permitted the fdeq behaves as a ring buffer.
Therefore, pushing a range that will exceed capacity will overwrite elements
at the start of the fdeq.

Pushing a range of elements prioritizes the range and allows the range to
overwrite elements instead of pushing those elements over the start of the
range. For example, push a range {3,4,5} over a fdeq with capacity 5 before
pos with value 6.

```
 front pos        front
┌─┬┴┬─┬┴┬─┐    ┌─┬─┬┴┬─┬─┐
│ │1│2│6│ │ -> │5│6│2│3│4│
└─┴─┴─┴─┴─┘    └─┴─┴─┴─┴─┘
```

Notice that 1 and 2 were NOT moved to overwrite the start of the range, the
values 3 and 4. The only way the start of a range will be overwritten is if
the range itself is too large for the capacity. For example, push a range
{0,0,3,3,4,4,5,5} over the same fdeq.

```
 front pos    front
┌─┬┴┬─┬┴┬─┐    ┌┴┬─┬─┬─┬─┐
│ │1│2│6│ │ -> │3│4│4│5│5│
└─┴─┴─┴─┴─┘    └─┴─┴─┴─┴─┘
```

Notice that the start of the range, {0,0,3,...}, is overwritten. */
[[nodiscard]] void *ccc_fdeq_insert_range(ccc_flat_double_ended_queue *fdeq,
                                          void *pos, size_t n,
                                          void const *elems);

/** @brief Pop an element from the front of the fdeq. O(1).
@param [in] fdeq a pointer to the fdeq.
@return ok if the pop was successful. If fdeq is NULL or the fdeq is empty an
input error is returned. */
ccc_result ccc_fdeq_pop_front(ccc_flat_double_ended_queue *fdeq);

/** @brief Pop an element from the back of the fdeq. O(1).
@param [in] fdeq a pointer to the fdeq.
@return ok if the pop was successful. If fdeq is NULL or the fdeq is empty an
input error is returned. */
ccc_result ccc_fdeq_pop_back(ccc_flat_double_ended_queue *fdeq);

/**@}*/

/** @name Deallocation Interface
Destroy the container. */
/**@{*/

/** @brief Set size of fdeq to 0 and call destructor on each element if needed.
O(1) if no destructor is provided, else O(N).
@param [in] fdeq a pointer to the fdeq.
@param [in] destructor the destructor if needed or NULL.

Note that if destructor is non-NULL it will be called on each element in the
fdeq. However, the underlying buffer for the fdeq is not freed. If the
destructor is NULL, setting the size to 0 is O(1). */
ccc_result ccc_fdeq_clear(ccc_flat_double_ended_queue *fdeq,
                          ccc_destructor_fn *destructor);

/** @brief Set size of fdeq to 0 and call destructor on each element if needed.
Free the underlying buffer setting the capacity to 0. O(1) if no destructor is
provided, else O(N).
@param [in] fdeq a pointer to the fdeq.
@param [in] destructor the destructor if needed or NULL.

Note that if destructor is non-NULL it will be called on each element in the
fdeq. After all elements are processed the buffer is freed and capacity is 0.
If destructor is NULL the buffer is freed directly and capacity is 0. */
ccc_result ccc_fdeq_clear_and_free(ccc_flat_double_ended_queue *fdeq,
                                   ccc_destructor_fn *destructor);

/**@}*/

/** @name State Interface
Interact with the state of the FDEQ. */
/**@{*/

/** @brief Return a pointer to the front element of the fdeq. O(1).
@param [in] fdeq a pointer to the fdeq.
@return a pointer to the user type at the start of the fdeq. NULL if empty. */
[[nodiscard]] void *ccc_fdeq_begin(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return a pointer to the back element of the fdeq. O(1).
@param [in] fdeq a pointer to the fdeq.
@return a pointer to the user type at the back of the fdeq. NULL if empty. */
[[nodiscard]] void *ccc_fdeq_rbegin(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return the next element in the fdeq moving front to back. O(1).
@param [in] fdeq a pointer to the fdeq.
@param [in] iter_ptr the current element in the fdeq.
@return the element following iter_ptr or NULL if no elements follow. */
[[nodiscard]] void *ccc_fdeq_next(ccc_flat_double_ended_queue const *fdeq,
                                  void const *iter_ptr);

/** @brief Return the next element in the fdeq moving back to front. O(1).
@param [in] fdeq a pointer to the fdeq.
@param [in] iter_ptr the current element in the fdeq.
@return the element preceding iter_ptr or NULL if no elements follow. */
[[nodiscard]] void *ccc_fdeq_rnext(ccc_flat_double_ended_queue const *fdeq,
                                   void const *iter_ptr);

/** @brief Return a pointer to the end element. It may not be accessed. O(1).
@param [in] fdeq a pointer to the fdeq.
@return a pointer to the end sentinel element that may not be accessed. */
[[nodiscard]] void *ccc_fdeq_end(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return a pointer to the start element. It may not be accessed. O(1).
@param [in] fdeq a pointer to the fdeq.
@return a pointer to the start sentinel element that may not be accessed. */
[[nodiscard]] void *ccc_fdeq_rend(ccc_flat_double_ended_queue const *fdeq);

/**@}*/

/** @name State Interface
Interact with the state of the FDEQ. */
/**@{*/

/** @brief Return a reference to the element at index position i. O(1).
@param [in] fdeq a pointer to the fdeq.
@param [in] i the 0 based index in the fdeq.
@return a reference to the element at i if i < capacity.

Note that the front of the fdeq is considered index 0, so the user need not
worry about where the front is for indexing purposes. */
[[nodiscard]] void *ccc_fdeq_at(ccc_flat_double_ended_queue const *fdeq,
                                size_t i);

/** @brief Return a reference to the front of the fdeq. O(1).
@param [in] fdeq a pointer to the fdeq.
@return a reference to the front element or NULL if fdeq is NULL or the fdeq is
empty. */
[[nodiscard]] void *ccc_fdeq_front(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return a reference to the back of the fdeq. O(1).
@param [in] fdeq a pointer to the fdeq.
@return a reference to the back element or NULL if fdeq is NULL or the fdeq is
empty. */
[[nodiscard]] void *ccc_fdeq_back(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return true if the size of the fdeq is 0. O(1).
@param [in] fdeq a pointer to the fdeq.
@return true if the size is 0 or fdeq is NULL or false. */
[[nodiscard]] bool ccc_fdeq_is_empty(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return the size of the fdeq. O(1).
@param [in] fdeq a pointer to the fdeq.
@return the size of the fdeq or 0 if fdeq is NULL. */
[[nodiscard]] size_t ccc_fdeq_size(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return the capacity of the fdeq. O(1).
@param [in] fdeq a pointer to the fdeq.
@return the capacity of the fdeq or 0 if fdeq is NULL. */
[[nodiscard]] size_t ccc_fdeq_capacity(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return a reference to the base of backing array. O(1).
@param [in] fdeq a pointer to the fdeq.
@return a reference to the base of the backing array.
@note the reference is to the base of the backing array at index 0 with no
consideration to where the front index of the fdeq may be.
@warning it is the users responsibility to ensure that access to any data is
within the capacity of the backing buffer.

This method is exposed for serialization or writing purposes but the base of
the array may not point to valid data in terms of organization of the fdeq. */
[[nodiscard]] void *ccc_fdeq_data(ccc_flat_double_ended_queue const *fdeq);

/** @brief Return true if the internal invariants of the fdeq.
@param [in] fdeq a pointer to the fdeq.
@return true if the internal invariants of the fdeq are held, else false. */
[[nodiscard]] bool ccc_fdeq_validate(ccc_flat_double_ended_queue const *fdeq);

/**@}*/

/** Define this preprocessor directive if you wish to use shorter names for the
fdeq container. Ensure no namespace collisions occur before name shortening. */
#ifdef FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC
typedef ccc_flat_double_ended_queue flat_double_ended_queue;
#    define fdeq_init(args...) ccc_fdeq_init(args)
#    define fdeq_copy(args...) ccc_fdeq_copy(args)
#    define fdeq_emplace(args...) ccc_fdeq_emplace(args)
#    define fdeq_push_back(args...) ccc_fdeq_push_back(args)
#    define fdeq_push_back_range(args...) ccc_fdeq_push_back_range(args)
#    define fdeq_push_front(args...) ccc_fdeq_push_front(args)
#    define fdeq_push_front_range(args...) ccc_fdeq_push_front_range(args)
#    define fdeq_insert_range(args...) ccc_fdeq_insert_range(args)
#    define fdeq_pop_front(args...) ccc_fdeq_pop_front(args)
#    define fdeq_pop_back(args...) ccc_fdeq_pop_back(args)
#    define fdeq_front(args...) ccc_fdeq_front(args)
#    define fdeq_back(args...) ccc_fdeq_back(args)
#    define fdeq_is_empty(args...) ccc_fdeq_is_empty(args)
#    define fdeq_size(args...) ccc_fdeq_size(args)
#    define fdeq_clear(args...) ccc_fdeq_clear(args)
#    define fdeq_clear_and_free(args...) ccc_fdeq_clear_and_free(args)
#    define fdeq_at(args...) ccc_fdeq_at(args)
#    define fdeq_data(args...) ccc_fdeq_data(args)
#    define fdeq_begin(args...) ccc_fdeq_begin(args)
#    define fdeq_rbegin(args...) ccc_fdeq_rbegin(args)
#    define fdeq_next(args...) ccc_fdeq_next(args)
#    define fdeq_rnext(args...) ccc_fdeq_rnext(args)
#    define fdeq_end(args...) ccc_fdeq_end(args)
#    define fdeq_rend(args...) ccc_fdeq_rend(args)
#    define fdeq_validate(args...) ccc_fdeq_validate(args)
#endif /* FLAT_DOUBLE_ENDED_QUEUE_USING_NAMESPACE_CCC */

#endif /* CCC_FLAT_DOUBLE_ENDED_QUEUE_H */
