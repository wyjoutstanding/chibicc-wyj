/**
 * @file codegen.c
 * @author wuyangjun (wuyangjun21@163.com)
 * @brief Code generator: with stack implemetation, just post-traverse the AST
 * @version 0.1
 * @date 2022-07-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "chibicc_wyj.h"

static void push(void) {
    printf("  push %%rax\n");
}

static void pop(char *arg) {
    printf("  pop %s\n", arg);
}

void gen_expr(Node *node) {
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

void codegen(Node *node) {
    printf("  .global main\n");
    printf("main:\n");
    gen_expr(node);
    printf("  ret\n");
}