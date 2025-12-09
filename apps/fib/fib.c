#define USE_MAIN
#include "uvm32_target.h"

void write(int n) {
    printdec(n);
    println("");
}

// print n terms of fibonacci series
void printFib(int n) {
    int prev1 = 1;
    int prev2 = 0;

    for (int i = 1; i <= n; i++) {
        if (i > 2) {
            int curr = prev1 + prev2;
            prev2 = prev1;
            prev1 = curr;
            write(curr);
        } else if (i == 1) {
            write(prev2);
        } else if (i == 2) {
            write(prev1);
        }
    }
}

// print n terms of fibonacci series, recursively
void fib_recursive(int n, int prev1, int prev2) {
    if (n < 3) {
        return;
    }
    
    int curr = prev1 + prev2;
    write(curr);
  
    return fib_recursive(n - 1, prev2, curr);
}

void printFibRec(int n) {
    write(0);
    write(1);
    fib_recursive(n, 0, 1);
}

void main(void) {
    int n = 40;
    println("fib() loop");
    printFib(n);
    println("fib() recursive");
    printFibRec(n);
}

