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
//OK... We don't have enough ram to sum everything... can we do something wacky with multiple ocatives to sum everything better?
//i.e.
//
// 4332322132212210
//
// ++++++++++++++++-----------------
// ++++++++--------
// ++++----++++----
// ++--++--++--++--
// +-+-+-+-+-+-+-+-
//
// Don't forget we need to do this for sin and cos.
// Can we instead of making this plusses, make it a multiplier?
// How can we handle sin+cos?
//
// Is it possible to do this for every frame?  I.e. for each of the 24 notes, multiply with their current place in table?
//  That's interesting.  It's not like a sin table.
// There is no "multiply" in the attiny instruction set for attiny85.
// There is, however for attiny402

//Question:  Can we do five octaves, or does this need to be balanced?
//Question2: Should we weight higher octaves?


//ATTiny402: 256x8 RAM, 4096x8 FLASH  LPM: 3 cycles + FMUL: 2 cycles  << Do stacked sin waves?
//ATtiny85:  512x8 RAM, 8192x8 FLASH  LPM: 3 cycles + NO MULTIPLY     << Do square waves?


/* Approaches:

  on ATtiny402:  Stacked sin approach.
   Say 16 MHz, though 12 MHz is interesting...
   16k SPS: 1k cycles per; say 24 bins per; 41 cycles per bin = hard.  But is it too hard?
   20 cycles per s/c.
		read place in stacked table (8? bits) 3 cycles

		//Inner loop = 17 cycles.
		read stacked table (8 bits), 3 cycles
		fractional multiply table with current value. 2 cycles
		read current running for note 2 cycles  (LDS = 3 cycles)
		subtract a shifted version, to make it into an IIR. (4 cycles)
		add in current values. (2 cycles)
		store data back to ram (2 cycles)
		advance place in stacked table (8?bits) 1 cycle

		store place in stacked table (8? bits) 3 cycles?

	//What if we chunk ADC updates into groups of 4 or 8?
	//This is looking barely possible.

	on attiny85: scheduled adds/subtracts (like a stacked-square-wave-table)
		//XXX TODO!

*/

/* Ok... Let's think about the ATTiny402.  256x8 RAM + 4096x8 FLASH.

	* We can create a table which has all octaves overlaid.
	* We would need to keep track of:
		* 12 x 2 x 2 = 48 bytes = Current sin/cos values.
		* 12 x 2 = 24 bytes = Current place in table.  = 72 bytes
	* We would need to store:
		* The layered lookup table.  If possible, keep @ 256 bytes to simplify math ops.
		* The speed by which each note needs to advance.
	* We would need to:
		* Read current running place. X                8 cycles
		* Use that place to look up into sin table.    3 cycles
		* Read running val  4 cycles best case
		* Multiply out the sin + IIR                   5 cycles
		* Store running val 4 cycles best case
		* Cos-advance that place to look up into sin table.    4 cycles
		* Read running val 4 cycles best case
		* Multiply out the sin + IIR                   5 cycles
		* Store running val 4 cycles best case.
		* Read how much to advance X by.               4 cycles
        * (Cos^2+Sin^2)                                8?
		* Store it.                                    4 cycles best case.
        *                                                  = 48 x 12 = 576 cycles.  Assume 10 MHz @ 16k SPS.  We're OK (625 samples)
*/

// Observation: The two tables are actually mirror images of each other, well diagonally mirrored.  That's odd.  But, would take CPU to exploit.

#define SSTABLESIZE 256
int8_t  spikysin_interleved_cos[SSTABLESIZE][2];
uint32_t advancespeed[MAX_FREQS];

static int CompTableWithPhase( int nelements, float phase, int scaling )
{
	int highest = 0;
	int i;
	for( i = 0; i < nelements; i++ )
	{
		float taued = i * 3.141592 * 2.0 / nelements;
		int o;
		float combsin = 0;
		for( o = 0; o < OCTAVES; o++ )
		{
			combsin += sin( taued * (1<<o) + phase);
		}
		combsin /= OCTAVES;
		int csadapt =  combsin * scaling - 0.5;	//No value is higher with five octaves.  XXX TODO Lookout.  If you change # of octaves, need to change this, too.

		if( csadapt > highest ) highest = csadapt;
		if( -csadapt > highest ) highest = -csadapt;

		if( csadapt > 127 ) csadapt = 127;
		if( csadapt < -128 ) csadapt = -128;  //tricky: Keep balanced.
		spikysin_interleved_cos[i][0] = csadapt;

		float combcos = 0;
		for( o = 0; o < OCTAVES; o++ )
		{
			combcos += cos( taued * (1<<o) + phase );
		}
		combcos /= OCTAVES;
		csadapt = combcos * scaling - 0.5;	//No value is higher with five octaves.  XXX TODO Lookout.  If you change # of octaves, need to change this, too.

		if( csadapt > highest ) highest = csadapt;
		if( -csadapt > highest ) highest = -csadapt;

		if( csadapt > 127 ) csadapt = 127;
		if( csadapt < -128 ) csadapt = -128;  //tricky: Keep balanced.
		spikysin_interleved_cos[i][1] = csadapt;
	}
	return highest;
}


