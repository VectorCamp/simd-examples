#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// d contains the pixel values of a 4x4 block that needs to be transformed
// this function computes the dct on the input data (d) and stores the result
// into d
// REF:
// https://code.videolan.org/videolan/x264/-/blob/master/common/dct.c?ref_type=heads
static void dct4x4dc_c(int d[16]) {
  // hold the intermediate results
  int tmp[16];

  // iterate over each row of the 4x4 block (phase 1)
  for (int i = 0; i < 4; i++) {
    int s01 = d[i * 4 + 0] + d[i * 4 + 1]; // sum of the 1st and 2nd elements in the row
    int d01 = d[i * 4 + 0] - d[i * 4 + 1]; // diff between the 1st and the 2nd elements in the row
    int s23 = d[i * 4 + 2] + d[i * 4 + 3]; // sum of the 3d and 4th elements in the row
    int d23 = d[i * 4 + 2] - d[i * 4 + 3]; // diff between the 3d and the 4th elements in the row
    //printf("s01: %d, s23: %d, d01: %d, d23: %d\n", s01, s23, d01, d23);
    tmp[0 * 4 + i] = s01 + s23; // 1st element of the row
    tmp[1 * 4 + i] = s01 - s23; // 2nd element of the row
    tmp[2 * 4 + i] = d01 - d23; // 3d element of the row
    tmp[3 * 4 + i] = d01 + d23; // 4th element of the row
    //printf("tmp[0]: %d, tmp[1]: %d, tmp[2]: %d, tmp[3]: %d\n", tmp[0 * 4 + i], tmp[1 * 4 + i], tmp[2 * 4 + i], tmp[3 * 4 + i]);

  }

  // iterates over each row of the 4x4 block (phase 2)
  for (int i = 0; i < 4; i++) {
    
    int s01 = tmp[i * 4 + 0] + tmp[i * 4 + 1];
    int d01 = tmp[i * 4 + 0] - tmp[i * 4 + 1];
    int s23 = tmp[i * 4 + 2] + tmp[i * 4 + 3];
    int d23 = tmp[i * 4 + 2] - tmp[i * 4 + 3];
    //printf("AFTER: s01: %d, s23: %d, d01: %d, d23: %d\n", s01, s23, d01, d23);
    // The DCT coefficients are scaled by adding 1 and then right-shifting
    // by 1 (equivalent to integer division by 2) for rounding.
    d[i * 4 + 0] = (s01 + s23 + 1) >> 1;
    d[i * 4 + 1] = (s01 - s23 + 1) >> 1;
    d[i * 4 + 2] = (d01 - d23 + 1) >> 1;
    d[i * 4 + 3] = (d01 + d23 + 1) >> 1;
    
  }
}

void print_int32x4(const char* label, int32x4_t vector) {
    int32_t data[4];
    vst1q_s32(data, vector);
    printf("%s: [%d %d %d %d]\n", label, data[0], data[1], data[2], data[3]);
}



