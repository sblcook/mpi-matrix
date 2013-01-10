#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
int main (int argc, char* argv[]) {
  int myid, numprocs;
  char me[255];
  int n;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  gethostname(me, 254);
  printf("Hello from %s I am process %d of %d\n", me, myid, numprocs);
  if (myid == 0) {
    n = 12345;
  }
  printf("Call to MPI_Bcast n==%d on %s myid=%d\n", n, me, myid);
  MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
  printf("Return from MPI_Bcast n==%d on %s myid=%d\n", n, me, myid);
  MPI_Finalize();
  return 0;
}

