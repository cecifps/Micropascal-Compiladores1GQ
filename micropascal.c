#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ================================================================
   Micro-Pascal - Primeira parte de Construção de Compiladores
   Lexer + Parser (descida recursiva)
   C11, sem Flex/Bison.
   ================================================================ */

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENT,
    TOKEN_INTEGER,
    TOKEN_REAL,
    TOKEN_CHAR,

    TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE, TOKEN_EQ, TOKEN_NE,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MUL, TOKEN_DIV,
    TOKEN_DIV_INT, TOKEN_AND, TOKEN_OR, TOKEN_NOT,
    TOKEN_ASSIGN,

    TOKEN_RPAREN, TOKEN_LPAREN, TOKEN_COMMA, TOKEN_SEMI,
    TOKEN_DOT, TOKEN_COLON,

    TOKEN_PROGRAM, TOKEN_IF, TOKEN_THEN, TOKEN_ELSE,
    TOKEN_WHILE, TOKEN_DO, TOKEN_REPEAT, TOKEN_UNTIL,
    TOKEN_INTEGER_TYPE, TOKEN_REAL_TYPE, TOKEN_CHAR_TYPE,
    TOKEN_BEGIN, TOKEN_END, TOKEN_WRITE, TOKEN_VAR
} TokenType;

typedef struct {
    TokenType type;
    size_t start;
    size_t length;
    int line;
    int column;
} Token;

typedef struct {
    char *source;
    size_t length;
    size_t pos;
    int line;
    int column;
    Token current;
    int had_error;
} Lexer;

typedef struct {
    Lexer lexer;
    int had_error;
} Parser;

static const char *token_names[] = {
    "EOF", "IDENTIFICADOR", "INTEIRO_LITERAL", "REAL_LITERAL", "CHAR_LITERAL",
    "<", ">", "<=", ">=", "=", "<>",
    "+", "-", "*", "/", "div", "and", "or", "not", ":=",
    ")", "(", ",", ";", ".", ":",
    "program", "if", "then", "else", "while", "do", "repeat", "until",
    "integer", "real", "char", "begin", "end", "write", "var"
};

