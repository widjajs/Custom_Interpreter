#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"
#include "utility.h"
#include "value.h"

#define OBJ_TYPE(value) (GET_OBJ_VAL(value)->type)

#define IS_STR(value) is_obj_type(value, OBJ_STR)
#define IS_FUNC(value) is_obj_type(value, OBJ_FUNC)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE);
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE);

#define GET_STR_VAL(value) ((ObjectStr_t *)GET_OBJ_VAL(value))
#define GET_CSTR_VAL(value) (((ObjectStr_t *)GET_OBJ_VAL(value))->chars)
#define GET_FUNC(value) ((ObjectFunc_t *)GET_OBJ_VAL(value))
#define GET_NATIVE(value) (((ObjectNative_t *)GET_OBJ_VAL(value))->func)
#define GET_CLOSURE(value) ((ObjectClosure_t *)GET_OBJ_VAL(value))

typedef enum { OBJ_FUNC, OBJ_STR, OBJ_NATIVE, OBJ_CLOSURE, OBJ_UPVALUE } ObjectType_t;

// Object_t* can safely cast to ObjectStr_t* if Object_t* pts to ObjectStr_t field
struct Object_t {
    ObjectType_t type;
    struct Object_t *next; // for linked list allowing garbage collection
};

// ObjectStr_t* can be safely casted to Object_t*
struct ObjectStr_t {
    Object_t object;
    uint32_t hash;
    int length;
    char chars[]; // Flexible array member
};

typedef struct {
    Object_t obj;
    int num_params;
    int upvalue_cnt;
    Chunk_t chunk;
    ObjectStr_t *name;
} ObjectFunc_t;

typedef Value_t (*NativeFunc_t)(int arg_cnt, Value_t *args);

typedef struct {
    Object_t obj;
    NativeFunc_t func;
} ObjectNative_t;

typedef struct {
    Object_t obj;
    Value_t *location;
} ObjectUpvalue_t;

typedef struct {
    Object_t obj;
    ObjectFunc_t *func;
    ObjectUpvalue_t **upvalues;
    int upvalue_cnt;
} ObjectClosure_t;

static inline bool is_obj_type(Value_t value, ObjectType_t type) {
    return IS_OBJ_VAL(value) && GET_OBJ_VAL(value)->type == type;
}

ObjectStr_t *allocate_str(const char *chars, int length);
ObjectFunc_t *create_func();
ObjectNative_t *create_native(NativeFunc_t func);
ObjectClosure_t *create_closure(ObjectFunc_t *func);
ObjectUpvalue_t *create_upvalue(Value_t *slot);

#endif
