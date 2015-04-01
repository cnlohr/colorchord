//
// This is the teststrap for the Embedded ColorChord System.
// It is intended as a minimal scaffolding for testing Embedded ColorChord.
//

#include <stdio.h>

#include "dft.h"
#define DFREQ     8000
#define BASE_FREQ 55.0

const float bf_table[24] = {
        1.000000, 1.029302, 1.059463, 1.090508, 1.122462, 1.155353, 
        1.189207, 1.224054, 1.259921, 1.296840, 1.334840, 1.373954, 
        1.414214, 1.455653, 1.498307, 1.542211, 1.587401, 1.633915, 
        1.681793, 1.731073, 1.781797, 1.834008, 1.887749, 1.943064 };

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

/* The above table was generated using the following code:

#include <stdio.h>
#include <math.h>

int main()
{
	int i;
	#define FIXBPERO 24
	printf( "const float bf_table[%d] = {", FIXBPERO );
	for( i = 0; i < FIXBPERO; i++ )
	{
		if( ( i % 6 ) == 0 )
			printf( "\n\t" );
		printf( "%f, ", pow( 2, (float)i / (float)FIXBPERO ) );
	}
	printf( "};\n" );
	return 0;
}
*/

void UpdateFreqs()
{
	uint16_t fbins[FIXBPERO];
	int i;

	BUILD_BUG_ON( sizeof(bf_table) != FIXBPERO*4 );

	for( i = 0; i < FIXBPERO; i++ )
	{
		float frq =  ( bf_table[i] * BASE_FREQ );
		fbins[i] = ( 65536.0 ) / ( DFREQ ) * frq * 16;
	}

	UpdateBinsForProgressiveIntegerSkippyInt( fbins );
}

void Init()
{
	//Step 1: Initialize the Integer DFT.
	SetupDFTProgressiveIntegerSkippy();

	//Step 2: Set up the frequency list.
	UpdateFreqs();	
}

void HandleFrameInfo()
{
	uint16_t folded_bins[FIXBPERO];

	int i, j, k = 0;

	for( i = 0; i < FIXBPERO; i++ )
		folded_bins[i] = 0;

	for( j = 0; j < OCTAVES; j++ )
	{
		for( i = 0; i < FIXBPERO; i++ )
		{
			folded_bins[i] += embeddedbins[k++];
		}
	}

	

	//XXX TODO Taper the first and last octaves.
//	for( i = 0; i < freqbins; i++ )
//	{
//		nf->outbins[i] *= (i+1.0)/nf->freqbins;
//	}
//	for( i = 0; i < freqbins; i++ )
//	{
//		nf->outbins[freqs-i-1] *= (i+1.0)/nf->freqbins;
//	}

	//We now have the system folded into one

	for( i = 0; i < FIXBPERO; i++ )
	{
		printf( "%5d ", folded_bins[i] );
	}
	printf( "\n" );
}

int main()
{
	int wf = 0;
	int ci;
	Init();
	while( ( ci = getchar() ) != EOF )
	{
		int cs = ci - 0x80;
		Push8BitIntegerSkippy( (int8_t)cs );
		//printf( "%d ", cs ); fflush( stdout );
		wf++;
		if( wf == 64 )
		{
			HandleFrameInfo();
			wf = 0;
		}
	}
	return 0;
}

