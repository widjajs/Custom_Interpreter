#ifndef OBJECT_H
#define OBJECT_H

#include "utility.h"
#include "value.h"

#define OBJ_TYPE(value) (GET_OBJ_VAL(value)->type)
#define IS_STR(value) is_obj_type(value, OBJ_STR)

#define GET_STR_VAL(value) ((ObjectStr_t *)GET_OBJ_VAL(value))
#define GET_CSTR_VAL(value) (((ObjectStr_t *)GET_OBJ_VAL(value))->chars)

typedef enum {
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

static inline bool is_obj_type(Value_t value, ObjectType_t type) {
    return IS_OBJ_VAL(value) && GET_OBJ_VAL(value)->type == type;
}

ObjectStr_t *allocate_str(const char *chars, int length);

#endif
