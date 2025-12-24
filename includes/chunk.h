#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>

#include "line.h"
#include "utility.h"
#include "value.h"

// OpCodes
typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NONE,
    OP_TRUE,
    OP_FALSE,
    OP_NOT,
    OP_NEGATE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EQUAL,
    OP_GREATER_THAN,
    OP_LESS_THAN,
    OP_PRINT,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_DEFINE_GLOBAL_LONG, // if global id > 255
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG, // if global id > 255
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG, // if globa id > 255
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_LOCAL_LONG,
    OP_SET_LOCAL_LONG,
    OP_BRANCH_IF_FALSE,
    OP_BRANCH,
    OP_LOOP,
    OP_RETURN,
} OpCode_t;

// Data
typedef struct {
    int capacity;
    int count;
    uint8_t *code;
    ValueArray_t constants;
    LineRunArray_t line_runs;
} Chunk_t;

void init_chunk(Chunk_t *chunk);
void write_chunk(Chunk_t *chunk, uint8_t byte, int line);
void free_chunk(Chunk_t *chunk);
int add_constant(Chunk_t *chunk, Value_t value);
void write_constant(Chunk_t *chunk, Value_t value, int line);

#endif
