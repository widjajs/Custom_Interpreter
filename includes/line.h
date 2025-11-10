#include "../includes/utility.h"

typedef struct {
    int line;
    int count;
} LineRun_t;

typedef struct {
    int capacity;
    int count;
    LineRun_t *line_runs;
} LineRunArray_t;

int get_line(LineRunArray_t array, int offset);
void init_line_run_array(LineRunArray_t *array);
void write_line_array(LineRunArray_t *array, LineRun_t value);
void free_line_array(LineRunArray_t *array);