typedef struct {
    const char *word;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {
    {"program", TOKEN_PROGRAM}, {"if", TOKEN_IF}, {"then", TOKEN_THEN},
    {"else", TOKEN_ELSE}, {"while", TOKEN_WHILE}, {"do", TOKEN_DO},
    {"repeat", TOKEN_REPEAT}, {"until", TOKEN_UNTIL},
    {"integer", TOKEN_INTEGER_TYPE}, {"real", TOKEN_REAL_TYPE},
    {"char", TOKEN_CHAR_TYPE}, {"begin", TOKEN_BEGIN}, {"end", TOKEN_END},
    {"write", TOKEN_WRITE}, {"var", TOKEN_VAR},
    {"div", TOKEN_DIV_INT}, {"and", TOKEN_AND}, {"or", TOKEN_OR},
    {"not", TOKEN_NOT}
};

static void lexer_advance(Lexer *lx) {
    if (lx->pos >= lx->length) return;
    char c = lx->source[lx->pos++];
    if (c == '\n') {
        lx->line++;
        lx->column = 1;
    } else {
        lx->column++;
    }
}

static char lexer_peek(const Lexer *lx) {
    if (lx->pos >= lx->length) return '\0';
    return lx->source[lx->pos];
}

static char lexer_peek_next(const Lexer *lx) {
    if (lx->pos + 1 >= lx->length) return '\0';
    return lx->source[lx->pos + 1];
}

static int is_letter(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static void lexer_error(Lexer *lx, const char *lexeme) {
    fprintf(stderr, "Erro léxico no caracter [%s]\n", lexeme);
    lx->had_error = 1;
}

static Token make_token(Lexer *lx, TokenType type, size_t start, int line, int column) {
    Token t;
    t.type = type;
    t.start = start;
    t.length = lx->pos - start;
    t.line = line;
    t.column = column;
    return t;
}

static TokenType keyword_type(const char *text, size_t len) {
    size_t n = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < n; i++) {
        if (strlen(keywords[i].word) == len && strncmp(text, keywords[i].word, len) == 0)
            return keywords[i].type;
    }
    return TOKEN_IDENT;
}

static Token lexer_next(Lexer *lx) {
    while (1) {
        char c = lexer_peek(lx);
        if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
            lexer_advance(lx);
            continue;
        }
        /* Os exemplos do enunciado usam comentários //, então eles são ignorados. */
        if (c == '/' && lexer_peek_next(lx) == '/') {
            while (lexer_peek(lx) != '\0' && lexer_peek(lx) != '\n') lexer_advance(lx);
            continue;
        }
        break;
    }

    size_t start = lx->pos;
    int line = lx->line;
    int column = lx->column;
    char c = lexer_peek(lx);

    if (c == '\0') return make_token(lx, TOKEN_EOF, start, line, column);

    if (is_letter(c)) {
        lexer_advance(lx);
        while (is_letter(lexer_peek(lx)) || is_digit(lexer_peek(lx))) lexer_advance(lx);
        TokenType type = keyword_type(lx->source + start, lx->pos - start);
        return make_token(lx, type, start, line, column);
    }

    if (is_digit(c) || (c == '.' && is_digit(lexer_peek_next(lx)))) {
        if (c == '.') {
            lexer_advance(lx);
            while (is_digit(lexer_peek(lx))) lexer_advance(lx);
            return make_token(lx, TOKEN_REAL, start, line, column);
        }
        while (is_digit(lexer_peek(lx))) lexer_advance(lx);
        if (lexer_peek(lx) == '.' && is_digit(lexer_peek_next(lx))) {
            lexer_advance(lx);
            while (is_digit(lexer_peek(lx))) lexer_advance(lx);
            return make_token(lx, TOKEN_REAL, start, line, column);
        }
        return make_token(lx, TOKEN_INTEGER, start, line, column);
    }

    if (c == '\'') {
        lexer_advance(lx);
        char x = lexer_peek(lx);
        int valid = 0;
        if (is_letter(x) || is_digit(x)) {
            valid = 1;
            lexer_advance(lx);
        } else if (x == '\\' && (lexer_peek_next(lx) == 'n' || lexer_peek_next(lx) == 't')) {
            valid = 1;
            lexer_advance(lx);
            lexer_advance(lx);
        }
        if (valid && lexer_peek(lx) == '\'') {
            lexer_advance(lx);
            return make_token(lx, TOKEN_CHAR, start, line, column);
        }
        if (lexer_peek(lx) == '\0') {
            lexer_error(lx, "EOF");
        } else {
            char bad[2] = {lexer_peek(lx), '\0'};
            lexer_error(lx, bad);
            lexer_advance(lx);
        }
        return make_token(lx, TOKEN_EOF, lx->pos, line, column);
    }

    switch (c) {
        case '<':
            lexer_advance(lx);
            if (lexer_peek(lx) == '=') { lexer_advance(lx); return make_token(lx, TOKEN_LE, start, line, column); }
            if (lexer_peek(lx) == '>') { lexer_advance(lx); return make_token(lx, TOKEN_NE, start, line, column); }
            return make_token(lx, TOKEN_LT, start, line, column);
        case '>':
            lexer_advance(lx);
            if (lexer_peek(lx) == '=') { lexer_advance(lx); return make_token(lx, TOKEN_GE, start, line, column); }
            return make_token(lx, TOKEN_GT, start, line, column);
        case ':':
            lexer_advance(lx);
            if (lexer_peek(lx) == '=') { lexer_advance(lx); return make_token(lx, TOKEN_ASSIGN, start, line, column); }
            return make_token(lx, TOKEN_COLON, start, line, column);
        case '+': lexer_advance(lx); return make_token(lx, TOKEN_PLUS, start, line, column);
        case '-': lexer_advance(lx); return make_token(lx, TOKEN_MINUS, start, line, column);
        case '*': lexer_advance(lx); return make_token(lx, TOKEN_MUL, start, line, column);
        case '/': lexer_advance(lx); return make_token(lx, TOKEN_DIV, start, line, column);
        case '=': lexer_advance(lx); return make_token(lx, TOKEN_EQ, start, line, column);
        case ')': lexer_advance(lx); return make_token(lx, TOKEN_RPAREN, start, line, column);
        case '(': lexer_advance(lx); return make_token(lx, TOKEN_LPAREN, start, line, column);
        case ',': lexer_advance(lx); return make_token(lx, TOKEN_COMMA, start, line, column);
        case ';': lexer_advance(lx); return make_token(lx, TOKEN_SEMI, start, line, column);
        case '.': lexer_advance(lx); return make_token(lx, TOKEN_DOT, start, line, column);
        default: {
            char bad[2] = {c, '\0'};
            lexer_error(lx, bad);
            lexer_advance(lx);
            return make_token(lx, TOKEN_EOF, lx->pos, line, column);
        }
    }
}

