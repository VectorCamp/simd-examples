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
    float lamda = 5.0;
    int count = 0;
    float(*A)[N] = malloc(sizeof(float[N][N]));
    if (!A) {
        fprintf(stderr, "error allocating memory for matrix\n");
        exit(1);
    }
    float(*B)[N] = malloc(sizeof(float[N][N]));
    if (!B) {
        fprintf(stderr, "error allocating memory for matrix\n");
        exit(1);
    }
    // fill array with numbers
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            A[i][j] = (float) count++;
        }
    }
    count = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            B[i][j] = (float) count++;
        }
    }

    struct timeval tv1, tv2, tv3, diff1, diff2;
    gettimeofday(&tv1, NULL);

    // normal scalar version
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            A[i][j] = lamda * A[i][j];
        }
    }
    gettimeofday(&tv2, NULL);

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
    printf("scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
    // printf("SSE   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);

    // free the memory
    free(A);
    free(B);
}
