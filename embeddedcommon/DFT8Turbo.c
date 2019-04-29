#include <stdint.h>
#include <stdlib.h>
#include "DFT8Turbo.h"
#include <math.h>

#include <stdio.h>

#define MAX_FREQS (12)
#define OCTAVES   (4)
#define INITIAL_DECIMATE 1

//Right now, we need 8*freqs*octaves bytes.
//This is bad.
//What can we do to fix it?

//4x the hits (sin/cos and we need to do it once for each edge)
//8x for selecting a higher octave.
#define FREQREBASE 8.0 
#define TARGFREQ 10000.0

/* Tradeoff guide:

	* We will optimize for RAM size here.


	* INITIAL_DECIMATE; A larger decimation: {NOTE 1}
		+) Reduces the bit depth needed for the integral map.
			If you use "1" and a fully saturted map (highest note is every sample), it will not overflow a signed 12-bit number.
		-) Increases noise.  
			With full-scale: 0->1 minimal 1->2 minimal 2->3 significantly noticable, 3->4 major.
			If sound is quieter, it matters more.  I recommend no less than 1.
	Also, other things, like frequency of hits can manipulate the maximum bit depth needed for integral map.

	* If you weight the bins in advance see "mulmux", you can:	{NOTE 2}
		+) potentially use shallower bit depth but
		-) have to compute the multiply every time you update the bin.

	* You can use a modified-square-wave which only integrates for 1/2 of the duty cycle. {NOTE 3}
		+) uses 1/2 the integral memory.
		-) Not as pretty of an output.  See "integral_at"

	*TODO: Investigate using all unsigned (to make multiply and/or 12-bit storage easier)



	So, the idea here is we would keep a running total of the current ADC value, kept away in a int16_t.
	It is constantly summing, so we can take an integral of it.  Or rather an integral range.

	Over time, we perform operations like adding or subtracting from a current place.  It basically is
	a DFT where the kernel is computed using square waves (or modified square waves)
*/

//These live in RAM.
int16_t running_integral; //Realistically treat as 12-bits on ramjet8
int16_t integral_at[MAX_FREQS*OCTAVES];	//For ramjet8, make 12-bits
int32_t cossindata[MAX_FREQS*OCTAVES*2]; //Contains COS and SIN data.  (32-bit for now, will be 16-bit, potentially even 8.)
uint8_t which_octave_for_op[MAX_FREQS]; //counts up, tells you which ocative you are operating on.  PUT IN RAM.

#define NR_OF_OPS (4<<OCTAVES)
//Format is:
//  255 = DO NOT OPERATE
// bits 0..3 unfolded octave, i.e. sin/cos are offset by one.
// bit 4 = add or subtract.
uint8_t  optable[NR_OF_OPS]; //PUT IN FLASH


#define ACTIONTABLESIZE 256
uint16_t actiontable[ACTIONTABLESIZE]; //PUT IN FLASH // If there are more than 8 freqbins, this must be a uint16_t, otherwise if more than 16, 32.
uint8_t actiontableplace;
//Format is

uint8_t mulmux[MAX_FREQS*OCTAVES];	//PUT IN FLASH

