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
    float *vectorA;
    if (posix_memalign((void **) &vectorA, 16, N * sizeof(float)) != 0) {
        fprintf(stderr, "error allocating memory for vectorA\n");
        exit(1);
    }

    float(*B)[N];
    if (posix_memalign((void **) &B, 16, sizeof(float[N][N])) != 0) {
        fprintf(stderr, "error allocating memory for matrix B\n");
        exit(1);
    }

    float *result;
    if (posix_memalign((void **) &result, 16, N * sizeof(float)) != 0) {
        fprintf(stderr, "error allocating memory for result vector\n");
        exit(1);
    }

    float *implResult;
    if (posix_memalign((void **) &implResult, 16, N * sizeof(float)) != 0) {
        fprintf(stderr, "error allocating memory for implResult vector\n");
        exit(1);
    }
    // fill vector with numbers
    for (int i = 0; i < N; i++) {
        vectorA[i] = (float) i;
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            B[i][j] = (float) count++;
        }
    }

    struct timeval tv1, tv2, tv3, diff1, diff2;
    gettimeofday(&tv1, NULL);

    // normal scalar version
    for (int i = 0; i < N; i++) {
        result[i] = 0.0f;
        for (int j = 0; j < N; j++) {
            result[i] += B[i][j] * vectorA[j];
        }
    }
    gettimeofday(&tv2, NULL);

    // SSE version
    __m128 result_sse[N];

    for (int i = 0; i < N; i++) {
        result_sse[i] = _mm_setzero_ps();
        for (int j = 0; j < N; j += 4) {
            __m128 B_vec = _mm_load_ps(&B[i][j]);
            __m128 vectorA_vec = _mm_load_ps(&vectorA[j]);   // works because vectorA has 4 float elements
            result_sse[i] = _mm_fmadd_ps(B_vec, vectorA_vec, result_sse[i]);
        }
    }

    // Convert back to regular float array
    for (int i = 0; i < N; i++) {
        __m128 tmp = _mm_add_ps(result_sse[i], _mm_movehl_ps(result_sse[i], result_sse[i]));
        tmp = _mm_add_ss(tmp, _mm_shuffle_ps(tmp, tmp, 1));
        // Take first 32-bit float element
        implResult[i] = _mm_cvtss_f32(tmp);
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
        if (result[i] != implResult[i]) {
            are_equal = 0;
            break;
        }
    }
    if (are_equal) {
        printf("Vector result and implResult are the same\n");
    } else {
        printf("Vector result and implResult are not the same\n");
    }
    printf("scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
    printf("SSE   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);

    // free the memory
    free(result);
    free(implResult);
    free(vectorA);
    free(B);
}
