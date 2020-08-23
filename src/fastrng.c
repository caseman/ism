/****************************************************************************
*
* Copyright (c) 2008 by Casey Duncan and contributors
* All Rights Reserved.
*
* This software is subject to the provisions of the MIT License
* A copy of the license should accompany this distribution.
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*
****************************************************************************/
/* Fast RNGs
 *
 * Derived from code at: 
 * http://www.cse.yorku.ca/~oz/marsaglia-rng.html
 * http://www.jstatsoft.org/v05/i08/paper
 *
 * $Id$
 */

#include "fastrng.h"
#include <float.h>

/*
   SHR3 is a 3-shift-register generator with period
   2^32-1. It uses y(n)=y(n-1)(I+L^17)(I+R^13)(I+L^5),
   with the y's viewed as binary vectors, L the 32x32
   binary matrix that shifts a vector left 1, and R its
   transpose. SHR3 seems to pass all except those
   related to the binary rank test, since 32 successive
   values, as binary vectors, must be linearly
   independent, while 32 successive truly random 32-bit
   integers, viewed as binary vectors, will be linearly
   independent only about 29% of the time.
*/
static uint32_t jz, jsr=123456789;
#define SHR3 (jz=jsr, jsr^=(jsr<<13), jsr^=(jsr>>17), jsr^=(jsr<<5),jz+jsr)

/*
   The MWC generator concatenates two 16-bit multiply-
   with-carry generators, x(n)=36969x(n-1)+carry,
   y(n)=18000y(n-1)+carry mod 2^16, has period about
   2^60 and seems to pass all tests of randomness
*/
static uint32_t z=362436069, w=521288629;
#define znew (z=36969*(z&65535)+(z>>16))
#define wnew (w=18000*(w&65535)+(w>>16))
#define MWC ((znew<<16)+wnew )

/*
   CONG is a congruential generator with the widely used 69069
   multiplier: x(n)=69069x(n-1)+1234567. It has period
   2^32. The leading half of its 32 bits seem to pass
   tests, but bits in the last half are too regular.
*/
static uint32_t jcong=380116160;
#define CONG (jcong=69069*jcong+1234567)

static uint32_t kn[128], ke[256];
static float wn[128], fn[128], we[256], fe[256];

/*
	Set the random number seed and initialize the ziggurat tables
*/
void
rand_seed(uint32_t s) 
{
	const double m1 = 2147483648.0, m2 = 4294967296.;
	double dn = 3.442619855899, tn=dn, vn = 9.91256303526217e-3;
	double de = 7.697117470131487, te=de, ve = 3.949659822581572e-3;
	double q;
	int i;

	/* Seed RNGs */
	jsr = s;
	z = SHR3;
	w = SHR3;
	jcong = SHR3;

	/* Setup ziggurat tables for rand_norm() */
	q = vn / exp(-.5 * dn*dn);
	kn[0] = (uint32_t)((dn / q)*m1);
	kn[1] = 0;

	wn[0] = (float)(q / m1);
	wn[127] = (float)(dn / m1);

	fn[0] = 1.0f;
	fn[127] = (float)exp(-.5 * dn*dn);

    for (i = 126; i >= 1; i--) {
		dn = sqrt(-2. * log(vn / dn + exp(-.5 * dn*dn)));
		kn[i+1] = (uint32_t)((dn / tn)*m1);
		tn = dn;
		fn[i] = (float)exp(-.5 * dn*dn);
		wn[i] = (float)(dn / m1);
    }

	/* Setup tables for rand_expo() */
	q = ve / exp(-de);
	ke[0] = (uint32_t)((de / q)*m2);
	ke[1] = 0;

	we[0] = (float)(q / m2);
	we[255] = (float)(de / m2);

	fe[0] = 1.0f;
	fe[255] = (float)exp(-de);

	for (i=254; i>=1; i--) {
		de = -log(ve / de + exp(-de));
		ke[i+1] = (uint32_t)((de / te)*m2);
		te = de;
		fe[i] = (float)exp(-de);
		we[i] = (float)(de / m2);
	}
}

/*
   Generate a 32-bit random number in with the interval [0,0xffffffff]

   combines the two multiply-with-carry
   generators in MWC with the 3-shift register SHR3 and
   the congruential generator CONG, using addition and
   exclusive-or. Period about 2^123.
*/
uint32_t rand_int32(void) 
{
	return (MWC ^ CONG) + SHR3;
}

/*
	Generate a random number with uniform distribution in the interval (0, 1.0]
*/
float rand_uni(void) 
{
	return 0.5f + (signed)((MWC ^ CONG) + SHR3) * .2328306e-9f;
}

#define RIGHT_TAIL 3.442620f
#define ONE_OVER_RIGHT_TAIL 0.2904764f

/*
	Generate variates for rand_norm on rejection.

	This should be rarely called, and in practice it gets invoked 
	for about 2.75% of rand_norm() calls, which is higher than
	expected from the theory, but performance is still excellent.
*/
static float norm_outlier(int32_t hz, int32_t iz)
{
	float x, y;

	for(;;)
	{
		x = hz * wn[iz];

		/* handle the base strip */
		if (iz == 0) {
			do { 
				x = -logf(rand_uni()) * ONE_OVER_RIGHT_TAIL; 
				y = -logf(rand_uni());
			} while (y + y < x * x);
			return (hz > 0) ? RIGHT_TAIL + x : -RIGHT_TAIL - x;
		}

		/* handle the wedges of other strips */
		if (fn[iz] + rand_uni()*(fn[iz-1] - fn[iz]) < expf(-0.5f * x*x)) 
			return x;

		/* Try again from the top and see if we can exit */
		hz = rand_int32();
		iz = hz & 127;
		if ((uint32_t)labs(hz) < kn[iz]) 
			return hz * wn[iz];
	}
}

/*
	Generate a random number with normal distribution using
	the ziggurat method.

	mu is the mean and sigma is the std deviation
*/
float rand_norm(const float mu, const float sigma)
{
	int32_t hz = rand_int32();
	int32_t iz = hz & 127;
	return mu + (((uint32_t)labs(hz) < kn[iz]) ? hz * wn[iz] : norm_outlier(hz, iz)) * sigma;
}

/*
	Generate variates for rand_expo on rejection.

	This should be rarely called, and in practice it gets invoked 
	for about 2.22% of rand_expo() calls, which is higher than
	expected from the theory
*/
static float expo_outlier(uint32_t hz, uint32_t iz)
{
	float x;

	for(;;)
	{
		if (iz == 0) 
			return 7.69711f - logf(rand_uni());

		 x = jz * we[iz]; 
		 if (fe[iz] + rand_uni()*(fe[iz-1] - fe[iz]) < expf(-x)) 
		 	return x;

		/* Try again from the top and see if we can exit */
		hz = rand_int32();
		iz = hz & 255;
		if (hz < ke[iz]) 
			return hz * we[iz];
	}
}

/*
	Generate a random number with exponential distribution using
	the ziggurat method.

	mu is the desired mean.
*/
inline float
rand_expo(const float mu)
{
	uint32_t hz = rand_int32();
	uint32_t iz = hz & 255;
	return ((hz < ke[iz]) ? hz * we[iz] : expo_outlier(hz, iz)) * mu;
}

