#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <arm_neon.h>


//Take the number 1, treat it as long integer and shift its binary representation 
//30 positions to the left => N = 2^32
#define N (1L << 30)


int main() {

    //allocate memory for array
    //A array of size N
	float *A;
	posix_memalign((void**)&A, 16, N * sizeof(float));
	//float *A = aligned_alloc(16, N * sizeof(float));
	
	
	if (!A) {
		fprintf(stderr, "error allocating %ld bytes\n", N * sizeof(float));
		exit(1);
	}

	//fill array with numbers
	for (int i=0; i < N; i++) {
		A[i] = (float) i;
	}

	float avg1 = 0.0, avg2=0.0;
    struct timeval tv1, tv2, tv3, diff1, diff2;
	gettimeofday(&tv1, NULL);


	//SCALAR VERSION
	//calculates average in avg1 variable
	for (int i=0; i < N; i++) {
		avg1 += A[i];
	}
	avg1 /= (float)N;
	gettimeofday(&tv2, NULL);



    //NEON VERSION (george mermigkis)
    //declare vector of 4 floats values and initialize with 0
    //basically it duplicates float val 0 and assigns it to all 4 parts of vector
    float32x4_t vec = vdupq_n_f32(0.0f);

    //do N/4 iterations because every NEON vector will take 4 loops each time
    for(int i=0; i<N; i+=4){

        //load the corresponding 4 values from the loop, perform addition & store result in vec
        float32x4_t input = vld1q_f32(&A[i]);
        vec=vaddq_f32(vec,input);
    }

/* 	WAY 1 

	//we are using 32x2 because the vector will store only 2 floats after the additions
	//pairwise add the 2 upper halfs and the 2 lower halfs
	float32x2_t sum_halves = vpadd_f32(vget_low_f32(vec), vget_high_f32(vec));

	//extract the 1st element of the sum_halves (the upper half addition)
	//ectract the 2nd element of the sum_halves (the lower half addition)
	//add them and save them on avg2 variable 
	avg2 = vget_lane_f32(sum_halves, 0) + vget_lane_f32(sum_halves, 1);
*/

	//add all the 4 parts of the vector and save them in sum vector
	float32_t sum = vaddvq_f32(vec);
	
    //divided by the total number of elements (N) multiplied 
	//by 4 (since each iteration processes 4 elements at a time). 
    avg2 = sum / (float)(N * 4);
	gettimeofday(&tv3, NULL);



	//times & results
    diff1.tv_sec = tv2.tv_sec - tv1.tv_sec;
	diff1.tv_usec = tv2.tv_usec + (1000000 - tv1.tv_usec);
	diff2.tv_sec = tv3.tv_sec - tv2.tv_sec;
	diff2.tv_usec = tv3.tv_usec + (1000000 - tv2.tv_usec);
    printf("average of %ld elements = %f (Scalar)\n", N, avg1);
	printf("scalar: %ld sec, usec: %d\n", diff1.tv_sec, diff1.tv_usec);
    printf("average of %ld elements = %f (Neon)\n", N, avg2);
	printf("NEON   : %ld sec, usec: %d\n", diff2.tv_sec, diff2.tv_usec);
	

    //free the memory
	free(A);
}

