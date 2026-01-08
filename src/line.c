#include "../includes/line.h"
#include "../includes/memory.h"

void init_line_run_array(LineRunArray_t *array) {
    array->capacity = 0;
    array->count = 0;
    array->line_runs = NULL;
}

void write_line_array(LineRunArray_t *array, LineRun_t line_run) {
    if (array->count + 1 > array->capacity) {
        int old_capacity = array->capacity;
        array->capacity = grow_capacity(old_capacity);
        array->line_runs =
            resize(array->line_runs, sizeof(LineRun_t), old_capacity, array->capacity);
    }

    if (array->count != 0 && array->line_runs[array->count - 1].line == line_run.line) {
        array->line_runs[array->count - 1].count++;
    } else {
        array->line_runs[array->count] = line_run;
        array->count++;
    }
}

void free_line_array(LineRunArray_t *array) {
    free(array->line_runs);
    init_line_run_array(array);
}

int get_line(LineRunArray_t array, int offset) {
    int cur = 0;
    for (int i = 0; i < array.count; i++) {
        cur += array.line_runs[i].count;
        if (cur > offset) {
            return array.line_runs[i].line;
        }
    }
    return -1;
}
