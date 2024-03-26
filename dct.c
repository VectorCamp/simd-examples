#include <emmintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef uint16_t dctcoef;
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
    __m128i tmp[2];
    tmp[0] = _mm_setzero_si128();   // Set first vector to zero
    tmp[1] = _mm_setzero_si128();   // Set second vector to zero

    for (int i = 0; i < 2; i++) {
        __m128i row1 = _mm_loadu_si128((__m128i *) &d[i * 8]);
        __m128i row2 = _mm_loadu_si128((__m128i *) &d[i * 8 + 4]);

        __m128i s01s23 = _mm_add_epi16(row1, row2);
        __m128i d01d23 = _mm_sub_epi16(row1, row2);

        __m128i tmp1 = _mm_unpacklo_epi64(s01s23, d01d23);
        __m128i tmp2 = _mm_unpackhi_epi64(s01s23, d01d23);

        tmp[i] = _mm_unpacklo_epi64(tmp1, tmp2);
    }
    for (int i = 0; i < 2; i++) {
        __m128i vec = tmp[i];

        __m128i s01s23 = _mm_add_epi16(vec, _mm_shuffle_epi32(vec, _MM_SHUFFLE(1, 0, 3, 2)));
        __m128i d01d23 = _mm_sub_epi16(vec, _mm_shuffle_epi32(vec, _MM_SHUFFLE(1, 0, 3, 2)));

        __m128i res1 = _mm_add_epi16(s01s23, _mm_set1_epi16(1));
        __m128i res2 = _mm_add_epi16(d01d23, _mm_set1_epi16(1));

        res1 = _mm_srai_epi16(res1, 1);
        res2 = _mm_srai_epi16(res2, 1);

        _mm_storeu_si128((__m128i *) &d[i * 8], res1);
        _mm_storeu_si128((__m128i *) &d[i * 8 + 4], res2);
    }
}
int
main() {
    struct timespec start, mid, end;
    srand(time(NULL));
    dctcoef matrix[16];
    dctcoef matrix2[16];
    for (int i = 0; i < 16; i++) {
        matrix[i] = rand() % 65536;
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

    printf("\nMatrix after dct4x4dc:\n");
    for (int i = 0; i < 16; i += 4) {
        printf("%02x %02x %02x %02x\n", matrix[i], matrix[i + 1], matrix[i + 2], matrix[i + 3]);
    }

    printf("\nMatrix2 after dct4x4dc_sse:\n");
    for (int i = 0; i < 16; i += 4) {
        printf("%02x %02x %02x %02x\n", matrix2[i], matrix2[i + 1], matrix2[i + 2], matrix2[i + 3]);
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