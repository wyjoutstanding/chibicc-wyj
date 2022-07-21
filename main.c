#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    // input arguments error check
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    // output X86-64 ASM code
    printf("  .global main\n");
    printf("main:\n");
    printf("  mov $%d, %%rax\n", atoi(argv[1]));
    printf("  ret\n");
    return 0;
}
