/**
 * @file main.c
 * @author wuyangjun (wuyangjun21@163.com)
 * @brief 
 * @version 0.1
 * @date 2022-07-25
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
// Tokenizer
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

// Debug
static void dump_token(Token *tok) {
    fprintf(stderr, "kind = %d, value = %d, loc = %p, len = %d, next = %p\n", 
            tok->kind, tok->value, tok->loc, tok->len, tok->next);
}

// Input string
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
        if (ispunct(*p)) {
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

// skip a token
static Token *skip(Token *tok, char *s) {
    if (!equal(tok, s)) {
        error_tok(tok, "expected '%s'", s);
    }
    return tok->next;
}

//
// Parser: constructing abstract syntax tree(AST) from tokens using LL(1) and recursing down implementation
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

static Node *new_binary_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = (Node *)calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary_node(NodeKind kind, Node *lhs) {
    Node *node = (Node *)calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    return node;
}

// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | "(" expr ")"
static Node *expr(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

// expr       = equality
static Node *expr(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    *rest = tok;
    return node;
}

// equality   = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    for (;;) {
        if (equal(tok, "==")) {
            node = new_binary_node(ND_EQ, node, relational(&tok, tok->next));
            continue;
        }
        if (equal(tok, "!=")) {
            node = new_binary_node(ND_NE, node, relational(&tok, tok->next));
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
    Node *node = add(&tok, tok);

    for (;;) {
        if (equal(tok, "<")) {
            node = new_binary_node(ND_LT, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, "<=")) {
            node = new_binary_node(ND_LE, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, ">")) {
            node = new_binary_node(ND_GT, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, ">=")) {
            node = new_binary_node(ND_GE, node, add(&tok, tok->next));
            continue;
        }
        break;
    }

    *rest = tok;
    return node;

}

// add        = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
    Node *node = mul(&tok, tok);

    for (;;) {
        if (equal(tok, "+")) {
            node = new_binary_node(ND_ADD, node, mul(&tok, tok->next));
            continue;
        }
        
        if (equal(tok, "-")) {
            node = new_binary_node(ND_SUB, node, mul(&tok, tok->next));
            continue;
        }
        break;
    }

    *rest = tok;    // update the newest token pointer
    return node;
}

// mul     = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
    Node *node = unary(&tok, tok);

    for (;;) {
        if (equal(tok, "*")) {
            node = new_binary_node(ND_MUL, node, unary(&tok, tok->next));
            continue;
        }
        
        if (equal(tok, "/")) {
            node = new_binary_node(ND_DIV, node, unary(&tok, tok->next));
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}


// unary   = ("+" | "-")? primary
static Node *unary(Token **rest, Token *tok) {
    Node *node = NULL;
    if (equal(tok, "+")) {
        node = unary(&tok, tok->next);
        *rest = tok;
        return node;
    }

    if (equal(tok, "-")) {
        node = new_unary_node(ND_NEG, unary(&tok, tok->next));
        *rest = tok;
        return node;
    }
    
    node = primary(&tok, tok);
    *rest = tok;
    return node;
}

// // unary   = ("+" | "-")? primary
// static Node *unary(Token **rest, Token *tok) {
//     if (equal(tok, "+")) {
//         return unary(rest, tok->next);
//     }

//     if (equal(tok, "-")) {
//         return new_unary_node(ND_NEG, unary(rest, tok->next));
//     }
    
//     return primary(rest, tok);
// }

// primary  := num | "(" expr ")" 
static Node *primary(Token **rest, Token *tok) {
    if (tok->kind == TK_NUM) {
        Node *node = (Node *)calloc(1, sizeof(Node));
        node->kind = ND_NUM;
        node->value = tok->value;
        *rest = tok->next;
        return node;
    }

    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    error_tok(tok, "expected an expression");
}

// 
// Code generator: with stack implemetation, just post-traverse the AST
//

static void push(void) {
    printf("  push %%rax\n");
}

static void pop(char *arg) {
    printf("  pop %s\n", arg);
}

static void gen_expr(Node *node) {
    // simplify comparision generator code
    static char *cmp_asm_names[] = { "sete", "setne", "setl", "setle", "setg", "setge"};
    #define CMP_ASM_NAME(x) cmp_asm_names[(x) - ND_EQ]

    switch (node->kind)
    {
        case ND_NUM:
            printf("  mov $%d, %%rax\n", node->value);
            return;
        case ND_NEG:
            gen_expr(node->lhs);
            printf("  neg %%rax\n");
            return;
    }

    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");
    
    // detail calculation
    switch (node->kind)
    {
        /* arithmetic operators */
        case ND_ADD:
            printf("  add %%rdi, %%rax\n");
            break;
        case ND_SUB:
            printf("  sub %%rdi, %%rax\n");
            break;
        case ND_MUL:
            printf("  imul %%rdi, %%rax\n");
            break;
        case ND_DIV:
            printf("  cqo\n");  // RDX:RAX:= sign-extend of RAX
            printf("  idiv %%rdi\n");
            break;

        /* comparison operators */
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_GT:
        case ND_GE:
            printf("  cmp %%rdi, %%rax\n");     // rax - rdi
            printf("  %s %%al\n", CMP_ASM_NAME(node->kind));
            printf("  movzb %%al, %%rax\n");    // zero extend
            break;
        
        /* error handle */
        default:
            error("unexpected node kind '%s'", node->kind);
            break;
    }
    return;
}

int main(int argc, char **argv) {
    // input arguments error check
    if (argc != 2) {
        error("%s: invalid number of arguments\n", argv[0]);
        return 1;
    }
    
    // save input string into global pointer
    current_input = argv[1];
    // tokenize input string
    Token *tok = tokenize(argv[1]);
    
    // parse token into syntax tree
    Node *node = expr(&tok, tok);
    
    // generate asm code from syntax tree
    printf("  .global main\n");
    printf("main:\n");
    gen_expr(node);
    printf("  ret\n");
    
    return 0;
}
