
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




#define PROGIIR .005

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

//
void HandleProgressiveInt( int8_t sample1, int8_t sample2 )
{
	int i;
	uint16_t startpl = 0;
	int16_t ts, tc;
	int16_t tmp1;
	int8_t s1, c1;
	uint16_t ipl, localipl, adv;


	//startpl maps to 'Y'
	//


	//Estimated 68 minimum instructions... So for two pairs each... just under 5ksps, theoretical.
	//Running overall at ~2kHz.
	for( i = 0; i < gbins; i++ )            //Loop, fixed size = 3 + 2 cycles                       5
	{
		//12 cycles MIN
		adv = datspace[startpl++]; //Read, indirect from RAM (and increment)  2+2 cycles			4
		ipl = datspace[startpl++]; //Read, indirect from RAM (and increment)  2+2 cycles			4

		//13 cycles MIN
		ipl += adv;   				         //Advance, 16bit += 16bit, 1 + 1 cycles                2
		localipl = (ipl>>8)<<1;				//Select upper 8 bits  1 cycles							1

			// need to load Z with 'sintable' and add localipl										2
		s1 = sintable[localipl++];			//Read s1 component out of table. 2+2    cycles			2
		c1 = sintable[localipl++];			//Read c1 component out of table. 2    cycles			2

		ts = (s1 * sample1);				// 8 x 8 multiply signed + copy R1 out. zero MSB ts		2
		tc = (c1 * sample1);				// 8 x 8 multiply signed + copy R1 out. zero MSB tc   	2


		//15 cycles MIN
		ipl += adv;   				        //Advance, 16bit += 16bit, 1 + 1 cycles   				2
		localipl = (ipl>>8)<<1;				//Select upper 8 bits  1 cycles							1

			// need to load Z with 'sintable' and add localipl										2
		s1 = sintable[localipl++];			//Read s1 component out of table. 2   cycles			2	
		c1 = sintable[localipl++];			//Read c1 component out of table. 2    cycles			2

		ts += (s1 * sample2);				// 8 x 8 multiply signed + add R1 out.					3
		tc += (c1 * sample2);				// 8 x 8 multiply signed + add R1 out.					3


		//Add TS and TC to the datspace stuff. (24 instructions)
		tmp1 = datspace[startpl];			//Read out, sin component.								4
		tmp1 -= tmp1>>6;					//Subtract from the MSB (with carry)					2
		tmp1 += ts>>6;						//Add MSBs with carry									2

		datspace[startpl++] = tmp1;			//Store values back										4

		tmp1 = datspace[startpl];			//Read out, sin component.								4
		tmp1 -= tmp1>>6;					//Subtract from the MSB (with carry)					2
		tmp1 += tc>>6;						//Add MSBs with carry									2

		datspace[startpl++] = tmp1;			//Store values back										4

		datspace[startpl-3] = ipl;			//Store values back										4
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