static char *token_lexeme(const Lexer *lx, Token t) {
    char *s = (char *)malloc(t.length + 1);
    if (!s) exit(EXIT_FAILURE);
    memcpy(s, lx->source + t.start, t.length);
    s[t.length] = '\0';
    return s;
}

static void parser_syntax_error(Parser *p) {
    char *lexeme = token_lexeme(&p->lexer, p->lexer.current);
    fprintf(stderr, "Erro de sintaxe no token [%s]\n", lexeme);
    free(lexeme);
    p->had_error = 1;
}

static void parser_advance(Parser *p) {
    p->lexer.current = lexer_next(&p->lexer);
    if (p->lexer.had_error) p->had_error = 1;
}

static int accept(Parser *p, TokenType type) {
    if (p->lexer.current.type == type) {
        parser_advance(p);
        return 1;
    }
    return 0;
}

static int expect(Parser *p, TokenType type) {
    if (p->lexer.current.type == type) {
        parser_advance(p);
        return 1;
    }
    parser_syntax_error(p);
    return 0;
}

static void expression(Parser *p);
static void command(Parser *p);

static void primary(Parser *p) {
    if (p->had_error) return;
    if (accept(p, TOKEN_IDENT) || accept(p, TOKEN_INTEGER) ||
        accept(p, TOKEN_REAL) || accept(p, TOKEN_CHAR)) return;
    if (accept(p, TOKEN_LPAREN)) {
        expression(p);
        expect(p, TOKEN_RPAREN);
        return;
    }
    if (accept(p, TOKEN_NOT)) {
        expression(p);
        return;
    }
    parser_syntax_error(p);
}

static void product(Parser *p) {
    primary(p);
    while (!p->had_error && (p->lexer.current.type == TOKEN_MUL ||
           p->lexer.current.type == TOKEN_DIV || p->lexer.current.type == TOKEN_DIV_INT)) {
        parser_advance(p);
        primary(p);
    }
}

static void sum(Parser *p) {
    product(p);
    while (!p->had_error && (p->lexer.current.type == TOKEN_PLUS || p->lexer.current.type == TOKEN_MINUS)) {
        parser_advance(p);
        product(p);
    }
}

static int is_relational(TokenType type) {
    return type == TOKEN_EQ || type == TOKEN_NE || type == TOKEN_LT || type == TOKEN_GT ||
           type == TOKEN_LE || type == TOKEN_GE;
}

static void comparison(Parser *p) {
    sum(p);
    while (!p->had_error && is_relational(p->lexer.current.type)) {
        parser_advance(p);
        sum(p);
    }
}

static void expression(Parser *p) {
    comparison(p);
    while (!p->had_error && (p->lexer.current.type == TOKEN_AND || p->lexer.current.type == TOKEN_OR)) {
        parser_advance(p);
        comparison(p);
    }
}

static void variable_section(Parser *p) {
    expect(p, TOKEN_VAR);
    while (!p->had_error && p->lexer.current.type == TOKEN_IDENT) {
        expect(p, TOKEN_IDENT);
        while (accept(p, TOKEN_COMMA)) expect(p, TOKEN_IDENT);
        expect(p, TOKEN_COLON);
        if (!(accept(p, TOKEN_INTEGER_TYPE) || accept(p, TOKEN_REAL_TYPE) || accept(p, TOKEN_CHAR_TYPE))) {
            parser_syntax_error(p);
            return;
        }
        expect(p, TOKEN_SEMI);
    }
}

