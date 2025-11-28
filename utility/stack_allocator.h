#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include <stddef.h>

#include "ccc/types.h"

/** A stack allocator tracks a user provided, compile time, fixed size buffer
of user types. An initialization macro is provided to ensure these constraints
are met. A stack allocator only allocates. Freeing is a no-op that returns NULL
and no internal state is altered. From the allocator's perspective that
allocation is forever occupied. Attempts to resize will fail returning NULL.

This is good for spinning up a quick test on the stack for container interface
functions that require an allocator. We test those code paths while not dealing
with the system heap allocator. As the test harness grows, this speed up is
valuable. This should only be used for testing. */
struct Stack_allocator
{
    /** The stack array of a compile time known capacity of user types. */
    void *blocks;
    /** The size in bytes of the user type stored in this stack array. */
    size_t sizeof_type;
    /** The byte capacity of the compile time known array of user types. */
    size_t bytes_capacity;
    /** The bytes occupied within this array. Multiple of type size. */
    size_t bytes_occupied;
};

/** Provide the type name and how many of that type should be held in total
capacity for the stack allocator. While the stack allocator abstracts
allocations to bytes, it expects the user know the type and exactly the maximum
number of that type needed for the given test.

The stack allocator will then construct the correct fixed size buffer with
automatic storage duration on the stack. This should not be a Variable Length
Array, so ensure the capacity provided is a compile time constant.
@param[in] type_name the name of the type stored in this stack buffer.
@param[in] type_compile_time_known_capacity the compile time known constant
capacity desired for this stack buffer of user provided types.
@return an initialized stack allocator with the appropriately sized stack buffer
and capacity record. The allocator starts empty, assuming no allocations yet
exist. */
#define stack_allocator_initialize(type_name,                                  \
                                   type_compile_time_known_capacity)           \
    {                                                                          \
        .blocks = (type_name[type_compile_time_known_capacity]){}, /*NOLINT*/  \
        .bytes_capacity                                                        \
        = sizeof((type_name[type_compile_time_known_capacity]){}), /*NOLINT*/  \
        .bytes_occupied = 0,                                                   \
        .sizeof_type = sizeof(type_name),                                      \
    }

/** Implements a reduced allocator interface. Only allocates, does not resize
or free. If an attempt to resize or free occurs the program exits with an
error message. Intended for testing.
@param[in] context the input, bytes, and context fields for any request. If
input is non-NULL, bytes is 0, or there is no context provided NULL is returned.
Otherwise a bump allocation occurs.
@return new memory for a new allocation request, NULL for resizing or free
requests.
@warning The byte input argument is expected to be a multiple of the underlying
type size stored in the buffer. A stack allocator is not general purpose and
expects the user to only request the type specified in the initialization
argument. If a request that is not a multiple of type size is given NULL is
returned. */
void *stack_allocator_allocate(CCC_Allocator_context context);

/** Resets a stack allocator into thinking it is empty. This is safe because
conceptually it just a buffer of user types. This allows us to recycle the
underlying buffer memory whenever we wish. Use with caution.
@param[in] allocator the allocator to reset to 0 occupied bytes. */
void stack_allocator_reset(struct Stack_allocator *allocator);

#endif /* STACK_ALLOCATOR_H */
