#ifndef COMPILER_H
#define COMPILER_H

#include "chunk.h"
#include "debug.h"
// #include "object.h"
#include "scanner.h"
#include "value.h"

typedef enum {
    PREC_NONE,     // no precedence
    PREC_ASSIGN,   // =, numbers
    PREC_OR,       // or
    PREC_AND,      // and
    PREC_EQUALITY, // ==, !=
    PREC_COMPARE,  // < > <= >=
    PREC_ADD_SUB,  // + -
    PREC_MUL_DIV,  // * /
    PREC_UNARY,    // ! -
    PREC_ACCESSOR  // . () function calls and accesses
} Precedence_t;

typedef void (*ParseFunc_t)(); // used to "store" the parse function we need for each token

typedef struct {
    ParseFunc_t prefix_rule;
    ParseFunc_t infix_rule;
    Precedence_t precedence; // prefix precedence
} ParseRule_t;

bool compile(const char *code, Chunk_t *chunk);

#endif
