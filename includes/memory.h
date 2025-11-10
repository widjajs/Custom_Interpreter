#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

#include "../includes/utility.h"

#define ALLOCATE(type, count) (type *)reallocate(NULL, 0, sizeof(type) * count)

int grow_capacity(int old_capacity);
void *resize(size_t type_size, void *ptr, int new_capacity);

#endif
