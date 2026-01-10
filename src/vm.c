#include "../includes/vm.h"
#include "../includes/debug.h"
#include "../includes/memory.h"
#include "../includes/object.h"

#include <stdarg.h>
#include <stdint.h>
#include <time.h>

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

static void define_native(const char *name, NativeFunc_t func) {
    push(DECL_OBJ_VAL(allocate_str(name, (int)strlen(name))));
    push(DECL_OBJ_VAL(create_native(func)));
    insert(&vm.globals, GET_STR_VAL(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static Value_t clock_native(int arg_cnt, Value_t *args) {
    return DECL_NUM_VAL((double)clock() / CLOCKS_PER_SEC);
}

void init_vm() {
    vm.stack_top = vm.stack;
    vm.frame_cnt = 0;
    vm.objects = NULL;
    vm.grey_capacity = 0;
    vm.grey_cnt = 0;
    vm.grey_stack = NULL;
    vm.bytes_allocated = 0;
    vm.next_GC = 1024 * 1024;

    init_hash_table(&vm.strings);
    init_hash_table(&vm.globals);

    vm.init_str = NULL;
    vm.init_str = allocate_str("init", 4);

    define_native("clock", clock_native);
}

void free_vm() {
    free_hash_table(&vm.strings);
    free_hash_table(&vm.globals);
    vm.init_str = NULL;
    free_objects();
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

static bool call(ObjectClosure_t *closure, int arg_cnt) {
    if (arg_cnt != closure->func->num_params) {
        throw_runtime_error("Expected %d parameters but got %d", closure->func->num_params,
                            arg_cnt);
        return false;
    }

    if (vm.frame_cnt == 64) {
        throw_runtime_error("Stack overflow");
        return false;
    }
    CallFrame_t *frame = &vm.frames[vm.frame_cnt++];
    frame->closure = closure;
    frame->pc = closure->func->chunk.code;
    frame->slots = vm.stack_top - arg_cnt - 1;
    return true;
}

static bool call_value(Value_t callee, int arg_cnt) {
    if (IS_OBJ_VAL(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_CLOSURE:
                return call(GET_CLOSURE(callee), arg_cnt);
            case OBJ_NATIVE: {
                NativeFunc_t native = GET_NATIVE(callee);
                Value_t res = native(arg_cnt, vm.stack_top - arg_cnt);
                vm.stack_top -= arg_cnt + 1;
                push(res);
                return true;
            }
            case OBJ_CLASS: {
                ObjectClass_t *class_ = GET_CLASS(callee);
                vm.stack_top[-arg_cnt - 1] = DECL_OBJ_VAL(create_instance(class_));
                // constructor check
                Value_t *constructor = get(&class_->methods, vm.init_str);
                if (constructor) {
                    return call(GET_CLOSURE(*constructor), arg_cnt);
                } else if (arg_cnt != 0) {
                    throw_runtime_error("Class without initializer expected 0 arguments but got %d",
                                        arg_cnt);
                }
                return true;
            }
            case OBJ_BOUND_METHOD: {
                ObjectBoundMethod_t *bound = GET_BOUND_METHOD(callee);
                vm.stack_top[-arg_cnt - 1] = bound->receiver;
                return call(bound->method, arg_cnt);
            }
            default:
                break;
        }
    }
    throw_runtime_error("You attempted call something that isn't a function or class");
    return false;
}

static void reset_stack() {
    vm.stack_top = vm.stack;
    vm.frame_cnt = 0;
    vm.open_upvalues = NULL;
}

static void throw_runtime_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // print stack trace
    for (int i = vm.frame_cnt - 1; i >= 0; i--) {
        CallFrame_t *frame = &vm.frames[i];
        ObjectFunc_t *func = frame->closure->func;
        size_t instruction = frame->pc - func->chunk.code - 1;
        fprintf(stderr, "[line %d] in  ",
                get_line(frame->closure->func->chunk.line_runs, instruction));
        if (func->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", func->name->chars);
        }
    }

    reset_stack();
}

static bool is_falsey(Value_t value) {
    return IS_NONE_VAL(value) || (IS_BOOL_VAL(value) && GET_BOOL_VAL(value) == false);
}

static void concatenate() {
    ObjectStr_t *b = GET_STR_VAL(peek(0));
    ObjectStr_t *a = GET_STR_VAL(peek(1));

    int new_length = a->length + b->length;
    char *new_str = ALLOCATE(char, new_length + 1);
    memcpy(new_str, a->chars, a->length);
    memcpy(new_str + a->length, b->chars, b->length);
    new_str[new_length] = '\0';

    ObjectStr_t *res = allocate_str(new_str, new_length);
    pop(); // GC bug
    pop(); // GC bug
    push(DECL_OBJ_VAL(res));
}

static ObjectUpvalue_t *capture_upvalue(Value_t *local) {
    ObjectUpvalue_t *prev_upvalue = NULL;
    ObjectUpvalue_t *cur_upvalue = vm.open_upvalues;

    while (cur_upvalue != NULL && cur_upvalue->location > local) {
        prev_upvalue = cur_upvalue;
        cur_upvalue = cur_upvalue->next;
    }

    if (cur_upvalue != NULL && cur_upvalue->location == local) {
        return cur_upvalue;
    }

    ObjectUpvalue_t *new_upvalue = create_upvalue(local);
    new_upvalue->next = cur_upvalue;

    if (prev_upvalue == NULL) {
        vm.open_upvalues = new_upvalue;
    } else {
        prev_upvalue->next = new_upvalue;
    }
    return new_upvalue;
}

static void close_upvalues(Value_t *last) {
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
        ObjectUpvalue_t *upvalue = vm.open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues = upvalue->next;
    }
}

static void define_method(ObjectStr_t *name) {
    Value_t method = peek(0);
    ObjectClass_t *class_ = GET_CLASS(peek(1));
    insert(&class_->methods, name, method);
    pop();
}

static bool bind_method(ObjectClass_t *class_, ObjectStr_t *name) {
    Value_t *method = get(&class_->methods, name);
    if (method == NULL) {
        throw_runtime_error("Undefined field '%s'", name->chars);
        return false;
    }
    ObjectBoundMethod_t *bound = create_bound_method(peek(0), GET_CLOSURE(*method));
    pop();
    push(DECL_OBJ_VAL(bound));
    return true;
}

static bool invoke_from_class(ObjectClass_t *class_, ObjectStr_t *name, int arg_cnt) {
    Value_t *method = get(&class_->methods, name);
    if (!method) {
        throw_runtime_error("'%s' is undefined", name->chars);
        return false;
    }
    return call(GET_CLOSURE(*method), arg_cnt);
}

static bool invoke(ObjectStr_t *name, int arg_cnt) {
    Value_t receiver = peek(arg_cnt);
    if (!IS_INSTANCE(receiver)) {
        throw_runtime_error("You tried to invoke a method from something that wasn't an instance");
        return false;
    }

    ObjectInstance_t *instance = GET_INSTANCE(receiver);
    Value_t *value = get(&instance->fields, name);
    if (value) {
        vm.stack_top[-arg_cnt - 1] = *value;
        return call_value(*value, arg_cnt);
    }
    return invoke_from_class(instance->class_, name, arg_cnt);
}

static InterpretResult_t run() {
    CallFrame_t *frame = &vm.frames[vm.frame_cnt - 1];

#define READ_BYTE() (*frame->pc++)
#define READ_LONG()                                                                                \
    (frame->pc += 3, (uint32_t)((frame->pc[-3]) | (frame->pc[-2] << 8) | (frame->pc[-1] << 16)))
#define READ_SHORT() (frame->pc += 2, (uint16_t)((frame->pc[-2] << 8) | frame->pc[-1]))
#define READ_CONSTANT() (frame->closure->func->chunk.constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (frame->closure->func->chunk.constants.values[READ_LONG()])
#define READ_STRING() GET_STR_VAL(READ_CONSTANT())
#define READ_STRING_LONG() GET_STR_VAL(READ_CONSTANT_LONG())

    while (true) {

#ifdef DEBUG_TRACE_EXECUTION
        printf(("       "));
        for (Value_t *idx = vm.stack; idx < vm.stack_top; idx++) {
            printf("[ ");
            print_value(*idx);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(&frame->closure->func->chunk,
                                (int)(frame->pc - frame->closure->func->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value_t constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                Value_t constant = READ_CONSTANT_LONG();
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
                    throw_runtime_error(
                        "Runtime Error: Operands are not both strings or both numbers");
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
                    throw_runtime_error("Runtme Error: Operand is not a number ");
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
                ObjectStr_t *global_name = READ_STRING();
                insert(&vm.globals, global_name, peek(0));
                pop();
                break;
            }
            case OP_DEFINE_GLOBAL_LONG: {
                ObjectStr_t *global_name = READ_STRING_LONG();
                insert(&vm.globals, global_name, peek(0));
                pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjectStr_t *global_name = READ_STRING();
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
                ObjectStr_t *global_name = READ_STRING_LONG();
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
                ObjectStr_t *global_name = READ_STRING();
                if (insert(&vm.globals, global_name, peek(0))) {
                    drop(&vm.globals, global_name);
                    throw_runtime_error("Undefined variable name '%s' LET's define it!",
                                        global_name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_GLOBAL_LONG: {
                ObjectStr_t *global_name = READ_STRING_LONG();
                if (insert(&vm.globals, global_name, peek(0))) {
                    drop(&vm.globals, global_name);
                    throw_runtime_error("Undefined variable name '%s' LET's define it!",
                                        global_name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_LOCAL: {
                int idx = READ_BYTE();
                push(frame->slots[idx]);
                break;
            }
            case OP_GET_LOCAL_LONG: {
                int idx = READ_LONG();
                push(frame->slots[idx]);
                break;
            }
            case OP_SET_LOCAL: {
                int idx = READ_BYTE();
                frame->slots[idx] = peek(0);
                break;
            }
            case OP_SET_LOCAL_LONG: {
                int idx = READ_LONG();
                frame->slots[idx] = peek(0);
                break;
            }
            case OP_BRANCH_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (is_falsey(peek(0))) {
                    frame->pc += offset;
                }
                break;
            }
            case OP_BRANCH: {
                uint16_t offset = READ_SHORT();
                frame->pc += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->pc -= offset;
                break;
            }
            case OP_CALL: {
                int arg_cnt = READ_BYTE();
                if (!call_value(peek(arg_cnt), arg_cnt)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_cnt - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjectFunc_t *func = GET_FUNC(READ_CONSTANT());
                ObjectClosure_t *closure = create_closure(func);
                push(DECL_OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalue_cnt; i++) {
                    uint8_t is_local = READ_BYTE();
                    uint8_t idx = READ_BYTE();
                    if (is_local) {
                        closure->upvalues[i] = capture_upvalue(frame->slots + idx);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[idx];
                    }
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t idx = READ_BYTE();
                push(*frame->closure->upvalues[idx]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t idx = READ_BYTE();
                *frame->closure->upvalues[idx]->location = peek(0);
                break;
            }
            case OP_CLOSE_UPVALUE: {
                close_upvalues(vm.stack_top - 1);
                pop();
                break;
            }
            case OP_CLASS: {
                push(DECL_OBJ_VAL(create_class(READ_STRING())));
                break;
            }
            case OP_CLASS_LONG: {
                push(DECL_OBJ_VAL(create_class(READ_STRING_LONG())));
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    throw_runtime_error("Only instances of a class have fields");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjectInstance_t *instance = GET_INSTANCE(peek(0));
                ObjectStr_t *name = READ_STRING();
                Value_t *value = get(&instance->fields, name);
                if (value) {
                    pop();
                    push(*value);
                    break;
                }
                if (!bind_method(instance->class_, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    throw_runtime_error("Only instances can have fields");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjectInstance_t *instance = GET_INSTANCE(peek(1));
                insert(&instance->fields, READ_STRING(), peek(0));

                Value_t value = pop();
                pop();
                push(value);
                break;
            }
            case OP_METHOD: {
                define_method(READ_STRING());
                break;
            }
            case OP_METHOD_LONG: {
                define_method(READ_STRING_LONG());
                break;
            }
            case OP_INVOKE: {
                ObjectStr_t *method = READ_STRING();
                int arg_cnt = READ_BYTE();
                if (!invoke(method, arg_cnt)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_cnt - 1];
                break;
            }
            case OP_INHERIT: {
                Value_t superclass = peek(1);
                if (!IS_CLASS(superclass)) {
                    throw_runtime_error(
                        "You tried to inherit from something that wasn't a class :(");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjectClass_t *subclass = GET_CLASS(peek(0));
                table_add_all(&GET_CLASS(superclass)->methods, &subclass->methods);
                pop(); // pop off the subclass
                break;
            }
            case OP_GET_SUPER: {
                ObjectStr_t *name = READ_STRING();
                ObjectClass_t *superclass = GET_CLASS(pop());
                if (!bind_method(superclass, name)) {
                    // the superclass method doesn't exist
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_SUPER_LONG: {
                ObjectStr_t *name = READ_STRING_LONG();
                ObjectClass_t *superclass = GET_CLASS(pop());
                if (!bind_method(superclass, name)) {
                    // the superclass method doesn't exist
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjectStr_t *method = READ_STRING();
                int arg_cnt = READ_BYTE();
                ObjectClass_t *superclass = GET_CLASS(pop());
                if (!invoke_from_class(superclass, method, arg_cnt)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_cnt - 1];
                break;
            }
            case OP_SUPER_INVOKE_LONG: {
                ObjectStr_t *method = READ_STRING_LONG();
                int arg_cnt = READ_BYTE();
                ObjectClass_t *superclass = GET_CLASS(pop());
                if (!invoke_from_class(superclass, method, arg_cnt)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_cnt - 1];
                break;
            }
            case OP_RETURN: {
                Value_t res = pop();
                close_upvalues(frame->slots);
                vm.frame_cnt--;
                if (vm.frame_cnt == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stack_top = frame->slots; // go back to where caller locals are
                push(res);
                frame = &vm.frames[vm.frame_cnt - 1]; // return to callers frame
                break;
            }
        }
    }
#undef READ_BYTE
#undef READ_LONG
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef READ_STRING
#undef READ_STRING_LONG
}

InterpretResult_t interpret(const char *code) {
    ObjectFunc_t *func = compile(code);
    if (func == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }
    push(DECL_OBJ_VAL(func));

    ObjectClosure_t *closure = create_closure(func);
    pop();
    push(DECL_OBJ_VAL(closure));
    call_value(DECL_OBJ_VAL(closure), 0); // i.e. main()

    return run();
}
