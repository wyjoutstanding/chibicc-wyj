#include "chibicc_wyj.h"

// global variable
Type *ty_int = &(Type){TY_INT, NULL};

bool is_integer(Node *node) {
    return node->ty->kind == TY_INT;
}

// new a pointer type which points base type
static Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    return ty;
}

// add type from node in AST, every statement is a independent AST
void add_type(Node *node) {
    // node is null or node already assigned type
    if (!node || node->ty) {
        return;
    }
    // recursive add type
    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->next);
    add_type(node->init);
    add_type(node->inc);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    for (Node *n=node->body; n; n=n->next) {
        add_type(n);
    }

    // detail rules of infer type
    switch (node->kind)
    {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_NEG:
    case ND_ASSIGN:
        node->ty = node->lhs->ty;
        return;
    
    case ND_EQ:
    case ND_NE:
    case ND_LE:
    case ND_LT:
    case ND_GE:
    case ND_GT:
    case ND_NUM:
    case ND_VAR:
        node->ty = ty_int;
        return;

    case ND_ADDR:
        node->ty = pointer_to(node->lhs->ty);
        return;
    
    case ND_DEREF:
        if (node->lhs->ty->kind != TY_PTR)
            error_tok(node->tok, "DEREF kind isn't PTR [%s:%d]", __FILE__, __LINE__);
        // point the lhs base type
        node->ty = node->lhs->ty->base;
        return;
    }

}