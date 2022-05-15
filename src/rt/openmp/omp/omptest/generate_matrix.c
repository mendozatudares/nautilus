#include <stdio.h>
#include <stdlib.h>

#define MAXN 5100
int N = 0;

int cofactor(int matrix[N][N], int matrix2[N][N], int p, int q, int n) {
   int i = 0, j = 0;
   int row, col;
   // Looping for each element of the matrix
   for (row = 0; row < n; row++) {
      for (col = 0; col < n; col++) {
         // Copying into temporary matrix only
         // those element which are not in given
         // row and column
         if (row != p && col != q) {
            matrix2[i][j++] = matrix[row][col];
            // Row is filled, so increase row
            // index and reset col index
            if (j == n - 1) {
               j = 0;
               i++;
            }
         }
      }
   }
   return 0;
}
/* Recursive function to check if matrix[][] is singular or not. */
int check_singular(int matrix[N][N], int n) {
   int D = 0; // Initialize result
   // Base case : if matrix contains single element
   if (n == 1)
   return matrix[0][0];
   int matrix2[N][N]; // To store cofactors
   int sign = 1; // To store sign multiplier
   // Iterate for each element of first row
   for (int f = 0; f < n; f++) {
      // Getting Cofactor of matrix[0][f]
      cofactor(matrix, matrix2, 0, f, n);
      D += sign * matrix[0][f] * check_singular(matrix2, n - 1);
      // terms are to be added with alternate sign
      sign = -sign;
   }
   return D;
}

int main(int argc, char *args[]){
  // check legal input
  if(argc<3){
    printf("Please specify array size and seed. Abort!\n");
    return -1;
  }
  int seed;
  seed = atoi(args[1]);
  N = atoi(args[2]);
  printf("Generating array with seed=%d, size = %d.\n", seed, N);
  // check array size
  if( N<=1 || N>MAXN){
    printf("N=%d is out of the range of [2,5100]!Abort.\n", N);
    return 1;
  }
  srand(seed);
  // allocate space for arrays
  float **ORA,**A;
  float *ORB,*B;
  ORA = malloc(N * sizeof(float *));
  A = malloc(N * sizeof(float *));
  ORB = malloc(N * sizeof(float));
  B = malloc(N * sizeof(float));
  
  for(int i = 0;i<N;i++){
    ORA[i] = malloc(N * sizeof(float));
    A[i] = malloc(N * sizeof(float));
  }
  int col,row;
  // initialize array with numbers
   for (col = 0; col < N; col++) {
   
    for (row = 0; row < N; row++) {
      ORA[col][row] = (float) rand()/32768.0;
    }
    ORB[col] = (float)rand()/32768.0;
  }

   FILE *fp;
   fp = fopen("gomptestdata.h","w+");
   if(fp){
     //fprintf(fp,"#ifndef _GOMP_TEST_DATA_\n#define _GOMP_TEST_DATA_\n");
     fprintf(fp,"#define N %d\n",N);
     // parse ORA
     char ORAbuf[32];
     snprintf(ORAbuf,32,"static float ORA[%d][%d]=",N,N);
     
     fprintf(fp,"%s",ORAbuf);
     
     
     fprintf(fp,"{\n");
   
     
     for(row = 0;row < N; row++){
       fprintf(fp,"{");
      

       for(col = 0; col < N; col++){
	 if(col == N - 1){
	   fprintf(fp, "%f",ORA[col][row]);
	  
	 }else{
	   fprintf(fp, "%f, ",ORA[col][row]);
	 }
       }

       if(row == N - 1){
	 fprintf(fp, "}\n");
       }else{
	 fprintf(fp, "},\n");
       }
     }
     fprintf(fp, "};\n");


     //ORB

     char ORBbuf[32];
     snprintf(ORBbuf,32,"static float ORB[%d]=",N);
     
     fprintf(fp,"%s",ORBbuf);
     fprintf(fp, "{");
     for(col = 0; col < N; col++){
       if(col == N-1){
	 fprintf(fp,"%f",ORB[col]);
       }else{
	 fprintf(fp,"%f,",ORB[col]);
       }
     }
     fprintf(fp, "};\n");

     //COPA
     snprintf(ORAbuf,32,"static float COPA[%d][%d]=",N,N);
 
     fprintf(fp,"%s",ORAbuf);
     
     
     fprintf(fp,"{\n");
   
     
     for(row = 0;row < N; row++){
       fprintf(fp,"{");
      

       for(col = 0; col < N; col++){
	 if(col == N - 1){
	   fprintf(fp, "%f",ORA[col][row]);
	  
	 }else{
	   fprintf(fp, "%f, ",ORA[col][row]);
	 }
       }

       if(row == N - 1){
	 fprintf(fp, "}\n");
       }else{
	 fprintf(fp, "},\n");
       }
     }
     fprintf(fp, "};\n");


     //COPB
     snprintf(ORBbuf,32,"static float COPB[%d]=",N);
     
     fprintf(fp,"%s",ORBbuf);
     fprintf(fp, "{");
     for(col = 0; col < N; col++){
       if(col == N-1){
	 fprintf(fp,"%f",ORB[col]);
       }else{
	 fprintf(fp,"%f,",ORB[col]);
       }
     }
     fprintf(fp, "};\n");

   }


   
   fclose(fp);
  // parse array

  // write array into file
  
  
    
  

  return 0;

}

static void print_state(){
  


}
