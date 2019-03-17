//Copyright 2015 <>< Charles Lohr under the ColorChord License.

//Really basic driver, that just selects the most prominent color and washes all the LEDs with that.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <math.h>

#include "outdrivers.h"

#include "color.h"

struct ProminentDriver
{
	int did_init;
	int total_leds;
	float satamp;
};

static void LEDUpdate(void * id, struct NoteFinder*nf)
{
	struct ProminentDriver * led = (struct ProminentDriver*)id;


	//Step 1: Calculate the quantity of all the LEDs we'll want.
	int totbins = nf->note_peaks;//nf->dists;
	int i;
	float selected_amp = 0;
	float selected_note = 0;

//	if( totbins > led_bins ) totbins = led_bins;

	for( i = 0; i < totbins; i++ )
	{
		float freq = nf->note_positions[i] / nf->freqbins;
		float amp = nf->note_amplitudes2[i] * led->satamp;
		if( amp > selected_amp )
		{
			selected_amp = amp;
			selected_note = freq;
		}
	}



	//Advance the LEDs to this position when outputting the values.
	for( i = 0; i < led->total_leds; i++ )	
	{
		float sendsat = selected_amp;
		if( sendsat > 1 ) sendsat = 1;
		int r = CCtoHEX( selected_note, 1.0, sendsat );

		OutLEDs[i*3+0] = r & 0xff;
		OutLEDs[i*3+1] = (r>>8) & 0xff;
		OutLEDs[i*3+2] = (r>>16) & 0xff;
	}

}

static void LEDParams(void * id )
{
	struct ProminentDriver * led = (struct ProminentDriver*)id;

	led->satamp = 2;		RegisterValue( "satamp", PAFLOAT, &led->satamp, sizeof( led->satamp ) );
	led->total_leds = 4;	RegisterValue( "leds", PAINT, &led->total_leds, sizeof( led->total_leds ) );


	printf( "Found Prominent for output.  leds=%d\n", led->total_leds );

}


static struct DriverInstances * OutputProminent()
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	memset( ret, 0, sizeof( struct DriverInstances ) );
	struct ProminentDriver * led = ret->id = malloc( sizeof( struct ProminentDriver ) );
	memset( led, 0, sizeof( struct ProminentDriver ) );

	ret->Func = LEDUpdate;
	ret->Params = LEDParams;
	LEDParams( led );
	return ret;

}

REGISTER_OUT_DRIVER(OutputProminent);


