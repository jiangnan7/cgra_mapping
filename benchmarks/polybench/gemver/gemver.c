#include <stdio.h>

volatile int * N;
volatile int *a;
volatile int *b;
volatile int **A;
volatile int *u1;
volatile int *v1;
volatile int *u2;
volatile int *v2;

int *c;

int main() {
    int i,j;
    int n = *N;

    int sum = 0;
    for (j = 1; j < n-1; j++) {
    //DFGLoop: loop
        A[i][j] = A[i][j] + u1[i] * v1[j] + u2[i] * v2[j];

    }

    return sum;
}
