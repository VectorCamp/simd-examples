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

// NEON version
static void dct4x4dc_neon_real(int *d) {
  // hold the intermediate results
  int tmp[16];
  int32x4_t one_vector = vdupq_n_s32(1);

  // iterate over each row of the 4x4 block (phase 1)
  for (int i = 0; i < 4; i++) {

    //[a b c d]
    int32x4_t row1 = vld1q_s32(&d[i * 4]);

    //[b c d a] by shuffling the first row
    int32x4_t shuffled_row = vextq_s32(row1, row1, 1);

    //[s01 rand s23 rand]
    int32x4_t result_add = vaddq_s32(row1, shuffled_row);
    //[s23 rand s01 rand]
    int32x4_t result_add_shuff = vextq_s32(result_add, result_add, 2);


    //[d01 rand d23 rand]
    int32x4_t result_sub = vsubq_s32(row1, shuffled_row);
    //[d23 rand d01 rand]
    int32x4_t result_sub_shuff = vextq_s32(result_sub, result_sub, 2);

    int32x4_t tmp0 =vaddq_s32(result_add, result_add_shuff);
    int32x4_t tmp1 =vsubq_s32(result_add, result_add_shuff);
    int32x4_t tmp2 =vsubq_s32(result_sub, result_sub_shuff);
    int32x4_t tmp3 =vaddq_s32(result_sub, result_sub_shuff);
    
    tmp[0 * 4 + i] = vgetq_lane_s32(tmp0, 0);
    tmp[1 * 4 + i] = vgetq_lane_s32(tmp1, 0);
    tmp[2 * 4 + i] = vgetq_lane_s32(tmp2, 0);
    tmp[3 * 4 + i] = vgetq_lane_s32(tmp3, 0);

  }

  // iterates over each row of the 4x4 block (phase 2)
  for (int i = 0; i < 4; i++) {

    int32x4_t d_vector = vdupq_n_s32(0);
    
    //[tmp0 tmp1 tmp2 tmp3]
    int32x4_t row1 = vld1q_s32(&tmp[i * 4]);

    //[tmp1 tmp2 tmp3 tmp0] by shuffling the first row
    int32x4_t shuffled_row = vextq_s32(row1, row1, 1);
    
    //element 1 = s01, element 3 = s23
    int32x4_t result_add = vaddq_s32(row1, shuffled_row);
    int32x4_t result_add_shuff = vextq_s32(result_add, result_add, 2);

    //element 1 = d01, element 3 = d23
    int32x4_t result_sub = vsubq_s32(row1, shuffled_row);
    int32x4_t result_sub_shuff = vextq_s32(result_sub, result_sub, 2);

    int32x4_t tmp0 =vaddq_s32(result_add, result_add_shuff);
    int32x4_t tmp1 =vsubq_s32(result_add, result_add_shuff);
    int32x4_t tmp2 =vsubq_s32(result_sub, result_sub_shuff);
    int32x4_t tmp3 =vaddq_s32(result_sub, result_sub_shuff);

    d_vector= vsetq_lane_s32(vgetq_lane_s32(tmp0, 0),d_vector,0);
    d_vector=vsetq_lane_s32(vgetq_lane_s32(tmp1, 0),d_vector,1);
    d_vector=vsetq_lane_s32(vgetq_lane_s32(tmp2, 0),d_vector,2);
    d_vector=vsetq_lane_s32(vgetq_lane_s32(tmp3, 0),d_vector,3);

    d_vector=vshrq_n_s32(vaddq_s32(d_vector, one_vector),1);
    vst1q_s32(&d[i * 4], d_vector);
  }
}


