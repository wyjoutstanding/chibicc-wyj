/**
 * @file tokenize.c
 * @author wuyangjun (wuyangjun21@163.com)
 * @brief tokenize input string and support error location
 * @version 0.1
 * @date 2022-07-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "chibicc_wyj.h"

// Input string
static char *current_input;

// Debug
static void dump_token(Token *tok) {
    fprintf(stderr, "kind = %d, value = %d, loc = %p, len = %d, next = %p\n", 
            tok->kind, tok->value, tok->loc, tok->len, tok->next);
}

// Report a error and exit
void error(char *fmt, ...) {
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

void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...) {
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

// get value from number token
static int getNumber(Token *tok) {
    if (tok->kind != TK_NUM) {
        error_tok(tok, "expected a number");
    }
    return tok->value;
}

// compare token with operator 
bool equal(Token *tok, char *op) {
    return (strlen(op) == tok->len) && (memcmp(tok->loc, op, tok->len) == 0);
}

// skip a token
Token *skip(Token *tok, char *s) {
    if (!equal(tok, s)) {
        error_tok(tok, "expected '%s'", s);
    }
    return tok->next;
}


/**
 * @brief split input into a list of tokens
 * 
 * @param p pointer of input string
 * @return Token* 
 */
Token *tokenize(char *p) {
    // save input string into global pointer
    current_input = p;
    Token head = {};
    Token *cur = &head;

    while (*p) {
        // white space
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p >= 'a' && *p <= 'z') {
            cur->next = new_token(TK_IDENT, p, p+1);
            cur = cur->next;
            p = p + 1;
        }
        else if (ispunct(*p)) {
            if ((*p == '=' && *(p+1) == '=') ||
                (*p == '!' && *(p+1) == '=') || 
                (*p == '<' && *(p+1) == '=') ||
                (*p == '>' && *(p+1) == '=')) {
               cur->next = new_token(TK_PUNCT, p, p+2);
               cur = cur->next;
               p = p + 2;
               continue;
            }
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
