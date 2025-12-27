#include "../includes/compiler.h"
#include "../includes/hash_table.h"
#include "../includes/object.h"

Parser_t parser;
Chunk_t *cur_chunk = NULL;
Compiler_t *cur_compiler = NULL;

static void go_next();
static void expression();
static bool check(TokenType_t type);
static void consume(TokenType_t type, const char *msg);
static void report_error(Token_t *token, const char *msg);
static void stop_compiler();

static void number(bool can_assign);
static void grouping(bool can_assign);
static void unary(bool can_assign);
static void binary(bool can_assign);
static void literal(bool can_assign);
static void string(bool can_assign);
static void let(bool can_assign);
static void parse_precedence(Precedence_t prec);
static void and_(bool can_assign);
static void or_(bool can_assign);

static bool match(TokenType_t type);
static void statement();
static void declaration();
static int parse_let(const char *msg);

void init_compiler(Compiler_t *compiler);
static bool identifiers_equals(Token_t *a, Token_t *b);

HashTable_t compiler_ids;

bool compile(const char *code, Chunk_t *chunk) {
    init_scanner(code);
    init_hash_table(&compiler_ids);
    Compiler_t compiler;
    init_compiler(&compiler);
    cur_chunk = chunk;
    parser.has_error = false;
    parser.is_panicking = false;
    go_next();
    while (!match(TOKEN_END_FILE)) {
        declaration();
    }
    stop_compiler();
    return !parser.has_error;
}

// ===================================================================================================

// TODO: 425

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

void init_compiler(Compiler_t *compiler) {
    compiler->local_cnt = 0;
    compiler->scope_depth = 0;
    compiler->local_cap = 0;
    compiler->locals = NULL;
    cur_compiler = compiler;
}

static void stop_compiler() {
    emit_byte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
    if (!parser.has_error) {
        disassemble_chunk(get_cur_chunk(), "Code");
    }
#endif
    free_hash_table(&compiler_ids);
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
    [TOKEN_IDENTIFIER] = {let, NULL, PREC_NONE},
    [TOKEN_STR] = {string, NULL, PREC_NONE},
    [TOKEN_NUM] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUNC] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NONE] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
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

static void print_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';'. Got empty :(");
    emit_byte(OP_PRINT);
}

static void expression_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';'. Put the semicolon please!");
    emit_byte(OP_POP);
}

static void mark_initialized() {
    cur_compiler->locals[cur_compiler->local_cnt - 1].depth = cur_compiler->scope_depth;
}

void define_let(int global_id) {
    if (cur_compiler->scope_depth > 0) {
        mark_initialized();
        return;
    }
    if (global_id <= 255) {
        emit_bytes(OP_DEFINE_GLOBAL, global_id);
    } else {
        emit_byte(OP_DEFINE_GLOBAL_LONG);
        emit_byte(global_id & 0xFF);         // lowest 8 bits
        emit_byte((global_id >> 8) & 0xFF);  // middle 8 bits
        emit_byte((global_id >> 16) & 0xFF); // front 8 bits
    }
}

static void let_declaration() {
    int global_id = parse_let("Expected variable name. LET's put a great name :)");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NONE);
    }
    consume(TOKEN_SEMICOLON, "Expected ';'. Put the semicolon please!");
    define_let(global_id);
}

// get us out of panic mode by consuming till the next semicolon
static void synchronize() {
    parser.is_panicking = false;
    while (parser.cur.type != TOKEN_END_FILE) {
        if (parser.prev.type == TOKEN_SEMICOLON) {
            return;
        }
        if (parser.cur.type == TOKEN_RETURN) {
            return;
        }
        go_next();
    }
}

static void declaration() {
    if (match(TOKEN_LET)) {
        let_declaration();
    } else {
        statement();
    }
    if (parser.is_panicking) {
        synchronize();
    }
}

static bool match(TokenType_t type) {
    if (parser.cur.type == type) {
        go_next();
        return true;
    }
    return false;
}

static void block() {
    while (!check(TOKEN_CLOSE_CURLY) && !check(TOKEN_END_FILE)) {
        declaration();
    }
    consume(TOKEN_CLOSE_CURLY, "Expected '}' to end block");
}

static void end_scope() {
    cur_compiler->scope_depth--;
    // clean up locals at the end of the block
    while (cur_compiler->local_cnt > 0 &&
           cur_compiler->locals[cur_compiler->local_cnt - 1].depth > cur_compiler->scope_depth) {
        emit_byte(OP_POP);
        cur_compiler->local_cnt--;
    }
}

// put a temporary offset while we calculate the actual offset of branch then replace temp later
static int emit_branch(uint8_t instruction) {
    emit_byte(instruction);
    emit_byte(0xff);
    emit_byte(0xff);
    return get_cur_chunk()->count - 2;
}

