#include <smmintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N    100
#define ITER 100000
typedef uint16_t dctcoef;
void
print_vector(__m128i v) {
    uint16_t buf[8];
    _mm_storeu_si128((__m128i *) buf, v);
    for (int i = 0; i < 8; i++) {
        printf("%x ", buf[i]);
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
    /*
    printf("\n");
    for (int i = 0; i < 16; i += 4) {
        printf("%02x %02x %02x %02x\n", tmp[i], tmp[i + 1], tmp[i + 2], tmp[i + 3]);
    }*/

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
    // Load the rows of d into 128-bit vectors
    // transpose d and keep them in row1row2, row3row4
    // transpose 1 time
    __m128i row1row2 = _mm_loadu_si128((__m128i *) &d[0]);
    __m128i row3row4 = _mm_loadu_si128((__m128i *) &d[8]);   // load instead of set
    __m128i tmp1 = _mm_unpacklo_epi16(row1row2, row3row4);
    __m128i tmp3 = _mm_unpackhi_epi16(row1row2, row3row4);
    row1row2 = _mm_unpacklo_epi16(tmp1, tmp3);
    row3row4 = _mm_unpackhi_epi16(tmp1, tmp3);

    // 1st(d0 d4 d8 d12) + 2nd(d1 d5 d9 d13) +
    // 3rd(d2 d6 d10 d14) +4rth(d3 d7 d11 d15)

    // 1st + 2nd -3rd -4rth (same logic)
    __m128i totalSum1 = _mm_add_epi16(row1row2, row3row4);
    __m128i totalSum2 = _mm_sub_epi16(row1row2, row3row4);
    __m128i shuffled1 = _mm_shuffle_epi32(totalSum1, _MM_SHUFFLE(2, 3, 3, 2));
    __m128i shuffled2 = _mm_shuffle_epi32(totalSum2, _MM_SHUFFLE(2, 3, 3, 2));
    totalSum1 = _mm_add_epi16(totalSum1, shuffled1);
    totalSum2 = _mm_add_epi16(totalSum2, shuffled2);
    // 1st - 2nd -3rd +4rth (same logic)
    __m128i maskFF = _mm_set1_epi16(0xFF);
    __m128i zero = _mm_setzero_si128();
    __m128i mask1 = _mm_slli_si128(maskFF, 8);
    __m128i mask2 = _mm_srli_si128(maskFF, 8);
    __m128i masked_part = _mm_and_si128(mask1, row1row2);
    __m128i neg_masked_part = _mm_sub_epi16(zero, masked_part);
    row1row2 = _mm_or_si128(_mm_andnot_si128(mask1, row1row2), neg_masked_part);
    masked_part = _mm_and_si128(mask2, row3row4);
    neg_masked_part = _mm_sub_epi16(zero, masked_part);
    row3row4 = _mm_or_si128(_mm_andnot_si128(mask2, row3row4), neg_masked_part);
    __m128i totalSum3 = _mm_add_epi16(row1row2, row3row4);
    __m128i shuffled3 = _mm_shuffle_epi32(totalSum3, _MM_SHUFFLE(2, 3, 3, 2));
    totalSum3 = _mm_add_epi16(totalSum3, shuffled3);
    // 1st - 2nd +3rd -4rth (same logic)
    __m128i totalSum4 = _mm_sub_epi16(row1row2, row3row4);
    __m128i shuffled4 = _mm_shuffle_epi32(totalSum4, _MM_SHUFFLE(2, 3, 3, 2));
    totalSum4 = _mm_add_epi16(totalSum4, shuffled4);

    // instead of storing back to d ,keep the intermediate results
    // PHASE 2
    // transpose in vectors again(no stores)
    row1row2 = _mm_unpacklo_epi64(totalSum1, totalSum2);
    row3row4 = _mm_unpacklo_epi64(totalSum3, totalSum4);

    __m128i ones = _mm_set1_epi32(1);   // to divide
    tmp1 = _mm_unpacklo_epi16(row1row2, row3row4);
    tmp3 = _mm_unpackhi_epi16(row1row2, row3row4);
    row1row2 = _mm_unpacklo_epi16(tmp1, tmp3);
    row3row4 = _mm_unpackhi_epi16(tmp1, tmp3);
    __m128i row1_32 = _mm_unpacklo_epi16(row1row2, zero);
    __m128i row2_32 = _mm_unpackhi_epi16(row1row2, zero);
    __m128i row3_32 = _mm_unpacklo_epi16(row3row4, zero);
    __m128i row4_32 = _mm_unpackhi_epi16(row3row4, zero);
    // 1st + 2nd +3rd +4rth (same logic)
    __m128i totalSum11 = _mm_add_epi32(row1_32, row2_32);
    __m128i totalSum12 = _mm_add_epi32(row3_32, row4_32);
    totalSum11 = _mm_add_epi32(totalSum11, totalSum12);
    totalSum11 = _mm_add_epi32(totalSum11, ones);
    totalSum11 = _mm_srli_epi32(totalSum11, 1);
    __m128i mask = _mm_set1_epi32(0x0000FFFF);

    // 1st + 2nd -3rd -4rth (same logic)
    // Perform bitwise AND operation to keep only the lower 16 bits of each 32-bit integer
    totalSum11 = _mm_and_si128(totalSum11, mask);
    totalSum11 = _mm_packus_epi32(totalSum11, zero);

    __m128i totalSum21 = _mm_add_epi32(row1_32, row2_32);
    __m128i totalSum22 = _mm_add_epi32(row3_32, row4_32);
    totalSum21 = _mm_sub_epi32(totalSum21, totalSum22);
    totalSum21 = _mm_add_epi32(totalSum21, ones);
    totalSum21 = _mm_srli_epi32(totalSum21, 1);
    totalSum21 = _mm_and_si128(totalSum21, mask);   // keep only the lower 16 bits of each 32-bit integer
    totalSum21 = _mm_packus_epi32(totalSum21, zero);
    // 1st - 2nd -3rd +4rth (same logic)
    __m128i totalSum31 = _mm_sub_epi32(row1_32, row2_32);
    __m128i totalSum32 = _mm_sub_epi32(row4_32, row3_32);
    totalSum31 = _mm_add_epi32(totalSum31, totalSum32);
    totalSum31 = _mm_add_epi32(totalSum31, ones);
    totalSum31 = _mm_srli_epi32(totalSum31, 1);
    totalSum31 = _mm_and_si128(totalSum31, mask);   // keep only the lower 16 bits of each 32-bit integer
    totalSum31 = _mm_packus_epi32(totalSum31, zero);
    // 1st - 2nd +3rd -4rth (same logic)
    __m128i totalSum41 = _mm_sub_epi32(row1_32, row2_32);
    __m128i totalSum42 = _mm_sub_epi32(row3_32, row4_32);
    totalSum41 = _mm_add_epi32(totalSum41, totalSum42);
    totalSum41 = _mm_add_epi32(totalSum41, ones);
    totalSum41 = _mm_srli_epi32(totalSum41, 1);
    totalSum41 = _mm_and_si128(totalSum41, mask);   // keep only the lower 16 bits of each 32-bit integer
    totalSum41 = _mm_packus_epi32(totalSum41, zero);

    // transpose in vectors again(no stores)
    row1row2 = _mm_unpacklo_epi64(totalSum11, totalSum21);
    row3row4 = _mm_unpacklo_epi64(totalSum31, totalSum41);
    tmp1 = _mm_unpacklo_epi16(row1row2, row3row4);
    tmp3 = _mm_unpackhi_epi16(row1row2, row3row4);
    _mm_storel_epi64((__m128i *) &d[0], _mm_move_epi64(_mm_unpacklo_epi16(tmp1, tmp3)));
    _mm_storel_epi64((__m128i *) &d[4], _mm_srli_si128(_mm_unpacklo_epi16(tmp1, tmp3), 8));
    _mm_storel_epi64((__m128i *) &d[8], _mm_move_epi64(_mm_unpackhi_epi16(tmp1, tmp3)));
    _mm_storel_epi64((__m128i *) &d[12], _mm_srli_si128(_mm_unpackhi_epi16(tmp1, tmp3), 8));
}
int
main() {
    struct timespec start, mid, end;
    long s1sum = 0, s2sum = 0, n1sum = 0, n2sum = 0;
    int z = ITER;
    while (z--) {
        srand(time(NULL));
        dctcoef matrix[16];
        dctcoef matrix2[16];

        for (int i = 0; i < 16; i++) {
            matrix[i] = rand() & 0xFF;   // 8 bit unsigned
            matrix2[i] = matrix[i];
        }
        /*
        printf("Original matrix:\n");
        for (int i = 0; i < 16; i += 4) {
            printf("%02x %02x %02x %02x\n", matrix[i], matrix[i + 1], matrix[i + 2], matrix[i + 3]);
        }
        */
        clock_gettime(CLOCK_MONOTONIC, &start);
        dct4x4dc(matrix);
        clock_gettime(CLOCK_MONOTONIC, &mid);
        dct4x4dc_sse(matrix2);
        clock_gettime(CLOCK_MONOTONIC, &end);
        /*
        printf("\nMatrix after dct4x4dc:\n");
        for (int i = 0; i < 16; i += 4) {
            printf("%02x %02x %02x %02x\n", matrix[i], matrix[i + 1], matrix[i + 2], matrix[i + 3]);
        }

        printf("\nMatrix2 after dct4x4dc_sse:\n");
        for (int i = 0; i < 16; i += 4) {
            printf("%02x %02x %02x %02x\n", matrix2[i], matrix2[i + 1], matrix2[i + 2], matrix2[i + 3]);
        }
        */
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
        s1sum += seconds1;
        s2sum += seconds2;
        n1sum += nanoseconds1;
        n2sum += nanoseconds2;
        if (n1sum > 1000000000) {
            s1sum++;
            n1sum -= 1000000000;
        }
        if (n2sum > 1000000000) {
            s2sum++;
            n2sum -= 1000000000;
        }
    }
    /*
    printf("scalar: %ld.%09ld seconds\n", seconds1, nanoseconds1);
    printf("SSE   : %ld.%09ld seconds\n", seconds2, nanoseconds2);
    */
    printf("scalar: %ld.%09ld seconds\n", s1sum, n1sum);
    printf("SSE   : %ld.%09ld seconds\n", s2sum, n2sum);
    return 0;
}