static int Setup( float * frequencies, int bins )
{
	int i;
	printf( "BINS: %d\n", bins );

	float highestf = frequencies[bins-1];
	for( i = 0; i < bins; i++ )
	{
		mulmux[i] = (uint8_t)( highestf / frequencies[i] * 255 + 0.5 );
		printf( "MM: %d  %f / %f\n", mulmux[i], frequencies[i], highestf );
	}

	for( i = bins-MAX_FREQS; i < bins; i++ )
	{
		int topbin = i - (bins-MAX_FREQS);
		float f = frequencies[i]/FREQREBASE; 
		float hits_per_table = (float)ACTIONTABLESIZE/f;
		int dhrpertable = (int)(hits_per_table+.5);//TRICKY: You might think you need to have even number of hits (sin/cos), but you don't!  It can flip sin/cos each time through the table!
		float err = (TARGFREQ/((float)ACTIONTABLESIZE/dhrpertable) - (float)TARGFREQ/f)/((float)TARGFREQ/f);
		//Perform an op every X samples.  How well does this map into units of 1024?
		printf( "%d %f -> hits per %d: %f %d (%.2f%% error)\n", topbin, f, ACTIONTABLESIZE, (float)ACTIONTABLESIZE/f, dhrpertable, err * 100.0 );
		if( dhrpertable >= ACTIONTABLESIZE )
		{
			fprintf( stderr, "Error: Too many hits.\n" );
			exit(0);
		}

		float advance_per_step = dhrpertable/(float)ACTIONTABLESIZE;
		float fvadv = 0.5;
		int j;
		int countset = 0;

		//Tricky: We need to start fadv off at such a place that there won't be a hicchup when going back around to 0.
		//	I believe this is done by setting fvadv to 0.5 initially.  Unsure.

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
	//exit(1);


	int phaseinop[OCTAVES] = { 0 };
	int already_hit_octaveplace[OCTAVES*2] = { 0 };
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
			optable[i] = 255;
		}
		else
		{
			longestzeroes = OCTAVES-1-longestzeroes;	//Actually do octave 0 least often.
			int iop = phaseinop[longestzeroes]++;
			int toop = (longestzeroes<<1) | (iop & 1);

			//if it's the first time an octave happened this set, flag it. This may be used later in the process.
			if( !already_hit_octaveplace[toop] )
			{
				already_hit_octaveplace[toop] = 1;
				toop |= 1<<5;
			}

			//Handle add/subtract bit.
			if( iop & 2 ) toop |= 1<<4;

			optable[i] = toop;

			//printf( "  %d %d %d\n", iop, val, longestzeroes );
		}
		//printf( "HBT: %d = %d\n", i, optable[i] );
	}
	//exit(1);

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
	running_integral += adcval>>INITIAL_DECIMATE;

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

				//Tricky: We share the integral with SIN and COS.
				//We don't need to. It would produce a slightly cleaner signal. See: NOTE 3
				int intindex = (octaveplace>>1) * MAX_FREQS + n;

				//int invoct = OCTAVES-1-octaveplace;
				int16_t diff;

				if( op & 0x10 )	//ADD
				{
					diff = integral_at[intindex] - running_integral;
					dprintf( "%c", 'a' + octaveplace );
				}
				else	//SUBTRACT
				{
					diff = running_integral - integral_at[intindex];
					dprintf( "%c", 'A' + octaveplace );
				}

				if( diff > 2000 || diff < -2000 ) printf( "!!!!!!!!!!!! %d !!!!!!!!!!!\n", diff );

				integral_at[intindex] = running_integral;

				int idx = intindex * 2 + (octaveplace&1);

				//if( n == 1 ) printf( "%d %d %d  %d\n", n, idx, diff, op & 0x10 );
				//dprintf( "%d\n", idx );

#if 0
		//Apply IIR operation 1; This is rough because the Q changes and goes higher as a function of frequency.  This is probably a bad move.
				cossindata[idx] += diff>>4;
				if( op & 0x20 )
				{
					cossindata[idx] = cossindata[idx] 
						- (cossindata[idx]>>2);
				}
#else
		//Apply IIR.
				//printf( "%d: %d + %d * %d >> 8 - %d\n", idx, cossindata[idx], diff, mulmux[idx/2], cossindata[idx]>>4 );
				cossindata[idx] = cossindata[idx] 
					+ (((int32_t)diff * (int32_t)mulmux[idx/2])>>6)
					- (cossindata[idx]>>4)
					;
			//	if( cossindata[idx] > 2047 ) cossindata[idx] = 2047;
			//	if( cossindata[idx] < -2048 ) cossindata[idx] = -2048;
#endif
			//	if( cossindata[idx] > 1 ) cossindata[idx]--;
			//	if( cossindata[idx] < -1 ) cossindata[idx]++;
			//	if( cossindata[idx] > 16 ) cossindata[idx]-=8;
			//	if( cossindata[idx] < -16 ) cossindata[idx]+=8;
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

	static int idiv;
	idiv++;
#if 1
	for( i = 0; i < bins; i++ )
	{
		outbins[i] = 0;
	}
	for( i = 0; i < bins; i++ )
	{
		int iss = cossindata[i*2+0]>>8;
		int isc = cossindata[i*2+1]>>8;
		int issdiv = 0;
		int iscdiv = 0;
		int FWDOFFSET = 19;//MAX_FREQS*3/2;
		if( i < bins-FWDOFFSET )
		{
			issdiv = cossindata[(i+FWDOFFSET)*2+0]/256;
			iscdiv = cossindata[(i+FWDOFFSET)*2+1]/256;
		}
		int mux = iss * iss + isc * isc;
		int muxdiv = issdiv * issdiv + iscdiv * iscdiv;

		//if( (idiv % 100) > 50 ) { printf( "*" ); mux -= muxdiv; }
		//mux -= muxdiv;

		if( mux <= 0 ) 
		{
			outbins[i] = 0;
		}
		else
		{
			//if( i == 0 )
			//printf( "MUX: %d %d = %d\n", isc, iss, mux );
			outbins[i] = sqrt((float)mux)/50.0;

			if( abs( cossindata[i*2+0] ) > 2000 || abs( cossindata[i*2+1] ) > 2000 )
				printf( "%d/%d/%d/%f ", i, cossindata[i*2+0], cossindata[i*2+1],outbins[i] );
			//outbins[i] = (cossindata[i*2+0]/10000.0);
		}
	} 
	printf( "\n" );
#endif
}


