#ifndef CCC_RANDOM_H
#define CCC_RANDOM_H

#include <stddef.h>

void rand_seed(unsigned int);
int rand_range(int, int);
void rand_shuffle(size_t, void *, size_t, void *tmp);

#endif /* CCC_RANDOM_H */