static void dct4x4dc_neon(int *d) {
    int32x4x4_t input;

    input = vld4q_s32(d);

    int32x4_t result_add_s01 = vaddq_s32(input.val[0], input.val[1]);
    int32x4_t result_add_s23 = vaddq_s32(input.val[2], input.val[3]);
    int32x4_t result_sub_d01 = vsubq_s32(input.val[0], input.val[1]);
    int32x4_t result_sub_d23 = vsubq_s32(input.val[2], input.val[3]);

    input.val[0] = vaddq_s32(result_add_s01, result_add_s23);
    input.val[1] = vsubq_s32(result_add_s01, result_add_s23);
    input.val[2] = vsubq_s32(result_sub_d01, result_sub_d23);
    input.val[3] = vaddq_s32(result_sub_d01, result_sub_d23);

    int32x4x4_t input_transposed;

    input_transposed.val[0] = vsetq_lane_s32(vgetq_lane_s32(input.val[0], 0), input_transposed.val[0], 0);
    input_transposed.val[0] = vsetq_lane_s32(vgetq_lane_s32(input.val[1], 0), input_transposed.val[0], 1);
    input_transposed.val[0] = vsetq_lane_s32(vgetq_lane_s32(input.val[2], 0), input_transposed.val[0], 2);
    input_transposed.val[0] = vsetq_lane_s32(vgetq_lane_s32(input.val[3], 0), input_transposed.val[0], 3);

    input_transposed.val[1] = vsetq_lane_s32(vgetq_lane_s32(input.val[0], 1), input_transposed.val[1], 0);
    input_transposed.val[1] = vsetq_lane_s32(vgetq_lane_s32(input.val[1], 1), input_transposed.val[1], 1);
    input_transposed.val[1] = vsetq_lane_s32(vgetq_lane_s32(input.val[2], 1), input_transposed.val[1], 2);
    input_transposed.val[1] = vsetq_lane_s32(vgetq_lane_s32(input.val[3], 1), input_transposed.val[1], 3);

    input_transposed.val[2] = vsetq_lane_s32(vgetq_lane_s32(input.val[0], 2), input_transposed.val[2], 0);
    input_transposed.val[2] = vsetq_lane_s32(vgetq_lane_s32(input.val[1], 2), input_transposed.val[2], 1);
    input_transposed.val[2] = vsetq_lane_s32(vgetq_lane_s32(input.val[2], 2), input_transposed.val[2], 2);
    input_transposed.val[2] = vsetq_lane_s32(vgetq_lane_s32(input.val[3], 2), input_transposed.val[2], 3);

    input_transposed.val[3] = vsetq_lane_s32(vgetq_lane_s32(input.val[0], 3), input_transposed.val[3], 0);
    input_transposed.val[3] = vsetq_lane_s32(vgetq_lane_s32(input.val[1], 3), input_transposed.val[3], 1);
    input_transposed.val[3] = vsetq_lane_s32(vgetq_lane_s32(input.val[2], 3), input_transposed.val[3], 2);
    input_transposed.val[3] = vsetq_lane_s32(vgetq_lane_s32(input.val[3], 3), input_transposed.val[3], 3);

    int32x4_t result_add_s01_after = vaddq_s32(input_transposed.val[0], input_transposed.val[1]);
    int32x4_t result_add_s23_after = vaddq_s32(input_transposed.val[2], input_transposed.val[3]);
    int32x4_t result_sub_d01_after = vsubq_s32(input_transposed.val[0], input_transposed.val[1]);
    int32x4_t result_sub_d23_after = vsubq_s32(input_transposed.val[2], input_transposed.val[3]);

    int32x4_t result_add_all_tmp0_after = vaddq_s32(result_add_s01_after, result_add_s23_after);
    int32x4_t result_sub_all_tmp1_after = vsubq_s32(result_add_s01_after, result_add_s23_after);
    int32x4_t result_sub_all_tmp2_after = vsubq_s32(result_sub_d01_after, result_sub_d23_after);
    int32x4_t result_add_all_tmp3_after = vaddq_s32(result_sub_d01_after, result_sub_d23_after);

    int32x4_t one_vector = vdupq_n_s32(1);
    input.val[0] = vshrq_n_s32(vaddq_s32(result_add_all_tmp0_after, one_vector), 1);
    input.val[1] = vshrq_n_s32(vaddq_s32(result_sub_all_tmp1_after, one_vector), 1);
    input.val[2] = vshrq_n_s32(vaddq_s32(result_sub_all_tmp2_after, one_vector), 1);
    input.val[3] = vshrq_n_s32(vaddq_s32(result_add_all_tmp3_after, one_vector), 1);

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
    random_value[i] = rand() % 256; // integers between 0-255
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