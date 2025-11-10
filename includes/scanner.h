#ifndef SCANNER_H
#define SCANNER_H

#include "utility.h"

typedef struct {
    const char *start; // marks beginning of current "word" we're looking at
    const char *cur;   // marks cur idx of current "word" we're looking at
    int line;
} Scanner_t;

typedef enum {
    // one char tokens
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_CURLY,
    TOKEN_CLOSE_CURLY,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_SUB,
    TOKEN_ADD,
    TOKEN_DIV,
    TOKEN_MUL,
    TOKEN_SEMICOLON,

    // one or two char tokens
    TOKEN_NOT,
    TOKEN_NOT_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER_THAN,
    TOKEN_GREATER_THAN_EQUAL,
    TOKEN_LESS_THAN,
    TOKEN_LESS_THAN_EQUAL,
    TOKEN_LEFT_SHIFT,
    TOKEN_RIGHT_SHIFT,

    // literal types
    TOKEN_IDENTIFIER,
    TOKEN_STR,
    TOKEN_NUM,

    // reserved words
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_CLASS,
    TOKEN_THIS,
    TOKEN_SUPER,
    TOKEN_RETURN,
    TOKEN_FUNC,
    TOKEN_LET,
    TOKEN_NONE,
    TOKEN_PRINT,
    TOKEN_END_FILE,
    TOKEN_ERROR
} TokenType_t;

typedef struct {
    TokenType_t type;
    const char *start;
    int length;
    int line;
} Token_t;

typedef struct {
    Token_t cur;
    Token_t prev;
    bool has_error;
    bool is_panicking;
} Parser_t;

void init_scanner(const char *file);
Token_t scan_token();

#endif
