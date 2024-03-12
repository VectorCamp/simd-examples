#include <stdio.h>
#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define N (1L << 30)

int main() {

	// allocate memory for array
	float *A = (float *)malloc(N * sizeof(float));
	if (!A) {
		fprintf(stderr, "error allocating %ld bytes\n", N * sizeof(float));
		exit(1);
	}

	// fill array with numbers
	for (int i=0; i < N; i++) {
		A[i] = (float) i;
	}

	// Calculate the average
	struct timeval tv1, tv2, tv3, diff1, diff2;
	float avg1 = 0.0, avg2 = 0.0;
	gettimeofday(&tv1, NULL);
	
	// normal scalar version
	for (int i=0; i < N; i++) {
		avg1 += A[i];
	}
	avg1 /= (float)N;
	gettimeofday(&tv2, NULL);

	// SSE version
	__m128 va = _mm_setzero_ps();
	// Do N/4 iterations
	for (int i=0; i < N; i += 4) {
		// Add 4 elements at a time, add to previous result
		va = _mm_add_ps(va, _mm_load_ps(&A[i]));
	}
	// Add horizontally, use 2 steps in SSE
	__m128 tmp = _mm_add_ps(va, _mm_movehl_ps(va, va));
        tmp = _mm_add_ss(tmp, _mm_shuffle_ps(tmp, tmp, 1));
	// Take first 32-bit float element
	avg2 = _mm_cvtss_f32(tmp);
	// Divide by N of elements
	avg2 /= (float)N * 4;
	gettimeofday(&tv3, NULL);

	// Get time difference
	// https://stackoverflow.com/questions/1444428/time-stamp-in-the-c-programming-language
	diff1.tv_sec = tv2.tv_sec - tv1.tv_sec;
	diff1.tv_usec = tv2.tv_usec + (1000000 - tv1.tv_usec);
	diff2.tv_sec = tv3.tv_sec - tv2.tv_sec;
	diff2.tv_usec = tv3.tv_usec + (1000000 - tv2.tv_usec);

	while(diff1.tv_usec > 1000000)
	{
		diff1.tv_sec++;
		diff1.tv_usec -= 1000000;
		printf("updated diff1 tv_sec:%ld tv_usec:%ld\n", diff1.tv_sec, diff1.tv_usec);
	}
	while(diff2.tv_usec > 1000000)
	{
		diff2.tv_sec++;
		diff2.tv_usec -= 1000000;
		printf("updated diff2 tv_sec:%ld tv_usec:%ld\n", diff2.tv_sec, diff2.tv_usec);
	}

	printf("average of %d elements = %f (scalar), %f (SSE)\n", N, avg1, avg2);
	printf("scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
	printf("SSE   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);

	// free the memory
	free(A);
}
