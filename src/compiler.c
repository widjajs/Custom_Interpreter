#include "../includes/compiler.h"
#include "../includes/object.h"

Parser_t parser;
Chunk_t *cur_chunk;

static void go_next();
static void expression();
static void consume(TokenType_t type, const char *msg);
static void report_error(Token_t *token, const char *msg);
static void stop_compiler();

static void number();
static void grouping();
static void unary();
static void binary();
static void literal();
static void string();
static void parse_precedence(Precedence_t prec);

bool compile(const char *code, Chunk_t *chunk) {
    init_scanner(code);
    cur_chunk = chunk;
    parser.has_error = false;
    parser.is_panicking = false;
    go_next();
    expression();
    consume(TOKEN_END_FILE, "End of expression");
    stop_compiler();
    return !parser.has_error;
}

// ===================================================================================================

static Chunk_t *get_cur_chunk() {
    return cur_chunk;
}

static void emit_byte(uint8_t byte) {
    write_chunk(get_cur_chunk(), byte, parser.prev.line);
}

// convenience function for writing opcode followed by 1-byte operand
static void emit_bytes(uint8_t byte_1, uint8_t byte_2) {
    emit_byte(byte_1);
    emit_byte(byte_2);
}

static void stop_compiler() {
    emit_byte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
    if (!parser.has_error) {
        disassemble_chunk(get_cur_chunk(), "Code");
    }
#endif
}

// ===================================================================================================

ParseRule_t rules[] = {
    [TOKEN_OPEN_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_CLOSE_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_OPEN_CURLY] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLOSE_CURLY] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUB] = {unary, binary, PREC_ADD_SUB},
    [TOKEN_ADD] = {NULL, binary, PREC_ADD_SUB},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_DIV] = {NULL, binary, PREC_MUL_DIV},
    [TOKEN_MUL] = {NULL, binary, PREC_MUL_DIV},
    [TOKEN_NOT] = {unary, NULL, PREC_NONE},
    [TOKEN_NOT_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER_THAN] = {NULL, binary, PREC_COMPARE},
    [TOKEN_GREATER_THAN_EQUAL] = {NULL, binary, PREC_COMPARE},
    [TOKEN_LESS_THAN] = {NULL, binary, PREC_COMPARE},
    [TOKEN_LESS_THAN_EQUAL] = {NULL, binary, PREC_COMPARE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STR] = {string, NULL, PREC_NONE},
    [TOKEN_NUM] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUNC] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NONE] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_LET] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_END_FILE] = {NULL, NULL, PREC_NONE},
};

static void expression() {
    parse_precedence(PREC_ASSIGN);
}

static void string() {
    write_constant(get_cur_chunk(),
                   DECL_OBJ_VAL(allocate_str(parser.prev.start + 1, parser.prev.length - 2)),
                   parser.prev.line);
}

static void parse_precedence(Precedence_t prec) {
    go_next();
    ParseFunc_t prefix_rule = rules[parser.prev.type].prefix_rule;
    if (prefix_rule == NULL) {
        report_error(&parser.prev, "Expected expression");
        return;
    }

    prefix_rule();

    while (prec <= rules[parser.cur.type].precedence) {
        go_next();
        ParseFunc_t infix_rule = rules[parser.prev.type].infix_rule;
        infix_rule();
    }
}

static void literal() {
    switch (parser.prev.type) {
        case TOKEN_FALSE:
            emit_byte(OP_FALSE);
            break;
        case TOKEN_TRUE:
            emit_byte(OP_TRUE);
            break;
        case TOKEN_NONE:
            emit_byte(OP_NONE);
            break;
        default:
            return;
    }
}

static void number() {
    double val = strtod(parser.prev.start, NULL);
    write_constant(get_cur_chunk(), DECL_NUM_VAL(val), parser.prev.line);
}

static void grouping() {
    expression();
    consume(TOKEN_CLOSE_PAREN, "Expect ')' after expression");
}

static void unary() {
    TokenType_t op_type = parser.prev.type;
    parse_precedence(PREC_UNARY);

    // negate operator emitted last bc we need value first so we have smtg to negate
    switch (op_type) {
        case TOKEN_NOT:
            emit_byte(OP_NOT);
            break;
        case TOKEN_SUB:
            emit_byte(OP_NEGATE);
            break;
        default:
            return;
    }
}

static void binary() {
    // left operator
    TokenType_t op_type = parser.prev.type;

    // parse right expression
    ParseRule_t *rule = &rules[op_type];
    parse_precedence((Precedence_t)(rule->precedence + 1));

    // write the op instruction
    switch (op_type) {
        case TOKEN_NOT_EQUAL:
            emit_bytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_LESS_THAN:
            emit_byte(OP_LESS_THAN);
            break;
        case TOKEN_LESS_THAN_EQUAL:
            emit_bytes(OP_GREATER_THAN, OP_NOT);
            break;
        case TOKEN_GREATER_THAN:
            emit_byte(OP_GREATER_THAN);
            break;
        case TOKEN_GREATER_THAN_EQUAL:
            emit_bytes(OP_LESS_THAN, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emit_byte(OP_EQUAL);
            break;
        case TOKEN_ADD:
            emit_byte(OP_ADD);
            break;
        case TOKEN_SUB:
            emit_byte(OP_SUB);
            break;
        case TOKEN_MUL:
            emit_byte(OP_MUL);
            break;
        case TOKEN_DIV:
            emit_byte(OP_DIV);
            break;
        default:
            return;
    }
}

// ===================================================================================================

static void consume(TokenType_t type, const char *msg) {
    if (parser.cur.type == type) {
        go_next();
        return;
    }
    report_error(&parser.cur, msg);
}

static void go_next() {
    parser.prev = parser.cur;
    while (true) {
        parser.cur = scan_token();
        if (parser.cur.type != TOKEN_ERROR) {
            break;
        }
        report_error(&parser.cur, parser.cur.start);
    }
}

static void report_error(Token_t *token, const char *msg) {
    if (parser.is_panicking) {
        // if parser is panicking (err was found earlier) just ignore the errors and keep going
        return;
    }
    parser.is_panicking = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_END_FILE) {
        fprintf(stderr, " end of file");
    } else if (token->type != TOKEN_ERROR) {
        // error tokens are not stored in entirety so only print the lexme if token != error
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser.has_error = true;
}
