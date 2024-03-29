#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define N 4

// use this function to transpose matrix B
// ref:
// https://developer.arm.com/documentation/102107a/0100/Floating-point-4x4-matrix-transposition
void mat_transpose_inp_4x4_helium_f32(float32_t *matrix) {
  float32x4x4_t rows;

  rows.val[0] = vld1q_f32(matrix);
  rows.val[1] = vld1q_f32(matrix + 4);
  rows.val[2] = vld1q_f32(matrix + 8);
  rows.val[3] = vld1q_f32(matrix + 12);

  vst4q_f32(matrix, rows);
}

void matXmat_c(float *C[], float *A[], float B[N][N]) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      C[i][j] = 0.0f;
      for (int k = 0; k < 4; k++) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}
void matXmat_neon(float *A[], float B[N][N], float *D[]) {

  float32x4_t row = vdupq_n_f32(0.0f);
  float32x4_t col = vdupq_n_f32(0.0f);
  float32x4_t res = vdupq_n_f32(0.0f);
  float32_t sum = 0.0f;

  for (int i = 0; i < N; i++) {
    for (int k = 0; k < N; k++) {
      for (int j = 0; j < N; j += 4) {
        // load the i-th row from matrix A
        row = vld1q_f32(&A[i][j]);
        // load the k-th column from matrix B
        col = vld1q_f32(&B[k][j]);
        // calculate result: multiply corresponding elements
        res = vmulq_f32(row, col);
      }
      // store the sum into D[i][k]
      sum = vaddvq_f32(res);
      D[i][k] = sum;
    }
  }
}

int main(int argc, char **argv) {
  long int LOOPS = 100000000;

  if (argc == 2) {
    char *endptr;
    LOOPS = strtol(argv[1], &endptr, 10);

    // check for conversion errors
    if (*endptr != '\0' || argv[1][0] == '0') {
      fprintf(stderr, "Error: Invalid input\n");
      return EXIT_FAILURE;
    }
  }
  int count = 0;
  struct timeval tv1, tv2, tv3, diff1, diff2;

  // Declare and allocate memory for matrix A
  float *A[N];
  for (int i = 0; i < N; i++) {
    if (posix_memalign((void **)&A[i], 16, N * sizeof(float)) != 0) {
      perror("Memory allocation failed for A");
      exit(EXIT_FAILURE);
    }
  }

  // Declare and allocate memory for matrix B
  float B[N][N] __attribute__((aligned(16)));

  // Declare and allocate memory for matrix C that will hold the scalar result
  // float C[N][N] __attribute__((aligned(16)));
  float *C[N];
  for (int i = 0; i < N; i++) {
    if (posix_memalign((void **)&C[i], 16, N * sizeof(float)) != 0) {
      perror("Memory allocation failed for C");
      exit(EXIT_FAILURE);
      for (int j = 0; j < i; j++) {
        free(A[j]);
      }
    }
  }
  // Declare and allocate memory for matrix D that will hold the scalar result
  // float D[N][N] __attribute__((aligned(16)));
  float *D[N];
  for (int i = 0; i < N; i++) {
    if (posix_memalign((void **)&D[i], 16, N * sizeof(float)) != 0) {
      perror("Memory allocation failed for D");
      exit(EXIT_FAILURE);

      for (int j = 0; j < i; j++) {
        free(A[j]);
        free(C[j]);
      }
    }
  }

  // fill matrix A
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      A[i][j] = (float)count++;
    }
  }

  // fill matrix B
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      B[i][j] = (float)count++;
    }
  }

  // SCALAR version of matrix matrix (A x B)
  gettimeofday(&tv1, NULL);
  for (int loops = 0; loops < LOOPS; loops++) {
    matXmat_c(C, A, B);
  }

  // print the scalar result
  printf("Scalar matrix: \n");
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      printf("%.2f\t", C[i][j]);
    }
    printf("\n");
  }

  gettimeofday(&tv2, NULL);

  // NEON version of matrix x matrix (A x B)
  // transpose matrix B
  mat_transpose_inp_4x4_helium_f32((float32_t *)B);

  for (int loops = 0; loops < LOOPS; loops++) {
    matXmat_neon(A, B, D);
  }

  printf("NEON matrix: \n");
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      printf("%.2f\t", D[i][j]);
    }
    printf("\n");
  }
  gettimeofday(&tv3, NULL);

  diff1.tv_sec = tv2.tv_sec - tv1.tv_sec;
  diff1.tv_usec = tv2.tv_usec + (1000000 - tv1.tv_usec);
  diff2.tv_sec = tv3.tv_sec - tv2.tv_sec;
  diff2.tv_usec = tv3.tv_usec + (1000000 - tv2.tv_usec);

  printf("Scalar Matrix x Matrix: %ld sec, usec: %d\n", diff1.tv_sec,
         diff1.tv_usec);
  printf("NEON Matrix x Matrix: %ld sec, usec: %d\n", diff2.tv_sec,
         diff2.tv_usec);

  for (int j = 0; j < N; j++) {
    free(A[j]);
    free(C[j]);
    free(D[j]);
  }
}