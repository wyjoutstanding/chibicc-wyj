/**
 * @file parse.c
 * @author wuyangjun (wuyangjun21@163.com)
 * @brief Parse: constructing abstract syntax tree(AST) from tokens using LL(1) and recursing down implementation
 * @version 0.1
 * @date 2022-07-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "chibicc_wyj.h"

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
Node *expr(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

// expr       = equality
Node *expr(Token **rest, Token *tok) {
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

Node *parse(Token *tok) {
    Node *node = expr(&tok, tok);
    if (tok->kind != TK_EOF) {
        error_tok(tok, "extra token");
    }
    return node;
}