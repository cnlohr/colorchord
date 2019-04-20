#include <stdint.h>
#include <stdlib.h>
#include "DFT8Turbo.h"
#include <math.h>

#include <stdio.h>

#define MAX_FREQS (24)
#define OCTAVES   (5)


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

int16_t running_integral;
int16_t cossindata[MAX_FREQS*OCTAVES*2]; //Contains COS and SIN data.



uint8_t  which_octave_for_op[MAX_FREQS]; //counts up, tells you which ocative you are operating on.
uint8_t  highbit_table[2<<OCTAVES]; //PUT IN FLASH


#define ACTIONTABLESIZE 512

uint16_t * placeintable;

//Put this in flash.
uint32_t actiontable[ACTIONTABLESIZE];

static int Setup( float * frequencies, int bins )
{
	int i;
	printf( "BINS: %d\n", bins );
	for( i = bins-MAX_FREQS; i < bins; i++ )
	{
		int topbin = i - (bins-MAX_FREQS);
		float f = frequencies[i]/2.0; //2x the hits (sin/cos)
		float hits_per_table = (float)ACTIONTABLESIZE/f;
		int dhrpertable = (int)(hits_per_table+.5);//TRICKY: You might think you need to have even number of hits (sin/cos), but you don't!  It can flip sin/cos each time through the table!
		float err = (8000./((float)ACTIONTABLESIZE/dhrpertable) - 8000./f)/(8000./f);
		//Perform an op every X samples.  How well does this map into units of 1024?
		printf( "%d %f -> hits per 1024: %f %d (%f error)\n", topbin, f, (float)ACTIONTABLESIZE/f, dhrpertable, err * 100.0 );

		float advance_per_step = dhrpertable/(float)ACTIONTABLESIZE;
		float fvadv = 0.0;
		int j;
		int actions = 0;
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

	for( i = 0; i < (1<<OCTAVES); i++ )
	{
		int longestzeroes = 0;
		for( longestzeroes = 0; longestzeroes < 255 && ( ((i >> longestzeroes) & 1) == 0 ); longestzeroes++ );
		//longestzeroes goes: 255, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, ...
		//This isn't great, because we need to also know whether we are attacking the SIN side or the COS side.
		highbit_table[i] = longestzeroes;
	}
	//Repeat the highbit table in the second half.
	//XXX PICK UP HERE
	//Encode into highbit_table which cell is being operated on
	//Also, do the * MAX_FREQS here.  That will 



	placeintable = actiontable;
	//	for( i = 0; i < ACTIONTABLESIZE; i++ )		printf( "%08x\n", actiontable[i] );
}



int16_t running_integral;
int16_t cossindata[MAX_FREQS*OCTAVES*2];
uint8_t  which_octave_for_op[MAX_FREQS]; //counts up, tells you which ocative you are operating on.
uint16_t * placeintable;

//Put this in flash.
uint32_t actiontable[ACTIONTABLESIZE];


void Turbo8BitRun( int8_t adcval )
{
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

#if 0
	for( i = 0; i < bins; i++ )
	{
		outbins[i] = 0;
	}
	for( i = 0; i < MAX_FREQS; i++ )
	{
		int iss = nd[i].sinm>>8;
		int isc = nd[i].cosm>>8;
		int mux = iss * iss + isc * isc;
		if( mux == 0 ) mux = 1;
		if( i == 0 )
		printf( "MUX: %d %d\n", isc, iss );
		outbins[i+MAX_FREQS] = sqrt(mux)/200.0;
	} 
#endif
}


