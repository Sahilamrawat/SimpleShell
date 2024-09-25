#include <stdio.h>
#include <stdlib.h>

/*
 * No changes are allowed in this file
 */
int fib(int n) {
  if(n < 2) return n;
  else return fib(n-1) + fib(n-2);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please provide a number as a command-line argument.\n");
        return 1;
    }

    int n = atoi(argv[1]);  // Convert the first argument to an integer

    int val = fib(n);
    printf("Answer is '%d'\n", val);

    return 0;
}
