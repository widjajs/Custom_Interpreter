#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "compiler.h"
#include "hash_table.h"
#include "object.h"

typedef struct {
    ObjectClosure_t *closure;
    uint8_t *pc;
    Value_t *slots; // actually pts to first frame slot a func can use
} CallFrame_t;

typedef struct {
    Chunk_t *chunk;
    // uint8_t *pc;
    Value_t stack[64 * 256]; // 64 frames with 256 slots each
    Value_t *stack_top;
    HashTable_t strings;
    HashTable_t globals;
    Object_t *objects;
    CallFrame_t frames[64];
    int frame_cnt;
    ObjectUpvalue_t *open_upvalues;
    int grey_cnt;
    int grey_capacity;
    Object_t **grey_stack;
    size_t bytes_allocated;
    size_t next_GC;
} vm_t;

typedef enum { INTERPRET_OK, INTERPRET_COMPILE_ERROR, INTERPRET_RUNTIME_ERROR } InterpretResult_t;

extern vm_t vm;

void init_vm();
void free_vm();
void push(Value_t value);
Value_t pop();
InterpretResult_t interpret(const char *code);

#endif
