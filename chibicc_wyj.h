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
#ifndef CHIBICC_WYJ
#define CHIBICC_WYJ

// need for `strndup`
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct Type Type;
// type kind
typedef enum {
    TY_INT,     // int
    TY_PTR      // pointer
} TypeKind;

struct Type {
    TypeKind kind;  // type kind
    Type *base;     // if (kind == TY_PTR) current pointer point to the type of base
};

//
// Tokenize
//

typedef enum {
    TK_KEYWORD, // keywords
    TK_IDENT,   // identifiers
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
typedef struct Node Node;
typedef struct Variable Variable;
typedef struct Function Function;

typedef enum {
    /* arithmetic operators */
    ND_ADD,         // +
    ND_SUB,         // -
    ND_MUL,         // *
    ND_DIV,         // /
    ND_NEG,         // unary -
    /* comparison operators */
    ND_EQ,          // ==
    ND_NE,          // !=
    ND_LT,          // <
    ND_LE,          // <=
    ND_GT,          // >
    ND_GE,          // >=
    /* others */
    ND_VAR,         // variable
    ND_ADDR,        // unary &
    ND_DEREF,       // unary *
    ND_ASSIGN,      // =
    ND_NUM,         // Integer
    /* statement */
    ND_FOR,         // for / while statement
    ND_IF,          // if statement
    ND_BLOCK,       // {...}: block statement
    ND_RETURN,      // keyword: return statement
    ND_EXPR_STMT,   // expression statement
} NodeKind;

// AST node type
struct Node {
    NodeKind kind;  // Node type
    int value;      // Number literal (if kind == ND_NUM)
    Token *tok;     // Representative token, which shows error message better
    Node *lhs;      // Left-Hand Side
    Node *rhs;      // Right-Hand Side
    Node *next;     // Next stmt
    Type *ty;       // Node type system
    // variable
    char *name;     // Variable name (if kind == ND_VAR)
    Variable *lvar; // Variable info (if kind == ND_VAR) 
    // block statement
    Node *body;     // Block body, need by BLOCK Statement {...}
    // if / for / while statement
    Node *cond;     // condition
    Node *then;     // then statements
    Node *els;      // else statements
    Node *init;     // for init statement
    Node *inc;      // for increment statement
};

// Variable
struct Variable {
    char *name;     // variable name
    int offset;       // displacement used by stack
    Variable *next; // used by list
};

// Functions
struct Function {
    Variable *locals;   // local variable
    Node *body;         // stmt body
    int stacksize;      // stack size
};


//
// API
//

// key api
Token *tokenize(char *p);
Function *parse(Token *tok);
void codegen(Function *func);

// utils
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *s);

// type
extern Type *ty_int;
bool is_integer(Node *node);
void add_type(Node *node);

#endif