// after we compile thru the then branch we can calculate the true offset
static void fix_branch(int offset) {
    int branch = get_cur_chunk()->count - offset - 2;

    if (branch > UINT16_MAX) {
        report_error(&parser.prev, "Too much code");
    }

    get_cur_chunk()->code[offset] = (branch >> 8) & 0xff;
    get_cur_chunk()->code[offset + 1] = branch & 0xff;
}

/*
 * [condition]
 * BRANCH_IF_FALSE -> L1
 * POP
 * then_stmt
 * L1:
 * POP
 * else_stmt
 * L2:
 */
static void if_statement() {
    consume(TOKEN_OPEN_PAREN, "Expected '(' after if");
    expression(); // condition
    consume(TOKEN_CLOSE_PAREN, "Expected ')' after condition statement");

    int then_offset = emit_branch(OP_BRANCH_IF_FALSE);
    emit_byte(OP_POP);
    statement();
    int else_offset = emit_branch(OP_BRANCH);
    fix_branch(then_offset);

    emit_byte(OP_POP);
    if (match(TOKEN_ELSE)) {
        statement();
    }
    fix_branch(else_offset);
}

/*
 * left condition
 * BRANCH_IF_FALSE -> end
 * POP
 * right condition
 * end:
 */
static void and_(bool can_assign) {
    int end_branch = emit_branch(OP_BRANCH_IF_FALSE);

    emit_byte(OP_POP);
    parse_precedence(PREC_AND);
    fix_branch(end_branch);
}

/*
 * left condition
 * BRANCH_IF_FALSE -> else
 * BRANCH -> end
 * else:
 * POP
 * right condition
 * end:
 */
static void or_(bool can_assign) {
    int else_branch = emit_branch(OP_BRANCH_IF_FALSE);
    int end_branch = emit_branch(OP_BRANCH);

    fix_branch(else_branch);
    emit_byte(OP_POP);
    parse_precedence(PREC_OR);
    fix_branch(end_branch);
}

static void emit_loop(int loop_start) {
    emit_byte(OP_LOOP);

    int offset = get_cur_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) {
        report_error(&parser.prev, "Loop has too much code");
    }

    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

/*
 * condition
 * BRANCH_IF_FALSE -> exit
 * POP
 * statement
 * exit:
 * POP
 */
static void while_statement() {
    int loop_start = get_cur_chunk()->count;

    consume(TOKEN_OPEN_PAREN, "Expected '(' after if");
    expression(); // condition
    consume(TOKEN_CLOSE_PAREN, "Expected ')' after condition statement");

    int exit_branch = emit_branch(OP_BRANCH_IF_FALSE);
    emit_byte(OP_POP);
    statement();
    emit_loop(loop_start);
    fix_branch(exit_branch);
    emit_byte(OP_POP);
}

/*
 * initializer
 * loop_start:
 * JUMP_IF_FALSE
 * POP
 * body:
 * LOOP
 * POP
 * increment:
 * POP
 * LOOP
 * exit:
 */
static void for_statement() {
    cur_compiler->scope_depth++;
    consume(TOKEN_OPEN_PAREN, "Expected '(' after if");

    // potential loop initializer
    if (match(TOKEN_SEMICOLON)) {
        // empty initializer in for loop
    } else if (match(TOKEN_LET)) {
        let_declaration();
    } else {
        expression_statement();
    }

    // potential loop condition
    int loop_start = get_cur_chunk()->count;
    int exit_branch = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after condition statement");

        // exit loop if condition = False
        exit_branch = emit_branch(OP_BRANCH_IF_FALSE);
        emit_byte(OP_POP);
    }

    // potential incremetor
    if (!match(TOKEN_CLOSE_PAREN)) {
        int body_branch = emit_branch(OP_BRANCH);

        int increment_start = get_cur_chunk()->count;
        expression();
        emit_byte(OP_POP);
        consume(TOKEN_CLOSE_PAREN, "Expect ')'");

        emit_loop(loop_start);
        loop_start = increment_start;
        fix_branch(body_branch);
    }

    statement();

    emit_loop(loop_start);

    if (exit_branch != -1) {
        fix_branch(exit_branch);
        emit_byte(OP_POP);
    }

    end_scope();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        print_statement();
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if (match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_FOR)) {
        for_statement();
    } else if (match(TOKEN_OPEN_CURLY)) {
        cur_compiler->scope_depth++;
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

static void string(bool can_assign) {
    write_constant(get_cur_chunk(),
                   DECL_OBJ_VAL(allocate_str(parser.prev.start + 1, parser.prev.length - 2)),
                   parser.prev.line);
}

static void emit_let_opcode(OpCode_t short_op, OpCode_t long_op, int operand) {
    if (operand <= 255) {
        emit_bytes(short_op, (uint8_t)(operand));
    } else {
        emit_byte(long_op);                // new opcode for long globals
        emit_byte(operand & 0xFF);         // lowest 8 bits
        emit_byte((operand >> 8) & 0xFF);  // middle 8 bits
        emit_byte((operand >> 16) & 0xFF); // highest 8 bits
    }
}

// will either get consant id if exists or add it if not
// helps prevent uneeded duplication of strings in constants array
static int constant_identifier(Chunk_t *chunk, HashTable_t *ids, ObjectStr_t *name) {
    Value_t *existing = get(ids, name);
    if (existing != NULL) {
        // alr exists so return saved idx instead of allcoating new one
        return (int)(GET_NUM_VAL(*existing));
    }
    int idx = add_constant(get_cur_chunk(), DECL_OBJ_VAL(name));
    insert(ids, name, DECL_NUM_VAL(idx));
    return idx;
}

static int resolve_local(Token_t *name) {
    for (int i = cur_compiler->local_cnt - 1; i >= 0; i--) {
        Local_t *local = &cur_compiler->locals[i];
        if (identifiers_equals(name, &local->name)) {
            return i;
        }
        if (local->depth == -1) {
            report_error(&parser.prev, "Can't read local variable when it's being initialized");
        }
    }
    return -1;
}

static void named_let(Token_t name, bool can_assign) {
    ObjectStr_t *global_name = allocate_str(parser.prev.start, parser.prev.length);
    int operand = resolve_local(&name);
    bool is_local = true;
    if (operand == -1) {
        operand = constant_identifier(get_cur_chunk(), &compiler_ids, global_name);
        is_local = false;
    }
    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        if (is_local) {
            emit_let_opcode(OP_SET_LOCAL, OP_SET_LOCAL_LONG, operand);
        } else {
            emit_let_opcode(OP_SET_GLOBAL, OP_SET_GLOBAL_LONG, operand);
        }
    } else {
        if (is_local) {
            emit_let_opcode(OP_GET_LOCAL, OP_GET_LOCAL_LONG, operand);
        } else {
            emit_let_opcode(OP_GET_GLOBAL, OP_GET_GLOBAL_LONG, operand);
        }
    }
}

