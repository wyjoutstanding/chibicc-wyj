#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

typedef enum {
    TK_PUNCT,   // punctuators
    TK_NUM,     // numeric literals
    TK_EOF      // end-of-file maker
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
    TokenKind kind; // token type
    int value;      // if (kind == TK_NUM) literal value of it's
    char *loc;      // token location of start
    int len;        // token length
    Token *next;    // next token
};

// Debug
static void dump_token(Token *tok) {
    fprintf(stderr, "kind = %d, value = %d, loc = %p, len = %d, next = %p\n", 
            tok->kind, tok->value, tok->loc, tok->len, tok->next);
}

// input string
static char *current_input;

// Report a error and exit
static void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// print error masseage and detail location, then exit
static void verror_at(char *loc, char *fmt, va_list ap) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, " ");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

static void error_tok(Token *tok, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

// Create a new token with calloc
static Token *new_token(TokenKind kind, char *start, char *end) {
    Token *tok = (Token *)calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    return tok;
}

/**
 * @brief split input into a list of tokens
 * 
 * @param p pointer of input string
 * @return Token* 
 */
static Token *tokenize(char *p) {
    Token head = {};
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }
        if (*p == '+' || *p == '-') {
            cur->next = new_token(TK_PUNCT, p, p+1);
            cur = cur->next;
            p++;
        } else if (isdigit(*p)) {
            cur->next = new_token(TK_NUM, p, p);
            char *p_pre = p;
            cur->next->value = strtol(p, &p, 10);
            cur->next->len = p - p_pre;
            cur = cur->next;
        }
        else {
            error_at(p, "invalid token");
        }
    }
    cur->next = new_token(TK_EOF, p, p);
    return head.next;
}

// get value from number token
static int getNumber(Token *tok) {
    if (tok->kind != TK_NUM) {
        error_tok(tok, "expected a number");
    }
    return tok->value;
}

// compare token with operator 
static bool equal(Token *tok, char *op) {
    return (strlen(op) == tok->len) && (memcmp(tok->loc, op, tok->len) == 0);
}

int main(int argc, char **argv) {
    // input arguments error check
    if (argc != 2) {
        error("%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    // output X86-64 ASM code
    printf("  .global main\n");
    printf("main:\n");
    
    // save input string into global pointer
    current_input = argv[1];
    // parse input string
    Token *tok = tokenize(argv[1]);
    // first token must be number
    printf("  mov $%d, %%rax\n", getNumber(tok));
    tok = tok->next;

    while (tok != NULL && tok->kind != TK_EOF) {
        if (equal(tok, "+")) {
            printf("  add $%d, %%rax\n", getNumber(tok->next));
            tok = tok->next->next;
            continue;
        }

        if (equal(tok, "-")) {
            printf("  sub $%d, %%rax\n", getNumber(tok->next));
            tok = tok->next->next;
            continue;
        }
        
        // error handler
        error_tok(tok, "expected token '+' or '-'");
        return 1;
    }

    // return the result of the input expression
    printf("  ret\n");
    return 0;
}
