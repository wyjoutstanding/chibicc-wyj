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

    // using `strtol` parse input string to long int
    char *p = argv[1];  // temp pointer for call `strtol`
    printf("  mov $%ld, %%rax\n", strtol(p, &p, 10));
    while (*p) {
        if (*p == '+') {
            p++;
            printf("  add $%ld, %%rax\n", strtol(p, &p, 10));
            continue;
        }

        if (*p == '-') {
            p++;
            printf("  sub $%ld, %%rax\n", strtol(p, &p, 10));
            continue;
        }
        fprintf(stderr, "unexpected character: '%c'\n", *p);
        return 1;
    }

    // return the result of the input expression
    printf("  ret\n");
    return 0;
}
