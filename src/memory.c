#include "../includes/memory.h"

int grow_capacity(int old_capacity) {
    return old_capacity < 8 ? 8 : old_capacity * 2;
}

void *resize(size_t type_size, void *ptr, int new_capacity) {
    int new_size = type_size * new_capacity;

    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    void *res = realloc(ptr, new_size);
    if (res == NULL) {
        exit(1);
    }
    return res;
}
