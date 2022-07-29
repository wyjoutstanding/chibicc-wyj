/**
 * @file chibicc_wyj.h
 * @author wuyangjun (wuyangjun21@163.com)
 * @brief common data structures, utils, apis
 * @version 0.1
 * @date 2022-07-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

//
// Tokenize
//

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


//
// Parse: constructing abstract syntax tree(AST) from tokens using LL(1) and recursing down implementation
//

typedef enum {
    /* arithmetic operators */
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NEG, // unary -
    /* comparison operators */
    ND_EQ,  // ==
    ND_NE,  // !=
    ND_LT,  // <
    ND_LE,  // <=
    ND_GT,  // >
    ND_GE,  // >=
    /* integer */
    ND_NUM  // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node
{
    NodeKind kind;  // Node type
    int value;      // Number literal
    Node *lhs;      // Left-Hand Side
    Node *rhs;      // Right-Hand Side
};


//
// API
//

// key api
Token *tokenize(char *p);
Node *parse(Token *tok);
void codegen(Node *node);

// utils
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *s);