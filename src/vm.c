#include "../includes/vm.h"
#include "../includes/debug.h"
#include "../includes/memory.h"
#include "../includes/object.h"

#include <stdarg.h>
#include <stdint.h>

static Value_t peek(int offset);
static void throw_runtime_error(const char *format, ...);

#define BINARY_OP(type, op)                                                                        \
    if (!IS_NUM_VAL(peek(0)) || !IS_NUM_VAL(peek(1))) {                                            \
        throw_runtime_error("Operands are not numbers");                                           \
        return INTERPRET_RUNTIME_ERROR;                                                            \
    }                                                                                              \
    double b = GET_NUM_VAL(pop());                                                                 \
    double a = GET_NUM_VAL(pop());                                                                 \
    push(type(a op b));

vm_t vm;

void init_vm() {
    vm.stack_top = vm.stack;
    vm.objects = NULL;
    init_hash_table(&vm.strings);
    init_hash_table(&vm.globals);
}

void free_vm() {
    free_objects();
    free_hash_table(&vm.strings);
    free_hash_table(&vm.globals);
}

void push(Value_t value) {
    *vm.stack_top = value;
    vm.stack_top++;
}

Value_t pop() {
    vm.stack_top--;
    return *vm.stack_top;
}

static Value_t peek(int offset) {
    return vm.stack_top[-1 - offset];
}

static void reset_stack() {
    vm.stack_top = vm.stack;
}

static void throw_runtime_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t offset = (vm.pc - vm.chunk->code) - 1;
    int line = get_line(vm.chunk->line_runs, offset);
    fprintf(stderr, "[line %d] in program\n", line);
    reset_stack();
}

static bool is_falsey(Value_t value) {
    return IS_NONE_VAL(value) || (IS_BOOL_VAL(value) && GET_BOOL_VAL(value) == false);
}

static void concatenate() {
    ObjectStr_t *b = GET_STR_VAL(pop());
    ObjectStr_t *a = GET_STR_VAL(pop());

    int new_length = a->length + b->length;
    char *new_str = ALLOCATE(char, new_length + 1);
    memcpy(new_str, a->chars, a->length);
    memcpy(new_str + a->length, b->chars, b->length);
    new_str[new_length] = '\0';

    ObjectStr_t *res = allocate_str(new_str, new_length);
    push(DECL_OBJ_VAL(res));
}

