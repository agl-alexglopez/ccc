#include "stack_allocator.h"
#include "ccc/types.h"

void *
stack_allocator_allocate(CCC_Allocator_context const context)
{
    if (!context.bytes || context.input || !context.context)
    {
        return NULL;
    }
    struct Stack_allocator *const allocator = context.context;
    if (allocator->bytes_occupied + context.bytes > allocator->bytes_capacity)
    {
        return NULL;
    }
    void *const block = (char *)allocator->blocks + allocator->bytes_occupied;
    allocator->bytes_occupied += context.bytes;
    return block;
}

void
stack_allocator_reset(struct Stack_allocator *const allocator)
{
    if (!allocator)
    {
        return;
    }
    allocator->bytes_occupied = 0;
}
