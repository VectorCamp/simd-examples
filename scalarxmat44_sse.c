#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define N 4   // #define N (1L << 10)

int
main() {

    // allocate memory for array
    int are_equal = 1;
    float lamda = 5.0;
    int count = 0;
    float A[N][N] __attribute__((aligned(16)));
    float B[N][N] __attribute__((aligned(16)));
    // fill array with numbers
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (float) count;
            count = count + 1;
        }
    }
    count = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            B[i][j] = (float) count;
            count = count + 1;
        }
    }

    struct timeval tv1, tv2, tv3, diff1, diff2;
    gettimeofday(&tv1, NULL);

    // normal scalar version
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = lamda * A[i][j];
        }
    }
    gettimeofday(&tv2, NULL);

    // SSE version
    __m128 lamda_vec = _mm_set1_ps(lamda);   // create a vector with 4 copies of lambda

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j += 4) {
            __m128 B_vec = _mm_load_ps(&B[i][j]);   // load 4 floats from B
            B_vec = _mm_mul_ps(B_vec, lamda_vec);   // multiply by lamda
            _mm_store_ps(&B[i][j], B_vec);          // store the result back in B
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
            {
                if (A[i][j] != B[i][j]) {
                    are_equal = 0;
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
    printf("scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
    printf("SSE   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);
}
