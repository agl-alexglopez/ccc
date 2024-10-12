#include "random.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int
rand_range(int const min, int const max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + (rand() / (RAND_MAX / (max - min + 1) + 1));
}

void
rand_shuffle(size_t const elem_size, void *const elems, size_t const n,
             void *tmp)
{
    if (n <= 1)
    {
        return;
    }
    uint8_t *elem_view = elems;
    size_t const step = elem_size * sizeof(uint8_t);
    for (size_t i = 0; i < n - 1; ++i)
    {
        /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
        size_t const rnd = (size_t)rand();
        size_t const j = i + (rnd / (RAND_MAX / (n - i) + 1));
        memcpy(tmp, elem_view + (j * step), elem_size);
        memcpy(elem_view + (j * step), elem_view + (i * step), elem_size);
        memcpy(elem_view + (i * step), tmp, elem_size);
    }
}
