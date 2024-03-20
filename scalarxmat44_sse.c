#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define N 1000000000   // loops

int
main() {
    float epsilon = 0.00001f;
    int are_equal = 1;
    float lamda = 5.0;
    float A[4][4] __attribute__((aligned(16)));
    float B[4][4] __attribute__((aligned(16)));
    struct timespec start, mid, end;
    // Use the current time in microseconds to seed the random number generator
    srand(time(NULL));
    // fill array with random numbers
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float random_number = (float) rand() / RAND_MAX * 4;
            A[i][j] = random_number;
            B[i][j] = random_number;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    // normal scalar version
    for (int k = 0; k < N; k++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                A[i][j] = lamda * A[i][j];
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &mid);

    // SSE version

    __m128 lamda_vec = _mm_set1_ps(lamda);   // create a vector with 4 copies of lambda
    for (int k = 0; k < N; k++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j += 4) {
                __m128 B_vec = _mm_load_ps(&B[i][j]);   // load 4 floats from B
                B_vec = _mm_mul_ps(B_vec, lamda_vec);   // multiply by lamda
                _mm_store_ps(&B[i][j], B_vec);          // store the result back in B
            }
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
            if (fabs(A[i][j] - B[i][j]) > epsilon) {
                are_equal = 0;
                break;
            }
        }
    }
    if (are_equal) {
        printf("Arrays A and B are the same\n");
    } else {
        printf("Arrays A and B are not the same\n");
    }
    printf("scalar: %ld.%09ld seconds\n", seconds1, nanoseconds1);
    printf("SSE   : %ld.%09ld seconds\n", seconds2, nanoseconds2);
}