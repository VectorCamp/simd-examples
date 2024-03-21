#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define N 4
#define LOOPS 1000000000

int main() {

  int are_equal = 1;
  float lamda = 5.0;
  int count = 0;

  // static alligned memory
  //(16 byte allignment because of NEON intrinsics)
  float A[N][N] __attribute__((aligned(16)));
  float B[N][N] __attribute__((aligned(16)));

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
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        A[i][j] = lamda * A[i][j];
      }
    }
  }
  gettimeofday(&tv2, NULL);

  // NEON Version
  float32x4_t lamda_vector = vdupq_n_f32(lamda);
  float32x4_t array_in = vdupq_n_f32(0.0f);

  // becase we have a 4x4 array we take each row with 1 vector
  // otherwise we would to use a different mechanism
  for (int k = 0; k < LOOPS; k++) {
    for (int i = 0; i < 4; i++) {

      // load 4 float values from the array B, multiply each value with lamda
      // and then store result back to B array
      array_in = vld1q_f32(&B[i][0]);
      array_in = vmulq_f32(array_in, lamda_vector);
      vst1q_f32(&B[i][0], array_in);
    }
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