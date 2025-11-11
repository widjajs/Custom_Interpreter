#include "../includes/value.h"
#include "../includes/memory.h"
#include "../includes/object.h"

// init / reset method for value arrays
void init_value_array(ValueArray_t *array) {
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

// write a new byte of value data
void write_value_array(ValueArray_t *array, Value_t value) {
    if (array->count + 1 > array->capacity) {
        int old_capacity = array->capacity;
        array->capacity = grow_capacity(old_capacity);
        array->values = resize(array->values, sizeof(Value_t), array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

// free value array helper function
void free_value_array(ValueArray_t *array) {
    free(array->values);
    init_value_array(array);
}

void print_object(Value_t value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STR:
            printf("%s", GET_CSTR_VAL(value));
            break;
    }
}

// print value helper function for other disassembler
void print_value(Value_t value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(GET_BOOL_VAL(value) ? "true" : "false");
            break;
        case VAL_NONE:
            printf("none");
            break;
        case VAL_NUM:
            printf("%g", GET_NUM_VAL(value));
            break;
        case VAL_OBJ:
            print_object(value);
            break;
    }
}

bool equals(Value_t a, Value_t b) {
    if (a.type != b.type) {
        return false;
    }
    switch (a.type) {
        case VAL_BOOL:
            return GET_BOOL_VAL(a) == GET_BOOL_VAL(b);
        case VAL_NUM:
            return GET_NUM_VAL(a) == GET_NUM_VAL(b);
        case VAL_NONE:
            return true;
        case VAL_OBJ: {
            return GET_OBJ_VAL(a) == GET_OBJ_VAL(b);
        }
        default:
            return false;
    }
}
