//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "filter.h"
#include <math.h>
#include <string.h>

/*
void FilterFoldedBinsIIRTWOPASS( float * folded, int bins, float iir )
{
	int i;
	float inv = 1.0 - iir;
	float v = 0.0;

	//Go through the data once, forward.
	for( i = 0; i < bins; i++ )
	{
		v = v * iir + folded[i] * inv;
	}

	//Start applying filter on second pass.
	for( i = 0; i < bins; i++ )
	{
		folded[i] = v * iir + folded[i] * inv;
	}

	//Go backwards.
	v = 0;
	for( i = bins-1; i >= 0; i-- )
	{
		v = v * iir + folded[i] * inv;
	}

	//Start applying filter on second pass.
	for( i = bins-1; i >= 0; i-- )
	{
		folded[i] = v * iir + folded[i] * inv;
	}
}*/


void FilterFoldedBinsBlob( float * folded, int bins, float strength, int iter )
{
	float tmp[bins];
	int i;
	int j;
	for( j = 0; j < iter; j++ )
	{
		memcpy( tmp, folded, sizeof( tmp ) );
		for( i = 0; i < bins; i++ )
		{
//			float left = tmp[(i-1+bins)%bins];
//			float right = tmp[(i-1+bins)%bins];
			float right = tmp[(i+bins+1)%bins];
			float left = tmp[(i+bins-1)%bins];
			folded[i] = folded[i] * (1.-strength) + (left + right) * strength * 0.5;
		}
	}

}

