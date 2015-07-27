#include "decompose.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SQRT2PI 2.506628253

#ifdef TURBO_DECOMPOSE

int DecomposeHistogram( float * histogram, int bins, float * out_means, float * out_amps, float * out_sigmas, int max_dists, double default_sigma, int iterations )
{
	//Step 1: Find the actual peaks.

	int i;
	int peak = 0;
	for( i = 0; i < bins; i++ )
	{
		float offset = 0;
		float prev = histogram[(i-1+bins)%bins];
		float this = histogram[i];
		float next = histogram[(i+1)%bins];

		if( prev > this || next > this ) continue;
		if( prev == this && next == this ) continue;

		//i is at a peak... 
		float totaldiff = (( this - prev ) + ( this - next ));
		float porpdiffP = (this-prev)/totaldiff; //close to 0 = closer to this side... 0.5 = in the middle ... 1.0 away.
		float porpdiffN = (this-next)/totaldiff;

		if( porpdiffP < porpdiffN )
		{
			//Closer to prev.
			offset = -(0.5 - porpdiffP);
		}
		else
		{
			offset = (0.5 - porpdiffN);
		}

		out_means[peak] = i + offset;

		//XXX XXX TODO Examine difference or relationship of "this" and "totaldiff"
		out_amps[peak] = this * 4;
			//powf( totaldiff, .8) * 10;//powf( totaldiff, .5 )*4; //
		out_sigmas[peak] = default_sigma;
		peak++;
	}

	for( i = peak; i < max_dists; i++ )
	{
		out_means[i] = -1;
		out_amps[i] = 0;
		out_sigmas[i] = default_sigma;
	}

	return peak;
}

//Yick: Doesn't match.. I guess it's only for debugging, right?
float CalcHistAt( float pt, int bins, float * out_means, float * out_amps, float * out_sigmas, int cur_dists )
{
	int i;
	float mark = 0.0;
	for( i = 0; i < cur_dists; i++ )
	{
		float amp  = out_amps[i];
		float mean = out_means[i];
		float var  = out_sigmas[i];

		float x = mean - pt;
		if( x < - bins / 2 ) x += bins;
		if( x > bins / 2 )   x -= bins;
		float nrm = amp / (var * SQRT2PI ) * expf( - ( x * x ) / ( 2 * var * var ) );
		mark += nrm;
	}
	return mark;
}


#else

float AttemptDecomposition( float * histogram, int bins, float * out_means, float * out_amps, float * out_sigmas, int cur_dists );
float CalcHistAt( float pt, int bins, float * out_means, float * out_amps, float * out_sigmas, int cur_dists );


int DecomposeHistogram( float * histogram, int bins, float * out_means, float * out_amps, float * out_sigmas, int max_dists, double default_sigma, int iterations )
{
	//NOTE: The sigma may change depending on all sorts of things, maybe?

	int sigs = 0;
	int i;

	int went_up = 0;

	float vhist = histogram[bins-1];

	for( i = 0; i <= bins; i++ )
	{
		float thishist = histogram[i%bins];
		if( thishist > vhist )
		{
			went_up = 1;
		}
		else if( went_up)
		{
			went_up = 0;
			out_amps[sigs] = thishist / (default_sigma + 1);
			out_sigmas[sigs] = default_sigma;
			out_means[sigs] = i-0.5;
			sigs++;
		}
		vhist = thishist;
	}
	if( sigs == 0 )
		return 0;

	int iteration;
	float errbest = AttemptDecomposition( histogram, bins, out_means, out_amps, out_sigmas, sigs );
	int dropped[bins];
	for( i = 0; i < bins; i++ )
	{
		dropped[i] = 0;
	}

	for( iteration = 0; iteration < iterations; iteration++ )
	{
		if( dropped[iteration%sigs] ) continue;
		//Tweak with the values until they are closer to what we want.
		float backup_mean = out_means[iteration%sigs];
		float backup_amps = out_amps[iteration%sigs];
		float backup_sigmas = out_sigmas[iteration%sigs];

		float mute = 20. / (iteration+20.);
#define RANDFN ((rand()%2)-0.5)*mute
//#define RANDFN ((rand()%10000)-5000.0) / 5000.0 * mute
//		out_sigmas[iteration%sigs] += RANDFN;
		out_means[iteration%sigs] += RANDFN;
		out_amps[iteration%sigs] += RANDFN;
		float err = AttemptDecomposition( histogram, bins, out_means, out_amps, out_sigmas, sigs );
		if( err > errbest )
		{
			out_means[iteration%sigs] = backup_mean;
			out_amps[iteration%sigs] = backup_amps;
			out_sigmas[iteration%sigs] = backup_sigmas;
		}
		else
		{
			if( out_amps[iteration%sigs] < 0.01 )
			{
				dropped[iteration%sigs] = 1;
				out_amps[iteration%sigs] = 0.0;
			}
			errbest = err;
		}

	}

//	printf( "%f / %f = %f\n", origerr, errbest, origerr/errbest );

	return sigs;
}

float CalcHistAt( float pt, int bins, float * out_means, float * out_amps, float * out_sigmas, int cur_dists )
{
	int i;
	float mark = 0.0;
	for( i = 0; i < cur_dists; i++ )
	{
		float amp  = out_amps[i];
		float mean = out_means[i];
		float var  = out_sigmas[i];

		float x = mean - pt;
		if( x < - bins / 2 ) x += bins;
		if( x > bins / 2 )   x -= bins;
		float nrm = amp / (var * SQRT2PI ) * expf( - ( x * x ) / ( 2 * var * var ) );
		mark += nrm;
	}
	return mark;
}

float AttemptDecomposition( float * histogram, int bins, float * out_means, float * out_amps, float * out_sigmas, int cur_dists )
{
	int i, j;
	float hist[bins];
	memcpy( hist, histogram, sizeof(hist) );
	for( i = 0; i < cur_dists; i++ )
	{
		float amp  = out_amps[i];
		float mean = out_means[i];
		float var  = out_sigmas[i];

		for( j = 0; j < bins; j++ )
		{
			float x = mean - j;
			if( x < - bins / 2 ) x += bins;
			if( x > bins / 2 )   x -= bins;
			float nrm = amp / (var * SQRT2PI ) * expf( - ( x * x ) / ( 2 * var * var ) );
			hist[j] -= nrm;
		}
	}

	float remain = 0;
	for( j = 0; j < bins; j++ )
	{
		remain += hist[j] * hist[j];
	}
	return remain;
}


#endif

