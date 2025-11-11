#include "../includes/object.h"
#include "../includes/value.h"
#include "../includes/vm.h"

static Object_t *allocate_object(size_t size, ObjectType_t type) {
    Object_t *new_object = (Object_t *)(malloc(size));
    new_object->type = type;
    new_object->next = vm.objects;
    vm.objects = new_object;
    return new_object;
}

static uint32_t hash_string(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjectStr_t *allocate_str(const char *chars, int length) {
    ObjectStr_t *new_str =
        (ObjectStr_t *)allocate_object(sizeof(ObjectStr_t) + sizeof(char) * (length + 1), OBJ_STR);
    new_str->length = length;
    memcpy(new_str->chars, chars, length);
    new_str->chars[length] = '\0';

    new_str->hash = hash_string(chars, length);

    // string object already exists in memory check
    ObjectStr_t *interned = find_str(&vm.strings, chars, length, new_str->hash);
    if (interned != NULL) {
        return interned;
    }

    insert(&vm.strings, new_str, DECL_NONE_VAL);

    return new_str;
}
