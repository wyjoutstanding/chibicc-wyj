/**
 * @file main.c
 * @author wuyangjun (wuyangjun21@163.com)
 * @brief the control program: input(String) -> tokenize(Token) -> parse(AST) -> codegen(ASM)
 * @version 0.1
 * @date 2022-07-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "chibicc_wyj.h"

int main(int argc, char **argv) {
    // input arguments error check
    if (argc != 2) {
        error("%s: invalid number of arguments\n", argv[0]);
        return 1;
    }
    
    // tokenize input string
    Token *tok = tokenize(argv[1]);
    
    // parse token into syntax tree
    Function *func = parse(tok);
    
    // generate asm code from syntax tree
    codegen(func);
    
    return 0;
}
