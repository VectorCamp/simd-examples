#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define N 1000000000
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
    //for (int i = 0; i < 16; i = i + 4) {
    //    printf("in c %04x %04x %04x %04x\n", tmp[0 + i], tmp[1 + i], tmp[2 + i], tmp[3 + i]);
    //}
   // 	printf("\n");
    for (int i = 0; i < 4; i++) {
        int s01 = tmp[i * 4 + 0] + tmp[i * 4 + 1];
        int d01 = tmp[i * 4 + 0] - tmp[i * 4 + 1];
        int s23 = tmp[i * 4 + 2] + tmp[i * 4 + 3];
        int d23 = tmp[i * 4 + 2] - tmp[i * 4 + 3];

/*
        d[i * 4 + 0] = (s01 + s23 + 1) >> 1;
        d[i * 4 + 1] = (s01 - s23 + 1) >> 1;
        d[i * 4 + 2] = (d01 - d23 + 1) >> 1;
        d[i * 4 + 3] = (d01 + d23 + 1) >> 1;
*/
	/* try skipping the shift for a sec */
        d[i * 4 + 0] = (s01 + s23 + 1);
        d[i * 4 + 1] = (s01 - s23 + 1);
        d[i * 4 + 2] = (d01 - d23 + 1);
        d[i * 4 + 3] = (d01 + d23 + 1);
    }
}


/* vectorized version of the above */


#include <altivec.h>
#define vec_u16_t vector unsigned short

typedef union {
  uint16_t s[8];
  vec_u16_t v;
} vec_u16_u;

