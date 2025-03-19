#ifndef CCC_RANDOM_H
#define CCC_RANDOM_H

#include <stddef.h>

/** Initialize the random number generator for the library. Must be called
before using other random generation functions in this library. */
void random_seed(unsigned int seed);

/** Provides an integer within range [min, max] */
int rand_range(int min, int max);

/** Shuffles n elems of elem_sz randomly. User must provide a pointer to tmp
storage that is elem_sz bytes large. User must seed random before calling
this function (e.g. srand(time(NULL));). */
void rand_shuffle(size_t elem_size, void *elems, size_t n, void *tmp);

#endif /* CCC_RANDOM_H */