static InterpretResult_t run() {
    while (true) {

#ifdef DEBUG_TRACE_EXECUTION
        printf(("       "));
        for (Value_t *idx = vm.stack; idx < vm.stack_top; idx++) {
            printf("[ ");
            print_value(*idx);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(vm.chunk, (int)(vm.pc - vm.chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = *vm.pc++) {
            case OP_CONSTANT: {
                Value_t constant = vm.chunk->constants.values[*(vm.pc++)];
                push(constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                int idx = (vm.pc[0]) | (vm.pc[1] << 8) | (vm.pc[2] << 16);
                vm.pc += 3;
                Value_t constant = vm.chunk->constants.values[idx];
                push(constant);
                break;
            }
            case OP_NONE: {
                push(DECL_NONE_VAL);
                break;
            }
            case OP_TRUE: {
                push(DECL_BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                push(DECL_BOOL_VAL(false));
                break;
            }
            case OP_EQUAL: {
                Value_t b = pop();
                Value_t a = pop();
                push(DECL_BOOL_VAL(equals(a, b)));
                break;
            }
            case OP_GREATER_THAN: {
                BINARY_OP(DECL_BOOL_VAL, >);
                break;
            }
            case OP_LESS_THAN: {
                BINARY_OP(DECL_BOOL_VAL, <);
                break;
            }
            case OP_NOT: {
                push(DECL_BOOL_VAL(is_falsey(pop())));
                break;
            }
            case OP_ADD: {
                if (IS_STR(peek(0)) && IS_STR(peek(1))) {
                    concatenate();
                } else if (IS_NUM_VAL(peek(0)) && IS_NUM_VAL(peek(1))) {
                    BINARY_OP(DECL_NUM_VAL, +);
                } else {
                    throw_runtime_error("Operands are not both strings or both numbers");
                }
                break;
            }
            case OP_SUB: {
                BINARY_OP(DECL_NUM_VAL, -);
                break;
            }
            case OP_MUL: {
                BINARY_OP(DECL_NUM_VAL, *);
                break;
            }
            case OP_DIV: {
                BINARY_OP(DECL_NUM_VAL, /);
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUM_VAL(peek(0))) {
                    throw_runtime_error("Operand is not a number ");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(DECL_NUM_VAL(-GET_NUM_VAL(pop())));
                break;
            }
            case OP_PRINT: {
                print_value(pop());
                printf("\n");
                break;
            }
            case OP_POP: {
                pop();
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjectStr_t *global_name = GET_STR_VAL(vm.chunk->constants.values[*vm.pc++]);
                insert(&vm.globals, global_name, peek(0));
                pop();
                break;
            }
            case OP_DEFINE_GLOBAL_LONG: {
                int idx = *vm.pc++;      // last byte
                idx |= (*vm.pc++ << 8);  // middle byte
                idx |= (*vm.pc++ << 16); // front byte
                ObjectStr_t *global_name = GET_STR_VAL(vm.chunk->constants.values[idx]);
                insert(&vm.globals, global_name, peek(0));
                pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjectStr_t *global_name = GET_STR_VAL(vm.chunk->constants.values[*vm.pc++]);
                Value_t *value = get(&vm.globals, global_name);
                if (value == NULL) {
                    throw_runtime_error("This variable has not been defined '%s'",
                                        global_name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(*value);
                break;
            }
            case OP_GET_GLOBAL_LONG: {
                int idx = *vm.pc++;      // last byte
                idx |= (*vm.pc++ << 8);  // middle byte
                idx |= (*vm.pc++ << 16); // front byte
                ObjectStr_t *global_name = GET_STR_VAL(vm.chunk->constants.values[idx]);
                Value_t *value = get(&vm.globals, global_name);
                if (value == NULL) {
                    throw_runtime_error("This variable has not been defined '%s'",
                                        global_name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(*value);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjectStr_t *global_name = GET_STR_VAL(vm.chunk->constants.values[*vm.pc++]);
                if (insert(&vm.globals, global_name, peek(0))) {
                    drop(&vm.globals, global_name);
                    throw_runtime_error("Undefined variable name '%s' LET's define it!",
                                        global_name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_GLOBAL_LONG: {
                int idx = *vm.pc++;      // last byte
                idx |= (*vm.pc++ << 8);  // middle byte
                idx |= (*vm.pc++ << 16); // front byte
                ObjectStr_t *global_name = GET_STR_VAL(vm.chunk->constants.values[idx]);
                if (insert(&vm.globals, global_name, peek(0))) {
                    drop(&vm.globals, global_name);
                    throw_runtime_error("Undefined variable name '%s' LET's define it!",
                                        global_name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_LOCAL: {
                int idx = *vm.pc++;
                push(vm.stack[idx]);
                break;
            }
            case OP_GET_LOCAL_LONG: {
                int idx = *vm.pc++;      // last byte
                idx |= (*vm.pc++ << 8);  // middle byte
                idx |= (*vm.pc++ << 16); // front byte
                push(vm.stack[idx]);
                break;
            }
            case OP_SET_LOCAL: {
                int idx = *vm.pc++;
                vm.stack[idx] = peek(0);
                break;
            }
            case OP_SET_LOCAL_LONG: {
                int idx = *vm.pc++;      // last byte
                idx |= (*vm.pc++ << 8);  // middle byte
                idx |= (*vm.pc++ << 16); // front byte
                vm.stack[idx] = peek(0);
                break;
            }
            case OP_BRANCH_IF_FALSE: {
                uint16_t offset = ((uint16_t)vm.pc[0] << 8 | (uint16_t)vm.pc[1]);
                vm.pc += 2;
                if (is_falsey(peek(0))) {
                    vm.pc += offset;
                }
                break;
            }
            case OP_BRANCH: {
                uint16_t offset = ((uint16_t)vm.pc[0] << 8 | (uint16_t)vm.pc[1]);
                vm.pc += 2;
                vm.pc += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = ((uint16_t)vm.pc[0] << 8 | (uint16_t)vm.pc[1]);
                vm.pc += 2;
                vm.pc -= offset;
                break;
            }
            case OP_RETURN:
                return INTERPRET_OK;
        }
    }
}

InterpretResult_t interpret(const char *code) {
    Chunk_t chunk;
    init_chunk(&chunk);

    if (!compile(code, &chunk)) {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.pc = vm.chunk->code;

    InterpretResult_t result = run();

    free_chunk(&chunk);
    return result;
}
