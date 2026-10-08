#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    EOF_T, IDENT, INTEGER, REAL, CHAR,
    LT, GT, LE, GE, EQ, NE, PLUS, MINUS, MUL, DIV, DIV_INT, AND, OR, NOT, ASSIGN,
    RPAREN, LPAREN, COMMA, SEMI, DOT, COLON,
    PROGRAM, IF, THEN, ELSE, WHILE, DO, REPEAT, UNTIL, INTEGER_T, REAL_T, CHAR_T,
    BEGIN_T, END_T, WRITE, VAR
} Type;

typedef struct { Type type; size_t start, length; int line, column; } Token;
typedef struct {
    char *source; size_t length, pos; int line, column, had_error; Token current;
} Lexer;
typedef struct { Lexer lexer; int had_error; } Parser;
typedef struct { const char *word; Type type; } Keyword;

static const char *names[] = {
    "EOF", "IDENTIFICADOR", "INTEIRO_LITERAL", "REAL_LITERAL", "CHAR_LITERAL",
    "<", ">", "<=", ">=", "=", "<>", "+", "-", "*", "/", "div", "and", "or", "not", ":=",
    ")", "(", ",", ";", ".", ":", "program", "if", "then", "else", "while", "do",
    "repeat", "until", "integer", "real", "char", "begin", "end", "write", "var"
};
static const Keyword keywords[] = {
    {"program",PROGRAM},{"if",IF},{"then",THEN},{"else",ELSE},{"while",WHILE},{"do",DO},
    {"repeat",REPEAT},{"until",UNTIL},{"integer",INTEGER_T},{"real",REAL_T},{"char",CHAR_T},
    {"begin",BEGIN_T},{"end",END_T},{"write",WRITE},{"var",VAR},{"div",DIV_INT},
    {"and",AND},{"or",OR},{"not",NOT}
};

