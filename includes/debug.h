#ifndef DEBUG_H
#define DEBUG_H

#include "../includes/chunk.h"
#include "../includes/object.h"

void disassemble_chunk(Chunk_t *chunk, const char *name);
int disassemble_instruction(Chunk_t *chunk, int offset);

#endif
