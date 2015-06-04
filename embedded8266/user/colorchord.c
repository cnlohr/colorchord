#include "colorchord.h"

static uint8_t donefirstrun;
static int8_t sintable[512]; //Actually [sin][cos] pairs.

//LDD instruction on AVR can read with constant offset.  We can set Y to be the place in the buffer, and read with offset.
static uint16_t datspace[bins*4];  //(advances,places,isses,icses)

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
	for( i = 0; i < bins; i++ )            //Loop, fixed size = 3 + 2 cycles                       5
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

void SetupConstants()
{
	int i;
	static int last_place;

	if( !donefirstrun )
	{
		donefirstrun = 1;
		for( i = 0; i < 256; i++ )
		{
			sintable[i*2+0] = i;//(int8_t)((sin( i / 256.0 * 6.283 ) * 127.0));
			sintable[i*2+1] = i+1;//(int8_t)((cos( i / 256.0 * 6.283 ) * 127.0));
		}
	}

	
	for( i = 0; i < bins; i++ )
	{
		float freq = i;//frequencies[i];
		datspace[i*4] = 65536.0/freq;
	}

}

/*
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

*/
