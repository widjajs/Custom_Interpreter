#include <stdint.h>
#include <stdio.h>

#include "../includes/debug.h"

int standard_instruction(const char *name, int offset);
int constant_instruction(const char *name, Chunk_t *chunk, int offset);
int constant_long_instruction(const char *name, Chunk_t *chunk, int offset);
int byte_instruction(const char *name, Chunk_t *chunk, int offset);
int byte_instruction_long(const char *name, Chunk_t *chunk, int offset);
int branch_instruction(const char *name, int sign, Chunk_t *chunk, int offset);

// given machine code -> output list of instructions
void disassemble_chunk(Chunk_t *chunk, const char *name) {
    printf("== %s ==\n", name);

    int offset = 0;
    while (offset < chunk->count) {
        offset = disassemble_instruction(chunk, offset);
    }
}

int disassemble_instruction(Chunk_t *chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 &&
        get_line(chunk->line_runs, offset) == get_line(chunk->line_runs, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", get_line(chunk->line_runs, offset));
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return standard_instruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL:
            return constant_instruction("OP_GET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constant_instruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_LOCAL:
            return byte_instruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byte_instruction("OP_SET_LOCAL", chunk, offset);
        case OP_NONE:
            return standard_instruction("OP_NONE", offset);
        case OP_TRUE:
            return standard_instruction("OP_TRUE", offset);
        case OP_FALSE:
            return standard_instruction("OP_FALSE", offset);
        case OP_NOT:
            return standard_instruction("OP_NOT", offset);
        case OP_EQUAL:
            return standard_instruction("OP_EQUAL", offset);
        case OP_GREATER_THAN:
            return standard_instruction("OP_GREATER_THAN", offset);
        case OP_LESS_THAN:
            return standard_instruction("OP_LESS_THAN", offset);
        case OP_CONSTANT_LONG:
            return constant_long_instruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_DEFINE_GLOBAL_LONG:
            return constant_long_instruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
        case OP_GET_GLOBAL_LONG:
            return constant_long_instruction("OP_GET_GLOBAL_LONG", chunk, offset);
        case OP_SET_GLOBAL_LONG:
            return constant_long_instruction("OP_SET_GLOBAL_LONG", chunk, offset);
        case OP_GET_LOCAL_LONG:
            return byte_instruction_long("OP_GET_LOCAL_LONG", chunk, offset);
        case OP_SET_LOCAL_LONG:
            return byte_instruction_long("OP_SET_LOCAL_LONG", chunk, offset);
        case OP_NEGATE:
            return standard_instruction("OP_NEGATE", offset);
        case OP_ADD:
            return standard_instruction("OP_ADD", offset);
        case OP_SUB:
            return standard_instruction("OP_SUB", offset);
        case OP_MUL:
            return standard_instruction("OP_MUL", offset);
        case OP_DIV:
            return standard_instruction("OP_DIV", offset);
        case OP_PRINT:
            return standard_instruction("OP_PRINT", offset);
        case OP_POP:
            return standard_instruction("OP_POP", offset);
        case OP_BRANCH:
            return branch_instruction("OP_BRANCH", 1, chunk, offset);
        case OP_BRANCH_IF_FALSE:
            return branch_instruction("OP_BRANCH_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return branch_instruction("OP_LOOP", -1, chunk, offset);

        default:
            printf("Unknown OpCode %d\n", instruction);
            return offset + 1;
    }
}

// ------------------------ Helper Functions ------------------------ //
int branch_instruction(const char *name, int sign, Chunk_t *chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int standard_instruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int constant_instruction(const char *name, Chunk_t *chunk, int offset) {
    uint8_t idx = chunk->code[offset + 1];
    // print left-aligned 16 char string then minimum 4-width integer
    printf("%-16s %4d '", name, idx);
    print_value(chunk->constants.values[idx]);
    printf("'\n");
    return offset + 2;
}

int constant_long_instruction(const char *name, Chunk_t *chunk, int offset) {
    // long constants idxs are stored in 3 separate bytes so "merge" them back together
    int idx = (chunk->code[offset + 1]) | (chunk->code[offset + 2] << 8) |
              (chunk->code[offset + 3] << 16);

    // print left-aligned 16 char string then minimum 4-width integer
    printf("%-16s %4d '", name, idx);
    print_value(chunk->constants.values[idx]);
    printf("'\n");
    return offset + 4;
}

int byte_instruction(const char *name, Chunk_t *chunk, int offset) {
    uint8_t idx = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, idx);
    return offset + 2;
}

int byte_instruction_long(const char *name, Chunk_t *chunk, int offset) {
    int idx = (chunk->code[offset + 1]) | (chunk->code[offset + 2] << 8) |
              (chunk->code[offset + 3] << 16);

    // print left-aligned 16 char string then minimum 4-width integer
    printf("%-16s %4d\n", name, idx);
    return offset + 4;
}
