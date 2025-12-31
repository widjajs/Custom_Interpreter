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
    uint32_t hash = hash_string(chars, length);
    // string object already exists in memory check
    ObjectStr_t *interned = find_str(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        return interned;
    }

    ObjectStr_t *new_str =
        (ObjectStr_t *)allocate_object(sizeof(ObjectStr_t) + sizeof(char) * (length + 1), OBJ_STR);
    new_str->length = length;
    memcpy(new_str->chars, chars, length);
    new_str->chars[length] = '\0';

    new_str->hash = hash_string(chars, length);

    insert(&vm.strings, new_str, DECL_NONE_VAL);

    return new_str;
}

ObjectFunc_t *create_func() {
    ObjectFunc_t *new_func = ALLOCATE_OBJ(ObjectFunc_t, OBJ_FUNC);
    new_func->num_params = 0;
    new_func->name = NULL;
    init_chunk(&new_func->chunk);
    new_func->upvalue_cnt = 0;
    return new_func;
}

ObjectNative_t *create_native(NativeFunc_t func) {
    ObjectNative_t *native = ALLOCATE_OBJ(ObjectNative_t, OBJ_NATIVE);
    native->func = func;
    return native;
}

ObjectClosure_t *create_closure(ObjectFunc_t *func) {
    ObjectUpvalue_t **upvalues = ALLOCATE(ObjectUpvalue_t *, func->upvalue_cnt);
    for (int i = 0; i < func->upvalue_cnt; i++) {
        upvalues[i] = NULL;
    }

    ObjectClosure_t *closure = ALLOCATE_OBJ(ObjectClosure_t, OBJ_CLOSURE);
    closure->func = func;
    closure->upvalues = upvalues;
    closure->upvalue_cnt = func->upvalue_cnt;
    return closure;
}

ObjectUpvalue_t *create_upvalue(Value_t *slot) {
    ObjectUpvalue_t *upvalue = ALLOCATE_OBJ(ObjectUpvalue_t, OBJ_UPVALUE);
    upvalue->location = slot;
    return upvalue;
}
