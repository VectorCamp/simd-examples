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
static void dct4x4dc_c(uint16_t d[16]) {
  // hold the intermediate results
  int tmp[16];

  // iterate over each row of the 4x4 block (phase 1)
  for (int i = 0; i < 4; i++) {

    int s01 = d[i * 4 + 0] + d[i * 4 + 1];
    int d01 = d[i * 4 + 0] - d[i * 4 + 1];
    int s23 = d[i * 4 + 2] + d[i * 4 + 3];
    int d23 = d[i * 4 + 2] - d[i * 4 + 3];
    tmp[0 * 4 + i] = s01 + s23;
    tmp[1 * 4 + i] = s01 - s23;
    tmp[2 * 4 + i] = d01 - d23;
    tmp[3 * 4 + i] = d01 + d23;
  }

  // iterates over each row of the 4x4 block (phase 2)
  for (int i = 0; i < 4; i++) {
    int s01 = tmp[i * 4 + 0] + tmp[i * 4 + 1];
    int d01 = tmp[i * 4 + 0] - tmp[i * 4 + 1];
    int s23 = tmp[i * 4 + 2] + tmp[i * 4 + 3];
    int d23 = tmp[i * 4 + 2] - tmp[i * 4 + 3];

    d[i * 4 + 0] = (s01 + s23 + 1) >> 1;
    d[i * 4 + 1] = (s01 - s23 + 1) >> 1;
    d[i * 4 + 2] = (d01 - d23 + 1) >> 1;
    d[i * 4 + 3] = (d01 + d23 + 1) >> 1;
  }
}

void print_uint16x4(const char *label, uint16x4_t vector) {
  uint16_t data[4];
  vst1_u16(data, vector);
  printf("%s: [%hu %hu %hu %hu]\n", label, data[0], data[1], data[2], data[3]);
}

void print_int32x4(const char *label, int32x4_t vector) {
  int32_t data[4];
  vst1q_s32(data, vector);
  printf("%s: [%d %d %d %d]\n", label, data[0], data[1], data[2], data[3]);
}

static void dct4x4dc_neon(uint16_t *d) {

  uint16x4_t input0_low = vld1_u16(d);
  uint16x4_t input1_low = vld1_u16(d + 4);
  uint16x4_t input2_low = vld1_u16(d + 8);
  uint16x4_t input3_low = vld1_u16(d + 12);

  int32x4_t input0 = vreinterpretq_s32_u32(vmovl_u16(input0_low));
  int32x4_t input1 = vreinterpretq_s32_u32(vmovl_u16(input1_low));
  int32x4_t input2 = vreinterpretq_s32_u32(vmovl_u16(input2_low));
  int32x4_t input3 = vreinterpretq_s32_u32(vmovl_u16(input3_low));

  // PHASE 1
  //  all s01
  int32x4_t result_add_s01 = vaddq_s32(input0, input1);
  // all s23
  int32x4_t result_add_s23 = vaddq_s32(input2, input3);
  // all d01
  int32x4_t result_sub_d01 = vsubq_s32(input0, input1);
  // all d23
  int32x4_t result_sub_d23 = vsubq_s32(input2, input3);

  // s01+s23 all
  input0 = vaddq_s32(result_add_s01, result_add_s23);
  // s01-s23 all
  input1 = vsubq_s32(result_add_s01, result_add_s23);
  // d01-d23 all
  input2 = vsubq_s32(result_sub_d01, result_sub_d23);
  // d01+d23 all
  input3 = vaddq_s32(result_sub_d01, result_sub_d23);

  // BEFORE GOING TO PHASE 2, I NEED TO TRANPOSE
  int32x4_t temp_trans0 = vtrn1q_s32(input0, input1);
  int32x4_t temp_trans1 = vtrn2q_s32(input0, input1);
  int32x4_t temp_trans2 = vtrn1q_s32(input2, input3);
  int32x4_t temp_trans3 = vtrn2q_s32(input2, input3);

  input0 = vcombine_s32(vget_low_s32(temp_trans0), vget_low_s32(temp_trans2));
  input1 = vcombine_s32(vget_low_s32(temp_trans1), vget_low_s32(temp_trans3));
  input2 = vcombine_s32(vget_high_s32(temp_trans0), vget_high_s32(temp_trans2));
  input3 = vcombine_s32(vget_high_s32(temp_trans1), vget_high_s32(temp_trans3));

  // PHASE 2
  //  all s01 after
  result_add_s01 = vaddq_s32(input0, input1);
  // all s23 after
  result_add_s23 = vaddq_s32(input2, input3);
  // all d01 after
  result_sub_d01 = vsubq_s32(input0, input1);
  // all d23 after
  result_sub_d23 = vsubq_s32(input2, input3);

  // s01+s23 all after
  input0 = vaddq_s32(result_add_s01, result_add_s23);
  // s01-s23 all after
  input1 = vsubq_s32(result_add_s01, result_add_s23);
  // d01-d23 all after
  input2 = vsubq_s32(result_sub_d01, result_sub_d23);
  // d01+d23 all after
  input3 = vaddq_s32(result_sub_d01, result_sub_d23);

  int32x4_t one_vector = vdupq_n_s32(1);

  input0 = vshrq_n_s32(vaddq_s32(input0, one_vector), 1);
  input1 = vshrq_n_s32(vaddq_s32(input1, one_vector), 1);
  input2 = vshrq_n_s32(vaddq_s32(input2, one_vector), 1);
  input3 = vshrq_n_s32(vaddq_s32(input3, one_vector), 1);

  // Store the results back to the memory
  vst1_u16(d, vmovn_u32(vreinterpretq_u32_s32(input0)));
  vst1_u16(d + 4, vmovn_u32(vreinterpretq_u32_s32(input1)));
  vst1_u16(d + 8, vmovn_u32(vreinterpretq_u32_s32(input2)));
  vst1_u16(d + 12, vmovn_u32(vreinterpretq_u32_s32(input3)));
}

int main(int argc, char **argv) {

  // handle user's argument
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

  uint16_t d[16];
  uint16_t *dd = NULL;
  if (posix_memalign((void **)&dd, 16, 16 * sizeof(uint16_t)) != 0) {
    perror("posix_memalign failed");
    exit(EXIT_FAILURE);
  }
  uint16_t random_value[16];

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
    printf("%5d ", d[i]);
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
    printf("%5d ", dd[i]);
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