static int Setup( float * frequencies, int bins )
{
	int i;

	//Since start position/phase is arbitrary, we should try several to see which gives us the best dynamic range.
	float tryphase = 0;

	float bestphase = 0;
	int highest_val_at_best_phase = 1000000;

	for( tryphase = 0; tryphase < 3.14159; tryphase += 0.001 )
	{
		int highest = CompTableWithPhase( SSTABLESIZE, tryphase, 65536 );
		if( highest < highest_val_at_best_phase )
		{
			highest_val_at_best_phase = highest;
			bestphase = tryphase;
		}
	}
	printf( "Best comp: %f : %d\n", bestphase, highest_val_at_best_phase );

	//Set this because we would overflow the sinm and cosm regs if we don't.  This is sort of like a master volume.
	//use this as that input volume knob thing.
	float further_reduce = 1.0;

	CompTableWithPhase( SSTABLESIZE, bestphase, (65536*128*further_reduce)/highest_val_at_best_phase );

//	for( i = 0; i < SSTABLESIZE; i++ )
//	{
//		printf( "%d %d\n", spikysin_interleved_cos[i*2+0], spikysin_interleved_cos[i*2+1] );
//	}

	for( i = 0; i < MAX_FREQS; i++ )
	{
		//frequencies[i] = SPS / Freq
		// Need to decide how quickly we sweep through the table.
		advancespeed[i] = 65536 * 256.0 /* fixed point */ * 256.0 /* size of table */ / frequencies[i];
		//printf( "%f\n", frequencies[i] );
	}
	return 0;
}


/*
uint8_t  spikysin_interleved_cos[256*2];
uint16_t advancespeed[MAX_FREQS];
*/

float toutbins[MAX_FREQS];

struct notedat
{
	uint32_t time;
	int32_t sinm;
	int32_t cosm;
};

static struct notedat nd[MAX_FREQS];

void Turbo8BitRun( int8_t adcval )
{
	int i;
	for( i = 0; i < MAX_FREQS; i++ )
	{
		uint32_t ct = nd[i].time;
		int32_t muxres;
		int32_t running;
		int32_t rdesc, rdess;
		uint8_t * spikysintable = &spikysin_interleved_cos[(ct>>24)][0];

		int8_t  ss = *(spikysintable++);

		#define DECIR 8

		muxres = ((int16_t)adcval * ss + (1<<(DECIR-1)) ) >> (DECIR);
		running = nd[i].cosm;
		running += muxres;
		rdesc = running >> 8;
		running -= rdesc >> 3;

		nd[i].cosm = running;
if( i == 0) printf( "MRX %5d  %9d %9d  %9d %9d\n", muxres, adcval, ss, running, nd[i].sinm );
		int8_t  sc = *(spikysintable++);
		muxres = ((int16_t)adcval * sc + (1<<(DECIR-1)) ) >> (DECIR);
		running = nd[i].sinm;
		running += muxres;

		rdess = running>>8;
		running -= rdess >> 3;

		nd[i].sinm = running;

		nd[i].time = ct + advancespeed[i];

		toutbins[i] = rdess * rdess + rdesc * rdesc;
		//printf( "%d %d = %f %p\n", rdess, rdesc, toutbins[i], &toutbins[i] );
	}

	static uint8_t stater;
/*	stater++;
	if( stater == 16 )
	{
		stater = 0;
		for( i = 0; i < MAX_FREQS; i++ )
		{
			nd[i].sinm -= nd[i].sinm >> 12;
			nd[i].cosm -= nd[i].cosm >> 12;
			nd[i].sinm += 8;
			nd[i].cosm += 8;
		}
	}*/
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
		//ifr1 += 4095;
		//ifr1 += 512;
		Turbo8BitRun( ifr1>>5 ); //6 = Actually only feed algorithm numbers from -64 to 63.
	}
	last_place = place_in_data_buffer;

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

}


