#include <emmintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1000000000
typedef uint16_t dctcoef;
void
print_vector(__m128i v) {
    uint16_t buf[8];
    _mm_storeu_si128((__m128i *) buf, v);
    for (int i = 0; i < 8; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

void
dct4x4dc(dctcoef d[16]) {
    dctcoef tmp[16];

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
    for (int i = 0; i < 16; i = i + 4) {
        printf("in dct_c %02x %02x %02x %02x\n", tmp[0 + i], tmp[1 + i], tmp[2 + i], tmp[3 + i]);
    }
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

void
dct4x4dc_sse(dctcoef d[16]) {
    dctcoef dT[4][4];   // This will hold the transposed matrix

    // Load the rows of d into 128-bit vectors
    __m128i row1row2 = _mm_set_epi16(d[7], d[6], d[5], d[4], d[3], d[2], d[1], d[0]);
    __m128i row3row4 = _mm_set_epi16(d[15], d[14], d[13], d[12], d[11], d[10], d[9], d[8]);

    __m128i tmp1 = _mm_unpacklo_epi16(row1row2, row3row4);
    __m128i tmp3 = _mm_unpackhi_epi16(row1row2, row3row4);
    __m128i tmp1_new = _mm_unpacklo_epi16(tmp1, tmp3);
    __m128i tmp3_new = _mm_unpackhi_epi16(tmp1, tmp3);
    __m128i upper_half_tmp1 = _mm_srli_si128(tmp1_new, 8);
    _mm_storel_epi64((__m128i *) &dT[1][0], upper_half_tmp1);
    _mm_storel_epi64((__m128i *) &dT[0][0], _mm_move_epi64(tmp1_new));
    _mm_storel_epi64((__m128i *) &dT[3][0], _mm_srli_si128(tmp3_new, 8));
    _mm_storel_epi64((__m128i *) &dT[2][0], _mm_move_epi64(tmp3_new));
    // load rows from transposed d
    row1row2 = _mm_loadu_si128((__m128i *) dT[0]);
    row3row4 = _mm_loadu_si128((__m128i *) dT[2]);
    // 1st + 2nd +3rd +4rth
    __m128i totalSum = _mm_add_epi16(row1row2, row3row4);
    __m128i shuffled = _mm_shuffle_epi32(totalSum, _MM_SHUFFLE(2, 3, 3, 2));
    totalSum = _mm_add_epi16(totalSum, shuffled);
    _mm_storel_epi64((__m128i *) dT[0], totalSum);
    // 1st + 2nd -3rd -4rth
    totalSum = _mm_sub_epi16(row1row2, row3row4);
    shuffled = _mm_shuffle_epi32(totalSum, _MM_SHUFFLE(2, 3, 3, 2));
    totalSum = _mm_add_epi16(totalSum, shuffled);
    _mm_storel_epi64((__m128i *) dT[1], totalSum);
    // 1st - 2nd -3rd +4rth
    __m128i mask1 = _mm_setr_epi16(0, 0, 0, 0, -1, -1, -1, -1);
    __m128i mask2 = _mm_setr_epi16(-1, -1, -1, -1, 0, 0, 0, 0);
    __m128i zero = _mm_setzero_si128();
    __m128i signMask1 = _mm_cmplt_epi16(mask1, zero);
    __m128i signMask2 = _mm_cmplt_epi16(mask2, zero);
    row1row2 = _mm_sub_epi16(_mm_andnot_si128(signMask1, row1row2), _mm_and_si128(signMask1, row1row2));
    row3row4 = _mm_sub_epi16(_mm_andnot_si128(signMask2, row3row4), _mm_and_si128(signMask2, row3row4));
    totalSum = _mm_add_epi16(row1row2, row3row4);
    shuffled = _mm_shuffle_epi32(totalSum, _MM_SHUFFLE(2, 3, 3, 2));
    totalSum = _mm_add_epi16(totalSum, shuffled);
    _mm_storel_epi64((__m128i *) dT[2], totalSum);
    // 1st - 2nd +3rd -4rth
    totalSum = _mm_sub_epi16(row1row2, row3row4);
    shuffled = _mm_shuffle_epi32(totalSum, _MM_SHUFFLE(2, 3, 3, 2));
    totalSum = _mm_add_epi16(totalSum, shuffled);
    _mm_storel_epi64((__m128i *) dT[3], totalSum);

    /*
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%02x ", (uint16_t) dT[i][j]);
        }
        printf("\n");
    }*/
}
int
main() {
    struct timespec start, mid, end;
    srand(time(NULL));
    dctcoef matrix[16];
    dctcoef matrix2[16];
    for (int i = 0; i < 16; i++) {
        matrix[i] = rand() & 0xFF;   // 8 bit unsigned
        matrix2[i] = matrix[i];
    }
    printf("Original matrix:\n");
    for (int i = 0; i < 16; i += 4) {
        printf("%02x %02x %02x %02x\n", matrix[i], matrix[i + 1], matrix[i + 2], matrix[i + 3]);
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    dct4x4dc(matrix);
    clock_gettime(CLOCK_MONOTONIC, &mid);
    dct4x4dc_sse(matrix2);
    clock_gettime(CLOCK_MONOTONIC, &end);

    // printf("\nMatrix after dct4x4dc:\n");
    for (int i = 0; i < 16; i += 4) {
        // printf("%02x %02x %02x %02x\n", matrix[i], matrix[i + 1], matrix[i + 2], matrix[i + 3]);
    }

    // printf("\nMatrix2 after dct4x4dc_sse:\n");
    for (int i = 0; i < 16; i += 4) {
        // printf("%02x %02x %02x %02x\n", matrix2[i], matrix2[i + 1], matrix2[i + 2], matrix2[i + 3]);
    }

    long seconds1 = mid.tv_sec - start.tv_sec;
    long nanoseconds1 = mid.tv_nsec - start.tv_nsec;
    if (nanoseconds1 < 0) {
        seconds1--;
        nanoseconds1 += 1000000000;
    }
    long seconds2 = end.tv_sec - mid.tv_sec;
    long nanoseconds2 = end.tv_nsec - mid.tv_nsec;
    if (nanoseconds2 < 0) {
        seconds2--;
        nanoseconds2 += 1000000000;
    }
    printf("scalar: %ld.%09ld seconds\n", seconds1, nanoseconds1);
    printf("SSE   : %ld.%09ld seconds\n", seconds2, nanoseconds2);
    return 0;
}