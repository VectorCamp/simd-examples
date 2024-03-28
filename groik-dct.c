
/* vectorized version of the above */
#include "ppc/ppccommon.h"
#include <altivec.h>
static void v_dct4x4dc( dctcoef d[16] )
{
    /* dctcoef is a uint32 
     * so this is 128 bits a row, 512b for the
     * whole matrix. 
     * vsx from the docs i see supports 16 byte /128b vector types,
     * so that's one row. 
     */
    /* we could do this ourselves but the exact same union exists in ppccommon.h */

	vector unsigned int ones = {1,1,1,1};
	vec_u32_u da0, da1, da2, da3;
	vec_u32_u tmp0, tmp1, tmp2, tmp3;
	vector unsigned int b0, b1, b2, b3;


    /* need to set up, da as, 
     *  vector unsigned int {d[0],d[4],d[8],d[12]}
     * ... d[1],d[5],d[9],d[13]
     *  d[2],d[6],d[10],d[14]
     *  d[3],d[7],d[11],d[15]
     */
	da0.s[0]=d[0]; da0.s[1]=d[4]; da0.s[2]=d[8]; da0.s[3]=d[12];
	da1.s[0]=d[1]; da1.s[1]=d[5]; da1.s[2]=d[9]; da1.s[3]=d[13];
	da2.s[0]=d[2]; da2.s[1]=d[6]; da2.s[2]=d[10]; da2.s[3]=d[14];
	da3.s[0]=d[3]; da3.s[1]=d[7]; da3.s[2]=d[11]; da3.s[3]=d[15];

/* then, a vector add between da[0] and da[1], into b[0].  - sums01
 * then a vector subtract between da[0] and da[1], into b[1] -difs01
 * then a vector add between da[2] and da[3], into b[2] - sums23
 * then a vector subtract between da[2] and da[3], into b[3] -difs23
 */

	b0=vec_add(da0.v,da1.v);
	b1=vec_sub(da0.v,da1.v);
	b2=vec_add(da2.v,da3.v);
	b3=vec_sub(da2.v,da3.v);

/* then, a vector add of b[0] + b[1], into tmp[0]
 * then, a vector subtract  of b[0] - b[1], into tmp[1]
 * then, a vector subtract of b[2] - b[3], into tmp[2]
 * then, a vector add of b[2] + b[3], into tmp[3]
 */
	tmp0.v=vec_add(b0,b1);
	tmp1.v=vec_sub(b0,b1);
	tmp2.v=vec_sub(b2,b3);
	tmp3.v=vec_add(b2,b3);

/* now were halfway through, with what the first loop did.
 * we have a similar job to do once more on the values in tmp. 
 */
    /* need to set up, da as, 
    *  vector unsigned int {tmp[0],tmp[4],tmp[8],tmp[12]}
    * ... tmp[1],tmp[5],tmp[9],tmp[13]
    *  tmp[2],tmp[6],tmp[10],tmp[14]
    *  tmp[3],tmp[7],tmp[11],tmp[15]
    */
	da0.s[0]=tmp0.s[0]; da0.s[1]=tmp1.s[0]; da0.s[2]=tmp2.s[0]; da0.s[3]=tmp3.s[0];
	da1.s[0]=tmp0.s[1]; da1.s[1]=tmp1.s[1]; da1.s[2]=tmp2.s[1]; da1.s[3]=tmp3.s[1];
	da2.s[0]=tmp0.s[2]; da2.s[1]=tmp1.s[2]; da2.s[2]=tmp2.s[2]; da2.s[3]=tmp3.s[2];
	da3.s[0]=tmp0.s[3]; da3.s[1]=tmp1.s[3]; da3.s[2]=tmp2.s[3]; da3.s[3]=tmp3.s[3];
/* then the same two sets of operations commented above, again leaving
 * results in tmp. */
	b0=vec_add(da0.v,da1.v);
	b1=vec_sub(da0.v,da1.v);
	b2=vec_add(da2.v,da3.v);
	b3=vec_sub(da2.v,da3.v);

	tmp0.v=vec_add(b0,b1);
	tmp1.v=vec_sub(b0,b1);
	tmp2.v=vec_sub(b2,b3);
	tmp3.v=vec_add(b2,b3);

/* now the final steps, adding one and dividing in half */
/* then, fill ones with {1,1,1,1} */

/* then add tmp[0]+=ones
 * then add tmp[1]+=ones
 * then add tmp[2]+=ones
 * then add tmp[3]+=ones
 */

	b0=vec_add(tmp0.v,ones);
	b1=vec_add(tmp1.v,ones);
	b2=vec_add(tmp2.v,ones);
	b3=vec_add(tmp3.v,ones);

/* then shift d[0]=tmp[0]<<1
 * then shift d[0]=[1]=tmp[1]<<1
 * then shift d[0]=[2]=tmp[2]<<1
 * then shift d[0]=[3]=tmp[3]<<1
 * , finally putting the result values back into d.
 */
	tmp0.v=vec_sl(b0,ones);
	tmp1.v=vec_sl(b1,ones);
	tmp2.v=vec_sl(b2,ones);
	tmp3.v=vec_sl(b3,ones);

	/* aha turned out i read the wrong typedef, 
	 * these are actually 16 bit vals, thats why this segfaulted
	memcpy(d,tmp0.s,4*sizeof(uint32_t));
	memcpy(d+4,tmp1.s,4*sizeof(uint32_t));
	memcpy(d+8,tmp2.s,4*sizeof(uint32_t));
	memcpy(d+12,tmp3.s,4*sizeof(uint32_t));
	*/
	/*
	d[0]=tmp0.s[0];
	d[1]=tmp0.s[1];
	d[2]=tmp0.s[2];
	d[3]=tmp0.s[3];
	d[4]=tmp1.s[0];
	d[5]=tmp1.s[1];
	d[6]=tmp1.s[2];
	d[7]=tmp1.s[3];
	d[8]=tmp2.s[0];
	d[9]=tmp2.s[1];
	d[10]=tmp2.s[2];
	d[11]=tmp2.s[3];
	d[12]=tmp3.s[0];
	d[13]=tmp3.s[1];
	d[14]=tmp3.s[2];
	d[15]=tmp3.s[3];
	*/
	memcpy(d,tmp0.s,4*sizeof(dctcoef));
	memcpy(d+4,tmp1.s,4*sizeof(dctcoef));
	memcpy(d+8,tmp2.s,4*sizeof(dctcoef));
	memcpy(d+12,tmp3.s,4*sizeof(dctcoef));

}
