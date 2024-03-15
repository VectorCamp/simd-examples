#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define N 4

int
main() {

    // allocate memory for array
    int are_equal = 1;
    int count = 0;
    float(*A)[N];
    if (posix_memalign((void **) &A, 16, sizeof(float[N][N])) != 0) {
        fprintf(stderr, "error allocating memory for matrix B\n");
        exit(1);
    }

    float(*B)[N];
    if (posix_memalign((void **) &B, 16, sizeof(float[N][N])) != 0) {
        fprintf(stderr, "error allocating memory for matrix B\n");
        exit(1);
    }

    float(*result)[N];
    if (posix_memalign((void **) &result, 16, sizeof(float[N][N])) != 0) {
        fprintf(stderr, "error allocating memory for matrix B\n");
        exit(1);
    }

    float(*implResult)[N];
    if (posix_memalign((void **) &implResult, 16, sizeof(float[N][N])) != 0) {
        fprintf(stderr, "error allocating memory for matrix B\n");
        exit(1);
    }
    // fill vector with numbers
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (float) count++;
        }
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            B[i][j] = (float) count--;
        }
    }

    struct timeval tv1, tv2, tv3, diff1, diff2;
    gettimeofday(&tv1, NULL);

    // normal scalar version
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            result[i][j] = 0.0f;
            for (int k = 0; k < N; k++) {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    gettimeofday(&tv2, NULL);

    //

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            implResult[i][j] = 0.0f;
            for (int k = 0; k < N; k++) {
                implResult[i][j] += A[i][k] * B[k][j];
            }
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
        printf("updated diff1 tv_sec:%ld tv_usec:%ld\n", diff1.tv_sec, diff1.tv_usec);
    }
    while (diff2.tv_usec > 1000000) {
        diff2.tv_sec++;
        diff2.tv_usec -= 1000000;
        printf("updated diff2 tv_sec:%ld tv_usec:%ld\n", diff2.tv_sec, diff2.tv_usec);
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (result[i][j] != implResult[i][j]) {
                are_equal = 0;
                break;
            }
        }
    }
    if (are_equal) {
        printf("Array result and implResult are the same\n");
    } else {
        printf("Array result and implResult are not the same\n");
    }
    printf("scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
    printf("SSE   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);

    // free the memory
    free(result);
    free(implResult);
    free(A);
    free(B);
}
