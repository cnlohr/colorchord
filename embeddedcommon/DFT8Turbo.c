#include <stdint.h>
#include <stdlib.h>
#include "DFT8Turbo.h"
#include <math.h>

#include <stdio.h>

#define MAX_FREQS (24)
#define OCTAVES   (5)

#define TARGFREQ 8000.0

/*
	* The first thought was using an integration map and only operating when we need to, to pull the data out.
	* Now we're doing the thing below this block comment
		int16_t accumulated_total;							//2 bytes
		int16_t last_accumulated_total_at_bin[MAX_FREQS*2];  //24 * 2 * sizeof(int16_t) = 96 bytes.
		uint8_t current_time;								//1 byte
		uint8_t placecode[MAX_FREQS];
*/
/* 
	So, the idea here is we would keep a running total of the current ADC value, kept away in a int16_t.
	It is constantly summing, so we can take an integral of it.  Or rather an integral range.

	Over time, we perform operations like adding or subtracting from a current place.


NOTE: 
	Optimizations:
		Only use 16 bins, lets action table be 16-bits wide.
*/

//These live in RAM.
int16_t running_integral;
int16_t integral_at[MAX_FREQS*OCTAVES*2];
int32_t cossindata[MAX_FREQS*OCTAVES*2]; //Contains COS and SIN data.  (32-bit for now, will be 16-bit)
uint8_t which_octave_for_op[MAX_FREQS]; //counts up, tells you which ocative you are operating on.  PUT IN RAM.

#define NR_OF_OPS (4<<OCTAVES)
//Format is:
//  255 = DO NOT OPERATE
// bits 0..3 unfolded octave, i.e. sin/cos are offset by one.
// bit 4 = add or subtract.
uint8_t  optable[NR_OF_OPS]; //PUT IN FLASH


#define ACTIONTABLESIZE 256
uint32_t actiontable[ACTIONTABLESIZE]; //PUT IN FLASH
uint8_t actiontableplace;
//Format is

static int Setup( float * frequencies, int bins )
{
	int i;
	printf( "BINS: %d\n", bins );
	for( i = bins-MAX_FREQS; i < bins; i++ )
	{
		int topbin = i - (bins-MAX_FREQS);
		float f = frequencies[i]/4.0; //4x the hits (sin/cos and we need to do it once for each edge)
		float hits_per_table = (float)ACTIONTABLESIZE/f;
		int dhrpertable = (int)(hits_per_table+.5);//TRICKY: You might think you need to have even number of hits (sin/cos), but you don't!  It can flip sin/cos each time through the table!
		float err = (TARGFREQ/((float)ACTIONTABLESIZE/dhrpertable) - (float)TARGFREQ/f)/((float)TARGFREQ/f);
		//Perform an op every X samples.  How well does this map into units of 1024?
		printf( "%d %f -> hits per %d: %f %d (%.2f%% error)\n", topbin, f, ACTIONTABLESIZE, (float)ACTIONTABLESIZE/f, dhrpertable, err * 100.0 );

		float advance_per_step = dhrpertable/(float)ACTIONTABLESIZE;
		float fvadv = 0.0;
		int j;
		int countset = 0;

		//XXX TODO Tricky: We need to start fadv off at such a place that there won't be a hicchup when going back around to 0.

		for( j = 0; j < ACTIONTABLESIZE; j++ )
		{
			if( fvadv >= 0.5 )
			{
				actiontable[j] |= 1<<topbin;
				fvadv -= 1.0;
				countset++;
			}
			fvadv += advance_per_step;
		}
		printf( "   countset: %d\n", countset );
	}

	int phaseinop[OCTAVES] = { 0 };
	for( i = 0; i < NR_OF_OPS; i++ )
	{
		int longestzeroes = 0;
		int val = i & ((1<<OCTAVES)-1);
		for( longestzeroes = 0; longestzeroes < 255 && ( ((val >> longestzeroes) & 1) == 0 ); longestzeroes++ );
		//longestzeroes goes: 255, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, ...
		//This isn't great, because we need to also know whether we are attacking the SIN side or the COS side, and if it's + or -.
		//We can actually decide that out.

		if( longestzeroes == 255 )
		{
			//This is a nop.  Emit a nop.
			optable[i] = longestzeroes;
		}
		else
		{
			int iop = phaseinop[longestzeroes]++;
			optable[i] = (longestzeroes<<1) | (iop & 1);
			if( iop & 2 ) optable[i] |= 1<<4;
			//printf( "  %d %d\n", iop, val );
		}
		//printf( "HBT: %d = %d\n", i, optable[i] );
	}
	

	return 0;
}


