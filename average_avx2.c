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

	// AVX2 version   256 in contrast to 128 SSE
    __m256 va = _mm256_setzero_ps();
	// Do N/8 iterations
    for (int i=0; i < N; i += 8) {
		// Add 8 elements at a time, add to previous result
		va = _mm256_add_ps(va, _mm256_load_ps(&A[i]));
	}
	// Add horizontally, use 4 steps in avx2
	// va=[a,b,c,d,e,f,g,h]
	__m256 tmp1 = _mm256_permute2f128_ps(va, va, 1);	//swap in half  
    // tmp1=[e,f,g,h,a,b,c,d]
	__m256 tmp2 = _mm256_add_ps(va, tmp1);	
	//tmp2=	[a+e, b+f, c+g, d+h, e+a, f+b, g+c, h+d]
    tmp2 = _mm256_add_ps(tmp2, _mm256_permute_ps(tmp2, 0x4E)); // 0x4E: 01001110 1 0 3 2
    //premutation = [b+f, a+e, d+h, c+g, b+f, a+e, d+h, c+g]
	//temp2 = [b+f+a+e, a+e+b+f, d+h+c+g, d+h+c+g, b+f+a+e, a+e+b+f, d+h+c+g, d+h+c+g]
	tmp2 = _mm256_add_ps(tmp2, _mm256_permute_ps(tmp2, 0xB1)); // 0xB1: 10110001 2 3 0 1
    //premutation = [d+h+c+g, d+h+c+g, b+f+a+e, a+e+b+f, d+h+c+g, d+h+c+g, b+f+a+e, a+e+b+f]
	//temp2 = [b+f+a+e+d+h+c+g, b+f+a+e+d+h+c+g, b+f+a+e+d+h+c+g, b+f+a+e+d+h+c+g,b+f+a+e+d+h+c+g, b+f+a+e+d+h+c+g, b+f+a+e+d+h+c+g, b+f+a+e+d+h+c+g]
    
	// Take first 32-bit float element  
    //(from 256bit temp2, it takes 1st 4 float and then it take the 1st float)
	avg2 = _mm_cvtss_f32(_mm256_castps256_ps128(tmp2));
	// Divide by N of elements * 8
	avg2 /= (float)N * 8;
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

	printf("average of %ld elements = %f (scalar), %f (AVX2)\n", N, avg1, avg2);
	printf("scalar: %ld sec, usec: %ld\n", diff1.tv_sec, diff1.tv_usec);
	printf("avx2   : %ld sec, usec: %ld\n", diff2.tv_sec, diff2.tv_usec);

	// free the memory
	free(A);
}
