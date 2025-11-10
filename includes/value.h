#ifndef VALUE_H
#define VALUE_H

#include "utility.h"

/*
// declaration in object.h; needed to avoid circular includes leading to errors
typedef struct Object_t Object_t;
typedef struct ObjectStr_t ObjectStr_t;
*/

typedef enum {
    VAL_BOOL,
    VAL_NONE,
    VAL_NUM,
    // VAL_OBJ
} ValueType_t;

typedef struct {
    ValueType_t type;
    union {
        bool boolean;
        double num;
        // Object_t *object;
    } data;
} Value_t;

#define IS_BOOL_VAL(value) ((value).type == VAL_BOOL)
#define IS_NUM_VAL(value) ((value).type == VAL_NUM)
#define IS_NONE_VAL(value) ((value).type == VAL_NONE)
// #define IS_OBJ_VAL(value) ((value).type == VAL_OBJ)

#define GET_BOOL_VAL(value) ((value).data.boolean)
#define GET_NUM_VAL(value) ((value).data.num)
// #define GET_OBJ_VAL(value) ((value).data.object)

#define DECL_BOOL_VAL(value) ((Value_t){.type = VAL_BOOL, .data.boolean = value})
#define DECL_NUM_VAL(value) ((Value_t){.type = VAL_NUM, .data.num = value})
// #define DECL_OBJ_VAL(object) ((Value_t){.type = VAL_OBJ, .data.object = (Object_t *)object})
#define DECL_NONE_VAL ((Value_t){.type = VAL_NONE, .data.num = 0})

typedef struct {
    int capacity;
    int count;
    Value_t *values;
} ValueArray_t;

void init_value_array(ValueArray_t *array);
void write_value_array(ValueArray_t *array, Value_t value);
void free_value_array(ValueArray_t *array);
void print_value(Value_t value);

bool equals(Value_t a, Value_t b);

#endif
