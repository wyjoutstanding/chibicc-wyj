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

static Node *new_node(NodeKind kind, Token *tok) {
    Node *node = (Node *)calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_binary_node(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary_node(NodeKind kind, Node *lhs, Token *tok) {
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    return node;
}

static Node *new_num_node(int val, Token *tok) {
    Node *node = new_node(ND_NUM, tok);
    node->value = 8;    // sizeof(dtype)
    return node;
}

// new a variable node
static Node *new_var_node(char *name, Variable *lvar, Token *tok) {
    Node *node = new_node(ND_VAR, tok);
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
//              | "for" "(" expr_stmt expr?; expr? ")" stmt
// compound_stmt = stmt* "}"
// expr_stmt  = expr? ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary   = ("+" | "-" | "&" | "*")? primary
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
//              | "for" "(" expr_stmt expr?; expr? ")" stmt
//              | "while" "(" expr ")" stmt
Node *stmt(Token **rest, Token *tok) {
    if (equal(tok, "return")) {
        Node *node = new_unary_node(ND_RETURN, expr(&tok->next, tok->next), tok->next);
        *rest = skip(tok->next, ";");
        return node;
    }

    if (equal(tok, "{")) {
        Node *node = new_node(ND_BLOCK, tok->next);
        node->body = compound_stmt(&tok->next, tok->next);
        *rest = tok->next;
        return node;
    }

    if (equal(tok, "if")) {
        tok = skip(tok->next, "(");
        Node *node = new_node(ND_IF, tok);
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

    if (equal(tok, "for")) {
        tok = skip(tok->next, "(");
        Node *node = new_node(ND_FOR, tok);
        // init;
        node->init = expr_stmt(&tok, tok);
        // cond;
        if (!equal(tok, ";"))
            node->cond = expr(&tok, tok);
        tok = skip(tok, ";");
        // inc
        if (!equal(tok, ")"))
            node->inc = expr(&tok, tok);
        tok = skip(tok, ")");
        // then statements
        node->then = stmt(&tok, tok);
        *rest = tok;
        return node;
    }

    if (equal(tok, "while")) {
        tok = skip(tok->next, "(");
        Node *node = new_node(ND_FOR, tok);
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
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
        return new_node(ND_BLOCK, tok->next);
    }
    Node *node = new_unary_node(ND_EXPR_STMT, expr(&tok, tok), tok);
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
        node = new_binary_node(ND_ASSIGN, node, assign(&tok, tok->next), tok->next);
    }
    *rest = tok;
    return node;
}

// equality   = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
    Node *node = relational(&tok, tok);

    for (;;) {
        if (equal(tok, "==")) {
            node = new_binary_node(ND_EQ, node, relational(&tok, tok->next), tok->next);
            continue;
        }
        if (equal(tok, "!=")) {
            node = new_binary_node(ND_NE, node, relational(&tok, tok->next), tok->next);
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
            node = new_binary_node(ND_LT, node, add(&tok, tok->next), tok->next);
            continue;
        }
        if (equal(tok, "<=")) {
            node = new_binary_node(ND_LE, node, add(&tok, tok->next), tok->next);
            continue;
        }
        if (equal(tok, ">")) {
            node = new_binary_node(ND_GT, node, add(&tok, tok->next), tok->next);
            continue;
        }
        if (equal(tok, ">=")) {
            node = new_binary_node(ND_GE, node, add(&tok, tok->next), tok->next);
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
            Node *rnode = mul(&tok, tok->next);

            // update node type
            add_type(node);
            add_type(rnode);

            // normal case: num + num
            if (is_integer(node) && is_integer(rnode)) {
                node = new_binary_node(ND_ADD, node, rnode, tok);
                continue;
            }

            if (node->ty->base && rnode->ty->base) {
                error_tok(tok, "pointer + pointer is invalid [%s:%d]", __FILE__, __LINE__);
            }
            
            // pointer case: num + pointer => pointer + num
            if (rnode->ty->base) {
                Node *tmp = rnode;
                rnode = node;
                node = tmp;
            }
            // construct mul node, which inserted to scale pointer offset unit
            rnode = new_binary_node(ND_MUL, rnode, new_num_node(8, tok), tok);
            node = new_binary_node(ND_ADD, node, rnode, tok);
            continue;
        }
        
        if (equal(tok, "-")) {
            Node *rnode = mul(&tok, tok->next);

            // update node type
            add_type(node);
            add_type(rnode);

            // normal case: num - num
            if (is_integer(node) && is_integer(rnode)) {
                node = new_binary_node(ND_SUB, node, rnode, tok->next);
                continue;
            }

            // construct num node used to scale pointer unit
            Node *num_node = new_num_node(8, tok);

            // pointer case: pointer - pointer
            if (node->ty->base && rnode->ty->base) {
                node = new_binary_node(ND_SUB, node, rnode, tok);
                node = new_binary_node(ND_DIV, node, num_node, tok);
                // must explicit set type, otherwise this will infered as TY_PTR
                node->ty = ty_int;
                continue;
            }

            // pointer case: pointer - num
            if (node->ty->base) {
                rnode = new_binary_node(ND_MUL, rnode, num_node, tok);
                node = new_binary_node(ND_SUB, node, rnode, tok);
                continue;
            }

            error_tok(tok, "num - pointer is invalid [%s:%d]", __FILE__, __LINE__);
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
            node = new_binary_node(ND_MUL, node, unary(&tok, tok->next), tok->next);
            continue;
        }
        
        if (equal(tok, "/")) {
            node = new_binary_node(ND_DIV, node, unary(&tok, tok->next), tok->next);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}


// unary   = ("+" | "-" | "&" | "*")? primary
static Node *unary(Token **rest, Token *tok) {
    Node *node = NULL;
    if (equal(tok, "+")) {
        node = unary(&tok, tok->next);
        *rest = tok;
        return node;
    }

    if (equal(tok, "-")) {
        node = new_unary_node(ND_NEG, unary(&tok, tok->next), tok->next);
        *rest = tok;
        return node;
    }

    if (equal(tok, "&")) {
        node = new_unary_node(ND_ADDR, unary(&tok, tok->next), tok->next);
        // node->ty->base = true;
        add_type(node);
        *rest = tok;
        return node;
    }

    if (equal(tok, "*")) {
        node = new_unary_node(ND_DEREF, unary(&tok, tok->next), tok->next);
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
        Node *node = new_node(ND_NUM, tok);
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
        Node *node = new_var_node(tok->loc, lvar, tok);
        *rest = tok->next;
        return node;
    }

    if (equal(tok, "(")) {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    error_tok(tok, "expected an expression [%s:%d]", __FILE__, __LINE__);
}

Function *parse(Token *tok) {
    // wrapper of Function
    Function *func = malloc(sizeof(Function));
    func->body = stmt(&tok, tok);
    // `locals` must after `body` calculated
    func->locals = locals;
    return func;
}