static void v_dct4x4dc( dctcoef d[16] )
{
	vector unsigned short ones = {1,1,1,1,1,1,1,1};
	vec_u16_u da0, da1;
	vec_u16_u tmp0, tmp1;
	vec_u16_u b0, b1, b2, b3;


    /* need to set up, da as, 
     *  vector unsigned int {d[0],d[4],d[8],d[12]}
     * ... d[1],d[5],d[9],d[13]
     *  d[2],d[6],d[10],d[14]
     *  d[3],d[7],d[11],d[15]
     */

/* as these are shorts we can fit 8 of them in now.
 * We can do both halves at once.  */

	da0.s[0]=d[0]; da0.s[1]=d[4]; da0.s[2]=d[8]; da0.s[3]=d[12];
	da0.s[4]=d[2]; da0.s[5]=d[6]; da0.s[6]=d[10]; da0.s[7]=d[14];

	da1.s[0]=d[1]; da1.s[1]=d[5]; da1.s[2]=d[9]; da1.s[3]=d[13];
	da1.s[4]=d[3]; da1.s[5]=d[7]; da1.s[6]=d[11]; da1.s[7]=d[15];


/* then, a vector add between da[0] and da[1], into b[0].  - sums01
 * then a vector subtract between da[0] and da[1], into b[1] -difs01
 * then a vector add between da[2] and da[3], into b[2] - sums23
 * then a vector subtract between da[2] and da[3], into b[3] -difs23
 */

	b0.v=vec_add(da0.v,da1.v);
	b1.v=vec_sub(da0.v,da1.v);

/* then, a vector add of b[0] + b[1], into tmp[0]
 * then, a vector subtract  of b[0] - b[1], into tmp[1]
 * then, a vector subtract of b[2] - b[3], into tmp[2]
 * then, a vector add of b[2] + b[3], into tmp[3]
 */

	/* now because were using 8-long vectors we need to swap the halves of
	 * two vectors - the first half of b0 and the first half of b1, and the 
	 * second half of b0 and b1 */  
	memcpy(b2.s,b0.s,8*sizeof(unsigned short));

	/* stick first half of b1 into second half of b0 */
	memcpy(b0.s+4,b1.s,4*sizeof(unsigned short));
	/* stick second half of b0 (saved in b2) in first half of b1 */
	memcpy(b1.s,b2.s+4,4*sizeof(unsigned short));

	tmp0.v=vec_add(b0.v,b1.v);
	tmp1.v=vec_sub(b0.v,b1.v);


/* now were halfway through, with what the first loop did.
 * we have a similar job to do once more on the values in tmp. 
 */
	/*
	printf("in v %04x %04x %04x %04x\n", tmp0.s[0], tmp0.s[1], tmp0.s[2], tmp0.s[3]);
	printf("in v %04x %04x %04x %04x\n", tmp1.s[0], tmp1.s[1], tmp1.s[2], tmp1.s[3]);
	printf("in v %04x %04x %04x %04x\n", tmp0.s[4], tmp0.s[5], tmp0.s[6], tmp0.s[7]);
	printf("in v %04x %04x %04x %04x\n", tmp1.s[4], tmp1.s[5], tmp1.s[6], tmp1.s[7]);
	printf("\n");
	*/

    /* need to set up, da as, 
    *  vector unsigned int {tmp[0],tmp[4],tmp[8],tmp[12]}
    * ... tmp[1],tmp[5],tmp[9],tmp[13]
    *  tmp[2],tmp[6],tmp[10],tmp[14]
    *  tmp[3],tmp[7],tmp[11],tmp[15]
    */
	/* this might be where a transpose operation would be handy. 
	 * for now we copy it manually */

	/* now with 8 element vectors, the 0 and 3 columns looking vertically are
	* from tmp0 and the middle two are from tmp1 */
	da0.s[0]=tmp0.s[0]; da0.s[1]=tmp1.s[0]; da0.s[2]=tmp1.s[4]; da0.s[3]=tmp0.s[4];
	da1.s[0]=tmp0.s[1]; da1.s[1]=tmp1.s[1]; da1.s[2]=tmp1.s[5]; da1.s[3]=tmp0.s[5];

	da0.s[4]=tmp0.s[2]; da0.s[5]=tmp1.s[2]; da0.s[6]=tmp1.s[6]; da0.s[7]=tmp0.s[6];
	da1.s[4]=tmp0.s[3]; da1.s[5]=tmp1.s[3]; da1.s[6]=tmp1.s[7]; da1.s[7]=tmp0.s[7];


/* then the same two sets of operations commented above, again leaving
 * results in tmp. */
	b0.v=vec_add(da0.v,da1.v);
	b1.v=vec_sub(da0.v,da1.v);

	/* again because were using 8-long vectors we need to swap the halves of
	 * two vectors - the first half of b0 and the first half of b1, and the 
	 * second half of b0 and b1 */  
	memcpy(b2.s,b0.s,8*sizeof(unsigned short));

	/* stick first half of b1 into second half of b0 */
	memcpy(b0.s+4,b1.s,4*sizeof(unsigned short));
	/* stick second half of b0 (saved in b2) in first half of b1 */
	memcpy(b1.s,b2.s+4,4*sizeof(unsigned short));

	tmp0.v=vec_add(b0.v,b1.v);
	tmp1.v=vec_sub(b0.v,b1.v);

/* now the final steps, adding one and dividing in half */
/* then, fill ones with {1,1,1,1} */

/* then add tmp[0]+=ones
 * then add tmp[1]+=ones
 * then add tmp[2]+=ones
 * then add tmp[3]+=ones
 */

	b0.v=vec_add(tmp0.v,ones);
	b1.v=vec_add(tmp1.v,ones);

/* then shift d[0]=tmp[0]>>1
 * then shift d[0]=[1]=tmp[1]>>1
 * then shift d[0]=[2]=tmp[2]>>1
 * then shift d[0]=[3]=tmp[3]>>1
 * , finally putting the result values back into d.
 */
	/* try skipping the shift 
	tmp0.v=vec_sr(b0.v,ones);
	tmp1.v=vec_sr(b1.v,ones);
	*/
	tmp0=b0;
	tmp1=b1;

	/* the mapping is again, tmp0 going down cols 0 and 3, tmp1 down cols 1,2 */

        d[0]=tmp0.s[0];
        d[4]=tmp0.s[1];
        d[8]=tmp0.s[2];
        d[12]=tmp0.s[3];
        d[1]=tmp1.s[0];
        d[5]=tmp1.s[1];
        d[9]=tmp1.s[2];
        d[13]=tmp1.s[3];
        d[2]=tmp1.s[4];
        d[6]=tmp1.s[5];
        d[10]=tmp1.s[6];
        d[14]=tmp1.s[7];
        d[3]=tmp0.s[4];
        d[7]=tmp0.s[5];
        d[11]=tmp0.s[6];
        d[15]=tmp0.s[7];

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
    v_dct4x4dc(matrix2);
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("\nMatrix after dct4x4dc:\n");
    for (int i = 0; i < 16; i += 4) {
        printf("%04x %04x %04x %04x\n", matrix[i], matrix[i + 1], matrix[i + 2], matrix[i + 3]);
    }

    printf("\nMatrix2 after v_dct4x4dc:\n");
    for (int i = 0; i < 16; i += 4) {
        printf("%04x %04x %04x %04x\n", matrix2[i], matrix2[i + 1], matrix2[i + 2], matrix2[i + 3]);
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
    printf("VSX   : %ld.%09ld seconds\n", seconds2, nanoseconds2);
    return 0;
}