#if 0
int16_t running_integral;
int16_t integral_at[MAX_FREQS*OCTAVES];
int16_t cossindata[MAX_FREQS*OCTAVES*2]; //Contains COS and SIN data.
uint8_t which_octave_for_op[MAX_FREQS]; //counts up, tells you which ocative you are operating on.  PUT IN RAM.

#define NR_OF_OPS (4<<OCTAVES)
//Format is:
//  255 = DO NOT OPERATE
// bits 0..3 unfolded octave, i.e. sin/cos are offset by one.
// bit 4 = add or subtract.
uint8_t  optable[NR_OF_OPS]; //PUT IN FLASH


#define ACTIONTABLESIZE 256
uint32_t actiontable[ACTIONTABLESIZE]; //PUT IN FLASH
//Format is
#endif

void Turbo8BitRun( int8_t adcval )
{
	running_integral += adcval;

#define dprintf( ... )

	uint32_t action = actiontable[actiontableplace++];
	int n;
	dprintf( "%4d ", actiontableplace );
	for( n = 0; n < MAX_FREQS; n++ )
	{
		if( action & (1<<n) )
		{
			int ao = which_octave_for_op[n];
			int op = optable[ao];
			ao++;
			if( ao >= NR_OF_OPS ) ao = 0;
			which_octave_for_op[n] = ao;

			if( op == 255 )
			{
				dprintf( "*" );	//NOP
			}
			else
			{
				int octaveplace = op & 0xf;
				int idx = (octaveplace>>1) * MAX_FREQS * 2 + n * (octaveplace&1)*2;

				int16_t diff;

				if( op & 0x10 )	//ADD
				{
					diff = integral_at[idx>>1] - running_integral;
					dprintf( "%c", 'a' + octaveplace );
				}
				else	//SUBTRACT
				{
					diff = running_integral - integral_at[idx>>1];
					dprintf( "%c", 'A' + octaveplace );
				}
				integral_at[idx>>1] = running_integral;
				printf( "%d\n", diff );
				//dprintf( "%d\n", idx );
				cossindata[idx] += diff;
				cossindata[idx] -= cossindata[idx] >> 8;
			}
		}
		else
		{
			dprintf( " " );
		}
	}
	dprintf( "\n" );

#if 0
	uint32_t actions = *(placeintable++);
	if( placeintable == &actiontable[ACTIONTABLESIZE] ) placeintable = actiontable;
	int b;
	for( b = 0; b < MAX_FREQS; b++ )
	{
		if( ! ((1<<b) & actions) ) continue;
		//If we get here, we need to do an action.
		int op = which_octave_for_op[b]++;
		int sinorcos = op & 1;
		op >>= 1;
		int octavebit = op & ((1<<OCTAVES)-1);
		if( !octavebit ) { continue; } //XXX TRICKY: In our octavebit table, we have 1 0 and 1 1 entry. 2, 3, 4, etc. are ok.  So, if we hit a 0, we abort.
		int whichoctave = highbit_table[octavebit];

		//Ok, actually we need to also know whether you're on SIN or COS.

		//if( b == 0 ) printf( "%d\n", whichoctave );
		//XXX TODO Optimization: Use a table, since octavebit can only be 0...31.
	}
#endif
}


void DoDFT8BitTurbo( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup )
{
	static int is_setup;
	if( !is_setup ) { is_setup = 1; Setup( frequencies, bins ); }
	static int last_place;
	int i;

	for( i = last_place; i != place_in_data_buffer; i = (i+1)%size_of_data_buffer )
	{
		int16_t ifr1 = (int16_t)( ((databuffer[i]) ) * 4095 );
		Turbo8BitRun( ifr1>>5 ); //6 = Actually only feed algorithm numbers from -64 to 63.
	}
	last_place = place_in_data_buffer;

#if 1
	for( i = 0; i < bins; i++ )
	{
		outbins[i] = 0;
	}
	for( i = 0; i < MAX_FREQS; i++ )
	{
		int iss = 0;//cossindata[i*2+0]>>8;
		int isc = 0;//cossindata[i*2+1]>>8;
		int mux = iss * iss + isc * isc;
		if( mux == 0 ) mux = 1;
		if( i == 0 )
		//printf( "MUX: %d %d = %d\n", isc, iss, mux );
		outbins[i+MAX_FREQS] = sqrt(mux);///200.0;
	} 
#endif
}


