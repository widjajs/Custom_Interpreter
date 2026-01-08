#ifndef MEMORY_H
#define MEMORY_H

#include "object.h"
#include "utility.h"

// convenience macros so don't have to cast (void *) over and over again
#define ALLOCATE(type, count) (type *)malloc(sizeof(type) * count)
#define ALLOCATE_OBJ(type, object_type) (type *)(allocate_object(sizeof(type), object_type))

int grow_capacity(int old_capacity);
void *resize(void *ptr, size_t type_size, int old_capacity, int new_capacity);
void free_objects();
void collect_garbage();
void mark_value(Value_t value);
void mark_object(Object_t *object);

#endif
