PGMS=mmult matrix_times_vector hello pi mxv_omp_mpi

all:	${PGMS}

mmult:	mmult.c
	gcc -o mmult -fopenmp -O3 mmult.c

matrix_times_vector:	matrix_times_vector.c
	mpicc -O3 -o matrix_times_vector matrix_times_vector.c

hello:	hello.c
	mpicc -O3 -o hello hello.c

pi:	pi.c
	mpicc -O3 -o pi pi.c

mxv_omp_mpi:	mxv_omp_mpi.c
	mpicc -fopenmp -O3 -o mvx_omp_mpi mxv_omp_mpi.c

clean:
	rm -f ${PGMS}

