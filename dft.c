
#include "dft.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void DoDFT( float * outbins, float * frequencies, int bins, float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q )
{
	int i, j;
	for( i = 0; i < bins; i++ )
	{
		float freq = frequencies[i];
		float phi = 0;
		int sampleplace = place_in_data_buffer;
		float advance = 3.14159*2.0/freq;

		float binqtys = 0;
		float binqtyc = 0;

		for( j = 0; j <= freq * q; j++ )
		{
			float sample = databuffer[sampleplace];
			sampleplace = (sampleplace-1+size_of_data_buffer)%size_of_data_buffer;
//printf( "%d\n", sampleplace );
			float sv = sin( phi ) * sample;
			float cv = cos( phi ) * sample;

			binqtys += sv;
			binqtyc += cv;

			phi += advance;
		}

		float amp = sqrtf( binqtys * binqtys + binqtyc * binqtyc );
		outbins[i] = amp / freq / q;
	}
}

void DoDFTQuick( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup )
{
	int i, j;

	for( i = 0; i < bins; i++ )
	{
		int flirts = 0;

		float freq = frequencies[i];
		float phi = 0;
		int ftq = freq * q; 
		int sampleplace = place_in_data_buffer;
		float advance = 3.14159*2.0/freq;

		float binqtys = 0;
		float binqtyc = 0;

		int skip = floor( ftq / speedup );
		if( skip == 0 ) skip = 1;
		advance *= skip;

		for( j = 0; j <= ftq; j += skip )
		{
			float sample = databuffer[sampleplace];
			sampleplace = (sampleplace-skip+size_of_data_buffer)%size_of_data_buffer;
//printf( "%d\n", sampleplace );
			float sv = sinf( phi ) * sample;
			float cv = cosf( phi ) * sample;

			binqtys += sv;
			binqtyc += cv;

			phi += advance;
			flirts++;
		}

		float amp = sqrtf( binqtys * binqtys + binqtyc * binqtyc );
		outbins[i] = amp / freq / q * skip;
	}
}



////////////////////////////DFT Progressive is for embedded systems, primarily.


static float * gbinqtys;
static float * gbinqtyc;
static float * phis;
static float * gfrequencies;
static float * goutbins;
static float * lastbins;
static float * advances;

static int     gbins;
static float   gq;
static float   gspeedup;

#define PROGIIR .005

void HandleProgressive( float sample )
{
	int i;

	for( i = 0; i < gbins; i++ )
	{
		float thiss = sinf( phis[i] ) * sample;
		float thisc = cosf( phis[i] ) * sample;

		float s = gbinqtys[i] = gbinqtys[i] * (1.-PROGIIR) + thiss * PROGIIR;
		float c = gbinqtyc[i] = gbinqtyc[i] * (1.-PROGIIR) + thisc * PROGIIR;

		phis[i] += advances[i];
		if( phis[i] > 6.283 ) phis[i]-=6.283;

		goutbins[i] = sqrtf( s * s + c * c );
	}
}


void DoDFTProgressive( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup )
{
	int i;
	static int last_place;

	if( gbins != bins )
	{
		if( gbinqtys ) free( gbinqtys );
		if( gbinqtyc ) free( gbinqtyc );
		if( phis ) free( phis );
		if( lastbins ) free( lastbins );
		if( advances ) free( advances );

		gbinqtys = malloc( sizeof(float)*bins );
		gbinqtyc = malloc( sizeof(float)*bins );
		phis = malloc(  sizeof(float)*bins );
		lastbins = malloc(  sizeof(float)*bins );
		advances = malloc(  sizeof(float)*bins );

		memset( gbinqtys, 0, sizeof(float)*bins );
		memset( gbinqtyc, 0, sizeof(float)*bins );
		memset( phis, 0, sizeof(float)*bins );
		memset( lastbins, 0, sizeof(float)*bins );

	}
	memcpy( outbins, lastbins, sizeof(float)*bins );

	for( i = 0; i < bins; i++ )
	{
		float freq = frequencies[i];
		advances[i] = 3.14159*2.0/freq;
	}

	gbins = bins;
	gfrequencies = frequencies;
	goutbins = outbins;
	gspeedup = speedup;
	gq = q;

	place_in_data_buffer = (place_in_data_buffer+1)%size_of_data_buffer;

	int didrun = 0;
	for( i = last_place; i != place_in_data_buffer; i = (i+1)%size_of_data_buffer )
	{
		float fin = ((float)((int)(databuffer[i] * 127))) / 127.0;  //simulate 8-bit input (it looks FINE!)
		HandleProgressive( fin );
		didrun = 1;
	}
	last_place = place_in_data_buffer;

	if( didrun )
	{
		memcpy( lastbins, outbins, sizeof(float)*bins );
	}

/*	for( i = 0; i < bins; i++ )
	{
		printf( "%0.2f ", outbins[i]*100 );
	}
	printf( "\n" );*/

}







