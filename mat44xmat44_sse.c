#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#define N 10004

int
main() {

    // allocate memory for array
    float epsilon = 0.00001f;
    int are_equal = 1;
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
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result[i][j] = 0.0f;
                for (int k = 0; k < 4; k++) {
                    result[i][j] += A[i][k] * B[k][j];
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &mid);

    // SSE version
    for (int l = 0; l < N; l++) {
        __m128 row1 = _mm_load_ps(&B[0][0]);
        __m128 row2 = _mm_load_ps(&B[1][0]);
        __m128 row3 = _mm_load_ps(&B[2][0]);
        __m128 row4 = _mm_load_ps(&B[3][0]);
        for (int i = 0; i < 4; i++) {
            __m128 brod1 = _mm_set1_ps(A[i][0]);
            __m128 brod2 = _mm_set1_ps(A[i][1]);
            __m128 brod3 = _mm_set1_ps(A[i][2]);
            __m128 brod4 = _mm_set1_ps(A[i][3]);
            __m128 row = _mm_add_ps(_mm_add_ps(_mm_mul_ps(brod1, row1), _mm_mul_ps(brod2, row2)), _mm_add_ps(_mm_mul_ps(brod3, row3), _mm_mul_ps(brod4, row4)));
            _mm_store_ps(&implResult[i][0], row);
        }
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
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (fabs(result[i][j] - implResult[i][j]) > epsilon) {
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

    printf("scalar: %ld.%09ld seconds\n", seconds1, nanoseconds1);
    printf("SSE   : %ld.%09ld seconds\n", seconds2, nanoseconds2);
}
