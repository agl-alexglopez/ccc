#include "random.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void
random_seed(unsigned int const seed)
{
    srand(seed);
}

int
rand_range(int const min, int const max)
{
    unsigned mn = min;
    unsigned mx = max;
    /* NOLINTNEXTLINE(cert-msc30-c, cert-msc50-cpp) */
    return (int)(mn + (rand() / (RAND_MAX / (mx - mn + 1) + 1)));
}

void
rand_shuffle(size_t const elem_size, void *const elems, size_t const n,
             void *const temp)
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
        memcpy(temp, elem_view + (j * step), elem_size);
        memcpy(elem_view + (j * step), elem_view + (i * step), elem_size);
        memcpy(elem_view + (i * step), temp, elem_size);
    }
}

void
iota(int *const array, size_t n, unsigned start_val)
{
    for (size_t i = 0; n--; ++i, ++start_val)
    {
        array[i] = (int)start_val;
    }
}
