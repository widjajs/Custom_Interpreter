#include "../includes/scanner.h"

Scanner_t scanner;

static bool at_end();
static bool is_digit(char c);
static bool is_alpha(char c);
static TokenType_t get_identifier_type();
static TokenType_t check_keyword(int start, int length, const char *rest, TokenType_t type);
static Token_t init_token(TokenType_t type);
static Token_t init_error_token(const char *err_msg);
static char peek();
static char peek_next();
static char consume();

void init_scanner(const char *file) {
    scanner.start = file;
    scanner.cur = file;
    scanner.line = 1;
}

Token_t scan_token() {
    while (true) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t') {
            consume();
        } else if (c == '\n') {
            scanner.line++;
            consume();
        } else if (c == '/' && peek_next() == '/') {
            while (peek() != '\n' && !at_end()) {
                consume();
            }
        } else {
            break;
        }
    }
    scanner.start = scanner.cur;
    if (at_end()) {
        return init_token(TOKEN_END_FILE);
    }

    char c = consume();

    // handle numbers
    if (is_digit(c) || (c == '.' && is_digit(peek_next()))) {
        while (is_digit(peek())) {
            consume();
        }

        // handle decimals
        if (peek() == '.' && is_digit(peek_next())) {
            consume(); // consume decimal
            while (is_digit(peek())) {
                consume();
            }
        }
        return init_token(TOKEN_NUM);
    }

    // handle identifiers
    if (is_alpha(c)) {
        while (is_alpha(peek()) || is_digit(peek())) {
            consume();
        }
        return init_token(get_identifier_type());
    }
    switch (c) {
        case '(':
            return init_token(TOKEN_OPEN_PAREN);
        case ')':
            return init_token(TOKEN_CLOSE_PAREN);
        case '{':
            return init_token(TOKEN_OPEN_CURLY);
        case '}':
            return init_token(TOKEN_CLOSE_CURLY);
        case ',':
            return init_token(TOKEN_COMMA);
        case '.':
            return init_token(TOKEN_DOT);
        case '-':
            return init_token(TOKEN_SUB);
        case '+':
            return init_token(TOKEN_ADD);
        case '*':
            return init_token(TOKEN_MUL);
        case '/':
            return init_token(TOKEN_DIV);
        case ';':
            return init_token(TOKEN_SEMICOLON);
        case '!':
            return init_token(check_next('=') ? TOKEN_NOT_EQUAL : TOKEN_NOT);
        case '=':
            return init_token(check_next('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': {
            if (check_next('<')) {
                return init_token(TOKEN_LEFT_SHIFT);
            }
            return init_token(check_next('=') ? TOKEN_LESS_THAN_EQUAL : TOKEN_LESS_THAN);
        }
        case '>': {
            if (check_next('>'))
                return init_token(TOKEN_RIGHT_SHIFT);
            return init_token(check_next('=') ? TOKEN_GREATER_THAN_EQUAL : TOKEN_GREATER_THAN);
        }
        case '"': {
            while (!at_end() && peek() != '"') {
                if (peek() == '\n') {
                    scanner.line++;
                }
                consume();
            }
            if (at_end()) {
                return init_error_token("Error: Unclosed string");
            }
            consume(); // closing quote
            return init_token(TOKEN_STR);
        }
    }

    return init_error_token("Unexpected token");
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static char peek() {
    return *scanner.cur;
}

static char peek_next() {
    if (at_end())
        return '\0';
    return *(scanner.cur + 1);
}

static char consume() {
    return *scanner.cur++;
}

static TokenType_t get_identifier_type() {
    // check if it's a keyword first
    switch (*scanner.start) {
        case 'a':
            return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'c':
            return check_keyword(1, 4, "lass", TOKEN_CLASS);
        case 'e':
            return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'i':
            return check_keyword(1, 1, "f", TOKEN_IF);
        case 'n':
            return check_keyword(1, 3, "one", TOKEN_NONE);
        case 'o':
            return check_keyword(1, 1, "r", TOKEN_OR);
        case 'l':
            return check_keyword(1, 2, "et", TOKEN_LET);
        case 'p':
            return check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 'r':
            return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 's':
            return check_keyword(1, 4, "uper", TOKEN_SUPER);
        case 'w':
            return check_keyword(1, 4, "hile", TOKEN_WHILE);
        case 'f': {
            if (scanner.cur - scanner.start > 1) {
                switch (*(scanner.start + 1)) {
                    case 'a':
                        return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return check_keyword(2, 3, "unc", TOKEN_FUNC);
                }
            }
            break;
        }
        case 't': {
            if (scanner.cur - scanner.start > 1) {
                switch (*(scanner.start + 1)) {
                    case 'h':
                        return check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r':
                        return check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        }
    }
    return TOKEN_IDENTIFIER;
}

static TokenType_t check_keyword(int start, int length, const char *rest, TokenType_t type) {
    if (scanner.cur - scanner.start == start + length &&
        strncmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static Token_t init_token(TokenType_t type) {
    Token_t new_token;
    new_token.type = type;
    new_token.start = scanner.start;
    new_token.length = scanner.cur - scanner.start;
    new_token.line = scanner.line;
    return new_token;
}

static Token_t init_error_token(const char *err_msg) {
    Token_t err_token;
    err_token.type = TOKEN_ERROR;
    err_token.start = scanner.start;
    err_token.length = strlen(err_msg);
    err_token.line = scanner.line;
    return err_token;
}

static bool at_end() {
    return *scanner.cur == '\0';
}

bool check_next(const char expected) {
    if (!at_end() && *scanner.cur == expected) {
        scanner.cur++;
        return true;
    }
    return false;
}
