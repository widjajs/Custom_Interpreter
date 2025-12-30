#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"
#include "utility.h"
#include "value.h"

#define OBJ_TYPE(value) (GET_OBJ_VAL(value)->type)
#define IS_STR(value) is_obj_type(value, OBJ_STR)
#define IS_FUNC(value) is_obj_type(value, OBJ_FUNC)

#define GET_STR_VAL(value) ((ObjectStr_t *)GET_OBJ_VAL(value))
#define GET_CSTR_VAL(value) (((ObjectStr_t *)GET_OBJ_VAL(value))->chars)
#define GET_FUNC(value) ((ObjectFunc_t *)GET_OBJ_VAL(value))

typedef enum {
    OBJ_FUNC,
    OBJ_STR,
} ObjectType_t;

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
    Chunk_t chunk;
    ObjectStr_t *name;
} ObjectFunc_t;

static inline bool is_obj_type(Value_t value, ObjectType_t type) {
    return IS_OBJ_VAL(value) && GET_OBJ_VAL(value)->type == type;
}

ObjectStr_t *allocate_str(const char *chars, int length);
ObjectFunc_t *create_func();

#endif
