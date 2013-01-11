int mmult_omp(double *c,
		   double *a, int aRows, int aCols,
		   double *b, int bRows, int bCols){
  int i, j, k;
#pragma omp parallel default(none)	\
  shared(a, b, c, aRows, aCols, bRows, bCols) private(i, k, j)
#pragma omp for
  for (i = 0; i < aRows; i++) {
    for (j = 0; j < bCols; j++) {
      c[i*bCols + j] = 0;
    }
    for (k = 0; k < aCols; k++) {
      for (j = 0; j < bCols; j++) {
	c[i*bCols + j] += a[i*aCols + k] * b[k*bCols + j];
      }
    }
  }
  return 0;
}

