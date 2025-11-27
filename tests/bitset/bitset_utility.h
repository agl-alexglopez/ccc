#ifndef BITSET_UTILITY_H
#define BITSET_UTILITY_H

#include <stddef.h>

#include "ccc/types.h"

/** A stack allocator tracks a user provided, compile time, fixed size buffer
of user types. An initialization macro is provided to ensure these constraints
are met. A stack allocator only allocates, cannot resize, and never frees
(support for resizing may be added later). The program will exit if banned
operations occur.

This is good for spinning up a quick test on the stack for container interface
functions that require an allocator. We test those code paths while not dealing
with the system heap allocator. This should only be used for small tests. */
struct Stack_allocator
{
    void *blocks;
    size_t bytes_capacity;
    size_t bytes_occupied;
};

/** Provide the type name and how many of that type should be held in total
capacity for the stack allocator. The stack allocator will then construct the
correct fixed size buffer with automatic storage duration on the stack. This
should not be a Variable Length Array, so ensure the capacity provided is a
compile time constant. */
#define stack_allocator_initialize(type_name, type_capacity)                   \
    {                                                                          \
        .blocks = (type_name[type_capacity]){},                 /*NOLINT*/     \
        .bytes_capacity = sizeof((type_name[type_capacity]){}), /*NOLINT*/     \
        .bytes_occupied = 0,                                                   \
    }

/** Implements a reduced allocator interface. Only allocates, does not resize
or free. If an attempt to resize or free occurs the program exits with an
error message. Intended for testing. */
void *stack_allocate(CCC_Allocator_context);

#endif /* BITSET_UTILITY_H */
