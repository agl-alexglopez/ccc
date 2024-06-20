#ifndef RANDOM_H
#define RANDOM_H

#include <stddef.h>

void rand_seed(unsigned int);
int rand_range(int, int);
void rand_shuffle(size_t, void *, size_t);

#endif
