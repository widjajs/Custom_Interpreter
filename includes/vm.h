#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "compiler.h"

typedef struct {
    Chunk_t *chunk;
    uint8_t *pc;
    Value_t stack[256];
    Value_t *stack_top;
} vm_t;

typedef enum { INTERPRET_OK, INTERPRET_COMPILE_ERROR, INTERPRET_RUNTIME_ERROR } InterpretResult_t;

void init_vm();
void free_vm();
void push(Value_t value);
Value_t pop();
InterpretResult_t interpret(const char *code);

#endif