static char peek(Lexer *l) { return l->pos < l->length ? l->source[l->pos] : '\0'; }
static char next_char(Lexer *l) { return l->pos + 1 < l->length ? l->source[l->pos + 1] : '\0'; }
static void advance(Lexer *l) {
    if (l->pos >= l->length) return;
    if (l->source[l->pos++] == '\n') { l->line++; l->column = 1; }
    else l->column++;
}
static int letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static int digit(char c) { return c >= '0' && c <= '9'; }
static void lex_error(Lexer *l, const char *s) {
    fprintf(stderr, "Erro léxico no caracter [%s]\n", s); l->had_error = 1;
}
static Token token(Lexer *l, Type type, size_t start, int line, int col) {
    Token t = {type, start, l->pos - start, line, col}; return t;
}
static Type keyword(const char *s, size_t len) {
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++)
        if (strlen(keywords[i].word) == len && !strncmp(s, keywords[i].word, len))
            return keywords[i].type;
    return IDENT;
}
static Token lex(Lexer *l) {
    char c;
    while (1) {
        c = peek(l);
        if (c == ' ' || c == '\n' || c == '\t' || c == '\r') { advance(l); continue; }
        if (c == '/' && next_char(l) == '/') {
            while (peek(l) && peek(l) != '\n') advance(l);
            continue;
        }
        break;
    }
    size_t start = l->pos; int line = l->line, col = l->column;
    c = peek(l);
    if (!c) {
        if (l->pos < l->length) { lex_error(l, "\\0"); advance(l); }
        return token(l, EOF_T, l->pos, line, col);
    }
    if (letter(c)) {
        advance(l);
        while (letter(peek(l)) || digit(peek(l))) advance(l);
        return token(l, keyword(l->source + start, l->pos - start), start, line, col);
    }
    if (digit(c) || (c == '.' && digit(next_char(l)))) {
        if (c == '.') advance(l);
        while (digit(peek(l))) advance(l);
        if (peek(l) == '.' && digit(next_char(l))) {
            advance(l); while (digit(peek(l))) advance(l);
            return token(l, REAL, start, line, col);
        }
        return token(l, c == '.' ? REAL : INTEGER, start, line, col);
    }
    if (c == '\'') {
        advance(l); c = peek(l); int valid = 0;
        if (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || digit(c))) {
            valid = 1; advance(l);
        } else if (c == '\\' && (next_char(l) == 'n' || next_char(l) == 't')) {
            valid = 1; advance(l); advance(l);
        }
        if (valid && peek(l) == '\'') { advance(l); return token(l, CHAR, start, line, col); }
        if (!peek(l) && l->pos >= l->length) lex_error(l, "EOF");
        else if (!peek(l)) { lex_error(l, "\\0"); advance(l); }
        else { char bad[2] = {peek(l), '\0'}; lex_error(l, bad); advance(l); }
        return token(l, EOF_T, l->pos, line, col);
    }
    switch (c) {
        case '<': advance(l); if (peek(l)=='=') { advance(l); return token(l,LE,start,line,col); }
                  if (peek(l)=='>') { advance(l); return token(l,NE,start,line,col); }
                  return token(l,LT,start,line,col);
        case '>': advance(l); if (peek(l)=='=') { advance(l); return token(l,GE,start,line,col); } return token(l,GT,start,line,col);
        case ':': advance(l); if (peek(l)=='=') { advance(l); return token(l,ASSIGN,start,line,col); } return token(l,COLON,start,line,col);
        case '+': advance(l); return token(l,PLUS,start,line,col);
        case '-': advance(l); return token(l,MINUS,start,line,col);
        case '*': advance(l); return token(l,MUL,start,line,col);
        case '/': advance(l); return token(l,DIV,start,line,col);
        case '=': advance(l); return token(l,EQ,start,line,col);
        case ')': advance(l); return token(l,RPAREN,start,line,col);
        case '(': advance(l); return token(l,LPAREN,start,line,col);
        case ',': advance(l); return token(l,COMMA,start,line,col);
        case ';': advance(l); return token(l,SEMI,start,line,col);
        case '.': advance(l); return token(l,DOT,start,line,col);
        default: { char bad[2] = {c, '\0'}; lex_error(l,bad); advance(l); return token(l,EOF_T,l->pos,line,col); }
    }
}
static char *lexeme(Lexer *l, Token t) {
    char *s = malloc(t.length + 1);
    if (!s) exit(EXIT_FAILURE);
    memcpy(s, l->source + t.start, t.length); s[t.length] = '\0'; return s;
}
static void syntax_error(Parser *p) {
    if (p->lexer.current.type == EOF_T) fprintf(stderr,"Erro de sintaxe no token [EOF]\n");
    else {
        char *s = lexeme(&p->lexer,p->lexer.current);
        fprintf(stderr,"Erro de sintaxe no token [%s]\n",s); free(s);
    }
    p->had_error = 1;
}
static void advance_parser(Parser *p) {
    p->lexer.current = lex(&p->lexer);
    if (p->lexer.had_error) p->had_error = 1;
}
static int accept(Parser *p, Type t) {
    if (p->lexer.current.type != t) return 0;
    advance_parser(p); return 1;
}
static int expect(Parser *p, Type t) {
    if (p->lexer.current.type == t) { advance_parser(p); return 1; }
    syntax_error(p); return 0;
}
static void expression(Parser *p);
static void command(Parser *p);
static void primary(Parser *p) {
    if (p->had_error) return;
    if (accept(p,IDENT) || accept(p,INTEGER) || accept(p,REAL) || accept(p,CHAR)) return;
    if (accept(p,LPAREN)) { expression(p); expect(p,RPAREN); return; }
    if (accept(p,NOT)) { expression(p); return; }
    syntax_error(p);
}
static void product(Parser *p) {
    primary(p);
    while (!p->had_error && (p->lexer.current.type==MUL || p->lexer.current.type==DIV || p->lexer.current.type==DIV_INT)) {
        advance_parser(p); primary(p);
    }
}
static void sum(Parser *p) {
    product(p);
    while (!p->had_error && (p->lexer.current.type==PLUS || p->lexer.current.type==MINUS)) {
        advance_parser(p); product(p);
    }
}
static int relational(Type t) { return t==EQ || t==NE || t==LT || t==GT || t==LE || t==GE; }
static void comparison(Parser *p) {
    sum(p);
    while (!p->had_error && relational(p->lexer.current.type)) { advance_parser(p); sum(p); }
}
static void expression(Parser *p) {
    comparison(p);
    while (!p->had_error && (p->lexer.current.type==AND || p->lexer.current.type==OR)) {
        advance_parser(p); comparison(p);
    }
}
static void variables(Parser *p) {
    expect(p,VAR);
    while (!p->had_error && p->lexer.current.type==IDENT) {
        expect(p,IDENT);
        while (accept(p,COMMA)) expect(p,IDENT);
        expect(p,COLON);
        if (!(accept(p,INTEGER_T) || accept(p,REAL_T) || accept(p,CHAR_T))) { syntax_error(p); return; }
        expect(p,SEMI);
    }
}
static void block(Parser *p);
static void assignment(Parser *p) { expect(p,IDENT); expect(p,ASSIGN); expression(p); expect(p,SEMI); }
static void writing(Parser *p) { expect(p,WRITE); expect(p,LPAREN); expression(p); expect(p,RPAREN); expect(p,SEMI); }
static void iteration(Parser *p) {
    if (accept(p,WHILE)) { expression(p); expect(p,DO); command(p); return; }
    expect(p,REPEAT); command(p); expect(p,UNTIL); expression(p); expect(p,SEMI);
}
static void decision(Parser *p) {
    expect(p,IF); expression(p); expect(p,THEN); command(p);
    if (accept(p,ELSE)) command(p);
}
static void command(Parser *p) {
    switch (p->lexer.current.type) {
        case IDENT: assignment(p); break;
        case BEGIN_T: block(p); expect(p,SEMI); break;
        case WHILE: case REPEAT: iteration(p); break;
        case IF: decision(p); break;
        case WRITE: writing(p); break;
        default: syntax_error(p); break;
    }
}
static void block(Parser *p) {
    expect(p,BEGIN_T);
    while (!p->had_error && p->lexer.current.type!=END_T && p->lexer.current.type!=EOF_T) command(p);
    expect(p,END_T);
}
static void program(Parser *p) {
    expect(p,PROGRAM); expect(p,IDENT); expect(p,SEMI); variables(p); block(p); expect(p,DOT);
    if (!p->had_error && p->lexer.current.type!=EOF_T) syntax_error(p);
}
static char *read_file(const char *path, size_t *length) {
    FILE *f=fopen(path,"rb"); if (!f) return NULL;
    if (fseek(f,0,SEEK_END)) { fclose(f); return NULL; }
    long size=ftell(f); if (size<0) { fclose(f); return NULL; }
    rewind(f); char *s=malloc((size_t)size+1);
    if (!s) { fclose(f); return NULL; }
    size_t n=fread(s,1,(size_t)size,f); fclose(f); s[n]='\0'; *length=n; return s;
}
static void print_tokens(Lexer *l) {
    while (!l->had_error) {
        Token t=lex(l); char *s=lexeme(l,t);
        printf("%d:%d\t%s\t%s\n",t.line,t.column,names[t.type],s); free(s);
        if (t.type==EOF_T) break;
    }
}
static int run_parser(char *source, size_t length) {
    Parser p={0}; p.lexer.source=source; p.lexer.length=length; p.lexer.line=1; p.lexer.column=1;
    advance_parser(&p); program(&p);
    if (p.lexer.had_error || p.had_error) return 1;
    puts("Análise léxica e sintática concluída com sucesso."); return 0;
}
int main(int argc, char **argv) {
    if (argc<2 || argc>3 || (argc==3 && strcmp(argv[1],"--tokens"))) {
        fprintf(stderr,"Uso: %s [--tokens] arquivo.pas\n",argv[0]); return 1;
    }
    int tokens=argc==3; const char *path=argv[tokens?2:1]; size_t length=0;
    char *source=read_file(path,&length);
    if (!source) { fprintf(stderr,"Não foi possível abrir o arquivo [%s].\n",path); return 1; }
    int result;
    if (tokens) {
        Lexer l={0}; l.source=source; l.length=length; l.line=1; l.column=1;
        print_tokens(&l); result=l.had_error ? 1 : 0;
    } else result=run_parser(source,length);
    free(source); return result;
}
