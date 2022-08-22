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

// local variables list in function
static Variable *locals;

static Node *new_node(NodeKind kind) {
    Node *node = (Node *)calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_binary_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary_node(NodeKind kind, Node *lhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    return node;
}

// new a variable node
static Node *new_var_node(char *name, Variable *lvar) {
    Node *node = new_node(ND_VAR);
    node->name = name;
    node->lvar = lvar;  // local variable
    return node;
}

// new a local variable info
static Variable *new_lvar(char *name, int len) {
    Variable *lvar = (Variable *)calloc(1, sizeof(Variable));
    // copy into new buffer: _POSIX_C_SOURCE >= 200809L
    lvar->name = strndup(name, len);
    // update global `locals`
    lvar->next = locals;
    locals = lvar;
    return lvar;
}

// find whether `name` exists in `locals`
static Variable *find_local_variable(char *name, int len) {
    // traverse all local variables
    for (Variable *pv = locals; pv != NULL; pv = pv->next) {
        if (len == strlen(pv->name) && strncmp(name, pv->name, len) == 0) {
            return pv;
        }
    }
    // unexist
    return NULL;
}

// stmt       = "return" expr ";" | expr_stmt | "{" compound_stmt
//              | "if" "(" expr ")" stmt ("else" stmt)?
// compound_stmt = stmt* "}"
// expr_stmt  = expr? ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | ident | "(" expr ")"
Node *stmt(Token **rest, Token *tok);
Node *compound_stmt(Token **rest, Token *tok);
Node *expr_stmt(Token **rest, Token *tok);
Node *expr(Token **rest, Token *tok);
Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

// stmt       = "return" expr ";" | expr_stmt | "{" compound_stmt
//              | "if" "(" expr ")" stmt ("else" stmt)?
Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "return")) {
        Node *node = new_unary_node(ND_RETURN, expr(&tok->next, tok->next));
        *rest = skip(tok->next, ";");
        return node;
    }

    if (equal(tok, "{")) {
        Node *node = new_node(ND_BLOCK);
        node->body = compound_stmt(&tok->next, tok->next);
        *rest = tok->next;
        return node;
    }

    if (equal(tok, "if")) {
        tok = skip(tok->next, "(");
        Node *node = new_node(ND_IF);
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else")) {
            tok = tok->next;
            node->els = stmt(&tok, tok);
        }
        *rest = tok;
        return node;
    }

    Node *node = expr_stmt(&tok, tok);
    *rest = tok;
    return node;
}

// compound_stmt = stmt* "}"
Node *compound_stmt(Token **rest, Token *tok) {
    Node head = {};
    Node *cur = &head;
    while (!equal(tok, "}")) {
        cur->next = stmt(&tok, tok);
        cur = cur->next;
    }
    *rest = skip(tok, "}");
    return head.next;
}

// expr_stmt  = expr? ";"
Node *expr_stmt(Token **rest, Token *tok) {
    if (equal(tok, ";")) {
        *rest = skip(tok, ";");
        return new_node(ND_BLOCK);
    }
    Node *node = new_unary_node(ND_EXPR_STMT, expr(&tok, tok));
    *rest = skip(tok, ";");
    return node;
}

// expr       = assign
Node *expr(Token **rest, Token *tok) {
    Node *node = assign(&tok, tok);
    *rest = tok;
    return node;
}

// assign     = equality ("=" assign)?
Node *assign(Token **rest, Token *tok) {
    Node *node = equality(&tok, tok);
    if (equal(tok, "=")) {
        node = new_binary_node(ND_ASSIGN, node, assign(&tok, tok->next));
    }
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

// primary    = num | ident | "(" expr ")"
static Node *primary(Token **rest, Token *tok) {
    if (tok->kind == TK_NUM) {
        Node *node = new_node(ND_NUM);
        node->value = tok->value;
        *rest = tok->next;
        return node;
    }

    if (tok->kind == TK_IDENT) {
        Variable *lvar = find_local_variable(tok->loc, tok->len);
        if (!lvar) {
            // new local variable
            lvar = new_lvar(tok->loc, tok->len);
        }
        Node *node = new_var_node(tok->loc, lvar);
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

Function *parse(Token *tok) {
    // wrapper of Function
    Function *func = malloc(sizeof(Function));
    func->body = stmt(&tok, tok);
    // `locals` must after `body` calculated
    func->locals = locals;
    return func;
}