#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <string.h>
#define min(x, y) ((x)<(y)?(x):(y))

double* populate_matrix(int m, int n, FILE* fp);
int mmult(double *c, double *a, int aRows, int aCols, double *b, int bRows, int bCols);
void compare_matrix(double *a, double *b, int nRows, int nCols);

/** 
    Program to multiply a matrix times a matrix using both
    mpi to distribute the computation among nodes and omp
    to distribute the computation among threads.
*/

int main(int argc, char* argv[])
{

  int nrows, ncols;
  double *aa;	/* the A matrix */
  double *bb;	/* the B matrix */
  double *cc1;	/* A x B computed using the omp-mpi code you write */
  double *cc2;	/* A x B computed using the conventional algorithm */
  int myid, numprocs;
  double starttime, endtime;
  MPI_Status status;
  /* insert other global variables here */ 
  int m1cols, m1rows, m2cols, m2rows;
  int i,j,success;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  FILE* fp1, *fp2;
  fp1 = fopen(argv[1], "r");
  fp2 = fopen(argv[2], "r");
 
  success = fscanf(fp1, "rows(%d) cols(%d)", &m1rows, &m1cols);
  success = fscanf(fp2, "rows(%d) cols(%d)", &m2rows, &m2cols);

  //check that matrix dimensions work  
  if(m1cols != m2rows){
    fprintf(stderr, "Invalid matrix dimensions\n");
    exit(1);
  }

   if (argc > 1) {
    if (myid == 0) {
     

      //fill a and b from file input
      aa = populate_matrix(m1rows, m1cols, fp1);
      bb = populate_matrix(m2rows, m2cols, fp2);
      
      //send data to slaves
      MPI_Bcast(bb, nrows * ncols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      for(i = 1; i < numprocs; i++) {
        //MPI_Send(bb, m2rows*m2cols, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
        MPI_Send(&aa[(i-1)*m1rows], m1cols, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
      }

      //receive data from slaves

      cc1 = malloc(sizeof(double) * m1rows * m1cols); 
      starttime = MPI_Wtime();
      /* Insert your master code here to store the product into cc1 */
      endtime = MPI_Wtime();
      printf("%f\n",(endtime - starttime));
      cc2  = malloc(sizeof(double) * m2rows * m2cols);
      mmult(cc2, aa, nrows, ncols, bb, ncols, nrows);
      compare_matrices(cc2, cc1, nrows, nrows);

      fclose(fp1);
      fclose(fp2);
    } else {
      // Slave Code goes here
      bb = malloc(sizeof(double) * m2rows * m2cols);
      //MPI_Recv(bb, m2rows*m2cols, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status);
      MPI_Bcast(bb, m2rows*m2cols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      
      if(m1rows <= myid) {
        while(1) {      
          double input[m1cols];
          double output[m2cols];
      
          MPI_Recv(input, m1cols, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status);
          for (i=0; i < m2cols; i++){
            for(j=0; j < m1cols; j++) {
              output[i] = output[i] + input[j] * bb[j*m2cols + i];
            }
            printf("output %f\n", output[i]);
          }
          MPI_Send(output, m2cols, MPI_DOUBLE, 1, myid, MPI_COMM_WORLD);
        }
      free(bb);
  } else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }
  MPI_Finalize();
  return 0;
}

double* populate_matrix(int m, int n, FILE* fp){
  int i,j;
  char buffer[11 * sizeof(double)];
  double* matrix = malloc(sizeof(double) * m * n);
  int success;

  for (i=0; i < n; i++){
    for(j=0; j < m; j++){ 
      success = fscanf(fp, "%s", buffer);
      matrix[i*m + j] = atof(buffer);
   }
  }
  return matrix;
}



