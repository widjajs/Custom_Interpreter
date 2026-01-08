#include "../includes/memory.h"
#include "../includes/object.h"
#include "../includes/vm.h"

#ifdef DEBUG_LOG_GC
#include "../includes/debug.h"
#include <stdio.h>
#endif

#define GC_HEAP_GROW_FACTOR 2

int grow_capacity(int old_capacity) {
    return old_capacity < 8 ? 8 : old_capacity * 2;
}

void *resize(void *ptr, size_t type_size, int old_capacity, int new_capacity) {
    int new_size = type_size * new_capacity;
    int old_size = type_size * old_capacity;
    vm.bytes_allocated += new_size - old_size;

    if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
        collect_garbage();
#endif
    }

    if (vm.bytes_allocated > vm.next_GC) {
        collect_garbage();
    }

    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    void *res = realloc(ptr, new_size);
    if (res == NULL) {
        exit(1);
    }
    return res;
}

static void free_object(Object_t *object) {
#ifdef DEBUG_LOG_GC
    printf("%p freed type %d\n", (void *)object, object->type);
#endif
    switch (object->type) {
        case OBJ_STR: {
            ObjectStr_t *str = (ObjectStr_t *)object;
            free(str);
            break;
        }
        case OBJ_FUNC: {
            ObjectFunc_t *func = (ObjectFunc_t *)object;
            free_chunk(&func->chunk);
            free(func);
            break;
        }
        case OBJ_NATIVE: {
            ObjectNative_t *native = (ObjectNative_t *)object;
            free(native);
            break;
        }
        case OBJ_CLOSURE: {
            ObjectClosure_t *closure = (ObjectClosure_t *)object;
            free(closure->upvalues);
            free(closure);
            break;
        }
        case OBJ_UPVALUE: {
            ObjectUpvalue_t *upvalue = (ObjectUpvalue_t *)object;
            free(upvalue);
            break;
        }
    }
}

void mark_object(Object_t *object) {
    if (!object || object->is_marked) {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void *)object);
    print_value(DECL_OBJ_VAL(object));
    printf("\n");
#endif

    object->is_marked = true;
    if (vm.grey_capacity < vm.grey_cnt + 1) {
        vm.grey_capacity = grow_capacity(vm.grey_capacity);
        vm.grey_stack = realloc(vm.grey_stack, sizeof(Object_t *) * vm.grey_capacity);
        if (vm.grey_stack == NULL) {
            // unlikely but just in case
            exit(1);
        }
    }
    vm.grey_stack[vm.grey_cnt++] = object;
}

void mark_value(Value_t value) {
    if (!IS_OBJ_VAL(value)) {
        return;
    }
    mark_object(GET_OBJ_VAL(value));
}

// mark anything that the VM can reach so we don't accidetally deallocate it
void mark_roots() {
    for (Value_t *idx = vm.stack; idx < vm.stack_top; idx++) {
        mark_value(*idx); // mark local variables on stack as needed
    }
    for (int i = 0; i < vm.frame_cnt; i++) {
        mark_object((Object_t *)vm.frames[i].closure); // mark closures too
    }
    for (ObjectUpvalue_t *upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
        mark_object((Object_t *)upvalue); // upvalues are reachable too
    }
    mark_table(&vm.globals); // mark globals
    mark_compiler_roots();
}

void mark_array(ValueArray_t *array) {
    for (int i = 0; i < array->count; i++) {
        mark_value(array->values[i]);
    }
}

void mark_black(Object_t *object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void *)object);
    print_value(DECL_OBJ_VAL(object));
    printf("\n");
#endif
    switch (object->type) {
        case OBJ_NATIVE:
            break;
        case OBJ_STR:
            break;
        case OBJ_UPVALUE: {
            mark_value(((ObjectUpvalue_t *)object)->closed);
            break;
        }
        case OBJ_FUNC: {
            ObjectFunc_t *func = (ObjectFunc_t *)object;
            mark_object((Object_t *)func->name);
            mark_array(&func->chunk.constants);
            break;
        }
        case OBJ_CLOSURE: {
            ObjectClosure_t *closure = (ObjectClosure_t *)object;
            mark_object((Object_t *)closure->func);
            for (int i = 0; i < closure->upvalue_cnt; i++) {
                mark_object((Object_t *)closure->upvalues[i]);
            }
            break;
        }
        default:
            break;
    }
}

void trace_references() {
    // greys are "marked grey" if they are in the grey stack
    while (vm.grey_cnt > 0) {
        Object_t *object = vm.grey_stack[--vm.grey_cnt];
        mark_black(object);
    }
}

void sweep() {
    Object_t *prev = NULL;
    Object_t *object = vm.objects;
    while (object) {
        if (object->is_marked) {
            object->is_marked = false; // mark everything white for next cycle
            prev = object;
            object = object->next;
        } else {
            // object is unreachable so safe to collect
            Object_t *to_del = object;
            object = object->next;
            if (prev) {
                prev->next = object;
            } else {
                vm.objects = object;
            }

            free_object(to_del);
        }
    }
}

void collect_garbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytes_allocated;
#endif

    mark_roots();
    trace_references();
    remove_table_whites(&vm.strings);
    sweep();

    vm.next_GC = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc done\n");
    printf(" collected %ld bytes (from %ld to %ld) next at %ld\n", before - vm.bytes_allocated,
           before, vm.bytes_allocated, vm.next_GC);
#endif
}

void free_objects() {
    Object_t *cur = vm.objects;
    while (cur != NULL) {
        Object_t *next = cur->next;
        free_object(cur);
        cur = next;
    }
    free(vm.grey_stack);
    cur = NULL;
}
