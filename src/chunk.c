#include "../includes/chunk.h"
#include "../includes/memory.h"

// init method for a new chunk
void init_chunk(Chunk_t *chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    init_value_array(&chunk->constants);
    init_line_run_array(&chunk->line_runs);
}

// append a new chunk
void write_chunk(Chunk_t *chunk, uint8_t byte, int line) {
    if (chunk->count + 1 > chunk->capacity) {
        int old_capacity = chunk->capacity;
        chunk->capacity = grow_capacity(old_capacity);
        chunk->code = resize(sizeof(uint8_t), chunk->code, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    write_line_array(&chunk->line_runs, (LineRun_t){.line = line, .count = 1});
    chunk->count++;
}

// cleanup free method for chunks
void free_chunk(Chunk_t *chunk) {
    free(chunk->code);
    free_value_array(&chunk->constants);
    free_line_array(&chunk->line_runs);
    init_chunk(chunk);
}

// write_constant() helper function -> returns idx of value written
int add_constant(Chunk_t *chunk, Value_t value) {
    write_value_array(&(chunk->constants), value);
    return chunk->constants.count - 1;
}

// helper method to write constants so we don't need to do separate write_chunk() calls in main
void write_constant(Chunk_t *chunk, Value_t value, int line) {
    int idx = add_constant(chunk, value);
    if (idx <= 255) {
        write_chunk(chunk, OP_CONSTANT, line);
        write_chunk(chunk, idx, line);
    } else {
        write_chunk(chunk, OP_CONSTANT_LONG, line);
        write_chunk(chunk, idx & 0xFF, line);         // lowest 8 bits
        write_chunk(chunk, (idx >> 8) & 0xFF, line);  // middle 8 bits
        write_chunk(chunk, (idx >> 16) & 0xFF, line); // front 8 bits
    }
}