/////////////////////////////INTEGER DFT




//NOTES to self:
//
// Let's say we want to try this on an AVR.
//  24 bins, 5 octaves = 120 bins.
// 20 MHz clock / 4.8k sps = 4096 IPS = 34 clocks per bin = :(
//  We can do two at the same time, this frees us up some 

static uint8_t donefirstrun;
static int8_t sintable[512]; //Actually [sin][cos] pairs.


//LDD instruction on AVR can read with constant offset.  We can set Y to be the place in the buffer, and read with offset.
static uint16_t * datspace;  //(advances,places,isses,icses)

void HandleProgressiveInt( int8_t sample1, int8_t sample2 )
{
	int i;
	int16_t ts, tc;
	int16_t tmp1;
	int8_t s1, c1;
	uint16_t ipl, localipl, adv;

	uint16_t * ds = &datspace[0];
	int8_t * st;
	//Clocks are listed for AVR.

	//Estimated 78 minimum instructions... So for two pairs each... just over 4ksps, theoretical.
	//Running overall at ~2kHz.  With GCC: YUCK! 102 cycles!!!
	for( i = 0; i < gbins; i++ )            //Loop, fixed size = 3 + 2 cycles                       N/A
	{
		//12 cycles MIN
		adv = *(ds++); //Read, indirect from RAM (and increment)  2+2 cycles			4
		ipl = *(ds++); //Read, indirect from RAM (and increment)  2+2 cycles			4

		//13 cycles MIN
		ipl += adv;   				         //Advance, 16bit += 16bit, 1 + 1 cycles                2
		localipl = (ipl>>8)<<1;				//Select upper 8 bits  1 cycles							1   *** AS/IS: 4

		st = &sintable[localipl];
		s1 = *(st++);						//Read s1 component out of table. 2+2    cycles			2
		c1 = *st;							//Read c1 component out of table. 2    cycles			2   *** AS/IS: 4

		ts = (s1 * sample1);				// 8 x 8 multiply signed + copy R1 out. zero MSB ts		2  ->Deferred
		tc = (c1 * sample1);				// 8 x 8 multiply signed + copy R1 out. zero MSB tc   	2  ->Deferred


		//15 cycles MIN
		ipl += adv;   				        //Advance, 16bit += 16bit, 1 + 1 cycles   				2
		localipl = (ipl>>8)<<1;				//Select upper 8 bits  1 cycles							1  *** AS/IS: 4

			// need to load Z with 'sintable' and add localipl										2
		st = &sintable[localipl];
		s1 = *(st++);						//Read s1 component out of table. 2   cycles			2	
		c1 = *st;							//Read c1 component out of table. 2    cycles			2 *** AS/IS: 4

		ts += (s1 * sample2);				// 8 x 8 multiply signed + add R1 out.					3  ->Deferred
		tc += (c1 * sample2);				// 8 x 8 multiply signed + add R1 out.					3  ->Deferred


		//Add TS and TC to the datspace stuff. (24 instructions)
		tmp1 = (*ds);			//Read out, sin component.								4  Accurate.
		tmp1 -= tmp1>>7;					//Subtract from the MSB (with carry)					2 -> 6  AS/IS: 7+7 = 14
		tmp1 += ts>>7;						//Add MSBs with carry									2 -> 6  AS/IS: 6

		*(ds++) = tmp1;			//Store values back										4

		tmp1 = *ds;			//Read out, sin component.								4
		tmp1 -= tmp1>>7;					//Subtract from the MSB (with carry)					2 -> 6 AS/IS: 7+7 = 14
		tmp1 += tc>>7;						//Add MSBs with carry									2 -> 6 AS/IS: 6

		*ds++ = tmp1;			//Store values back										4

		*(ds-3) = ipl;			//Store values back										4 AS/IS: 6

		//AS-IS: 8 loop overhead.
	}
}

