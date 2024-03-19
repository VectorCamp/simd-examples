#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define N 10000

int
main() {

    int are_equal = 1;
    float lamda = 5.0;
    float A[4][4] __attribute__((aligned(16)));
    float B[4][4] __attribute__((aligned(16)));
    struct timespec start, mid, end;
    // Use the current time in microseconds to seed the random number generator
    srand(time(NULL));
    // fill array with numbers
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float random_num = (float) rand() / RAND_MAX * 4;
            A[i][j] = random_num;
            B[i][j] = random_num;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    // normal scalar version
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            A[i][j] = lamda * A[i][j];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &mid);

    /*
    // SSE version
    __m128 lamda_vec = _mm_set1_ps(lamda);   // create a vector with 4 copies of lambda

    for (int i = 0; i < 4; i++) {
        __m128 A_vec = _mm_load_ps(A[i]);       // load 4 floats from A
        A_vec = _mm_mul_ps(A_vec, lamda_vec);   // multiply by lamda
        _mm_store_ps(A[i], A_vec);              // store the result back in B
    }
    */
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            B[i][j] = lamda * B[i][j];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    long seconds1 = mid.tv_sec - start.tv_sec;
    long nanoseconds1 = mid.tv_nsec - start.tv_nsec;
    if (nanoseconds1 < 0) {
        seconds1--;
        nanoseconds1 += 1000000000;
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (fabs(A[i] - B[i]) == 0) {
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
    // printf("SSE   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);
}
