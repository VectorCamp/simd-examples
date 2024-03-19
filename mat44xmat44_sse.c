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
    float A[N][N] __attribute__((aligned(16)));
    float B[N][N] __attribute__((aligned(16)));
    float result[N][N] __attribute__((aligned(16)));
    float implResult[N][N] __attribute__((aligned(16)));
    // fill array with numbers
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (float) count++;
        }
    }
    // fill array with different numbers
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            B[i][j] = 2 + (float) count--;
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

    // SSE version
    __m128 result_sse[N];   // 4 * 4(=128bit)
    // Initialize result_sse with zeros
    for (int i = 0; i < N; i++) {
        result_sse[i] = _mm_setzero_ps();
    }
    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            __m128 A_vec = _mm_set1_ps(A[i][k]);    // vector filled with 1 element
            __m128 B_vec = _mm_load_ps(&B[k][0]);   // column of B
            result_sse[i] = _mm_fmadd_ps(A_vec, B_vec, result_sse[i]);
        }
    }
    for (int i = 0; i < N; i++) {
        _mm_store_ps(&implResult[i][0], result_sse[i]);
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
}