static void dct4x4dc_neon(int *d) {
    int32x4x4_t input;

    //PHASE 1---------------------------------------------------------------------------
    input = vld4q_s32(d);

     //all s01
    int32x4_t result_add_s01 = vaddq_s32(input.val[0], input.val[1]);
    //all s23
    int32x4_t result_add_s23 = vaddq_s32(input.val[2], input.val[3]);
    //all d01
    int32x4_t result_sub_d01 = vsubq_s32(input.val[0], input.val[1]);
    //all d23
    int32x4_t result_sub_d23 = vsubq_s32(input.val[2], input.val[3]);

    //s01+s23 all
    input.val[0] = vaddq_s32(result_add_s01, result_add_s23);
    //s01-s23 all
    input.val[1] = vsubq_s32(result_add_s01, result_add_s23);
    //d01-d23 all
    input.val[2] = vsubq_s32(result_sub_d01, result_sub_d23);
    //d01+d23 all
    input.val[3] = vaddq_s32(result_sub_d01, result_sub_d23);

  
    //BEFORE GOING TO PHASE 2, I NEED TO TRANPOSE
    int32x4x2_t temp_transposed1 = vtrnq_s32(input.val[0], input.val[1]);
    int32x4x2_t temp_transposed2 = vtrnq_s32(input.val[2], input.val[3]);

    input.val[0] = vcombine_s32(vget_low_s32(temp_transposed1.val[0]), vget_low_s32(temp_transposed2.val[0]));
    input.val[1] = vcombine_s32(vget_low_s32(temp_transposed1.val[1]), vget_low_s32(temp_transposed2.val[1]));
    input.val[2] = vcombine_s32(vget_high_s32(temp_transposed1.val[0]), vget_high_s32(temp_transposed2.val[0]));
    input.val[3] = vcombine_s32(vget_high_s32(temp_transposed1.val[1]), vget_high_s32(temp_transposed2.val[1]));


    //PHASE 2---------------------------------------------------------------------------
    //all s01 after
    result_add_s01 = vaddq_s32(input.val[0], input.val[1]);
    //all s23 after
    result_add_s23 = vaddq_s32(input.val[2], input.val[3]);
    //all d01 after
    result_sub_d01 = vsubq_s32(input.val[0], input.val[1]);
    //all d23 after
    result_sub_d23 = vsubq_s32(input.val[2], input.val[3]);
    
    //s01+s23 all after
    input.val[0] = vaddq_s32(result_add_s01, result_add_s23);
    //s01-s23 all after
    input.val[1] = vsubq_s32(result_add_s01, result_add_s23);
    //d01-d23 all after 
    input.val[2] = vsubq_s32(result_sub_d01, result_sub_d23);
    //d01+d23 all after
    input.val[3] = vaddq_s32(result_sub_d01, result_sub_d23);

    //+1 and shift
    int32x4_t one_vector = vdupq_n_s32(1);
    
    input.val[0] = vshrq_n_s32(vaddq_s32(input.val[0], one_vector), 1);
    input.val[1] = vshrq_n_s32(vaddq_s32(input.val[1], one_vector), 1);
    input.val[2] = vshrq_n_s32(vaddq_s32(input.val[2], one_vector), 1);
    input.val[3] = vshrq_n_s32(vaddq_s32(input.val[3], one_vector), 1);

    //store back
    vst4q_s32(d, input);
}



int main(int argc, char **argv) {

  // handle user's arguement
  long int LOOPS = 10000000000;

  if (argc == 2) {
    char *endptr;
    LOOPS = strtol(argv[1], &endptr, 10);

    // check for conversion errors
    if (*endptr != '\0' || argv[1][0] == '0') {
      fprintf(stderr, "Error: Invalid input\n");
      return EXIT_FAILURE;
    }
  }

  // seed, times, arrays
  srand(time(NULL));
  struct timeval tv1, tv2, tv3, tv4, diff1, diff2;

  int d[16];
  int *dd = NULL;
  if (posix_memalign((void **)&dd, 16, 16 * sizeof(int)) != 0) {
    perror("posix_memalign failed");
    exit(EXIT_FAILURE);
  }
  int random_value[16];

  // initialize original matrix d
  for (int i = 0; i < 16; i++) {
    random_value[i] = rand() & 0xFF;
  }

  // call SCALAR function
  gettimeofday(&tv1, NULL);
  for (int loops = 0; loops < LOOPS; loops++) {
    for (int i = 0; i < 16; i++) {
      d[i] = random_value[i];
    }
    dct4x4dc_c(d);
  }
  gettimeofday(&tv2, NULL);

  // print the transformed matrix
  printf("Transformed Matrix (dct) from Scalar function:\n");
  for (int i = 0; i < 16; i++) {
    printf("%3d ", d[i]);
    if ((i + 1) % 4 == 0)
      printf("\n");
  }

  printf("--------------------------------------\n");

  // call NEON function
  gettimeofday(&tv3, NULL);
  for (int loops = 0; loops < LOOPS; loops++) {
    for (int i = 0; i < 16; i++) {
      dd[i] = random_value[i];
    }
    dct4x4dc_neon(dd);
  }
  gettimeofday(&tv4, NULL);

  // print the transformed matrix
  printf("Transformed Matrix (dct) from NEON function:\n");
  for (int i = 0; i < 16; i++) {
    printf("%3d ", dd[i]);
    if ((i + 1) % 4 == 0)
      printf("\n");
  }

  printf("\n");
  diff1.tv_sec = tv2.tv_sec - tv1.tv_sec;
  diff1.tv_usec = tv2.tv_usec + (1000000 - tv1.tv_usec);
  diff2.tv_sec = tv4.tv_sec - tv3.tv_sec;
  diff2.tv_usec = tv4.tv_usec + (1000000 - tv3.tv_usec);

  printf("Scalar DCT: %ld sec, usec: %d\n", diff1.tv_sec, diff1.tv_usec);
  printf("NEON DCT: %ld sec, usec: %d\n", diff2.tv_sec, diff2.tv_usec);

  return 0;
}