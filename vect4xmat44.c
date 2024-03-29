#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define N 1000000000

void
checkVectorsEqual(float result[4], float implResult[4], float epsilon) {
    int are_equal = 1;

    for (int i = 0; i < 4; i++) {
        if (fabs(result[i] - implResult[i]) > epsilon) {
            are_equal = 0;
            break;
        }
    }

    if (are_equal) {
        printf("Vector result and implResult are the same\n");
    } else {
        printf("Vector result and implResult are not the same\n");
    }
}
void
vect4xmat44_c(float B[4][4], float vectorA[4], float result[4]) {
    for (int i = 0; i < 4; i++) {
        result[i] = 0.0f;
        for (int j = 0; j < 4; j++) {
            result[i] += B[i][j] * vectorA[j];
        }
    }
}
void
vect4xmat44_sse(float B[4][4], float vectorA[4], float implResult[4]) {
    __m128 result_sse[4];

    for (int i = 0; i < 4; i++) {
        result_sse[i] = _mm_setzero_ps();
        for (int j = 0; j < 4; j += 4) {
            __m128 B_vec = _mm_load_ps(&B[i][j]);
            __m128 vectorA_vec = _mm_load_ps(&vectorA[j]);
            result_sse[i] = _mm_fmadd_ps(B_vec, vectorA_vec, result_sse[i]);
        }
    }

    for (int i = 0; i < 4; i++) {
        __m128 tmp = _mm_add_ps(result_sse[i], _mm_movehl_ps(result_sse[i], result_sse[i]));
        tmp = _mm_add_ss(tmp, _mm_shuffle_ps(tmp, tmp, 1));
        implResult[i] = _mm_cvtss_f32(tmp);
    }
}
int
main() {
    float epsilon = 0.00001f;
    float B[4][4] __attribute__((aligned(16)));
    float vectorA[4] __attribute__((aligned(16)));
    float result[4] __attribute__((aligned(16)));
    float implResult[4] __attribute__((aligned(16)));
    struct timespec start, mid, end;
    // Use the current time in microseconds to seed the random number generator
    srand(time(NULL));
    // fill vector with random numbers
    for (int i = 0; i < 4; i++) {
        vectorA[i] = (float) rand() / RAND_MAX * 4;
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            B[i][j] = (float) rand() / RAND_MAX * 4;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    // normal scalar version
    for (int k = 0; k < N; k++) {
        vect4xmat44_c(B, vectorA, result);
    }
    clock_gettime(CLOCK_MONOTONIC, &mid);

    // SSE version
    for (int k = 0; k < N; k++) {
        vect4xmat44_sse(B, vectorA, implResult);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    long seconds1 = mid.tv_sec - start.tv_sec;
    long nanoseconds1 = mid.tv_nsec - start.tv_nsec;
    if (nanoseconds1 < 0) {
        seconds1--;
        nanoseconds1 += 1000000000;
    }
    long seconds2 = end.tv_sec - mid.tv_sec;
    long nanoseconds2 = end.tv_nsec - mid.tv_nsec;
    if (nanoseconds2 < 0) {
        seconds2--;
        nanoseconds2 += 1000000000;
    }
    checkVectorsEqual(result, implResult, epsilon);
    printf("scalar: %ld.%09ld seconds\n", seconds1, nanoseconds1);
    printf("SSE   : %ld.%09ld seconds\n", seconds2, nanoseconds2);
}