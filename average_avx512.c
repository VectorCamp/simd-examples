#include <stdio.h>
#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define N (1L << 30)

int main() {

	// allocate memory for array
	float *A;
	int ret = posix_memalign((void **)&A, 32, N * sizeof(float));
	if (ret != 0 || !A) {
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

	// AVX512 version   512 bits in contrast to 128 SSE
    __m512 va = _mm512_setzero_ps();
	// Do N/16 iterations
    for (int i=0; i < N; i += 16) {
		// Add 16 elements at a time, add to previous result
		va = _mm512_add_ps(va, _mm512_load_ps(&A[i]));
	}
	// Add horizontally, use 1 step in avx512
	avg2 = _mm512_reduce_add_ps(va);
    
	// Divide by N of elements * 16
	avg2 /= (float)N * 16;
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

	printf("average of %ld elements = %f (scalar), %f (AVX512)\n", N, avg1, avg2);
	printf("scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
	printf("avx512   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);

	// free the memory
	free(A);
}
