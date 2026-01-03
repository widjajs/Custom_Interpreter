#ifndef COMPILER_H
#define COMPILER_H

#include "chunk.h"
#include "debug.h"
#include "object.h"
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

typedef void (*ParseFunc_t)(
    bool can_assign); // used to "store" the parse function we need for each token

typedef struct {
    ParseFunc_t prefix_rule;
    ParseFunc_t infix_rule;
    Precedence_t precedence; // prefix precedence
} ParseRule_t;

typedef struct {
    Token_t name; // variable name
    int depth;
    bool is_captured; // determines if we move var to heap after out of scope
} Local_t;

typedef struct {
    uint8_t idx;
    bool is_local;
} Upvalue_t;

typedef enum {
    TYPE_FUNCTION, // user-defined function
    TYPE_SCRIPT    // entire script-lvl function
} FuncType_t;

typedef struct Compiler_t {
    struct Compiler_t *enclosing;
    ObjectFunc_t *func;
    FuncType_t type;
    Local_t *locals;
    int local_cnt;
    int local_cap;
    Upvalue_t upvalues[256];
    int scope_depth;
} Compiler_t;

ObjectFunc_t *compile(const char *code);

#endif
