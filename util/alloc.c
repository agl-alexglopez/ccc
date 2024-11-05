#include "alloc.h"

#include <stddef.h>
#include <stdlib.h>

void *
std_alloc(void *const ptr, size_t const size)
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
