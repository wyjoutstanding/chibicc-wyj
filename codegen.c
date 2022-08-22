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

static void gen_addr(Node *node) {
    if (node->kind != ND_VAR)
        error("[gen_addr] expected variable: lvalue");

    int offset = node->lvar->offset;
    printf("  lea %d(%%rbp), %%rax\n", offset);
}

// calculate offset of local variables
static void gen_lvar_offset(Function *func) {
    int offset = 0;
    for (Variable *pv=func->locals; pv; pv=pv->next) {
        offset += 8;
        pv->offset -= offset;
    }
    func->stacksize = offset;
}

static int label_count() {
    static int cnt = 0;
    return cnt++;
}

void gen_expr(Node *node) {
    // simplify comparision generator code
    static char *cmp_asm_names[] = {"sete", "setne", "setl", "setle", "setg", "setge"};
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
        case ND_VAR:
            gen_addr(node);
            printf("  mov (%%rax), %%rax\n");
            return;
        case ND_ASSIGN:
            // Firstly, using %eax to push all left variable address into stack, then %eax only save the rightmost number value
            // e.g. a=b=c=3 => (1) push a; push b; push c; (2) %rax=3, %rdi=c; %rax=3, %rdi=b; %rax=3, %rdi=a
            gen_addr(node->lhs);
            push();
            gen_expr(node->rhs);
            pop("%rdi");
            printf("  mov %%rax, (%%rdi)\n");
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

void gen_stmt(Node *node) {
    if (node->kind == ND_EXPR_STMT) {
        gen_expr(node->lhs);
        return;
    }

    if (node->kind == ND_RETURN) {
        gen_expr(node->lhs);
        printf("  jmp .L.RETURN\n");
        return;
    }

    if (node->kind == ND_BLOCK) {
        for (Node *n = node->body; n; n = n->next) {
            gen_stmt(n);
        }
        return;
    }

    if (node->kind == ND_IF) {
        int lcnt = label_count();
        gen_expr(node->cond);
        printf("  cmp $0, %%rax\n");
        printf("  je .L.ELSE.%d\n", lcnt);
        gen_stmt(node->then);
        printf("  jmp .L.END.%d\n", lcnt);
        printf(".L.ELSE.%d:\n", lcnt);
        if (node->els) {
            gen_stmt(node->els);
        }
        printf(".L.END.%d:\n", lcnt);
        return;
    }

    if (node->kind == ND_FOR) {
        int lcnt = label_count();
        gen_stmt(node->init);
        printf(".L.BEGIN.%d:\n", lcnt);
        // consider condition is null / not null, this why the condition don't use `expr_stmt` directly.
        // "for" "(" expr_stmt expr?; expr? ")" stmt
        if (node->cond != NULL) {
            gen_expr(node->cond);
            printf("  cmp $0, %%rax\n");
            printf("  je .L.END.%d\n", lcnt);
        }
        gen_stmt(node->then);
        if (node->inc != NULL)
            gen_expr(node->inc);
        printf("  jmp .L.BEGIN.%d\n", lcnt);
        printf(".L.END.%d:\n", lcnt);
        return;
    }

    error("Invalid statement: node->kind=%d", node->kind);
}

void codegen(Function *func) {
    gen_lvar_offset(func);

    printf("  .global main\n");
    printf("main:\n");

    // allocate stack for local variables
    printf("  push %%rbp\n");
    printf("  mov %%rsp, %%rbp\n");
    printf("  sub $%d, %%rsp\n", func->stacksize);

    gen_stmt(func->body);

    // restore stack
    printf(".L.RETURN:\n");
    printf("  mov %%rbp, %%rsp\n");
    printf("  pop %%rbp\n");
    printf("  ret\n");
}