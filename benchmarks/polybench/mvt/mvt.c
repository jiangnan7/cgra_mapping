#include <stdio.h>

volatile int * N;
volatile int *a;
volatile int *b;
int *c;
int *x2;
int **A;
int *y_2;

int main() {
    int i,j;
    int n = *N;

    int sum = 0;
    for (j = 1; j < n-1; j++) {
    //DFGLoop: loop
        x2[i] = x2[i] + A[j][i] * y_2[j];
    }

    return sum;
}
