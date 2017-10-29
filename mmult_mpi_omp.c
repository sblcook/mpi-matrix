#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#define min(x, y) ((x)<(y)?(x):(y))

double* gen_matrix(int n, int m);
int mmult(double *c, double *a, int aRows, int aCols, double *b, int bRows, int bCols);
void compare_matrix(double *a, double *b, int nRows, int nCols);

/** 
    Program to multiply a matrix times a matrix using both
    mpi to distribute the computation among nodes and omp
    to distribute the computation among threads.
*/

int main(int argc, char* argv[])
{
  FILE* fp1;
  FILE* fp2;
  fp1 = fopen(argv[1], "r");
  fp2 = fopen(argv[2], "r");

  char m1dims[100];
  char m2dims[100];

  fgets(m1dims, 100, fp1);
  fgets(m2dims, 100, fp2);

  printf("%s", m1dims);
  //printf("%s", m2dims);

  int m1cols;
  int m1rows;
  int m2cols;
  int m2rows;
//  char rows[20];
//  char cols[20];

  char *rows;
  char *cols;
  char *ptr;

  rows = strstr(m1dims, "rows("); //move to beginning of rows
  cols = strstr(m1dims, "cols(");
  rows = rows + (sizeof(char) * 5);//offset to account for 'rows('
  cols = cols + (sizeof(char) * 5);
  m1rows = (int)strtol(rows, &ptr, 10);//read the rows
  m1cols = (int)strtol(cols, &ptr, 10);//read the cols

  rows = strstr(m2dims, "rows("); //move to beginning of rows
  cols = strstr(m2dims, "cols(");
  rows = rows + (sizeof(char) * 5);//offset to account for 'rows('
  cols = cols + (sizeof(char) * 5);
  m2rows = (int)strtol(rows, &ptr, 10);//read the rows
  m2cols = (int)strtol(cols, &ptr, 10);//read the cols

  printf("%d %d %d %d\n", m1rows, m1cols, m2rows, m2cols);
  
  int nrows, ncols;
  double *aa;	/* the A matrix */
  double *bb;	/* the B matrix */
  double *cc1;	/* A x B computed using the omp-mpi code you write */
  double *cc2;	/* A x B computed using the conventional algorithm */
  int myid, numprocs;
  double starttime, endtime;
  MPI_Status status;
  /* insert other global variables here */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  if (argc > 1) {
    nrows = atoi(argv[1]);
    ncols = nrows;
    if (myid == 0) {
      // Master Code goes here
      aa = gen_matrix(nrows, ncols);
      bb = gen_matrix(ncols, nrows);
      cc1 = malloc(sizeof(double) * nrows * nrows); 
      starttime = MPI_Wtime();
      /* Insert your master code here to store the product into cc1 */
      endtime = MPI_Wtime();
      printf("%f\n",(endtime - starttime));
      cc2  = malloc(sizeof(double) * nrows * nrows);
      mmult(cc2, aa, nrows, ncols, bb, ncols, nrows);
      compare_matrices(cc2, cc1, nrows, nrows);
    } else {
      // Slave Code goes here
    }
  } else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }
  MPI_Finalize();
  return 0;
}
