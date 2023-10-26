/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* jacobi-1d.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#define POLYBENCH_USE_SCALAR_LB
#include "polybench.h"

#define DATA_TYPE_IS_INT
#define MEDIUM_DATASET
/* Include benchmark-specific header. */
#include "jacobi-1d.h"

  /* Retrieve problem size. */
  int n = N;
  int tsteps = TSTEPS;

  /* Variable declaration/allocation. */
  // POLYBENCH_1D_ARRAY_DECL(A, DATA_TYPE, N, n);
  // POLYBENCH_1D_ARRAY_DECL(B, DATA_TYPE, N, n);
  int A[N],B[N];

/* Array initialization. */
static
void init_array ()
{
  int i;

  for (i = 0; i < n; i++)
      {
	A[i] = ((DATA_TYPE) i+ 2) / n;
	B[i] = ((DATA_TYPE) i+ 3) / n;
      }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array()

{
  int i;

  POLYBENCH_DUMP_START;
  POLYBENCH_DUMP_BEGIN("A");
  for (i = 0; i < n; i++)
    {
      if (i % 20 == 0) fprintf(POLYBENCH_DUMP_TARGET, "\n");
      fprintf(POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, A[i]);
    }
  POLYBENCH_DUMP_END("A");
  POLYBENCH_DUMP_FINISH;
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel()
{
  int t, i;

#pragma scop
  for (t = 0; t < _PB_TSTEPS; t++)
    {
      for (i = 1; i < _PB_N - 1; i++)
      {
        please_map_me();
	      B[i] = (A[i-1] + A[i] + A[i + 1])>>1;
      }/*
      for (i = 1; i < _PB_N - 1; i++)
      {
        //please_map_me();
	      A[i] = (B[i-1] + B[i] + B[i + 1])>>1;
      }*/
    }
#pragma endscop

}


int main(int argc, char** argv)
{

  /* Initialize array(s). */
  init_array ();

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel();

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array());

  /* Be clean. */
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);

  return 0;
}