static void block(Parser *p);

static void assignment(Parser *p) {
    expect(p, TOKEN_IDENT);
    expect(p, TOKEN_ASSIGN);
    expression(p);
    expect(p, TOKEN_SEMI);
}

static void writing(Parser *p) {
    expect(p, TOKEN_WRITE);
    expect(p, TOKEN_LPAREN);
    expression(p);
    expect(p, TOKEN_RPAREN);
    expect(p, TOKEN_SEMI);
}

static void iteration(Parser *p) {
    if (accept(p, TOKEN_WHILE)) {
        expression(p);
        expect(p, TOKEN_DO);
        command(p);
        return;
    }
    expect(p, TOKEN_REPEAT);
    command(p);
    expect(p, TOKEN_UNTIL);
    expression(p);
    expect(p, TOKEN_SEMI);
}

static void decision(Parser *p) {
    expect(p, TOKEN_IF);
    expression(p);
    expect(p, TOKEN_THEN);
    command(p);
    if (accept(p, TOKEN_ELSE)) command(p);
}

static void command(Parser *p) {
    switch (p->lexer.current.type) {
        case TOKEN_IDENT: assignment(p); break;
        case TOKEN_BEGIN: block(p); expect(p, TOKEN_SEMI); break;
        case TOKEN_WHILE:
        case TOKEN_REPEAT: iteration(p); break;
        case TOKEN_IF: decision(p); break;
        case TOKEN_WRITE: writing(p); break;
        default: parser_syntax_error(p); break;
    }
}

static void block(Parser *p) {
    expect(p, TOKEN_BEGIN);
    while (!p->had_error && p->lexer.current.type != TOKEN_END && p->lexer.current.type != TOKEN_EOF)
        command(p);
    expect(p, TOKEN_END);
}

static void program(Parser *p) {
    expect(p, TOKEN_PROGRAM);
    expect(p, TOKEN_IDENT);
    expect(p, TOKEN_SEMI);
    variable_section(p);
    block(p);
    expect(p, TOKEN_DOT);
    if (!p->had_error && p->lexer.current.type != TOKEN_EOF) parser_syntax_error(p);
}

static char *read_file(const char *path, size_t *length) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) { fclose(f); return NULL; }
    size_t n = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    buffer[n] = '\0';
    *length = n;
    return buffer;
}

static void print_tokens(Lexer *lx) {
    while (!lx->had_error) {
        Token t = lexer_next(lx);
        char *lexeme = token_lexeme(lx, t);
        printf("%d:%d\t%s\t%s\n", t.line, t.column, token_names[t.type], lexeme);
        free(lexeme);
        if (t.type == TOKEN_EOF) break;
    }
}

static int run_parser(char *source, size_t length) {
    Parser p;
    memset(&p, 0, sizeof(p));
    p.lexer.source = source;
    p.lexer.length = length;
    p.lexer.pos = 0;
    p.lexer.line = 1;
    p.lexer.column = 1;
    parser_advance(&p);
    program(&p);
    if (p.lexer.had_error) return 1;
    if (p.had_error) return 1;
    printf("Análise léxica e sintática concluída com sucesso.\n");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Uso: %s [--tokens] arquivo.pas\n", argv[0]);
        return 1;
    }

    int only_tokens = (argc == 3 && strcmp(argv[1], "--tokens") == 0);
    const char *path = only_tokens ? argv[2] : argv[1];
    if (argc == 3 && !only_tokens) {
        fprintf(stderr, "Uso: %s [--tokens] arquivo.pas\n", argv[0]);
        return 1;
    }

    size_t length = 0;
    char *source = read_file(path, &length);
    if (!source) {
        fprintf(stderr, "Não foi possível abrir o arquivo [%s].\n", path);
        return 1;
    }

    int result;
    if (only_tokens) {
        Lexer lx;
        memset(&lx, 0, sizeof(lx));
        lx.source = source;
        lx.length = length;
        lx.line = 1;
        lx.column = 1;
        print_tokens(&lx);
        result = lx.had_error ? 1 : 0;
    } else {
        result = run_parser(source, length);
    }

    free(source);
    return result;
}
