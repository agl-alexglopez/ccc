#include <stdio.h>
#include <stdlib.h>

#include "bitset_utility.h"

void *
stack_allocate(CCC_Allocator_context const context)
{
    if (!context.bytes && !context.input)
    {
        return NULL;
    }
    if (!context.bytes && context.input)
    {
        (void)fprintf(stderr, "cannot free in a stack bump allocator\n");
        exit(1);
    }
    if (context.input && context.bytes)
    {
        (void)fprintf(stderr, "cannot resize in this stack bump allocator\n");
        exit(1);
    }
    if (context.bytes)
    {
        if (!context.context)
        {
            return NULL;
        }
        struct Stack_allocator *const allocator = context.context;
        if (allocator->bytes_occupied + context.bytes
            > allocator->bytes_capacity)
        {
            return NULL;
        }
        void *const block
            = (char *)allocator->blocks + allocator->bytes_occupied;
        allocator->bytes_occupied += context.bytes;
        return block;
    }
    return NULL;
}
