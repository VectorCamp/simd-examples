#include <altivec.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Take the number 1, treat it as long integer and shift its binary
// representation 30 positions to the left => N = 2^32
#define N (1L << 30)

int main() {

  // allocate memory for array
  // A array of size N
  float *A = NULL;
  if (posix_memalign((void **)&A, 16, N * sizeof(float)) != 0) {
    perror("posix_memalign failed");
    exit(EXIT_FAILURE);
  }
  // float *A = aligned_alloc(16, N * sizeof(float)); (maybe use this instead)

  // fill array with numbers
  for (int i = 0; i < N; i++) {
    A[i] = (float)i;
  }

  float avg1 = 0.0, avg2 = 0.0;
  struct timeval tv1, tv2, tv3, diff1, diff2;
  gettimeofday(&tv1, NULL);

  // SCALAR VERSION
  // calculates average in avg1 variable
  for (int i = 0; i < N; i++) {
    avg1 += A[i];
  }
  avg1 /= (float)N;
  gettimeofday(&tv2, NULL);

  // POWER VERSION (george mermigkis)
  // declare vector that includes 4 floats & initialize them with val=0
  vector float vec = {0.0f, 0.0f, 0.0f, 0.0f};
  
  // do N/4 iterations because every vector will take 4 loops each time
  for (int i = 0; i < N; i += 4) {

    // load the corresponding 4 values from the loop, perform addition & store
    // result in vec
    vector float input = vec_ld(0, &A[i]);
    vec = vec_add(vec, input);
  }

  //shift vector 8 bytes to the left
  //shift vector 4 bytes to the left
  vec = vec_add(vec, vec_sld(vec, vec, 8));
  vec = vec_add(vec, vec_sld(vec, vec, 4));


  //store the result that is contained now in the first element of vector vec into avg2
  vec_ste(vec, 0, &avg2);

  // divided by the total number of elements (N) multiplied
  avg2 /= (float)(N * 4);
  gettimeofday(&tv3, NULL);

  // times & results
  diff1.tv_sec = tv2.tv_sec - tv1.tv_sec;
  diff1.tv_usec = tv2.tv_usec + (1000000 - tv1.tv_usec);
  diff2.tv_sec = tv3.tv_sec - tv2.tv_sec;
  diff2.tv_usec = tv3.tv_usec + (1000000 - tv2.tv_usec);
  printf("average of %ld elements = %f (Scalar)\n", N, avg1);
  printf("scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
  printf("average of %ld elements = %f (Power)\n", N, avg2);
  printf("POWER   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);

  // free the memory
  free(A);
}
