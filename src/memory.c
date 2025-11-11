#include "../includes/memory.h"
#include "../includes/object.h"
#include "../includes/vm.h"

int grow_capacity(int old_capacity) {
    return old_capacity < 8 ? 8 : old_capacity * 2;
}

void *resize(void *ptr, size_t type_size, int new_capacity) {
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

static void free_object(Object_t *object) {
    switch (object->type) {
        case OBJ_STR: {
            ObjectStr_t *str = (ObjectStr_t *)object;
            free(str);
            break;
        }
    }
}

void free_objects() {
    Object_t *cur = vm.objects;
    while (cur != NULL) {
        Object_t *next = cur->next;
        free_object(cur);
        cur = next;
    }
    cur = NULL;
}
