#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "compiler.h"
#include "hash_table.h"

typedef struct {
    Chunk_t *chunk;
    uint8_t *pc;
    Value_t stack[256];
    Value_t *stack_top;
    HashTable_t strings;
    Object_t *objects;
} vm_t;

typedef enum { INTERPRET_OK, INTERPRET_COMPILE_ERROR, INTERPRET_RUNTIME_ERROR } InterpretResult_t;

extern vm_t vm;

void init_vm();
void free_vm();
void push(Value_t value);
Value_t pop();
InterpretResult_t interpret(const char *code);

#endif
