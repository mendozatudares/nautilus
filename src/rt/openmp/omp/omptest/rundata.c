#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gomptestdata.h"

#define MAXN 5100  /* Max value of N */

//static int N;  /* Matrix size */
static int procs;  /* Number of processors to use */

/* Matrices and vectors */
//static volatile float A[MAXN][MAXN], B[MAXN], X[MAXN];
//static volatile float ORA[MAXN][MAXN], ORB[MAXN], ORX[MAXN];

//static float **ORB, **ORA;
static float *B, **A, *X, *ORX;
/* A * X = B, solve for X */

static int seed;
/* Prototype */
static void gauss();  /* The function you will provide.
		* It is this routine that is timed.
		* It is called only on the parent.
		*/

/* Initialize A and B (and X to 0.0s) */
static void initialize_inputs() {
  int row, col;
  //ORA = malloc(N * sizeof(float *));
  A = malloc (N * sizeof(float *));
  for(int i = 0; i < N ; i++ ){
    //ORA[i] = malloc(N * sizeof(float));
  A[i] = malloc(N * sizeof(float));
  }

  //ORB = malloc(N * sizeof(float));
  B = malloc(N * sizeof(float));
  ORX = malloc(N * sizeof(float));
  X = malloc(N * sizeof(float));
 
  /* printf("\nInitializing...\n"); */
  /* //   #pragma omp parallel num_threads(8) */
  /* { */
  /* //  #pragma omp for private(row,col) schedule(static,1) nowait */
  /*   for (col = 0; col < N; col++) { */
   
  /*   for (row = 0; row < N; row++) { */
  /*     ORA[col][row] = (float) rand()/32768.0; */
  /*   } */
  /*   ORB[col] = (float)rand()/32768.0; */
  /* } */
  /* } */
}


static void reset_inputs(){
  int row, col;
  printf("\n reseting...\n");
  for (col = 0; col < N; col++) {
    for (row = 0; row < N; row++) {
      A[row][col] = ORA[row][col];
    }
    B[col] = ORB[col];
    X[col] = 0.0;
  }

}

static void print_state() {
  int row, col;

  printf("A=%p, B=%p, X=%p\n", A, B, X);
  
  if (N < 1000) {
    printf("\nA =\n\t");
    for (row = 0; row < N; row++) {
      for (col = 0; col < N; col++) {
	printf("%f%s  ", A[row][col], (col < N-1) ? ", " : ";\n\t");
      }
    }
    printf("\nB = [");
    for (col = 0; col < N; col++) {
      printf("%5.2f%s", B[col], (col < N-1) ? "; " : "]\n");
    }
    printf("\nX = [");
    for (col = 0; col < N; col++) {
      printf("%f%s  ", X[col], (col < N-1) ? "; " : "]\n");
    }
  }
}

static void  serialgauss(){
  int norm, row, col;  /* Normalization row, and zeroing
			* element row and col */
  float multiplier;
  
  printf("Computing serially.\n");

  /* Gaussian elimination */
  
  for (norm = 0; norm < N - 1; norm++) {
    
    // int num = N - norm;
    

    {

      for (row = norm + 1; row < N; row++) {

        multiplier = A[row][norm] / A[norm][norm];

        for (col = norm; col < N; col++) {
            A[row][col] -= A[norm][col] * multiplier;
        }
        B[row] -= B[norm] * multiplier;
      }
      printf("Print iteration norm %d .\n", norm);
      print_state();
  }
  }
  /* (Diagonal elements are not normalized to 1.  This is treated in back
   * substitution.)
   */
  /* Back substitution */
  for (row = N - 1; row >= 0; row--) {
    X[row] = B[row];
    for (col = N-1; col > row; col--) {
      X[row] -= A[row][col] * X[col];
    }
    X[row] /= A[row][row];
    //printf("%5.2f ", X[row]);
  }

}

static void ompgauss() {
  int norm, row, col;  /* Normalization row, and zeroing
			* element row and col */
  float multiplier;
  //doneflag[0] = 1;
  
  printf("Computing using omp.\n");

  /* Gaussian elimination */
  
#pragma omp parallel private(row, col, multiplier, norm) num_threads(procs)
	{
		for (norm = 0; norm < N - 1; norm++) {
                       #pragma omp for schedule(static,1)
			for (row = norm + 1; row < N; row++) {
			  
			  multiplier = A[row][norm]/A[norm][norm];
				for (col = norm; col < N; col++) {
					A[row][col] -= A[norm][col] * multiplier;
				}
				B[row] -= B[norm] * multiplier;
			}
		}
	}
  printf("I am done\n");
  /* (Diagonal elements are not normalized to 1.  This is treated in back
   * substitution.)
   */
  /* Back substitution */
  for (row = N - 1; row >= 0; row--) {
    X[row] = B[row];
    for (col = N-1; col > row; col--) {
      X[row] -= A[row][col] * X[col];
    }
    X[row] /= A[row][row];
  }
}

#define MYTIME() (double)(clock())/(double)(CLOCKS_PER_SEC)
int main (int argc, char *argv[])
{
    int seed, size, np;
    if(argc<3){  
      printf("Please input seed, nprocs.\n");
      return -1;
    }
    seed = atoi(argv[1]);
    // size = atoi(argv[2]);
    np =atoi(argv[2]);
    srand(seed);
    procs = np;
    printf("seed %d, size, %d, nprocs: %d\n", seed, N, procs);
    
    initialize_inputs();
    reset_inputs();
    printf("state before openmp run\n");
  
 
    double start = MYTIME();
    //ompgauss();
    double  end = MYTIME();
    printf("state after openmp run\n");
    //print_state();
    double  omp = end-start;
    printf("openmp done %lf\n", omp);
    float OMP[N];
    for(int row =0; row<N; row++){
      OMP[row] = X[row];
    }

    reset_inputs();
    printf("state before serial run\n");
    start = MYTIME();
    serialgauss();
    end = MYTIME();
    printf("state after serial run\n");
    //print_state();
    double serial = end-start; 
    printf("serial done %lf\n", serial);
    float difference = 0.0;
    for(int row =0; row<N; row++){
      difference += (OMP[row]- X[row]);
    }

    printf("OMP difference %f speed up %f !\n", difference, serial/omp);
    return 0;

}



