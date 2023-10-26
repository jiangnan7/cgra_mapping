#include <stdio.h>

volatile int * N;
volatile int *a;
volatile int *b;
int *c;
int **tmp;
volatile int **A;
volatile int **B;

int main() {
    int i;
    int j;
    int k;
    int alpha = 10;
    int n = *N;

    int sum = 0;

 
    for (k = 1; k < n-1; ++k){
        //DFGLoop: loop
        sum += alpha * A[i][k] * B[k][j];
        sum += alpha * A[i][k+1] * B[k+1][j];
        sum += alpha * A[i][k+2] * B[k+2][j];
        sum += alpha * A[i][k+3] * B[k+3][j];

        sum += A[i][k+3];
    }

    // for (i = 1; i < n-1; i++) {
    
    //     c[i] *= a[i+1] + b[i-1];
    //     sum += c[i];
    // }


    return sum;
}