void DoDFTProgressiveInteger( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup )
{
	int i;
	static int last_place;

	if( !donefirstrun )
	{
		donefirstrun = 1;
		for( i = 0; i < 256; i++ )
		{
			sintable[i*2+0] = (int8_t)((sinf( i / 256.0 * 6.283 ) * 127.0));
			sintable[i*2+1] = (int8_t)((cosf( i / 256.0 * 6.283 ) * 127.0));
		}
	}

	if( gbins != bins )
	{
		gbins = bins;
		if( datspace ) free( datspace );
		datspace = malloc( bins * 2 * 4 );
	}

	
	for( i = 0; i < bins; i++ )
	{
		float freq = frequencies[i];
		datspace[i*4] = 65536.0/freq;
	}


	for( i = last_place; i != ( place_in_data_buffer&0xffffe ); i = (i+2)%size_of_data_buffer )
	{
		int8_t ifr1 = (int8_t)( ((databuffer[i+0]) ) * 127 );
		int8_t ifr2 = (int8_t)( ((databuffer[i+1]) ) * 127 );
//		printf( "%d %d\n", i, place_in_data_buffer&0xffffe );
		HandleProgressiveInt( ifr1, ifr2 );
	}

	last_place = place_in_data_buffer&0xfffe;

	//Extract bins.
	for( i = 0; i < bins; i++ )
	{
		int16_t isps = datspace[i*4+2];
		int16_t ispc = datspace[i*4+3];
		int16_t mux = ( (isps/256) * (isps/256)) + ((ispc/256) * (ispc/256));
//		printf( "%d (%d %d)\n", mux, isps, ispc );
		outbins[i] = sqrt( mux )/100.0;
	}
//	printf( "\n");
}














////////////////////////SKIPPY DFT

//Skippy DFT is a very ood one. 



#define OCTAVES  5
#define FIXBPERO 24
#define FIXBINS  (FIXBPERO*OCTAVES)
#define BINCYCLE (1<<OCTAVES)

//NOTES to self:
//
// Let's say we want to try this on an AVR.
//  24 bins, 5 octaves = 120 bins.
// 20 MHz clock / 4.8k sps = 4096 IPS = 34 clocks per bin = :(
//  We can do two at the same time, this frees us up some 

static uint8_t Sdonefirstrun;
static int8_t Ssintable[512]; //Actually [sin][cos] pairs.
static uint16_t Sdatspace[FIXBINS*4];  //(advances,places,isses,icses)

//For 
static uint8_t Sdo_this_octave[BINCYCLE];
static int16_t Saccum_octavebins[OCTAVES];
static uint8_t Swhichoctaveplace;

void HandleProgressiveIntSkippy( int8_t sample1 )
{
	int i;
	int16_t ts, tc;
	int16_t tmp1;
	int8_t s1, c1;
	uint16_t ipl, localipl, adv;

	uint8_t oct = Sdo_this_octave[Swhichoctaveplace];
	Swhichoctaveplace ++;
	Swhichoctaveplace &= BINCYCLE-1;

	if( oct > 128 )
	{
		//Special: This is when we can update everything.

/*		if( (rand()%100) == 0 )
		{
			for( i = 0; i < FIXBINS; i++ )
//				printf( "%0.2f ",goutbins[i]*100 );
				printf( "(%d %d)",Sdatspace[i*4+2], Sdatspace[i*4+3] );
			printf( "\n" );
		} */

		for( i = 0; i < FIXBINS; i++ )
		{
			int16_t isps = Sdatspace[i*4+2];
			int16_t ispc = Sdatspace[i*4+3];
			int16_t mux = ( (isps/256) * (isps/256)) + ((ispc/256) * (ispc/256));
	//		printf( "%d (%d %d)\n", mux, isps, ispc );

			int octave = i / FIXBPERO;
//			mux >>= octave; 
			goutbins[i] = sqrt( mux );
//			goutbins[i]/=100.0;
			goutbins[i]/=100*(1<<octave);
			Sdatspace[i*4+2] -= isps>>5;
			Sdatspace[i*4+3] -= ispc>>5;
		}

	}

	for( i = 0; i < OCTAVES;i++ )
	{
		Saccum_octavebins[i] += sample1;
	}

	uint16_t * ds = &Sdatspace[oct*FIXBPERO*4];
	int8_t * st;

	sample1 = Saccum_octavebins[oct]>>(OCTAVES-oct);
	Saccum_octavebins[oct] = 0;

	for( i = 0; i < FIXBPERO; i++ )            //Loop, fixed size = 3 + 2 cycles                       N/A
	{
		//12 cycles MIN
		adv = *(ds++); //Read, indirect from RAM (and increment)  2+2 cycles			4
		ipl = *(ds++); //Read, indirect from RAM (and increment)  2+2 cycles			4

		//13 cycles MIN
		ipl += adv;   				         //Advance, 16bit += 16bit, 1 + 1 cycles                2
		localipl = (ipl>>8)<<1;				//Select upper 8 bits  1 cycles							1   *** AS/IS: 4

		st = &Ssintable[localipl];
		s1 = *(st++);						//Read s1 component out of table. 2+2    cycles			2
		c1 = *st;							//Read c1 component out of table. 2    cycles			2   *** AS/IS: 4

		ts = (s1 * sample1);				// 8 x 8 multiply signed + copy R1 out. zero MSB ts		2  ->Deferred
		tc = (c1 * sample1);				// 8 x 8 multiply signed + copy R1 out. zero MSB tc   	2  ->Deferred


		//Add TS and TC to the datspace stuff. (24 instructions)
		tmp1 = (*ds);			//Read out, sin component.								4  Accurate.
//		tmp1 -= tmp1>>4;					//Subtract from the MSB (with carry)					2 -> 6  AS/IS: 7+7 = 14
		tmp1 += ts>>3;						//Add MSBs with carry									2 -> 6  AS/IS: 6

		*(ds++) = tmp1;			//Store values back										4

		tmp1 = *ds;			//Read out, sin component.								4
//		tmp1 -= tmp1>>4;					//Subtract from the MSB (with carry)					2 -> 6 AS/IS: 7+7 = 14
		tmp1 += tc>>3;						//Add MSBs with carry									2 -> 6 AS/IS: 6

		*ds++ = tmp1;			//Store values back										4

		*(ds-3) = ipl;			//Store values back										4 AS/IS: 6

		//AS-IS: 8 loop overhead.
	}
}

