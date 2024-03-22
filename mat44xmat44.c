#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#define N 1000000000

void
checkArraysEqual(float result[4][4], float implResult[4][4], float epsilon) {
    int are_equal = 1;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (fabs(result[i][j] - implResult[i][j]) > epsilon) {
                are_equal = 0;
                break;
            }
        }
        if (!are_equal) {
            break;
        }
    }

    if (are_equal) {
        printf("Array result and implResult are the same\n");
    } else {
        printf("Array result and implResult are not the same\n");
    }
}
void
mat44xmat44_c(float A[4][4], float B[4][4], float result[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}
void
mat44xmat44_sse(float (*A)[4], float (*B)[4], float (*implResult)[4]) {
    float BT[4][4];
    // Transpose B
    // Load the first four elements of each row of B into 128-bit vectors
    __m128 row1 = _mm_load_ps(&B[0][0]);
    __m128 row2 = _mm_load_ps(&B[1][0]);
    __m128 row3 = _mm_load_ps(&B[2][0]);
    __m128 row4 = _mm_load_ps(&B[3][0]);

    // Interleave the first two elements of row1 and row2, and the last two elements of row1 and row2
    __m128 tmp1 = _mm_shuffle_ps(row1, row2, _MM_SHUFFLE(1, 0, 1, 0));
    __m128 tmp2 = _mm_shuffle_ps(row1, row2, _MM_SHUFFLE(3, 2, 3, 2));
    __m128 tmp3 = _mm_shuffle_ps(row3, row4, _MM_SHUFFLE(1, 0, 1, 0));
    __m128 tmp4 = _mm_shuffle_ps(row3, row4, _MM_SHUFFLE(3, 2, 3, 2));
    // Interleave the first two elements of tmp1 and tmp3, and the last two elements of tmp1 and tmp3
    row1 = _mm_shuffle_ps(tmp1, tmp3, _MM_SHUFFLE(2, 0, 2, 0));
    row2 = _mm_shuffle_ps(tmp1, tmp3, _MM_SHUFFLE(3, 1, 3, 1));
    row3 = _mm_shuffle_ps(tmp2, tmp4, _MM_SHUFFLE(2, 0, 2, 0));
    row4 = _mm_shuffle_ps(tmp2, tmp4, _MM_SHUFFLE(3, 1, 3, 1));

    _mm_store_ps(&BT[0][0], row1);
    _mm_store_ps(&BT[1][0], row2);
    _mm_store_ps(&BT[2][0], row3);
    _mm_store_ps(&BT[3][0], row4);
    for (int i = 0; i < 4; i++) {
        __m128 a = _mm_load_ps(&A[i][0]);
        for (int j = 0; j < 4; j++) {
            __m128 b = _mm_load_ps(&BT[j][0]);
            __m128 result = _mm_dp_ps(a, b, 0xf1);
            _mm_store_ss(&implResult[i][j], result);
        }
    }
}
int
main() {

    // allocate memory for array
    float epsilon = 0.00001f;
    float A[4][4] __attribute__((aligned(16)));
    float B[4][4] __attribute__((aligned(16)));
    float result[4][4] __attribute__((aligned(16)));
    float implResult[4][4] __attribute__((aligned(16)));
    struct timespec start, mid, end;
    srand(time(NULL));
    // fill array with numbers
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            A[i][j] = (float) rand() / RAND_MAX * 4;
            B[i][j] = (float) rand() / RAND_MAX * 4;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    // normal scalar version
    for (int l = 0; l < N; l++) {
        mat44xmat44_c(A, B, result);
    }
    clock_gettime(CLOCK_MONOTONIC, &mid);
    // SSE version
    for (int l = 0; l < N; l++) {
        mat44xmat44_sse(A, B, implResult);
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
    checkArraysEqual(result, implResult, epsilon);

    printf("scalar: %ld.%09ld seconds\n", seconds1, nanoseconds1);
    printf("SSE   : %ld.%09ld seconds\n", seconds2, nanoseconds2);
}