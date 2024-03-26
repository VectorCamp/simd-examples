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

    tmp[0 * 4 + i] = s01 + s23; // 1st element of the row
    tmp[1 * 4 + i] = s01 - s23; // 2nd element of the row
    tmp[2 * 4 + i] = d01 - d23; // 3d element of the row
    tmp[3 * 4 + i] = d01 + d23; // 4th element of the row
  }

  // iterates over each row of the 4x4 block (phase 2)
  for (int i = 0; i < 4; i++) {

    int s01 = tmp[i * 4 + 0] + tmp[i * 4 + 1];
    int d01 = tmp[i * 4 + 0] - tmp[i * 4 + 1];
    int s23 = tmp[i * 4 + 2] + tmp[i * 4 + 3];
    int d23 = tmp[i * 4 + 2] - tmp[i * 4 + 3];

    // The DCT coefficients are scaled by adding 1 and then right-shifting
    // by 1 (equivalent to integer division by 2) for rounding.
    d[i * 4 + 0] = (s01 + s23 + 1) >> 1;
    d[i * 4 + 1] = (s01 - s23 + 1) >> 1;
    d[i * 4 + 2] = (d01 - d23 + 1) >> 1;
    d[i * 4 + 3] = (d01 + d23 + 1) >> 1;
  }
}

// NEON version
static void dct4x4dc_neon(int *d) {
  // hold the intermediate results
  int tmp[16];

  // phase 1
  for (int i = 0; i < 4; i++) {
    // Load the 4 elements of the current row into NEON registers
    int32x4_t row = vld1q_s32(&d[i * 4]);

    //s01+s23 = d1+d2+d3+d4
    tmp[0 * 4 + i] = vaddvq_s32(row);

    //first element is s01, second element is s23 and then I perform s01-s23
    int32x2_t sum_halves = vpadd_s32(vget_low_s32(row), vget_high_s32(row));
    tmp[1 * 4 + i] = vget_lane_s32(sum_halves, 0) - vget_lane_s32(sum_halves, 1);

    //Extracts the low half of the elements from the row, which is [d1,d2]
    //extends the low half of the row, shifting the elements by 1 position to the right, so it becomes [d2, 0]
    //Subtracts the extended low half [d2, 0] from the original low half [d1, d2], element-wise
    //d01 = [d1 - d2, d2 - 0] = [d1 - d2, d2]
    //same for d23
    
    int32x2_t d01 = vsub_s32(vget_low_s32(row), vext_s32(vget_low_s32(row), vget_low_s32(row), 1));
    int32x2_t d23 = vsub_s32(vget_high_s32(row),vext_s32(vget_high_s32(row), vget_high_s32(row), 1));
    tmp[2 * 4 + i] = vget_lane_s32(d01, 0) - vget_lane_s32(d23, 0);
    tmp[3 * 4 + i] = vget_lane_s32(d01, 0) + vget_lane_s32(d23, 0);
  }

  // phase 2
  for (int i = 0; i < 4; i++) {

    //follow the same logic as phase 1
    int32x4_t tmp_row = vld1q_s32(&tmp[i * 4]);
    int32x4_t d_vector = vdupq_n_s32(0);
    int32x4_t one_vector = vdupq_n_s32(1);

    // contains s01, s23
    int32x2_t sum_halves = vpadd_s32(vget_low_s32(tmp_row), vget_high_s32(tmp_row));
    d_vector = vsetq_lane_s32(vaddvq_s32(tmp_row), d_vector, 0);                                           
    d_vector = vsetq_lane_s32(vget_lane_s32(sum_halves, 0) - vget_lane_s32(sum_halves, 1), d_vector, 1);
    
    int32x2_t d01 = vsub_s32(vget_low_s32(tmp_row), vext_s32(vget_low_s32(tmp_row), vget_low_s32(tmp_row), 1));
    int32x2_t d23 = vsub_s32(vget_high_s32(tmp_row),vext_s32(vget_high_s32(tmp_row), vget_high_s32(tmp_row), 1));
    d_vector = vsetq_lane_s32(vget_lane_s32(d01, 0) - vget_lane_s32(d23, 0), d_vector, 2);
    d_vector = vsetq_lane_s32(vget_lane_s32(d01, 0) + vget_lane_s32(d23, 0), d_vector, 3);

    //add 1 to each element and devide by 2 with the use of shifting right
    d_vector=vshrq_n_s32(vaddq_s32(d_vector, one_vector),1);
    vst1q_s32(&d[i * 4], d_vector);
  }
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