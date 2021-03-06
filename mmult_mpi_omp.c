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
void print(double *matrix, int m, int n);
void writeToFile(double* matrix, int m, int n, char* fileName);

/** 
    Program to multiply a matrix times a matrix using both
    mpi to distribute the computation among nodes and omp
    to distribute the computation among threads.

    To run this code:
    1. Compile it using the makefile found with this project
    2. Run it using mpiexec
	a. For Temple specific instructions: 
	   i. SSH into cis-linux2.temple.edu
          ii. SSH into osprey
         iii. SSH into chinook
          iv. Compile with makefile
           v. Ensure that the 'hosts' file is set up correctly
          vi. Ensure that the matrix input files are set up like:
              rows(m) cols(n)
              a b c d
              For an m by n matrix, with spaces between numbers
          vi. Execute program with the following syntax:
              'mpiexec -f ~/hosts -n X ./mmult_mpi_omp $input1 $input2'
              where X is the number of processes, and $input1, $input2 are the matrix input files
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
  int m1cols, m1rows, m2cols, m2rows;
  int i,j,k,success,row,source;
  int rowsSent = 0;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  //open the two files with matrices
  FILE* fp1, *fp2;
  fp1 = fopen(argv[1], "r");
  fp2 = fopen(argv[2], "r");
 
  //read matrix dimensions from the files
  success = fscanf(fp1, "rows(%d) cols(%d)", &m1rows, &m1cols);
  success = fscanf(fp2, "rows(%d) cols(%d)", &m2rows, &m2cols);
 
  //check that matrix dimensions are ok, exit if not ok
  if(m1cols != m2rows){
    fprintf(stderr, "Invalid matrix dimensions\n");
    exit(1);
  }

   if (argc > 1) {
    if (myid == 0) {//I am the mighty master
      //fill a and b from file input
      aa = populate_matrix(m1rows, m1cols, fp1);
      bb = populate_matrix(m2rows, m2cols, fp2);
      
      //start timer
      starttime = MPI_Wtime(); 

      //send data to slaves
      //broadcast bb matrix
      MPI_Bcast(bb, m2rows*m2cols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      //send a row of A to each proccess
      for(i = 0; i < min(m1rows, numprocs-1); i++) {
        MPI_Send(&aa[(i)*m1rows], m1cols, MPI_DOUBLE, i+1, i+1, MPI_COMM_WORLD);
	rowsSent++;
      }

      //receive info back from slaves
      cc1 = malloc(sizeof(double) * m1rows * m2cols);
      double* receiveBuffer = malloc(sizeof(double) * m2cols);

      //listening for results from any process that has finished
      for(i = 0; i< m1rows; i++){ 
        MPI_Recv(receiveBuffer, m2cols, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        source = status.MPI_SOURCE;
        row = status.MPI_TAG;

        //fill in cc1 with data received from slave
        for(k=0; k< m2cols; k++) {
          cc1[(row-1)*m2cols + k] = receiveBuffer[k];
        }

        //theres more work to do, send it to slave
        if(rowsSent < m1rows) {
          MPI_Send(&aa[rowsSent*m1rows], m1cols, MPI_DOUBLE, source, rowsSent+1,MPI_COMM_WORLD);
          rowsSent++;
        }
	//no more work to do, tell slave to stop
        else {
          MPI_Send(MPI_BOTTOM, 0, MPI_DOUBLE, source, 0, MPI_COMM_WORLD);
        }
      }

      //end timer and calculate elapsed time
      endtime = MPI_Wtime();
      printf("time elapsed: %f\n",(endtime - starttime));

      //allocate cc2, compute it given a,b, and dimensions, and then compare the mpi vs non-nompi results
      cc2  = malloc(sizeof(double) * m2rows * m2cols);
      mmult(cc2, aa, m1rows, m1cols, bb, m2cols, m2rows);
      compare_matrices(cc2, cc1, m1rows, m2cols);

      //print and write resulting matrix to a file
      print(cc1, m1rows, m2cols);
      writeToFile(cc1, m1rows, m2cols, "output.txt");

      //close input file pointers
      fclose(fp1);
      fclose(fp2);

    } else {//I am a slave, stuck doing computations

      //allocate space for matrix bb and receive broadcast from master
      bb = malloc(sizeof(double) * m2rows * m2cols);
      MPI_Bcast(bb, m2rows*m2cols, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      if(myid <= m1rows) {//if there are more proccesses than rows, extra ones immediately exit
        while(1) {
          double input[m1cols];
          double output[m2cols];
          
	  //listening for work to do
          MPI_Recv(&input, m1cols, MPI_DOUBLE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
          if(status.MPI_TAG == 0){ //check if we get the 'quit' signal
            break;
          }
	  //calulate resultant vector from input vector times matrix b
          row = status.MPI_TAG;
          for (i=0; i < m2cols; i++){
	    double sum = 0;
            for(j=0; j < m1cols; j++) {
              sum = sum + (input[j] * bb[j*m2cols + i]);
            }
	    output[i] = sum;
          }
	  //send results to master
          MPI_Send(output, m2cols, MPI_DOUBLE, 0, row, MPI_COMM_WORLD);
        }
      }
      
    }
  }else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }

  MPI_Finalize();
  return 0;
}

double* populate_matrix(int m, int n, FILE* fp){
/*
returns a row-major order m by n matrix read from file pointer fp
*/
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

void print(double* matrix, int m, int n){
/*
prints the given m by n matrix with appropriate spacing and line breaks
*/
  int i,j;

  for(i = 0; i < m; i++){
    for(j = 0; j < n; j++){
      printf("%f ", matrix[i*n + j]);
    }
    printf("\n");
  }
}


void writeToFile(double* matrix, int m, int n, char* fileName){
/*
This functions writes the m by n matrix 'matrix' to the filename specified
*/
  int i,j;
  FILE* fp;
  fp = fopen(fileName, "w");

  for(i = 0; i < m; i++){
    for(j = 0; j < n; j++){
      fprintf(fp, "%f ", matrix[i*n + j]);
    }
    fprintf(fp, "\n");
  }

  fclose(fp);
}

