//Copyright 2015 <>< Charles Lohr under the ColorChord License.

//XXX TODO Figure out why it STILL fails when going around a loop


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <math.h>

#include "outdrivers.h"

#include "color.h"

struct LEDOutDriver
{
	int did_init;
	int total_leds;
	int is_loop;
	float light_siding;
	float last_led_pos[MAX_LEDS];
	float last_led_pos_filter[MAX_LEDS];
	float last_led_amp[MAX_LEDS];
	int steady_bright;
	float led_floor;
	float led_limit; //Maximum brightness
	float satamp;
	int lastadvance;
};

static float lindiff( float a, float b )  //Find the minimum change around a wheel.
{
	float diff = a - b;
	if( diff < 0 ) diff *= -1;

	float otherdiff = (a<b)?(a+1):(a-1);
	otherdiff -=b;
	if( otherdiff < 0 ) otherdiff *= -1;

	if( diff < otherdiff )
		return diff;
	else
		return otherdiff;
}

static void LEDUpdate(void * id, struct NoteFinder*nf)
{
	struct LEDOutDriver * led = (struct LEDOutDriver*)id;


	//Step 1: Calculate the quantity of all the LEDs we'll want.
	int totbins = nf->note_peaks;//nf->dists;
	int i, j;
	float binvals[totbins];
	float binvalsQ[totbins];
	float binpos[totbins];
	float totalbinval = 0;

//	if( totbins > led_bins ) totbins = led_bins;


	for( i = 0; i < totbins; i++ )
	{
		binpos[i] = nf->note_positions[i] / nf->freqbins;
		binvals[i] = pow( nf->note_amplitudes2[i], led->light_siding );
//		binvals[i] = (binvals[i]<led->led_floor)?0:binvals[i];
//		if( nf->note_positions[i] < 0 ) { binvals[i] = 0; binvalsQ[i] = 0; }

		binvalsQ[i] =pow( nf->note_amplitudes[i], led->light_siding );
			// nf->note_amplitudes[i];//
		totalbinval += binvals[i];
	}

	float newtotal = 0;

	for( i = 0; i < totbins; i++ )
	{

#define SMOOTHZERO
#ifdef SMOOTHZERO
		binvals[i] -= led->led_floor*totalbinval;
		if( binvals[i] / totalbinval < 0 )
			binvals[i] = binvalsQ[i] = 0;
#else
		if( binvals[i] / totalbinval < led->led_floor )
			binvals[i] = binvalsQ[i] = 0;
#endif
		newtotal += binvals[i];
	}
	totalbinval = newtotal;

	float rledpos[led->total_leds];
	float rledamp[led->total_leds];
	float rledampQ[led->total_leds];
	int rbinout = 0;



	for( i = 0; i < totbins; i++ )
	{
		int nrleds = (int)((binvals[i] / totalbinval) * led->total_leds);
//		if( nrleds < 40 ) nrleds = 0;
		for( j = 0; j < nrleds && rbinout < led->total_leds; j++ )
		{
			rledpos[rbinout] = binpos[i];
			rledamp[rbinout] = binvals[i];
			rledampQ[rbinout] = binvalsQ[i];
			rbinout++;
		}
	}

	if( rbinout == 0 )
	{
		rledpos[0] = 0;
		rledamp[0] = 0;
		rledampQ[0] = 0;
		rbinout++;
	}

	for( ; rbinout < led->total_leds; rbinout++ )
	{
		rledpos[rbinout] = rledpos[rbinout-1];
		rledamp[rbinout] = rledamp[rbinout-1];
		rledampQ[rbinout] = rledampQ[rbinout-1];

	}
	
	//Now we have to minimize "advance".
	int minadvance = 0;

	if( led->is_loop )
	{
		float mindiff = 1e20;

		//Uncomment this for a rotationally continuous surface.
		for( i = 0; i < led->total_leds; i++ )
		{
			float diff = 0;
			diff = 0;
			for( j = 0; j < led->total_leds; j++ )
			{
				int r = (j + i) % led->total_leds;
				float rd = lindiff( led->last_led_pos_filter[j], rledpos[r]);
				diff += rd;//*rd;
			}

			int advancediff = ( led->lastadvance - i );
			if( advancediff < 0 ) advancediff *= -1;
			if( advancediff > led->total_leds/2 ) advancediff = led->total_leds - advancediff;

			float ad = (float)advancediff/(float)led->total_leds;
			diff += ad * ad;// * led->total_leds;

			if( diff < mindiff )
			{
				mindiff = diff;
				minadvance = i;
			}
		}

	}
	led->lastadvance = minadvance;
//	printf( "MA: %d %f\n", minadvance, mindiff );

	//Advance the LEDs to this position when outputting the values.
	for( i = 0; i < led->total_leds; i++ )	
	{
		int ia = ( i + minadvance + led->total_leds ) % led->total_leds;
		float sat = rledamp[ia] * led->satamp;
		float satQ = rledampQ[ia] * led->satamp;
		if( satQ > 1 ) satQ = 1;
		led->last_led_pos[i] = rledpos[ia];
		led->last_led_amp[i] = sat;
		float sendsat = (led->steady_bright?sat:satQ);
		if( sendsat > 1 ) sendsat = 1;

		if( sendsat > led->led_limit ) sendsat = led->led_limit;

		int r = CCtoHEX( led->last_led_pos[i], 1.0, sendsat );

		OutLEDs[i*3+0] = r & 0xff;
		OutLEDs[i*3+1] = (r>>8) & 0xff;
		OutLEDs[i*3+2] = (r>>16) & 0xff;
	}


	if( led->is_loop )
	{
		for( i = 0; i < led->total_leds; i++ )
		{
			led->last_led_pos_filter[i] = led->last_led_pos_filter[i] * .9 + led->last_led_pos[i] * .1;
		}
	}

}

static void LEDParams(void * id )
{
	struct LEDOutDriver * led = (struct LEDOutDriver*)id;

	led->satamp = 2;		RegisterValue( "satamp", PAFLOAT, &led->satamp, sizeof( led->satamp ) );
	led->total_leds = 300;	RegisterValue( "leds", PAINT, &led->total_leds, sizeof( led->total_leds ) );
	led->led_floor = .1;	RegisterValue( "led_floor", PAFLOAT, &led->led_floor, sizeof( led->led_floor ) );
	led->light_siding = 1.4;RegisterValue( "light_siding", PAFLOAT, &led->light_siding, sizeof( led->light_siding ) );
	led->is_loop = 0;		RegisterValue( "is_loop", PAINT, &led->is_loop, sizeof( led->is_loop ) );
	led->steady_bright = 1;	RegisterValue( "steady_bright", PAINT, &led->steady_bright, sizeof( led->steady_bright ) );
	led->led_limit = 1;     RegisterValue( "led_limit", PAFLOAT, &led->led_limit, sizeof( led->led_limit ) );

	printf( "Found LEDs for output.  leds=%d\n", led->total_leds );

}


static struct DriverInstances * OutputLinear()
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	memset( ret, 0, sizeof( struct DriverInstances ) );
	struct LEDOutDriver * led = ret->id = malloc( sizeof( struct LEDOutDriver ) );
	memset( led, 0, sizeof( struct LEDOutDriver ) );

	ret->Func = LEDUpdate;
	ret->Params = LEDParams;
	LEDParams( led );
	return ret;

}

REGISTER_OUT_DRIVER(OutputLinear);