static void let(bool can_assign) {
    named_let(parser.prev, can_assign);
}

// ===================================================================================================

static void parse_precedence(Precedence_t prec) {
    go_next();
    ParseFunc_t prefix_rule = rules[parser.prev.type].prefix_rule;
    if (prefix_rule == NULL) {
        report_error(&parser.prev, "Expected expression");
        return;
    }

    // only assign if we are in the lowest precedence otherwise thing like a = b * c might break
    bool can_assign = prec <= PREC_ASSIGN;
    prefix_rule(can_assign);

    while (prec <= rules[parser.cur.type].precedence) {
        go_next();
        ParseFunc_t infix_rule = rules[parser.prev.type].infix_rule;
        infix_rule(can_assign);
    }

    if (can_assign && match(TOKEN_EQUAL)) {
        report_error(&parser.prev, "Invalid assignment");
    }
}

static void add_local(Token_t token) {
    if (cur_compiler->local_cnt + 1 > cur_compiler->local_cap) {
        int old_capacity = cur_compiler->local_cnt;
        cur_compiler->local_cap = grow_capacity(old_capacity);
        cur_compiler->locals =
            resize(cur_compiler->locals, sizeof(Local_t), cur_compiler->local_cap);
    }
    Local_t *local = &cur_compiler->locals[cur_compiler->local_cnt++];
    local->name = token;
    local->depth = -1;
}

static bool identifiers_equals(Token_t *a, Token_t *b) {
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

static void declare_let() {
    // this func does decl for locals so exit if we are declaring global
    if (cur_compiler->scope_depth == 0) {
        return;
    }
    Token_t name = parser.prev;
    for (int i = cur_compiler->local_cnt; i > 0; i--) {
        Local_t *local = &cur_compiler->locals[i];
        if (local->depth != -1 && local->depth < cur_compiler->scope_depth) {
            // out of scope of current block
            break;
        }
        if (identifiers_equals(&name, &local->name)) {
            report_error(&parser.prev, "Variable has already been declared");
        }
    }
    add_local(parser.prev);
}

static int parse_let(const char *msg) {
    // parse variable and add constant byte to chunk
    consume(TOKEN_IDENTIFIER, msg);
    declare_let();
    if (cur_compiler->scope_depth > 0) {
        // exit if we are in local scope
        return 0;
    }
    // global variable declaration
    return add_constant(get_cur_chunk(),
                        DECL_OBJ_VAL(allocate_str(parser.prev.start, parser.prev.length)));
}

static void literal(bool can_assign) {
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

static void number(bool can_assign) {
    double val = strtod(parser.prev.start, NULL);
    write_constant(get_cur_chunk(), DECL_NUM_VAL(val), parser.prev.line);
}

static void grouping(bool can_assign) {
    expression();
    consume(TOKEN_CLOSE_PAREN, "Expect ')' after expression");
}

static void unary(bool can_assign) {
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

static void binary(bool can_assign) {
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

static bool check(TokenType_t type) {
    return parser.cur.type == type;
}

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
