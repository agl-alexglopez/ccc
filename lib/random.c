#include "random.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int
rand_range(const int min, const int max)
{
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

void
rand_shuffle(const size_t elem_size, void *const elems, const size_t n)
{
    if (n <= 1)
    {
        return;
    }
    uint8_t tmp[elem_size];
    uint8_t *elem_view = elems;
    const size_t step = elem_size * sizeof(uint8_t);
    for (size_t i = 0; i < n - 1; ++i)
    {
        /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
        const size_t rnd = (size_t)rand();
        const size_t j = i + rnd / (RAND_MAX / (n - i) + 1);
        memcpy(tmp, elem_view + (j * step), elem_size);
        memcpy(elem_view + (j * step), elem_view + (i * step), elem_size);
        memcpy(elem_view + (i * step), tmp, elem_size);
    }
}
