#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define N 4

int main() {

    int are_equal_1 = 1;
    int are_equal_2 = 1;
    int count = 0;
    
     //declare vector and allocate alligned memory for it 
    float *vectorA = NULL;
    if (posix_memalign((void **)&vectorA, 16, N * sizeof(float)) != 0) {
        perror("vector's posix_memalign failed");
        exit(EXIT_FAILURE);
    }

    //declare matrix and allocate alligned memory for it
    float *B_memory = NULL;
    if (posix_memalign((void **)&B_memory, 16, sizeof(float[N][N])) != 0) {
        fprintf(stderr, "error allocating memory for matrix B\n");
        exit(1);
    }
    float(*B)[N] = (float(*)[N])B_memory;


    //declare and allocate alligned memory for 
    //result vector1 for Vector x Matrix
    //this will store the result from NEON 1x4
    float *resultVector1_neon = NULL;
    if (posix_memalign((void **)&resultVector1_neon, 16, N * sizeof(float)) != 0) {
        perror("resultVector1_neon's posix_memalign failed");
        exit(EXIT_FAILURE);
    }

    //declare and allocate alligned memory for
    //result vector2 for Matrix x Vector
    //this will store the result from NEON 4x1
    float *resultVector2_neon = NULL;
    if (posix_memalign((void **)&resultVector2_neon, 16, N * sizeof(float)) != 0) {
        perror("resultVector2_neon's posix_memalign failed");
        exit(EXIT_FAILURE);
    }

    //declare vectors for scalar version
    //this will store the results from SCALAR Vector x Matrix
    float *resultVector1_scalar = NULL;
    if (posix_memalign((void **)&resultVector1_scalar, 16, N * sizeof(float)) != 0) {
        perror("resultVector1_scalar's posix_memalign failed");
        exit(EXIT_FAILURE);
    }

    //this will store the results from SCALAR Matrix x Vector
    float *resultVector2_scalar = NULL;
    if (posix_memalign((void **)&resultVector2_scalar, 16, N * sizeof(float)) != 0) {
        perror("resultVector1_scalar's posix_memalign failed");
        exit(EXIT_FAILURE);
    }


    //fill vector
    for (int i = 0; i < N; i++) {
        vectorA[i] = (float) i;
    }

    //fill matrix
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            B[i][j] = (float) count++;
        }
    }


    struct timeval tv1, tv2, tv3, tv4, tv5;
    gettimeofday(&tv1, NULL);

    //SCALAR VERSION for Matrix x Vector
    for (int i = 0; i < N; i++) {
        resultVector1_scalar[i] = 0.0f; 
        for (int j = 0; j < N; j++) {
            resultVector1_scalar[i] += B[i][j] * vectorA[j]; 
        }
    }
    gettimeofday(&tv2, NULL);


    //NEON VERSION for Matrix x Vector
    float32x4_t result1_neon;
    float32x4_t BVec, AVec;

    for (int i = 0; i < N; i++) {
         result1_neon = vdupq_n_f32(0.0f);
        for (int j = 0; j < N; j += 4) {
            // Load 4 elements of B[i][j] into BVec
            BVec = vld1q_f32(&B[i][j]); 
            // Load 4 elements of vectorA[j] into AVec
            AVec = vld1q_f32(&vectorA[j]); 
            // Multiply and accumulate
            result1_neon = vmlaq_f32(result1_neon, BVec, AVec); 
        }
        // Sum the 4 elements in sumVec and store the result
        resultVector1_neon[i]= vaddvq_f32(result1_neon);
    }
    gettimeofday(&tv3, NULL);

    
    //SCALAR VERSION for Vector x Matrix
    for (int i = 0; i < N; i++) {
        resultVector2_scalar[i] = 0.0f;
        for (int j = 0; j < N; j++) {
            resultVector2_scalar[i] += vectorA[j] * B[j][i];
        }
    }
    gettimeofday(&tv4, NULL);
    
    //NEON VERSION for Vector x Matrix
    float32x4_t result2_neon = vdupq_n_f32(0.0f);

    for (int i = 0; i < N; i++) {
        float32x4_t B_row = vld1q_f32(&B[i][0]);
        float32x4_t A_value = vdupq_n_f32(vectorA[i]);
        result2_neon = vmlaq_f32(result2_neon, B_row, A_value);
    }
    
    //now the result2_neon vector contains the result and I need to save it 
    //into resultVector2_neon float array
    vst1q_f32(resultVector2_neon, result2_neon);
    gettimeofday(&tv5, NULL);



    for (int i = 0; i < 4; i++) {
        if (resultVector1_scalar[i] != resultVector1_neon[i]) {
            are_equal_1 = 0;
            break;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (resultVector2_scalar[i] != resultVector2_neon[i]) {
            are_equal_2 = 0;
            break;
        }
    }
    if (are_equal_1) {
        printf("Matrix x Vector (1) correct\n");
    } else {
        printf("Matrix x Vector (1) wrong\n");
    }
    if (are_equal_2) {
        printf("Vector x Matrix (2) correct\n");
    } else {
        printf("Vector x Matrix (2) wrong\n");
    }

    // free the memory
    free(vectorA);
    free(B);
    free(resultVector1_neon);
    free(resultVector2_neon);
    free(resultVector1_scalar);
    free(resultVector2_scalar);
}