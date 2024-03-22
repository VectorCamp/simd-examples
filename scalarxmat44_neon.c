#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define N 4
#define LOOPS 10000000

void scalarxmat44_c(float *A[], float lamda) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      A[i][j] = lamda * A[i][j];
    }
  }
}

void scalarxmat44_neon(float32x4_t lamda_vector, float *B[]) {
  for (int i = 0; i < 4; i++) {
    // load 4 float values from the array B, multiply each value with lamda
    // and then store result back to B array
    float32x4_t array_in = vld1q_f32(&B[i][0]);
    array_in = vmulq_f32(array_in, lamda_vector);
    vst1q_f32(&B[i][0], array_in);
  }
}

int main() {

  int are_equal = 1;
  float lamda = 5.0;
  int count = 0;

  // static alligned memory
  //(16 byte allignment because of NEON intrinsics)
  float *A[N];
  float *B[N];
  for (int i = 0; i < N; i++) {
    if (posix_memalign((void **)&A[i], 16, N * sizeof(float)) != 0) {
      perror("Memory allocation failed for A");
      exit(EXIT_FAILURE);
    }
    if (posix_memalign((void **)&B[i], 16, N * sizeof(float)) != 0) {
      perror("Memory allocation failed for B");
      for (int j = 0; j < i; j++) {
        free(A[j]);
      }
      exit(EXIT_FAILURE);
    }
  }

  // fill array with numbers
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      A[i][j] = (float)count++;
    }
  }
  count = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      B[i][j] = (float)count++;
    }
  }

  struct timeval tv1, tv2, tv3, diff1, diff2;
  gettimeofday(&tv1, NULL);

  // Scalar version
  for (int k = 0; k < LOOPS; k++) {
    scalarxmat44_c(A, lamda);
  }
  gettimeofday(&tv2, NULL);

  // NEON Version
  float32x4_t lamda_vector = vdupq_n_f32(lamda);

  // becase we have a 4x4 array we take each row with 1 vector
  // otherwise we would to use a different mechanism
  for (int k = 0; k < LOOPS; k++) {
    scalarxmat44_neon(lamda_vector, B);
  }

  gettimeofday(&tv3, NULL);

  // Get time difference
  // https://stackoverflow.com/questions/1444428/time-stamp-in-the-c-programming-language
  diff1.tv_sec = tv2.tv_sec - tv1.tv_sec;
  diff1.tv_usec = tv2.tv_usec + (1000000 - tv1.tv_usec);
  diff2.tv_sec = tv3.tv_sec - tv2.tv_sec;
  diff2.tv_usec = tv3.tv_usec + (1000000 - tv2.tv_usec);

  while (diff1.tv_usec > 1000000) {
    diff1.tv_sec++;
    diff1.tv_usec -= 1000000;
    printf("updated diff1 tv_sec:%ld tv_usec:%ld\n", diff1.tv_sec,
           diff1.tv_usec);
  }
  while (diff2.tv_usec > 1000000) {
    diff2.tv_sec++;
    diff2.tv_usec -= 1000000;
    printf("updated diff2 tv_sec:%ld tv_usec:%ld\n", diff2.tv_sec,
           diff2.tv_usec);
  }

  // check if equal and showcase the index they're not
  // afterwards print if the matrices are equal or not
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      {
        if (A[i][j] != B[i][j]) {
          are_equal = 0;
          printf("Arrays A and B differ at index %d\n", i);
          break;
        }
      }
    }
  }

  if (are_equal) {
    printf("Arrays A and B are the same\n");
  } else {
    printf("Arrays A and B are not the same\n");
  }
  printf("Scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
  printf("NEON   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);
}