void DoDFTProgressiveIntegerSkippy( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup )
{
	static float backupbins[FIXBINS];
	int i, j;
	static int last_place;

//printf( "SKIPPY\n" );

	if( !Sdonefirstrun )
	{
		memset( outbins, 0, bins * sizeof( float ) );
		goutbins = outbins;
		//Sdatspace = malloc(FIXBPERO*OCTAVES*8);
		//memset(Sdatspace,0,FIXBPERO*OCTAVES*8);
		//printf( "MS: %d\n", FIXBPERO*OCTAVES*8);
		Sdonefirstrun = 1;
		for( i = 0; i < 256; i++ )
		{
			Ssintable[i*2+0] = (int8_t)((sinf( i / 256.0 * 6.283 ) * 127.0));
			Ssintable[i*2+1] = (int8_t)((cosf( i / 256.0 * 6.283 ) * 127.0));
		}

		for( i = 0; i < BINCYCLE; i++ )
		{
			// Sdo_this_octave = 
			// 4 3 4 2 4 3 4 ...
			//search for "first" zero

			for( j = 0; j <= OCTAVES; j++ )
			{
				if( ((1<<j) & i) == 0 ) break;
			}
			if( j > OCTAVES )
			{
				fprintf( stderr, "Error: algorithm fault.\n" );
				exit( -1 );
			}
			Sdo_this_octave[i] = OCTAVES-j-1;
		}
	}

	memcpy( outbins, backupbins, FIXBINS*4 );

	if( FIXBINS != bins )
	{
		fprintf( stderr, "Error: Bins was reconfigured.  skippy requires a constant number of bins.\n" );
		return;
	}

	
	for( i = 0; i < bins; i++ )
	{
		float freq = frequencies[(i%FIXBPERO) + (FIXBPERO*(OCTAVES-1))];
		Sdatspace[i*4] = (65536.0/freq);// / oneoveroctave;
	}


	for( i = last_place; i != place_in_data_buffer; i = (i+1)%size_of_data_buffer )
	{
		int8_t ifr1 = (int8_t)( ((databuffer[i]) ) * 127 );
		HandleProgressiveIntSkippy( ifr1 );
		HandleProgressiveIntSkippy( ifr1 );
	}

	last_place = place_in_data_buffer;

	memcpy( backupbins, outbins, FIXBINS*4 );

	//Extract bins.
/*
	for( i = 0; i < bins; i++ )
	{
		int16_t isps = Sdatspace[i*4+2];
		int16_t ispc = Sdatspace[i*4+3];
		int16_t mux = ( (isps/256) * (isps/256)) + ((ispc/256) * (ispc/256));
//		printf( "%d (%d %d)\n", mux, isps, ispc );
		outbins[i] = sqrt( mux )/100.0;
	}
*/

//	printf( "\n");
}






