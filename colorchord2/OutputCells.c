//Copyright 2015 <>< Charles Lohr under the ColorChord License.

//XXX TODO Figure out why it STILL fails when going around a loop


#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include <string.h>
#include "parameters.h"
#include <stdlib.h>
#include "color.h"
#include <stdlib.h>
#include <math.h>

extern float DeltaFrameTime;
extern double Now;

struct CellsOutDriver
{
	int did_init;
	int total_leds;
	int led_note_attached[MAX_LEDS];
	double time_of_change[MAX_LEDS];

	float last_led_pos[MAX_LEDS];
	float last_led_pos_filter[MAX_LEDS];
	float last_led_amp[MAX_LEDS];
	float led_floor;
	float light_siding;
	float satamp;
	float qtyamp;
	int steady_bright;
	int timebased; //Useful for pies, turn off for linear systems.
	int snakey; //Advance head for where to get LEDs around.
	int snakeyplace;
};

static void LEDUpdate(void * id, struct NoteFinder*nf)
{
	struct CellsOutDriver * led = (struct CellsOutDriver*)id;

	//Step 1: Calculate the quantity of all the LEDs we'll want.
	int totbins = nf->note_peaks;//nf->dists;
	int i, j;
	float binvals[totbins];
	float binvalsQ[totbins];
	float binpos[totbins];
	float totalbinval = 0;

	float qtyHave[totbins];
	float qtyWant[totbins];
	float totQtyWant = 0;

	memset( qtyHave, 0, sizeof( qtyHave ) );

	for( i = 0; i < led->total_leds; i++ )
	{
		int l = led->led_note_attached[i];
		if( l >= 0 )
		{
			qtyHave[l] ++;
		}
	}


	for( i = 0; i < totbins; i++ )
	{
		binpos[i] = nf->note_positions[i] / nf->freqbins;
		binvals[i] = pow( nf->note_amplitudes2[i], led->light_siding ); //Slow
		binvalsQ[i] = pow( nf->note_amplitudes[i], led->light_siding ); //Fast
		totalbinval += binvals[i];

		float want = binvals[i] * led->qtyamp;
		totQtyWant += want;
		qtyWant[i] = want;
	}

	if( totQtyWant > led->total_leds )
	{
		float overage = led->total_leds/totQtyWant;
		for( i = 0; i < totbins; i++ )
			qtyWant[i] *= overage;
	}

	float qtyDiff[totbins];
	for( i = 0; i < totbins; i++ )
	{
		qtyDiff[i] = qtyWant[i] - qtyHave[i];
	}

	//Step 1: Relinquish LEDs
	for( i = 0; i < totbins; i++ )
	{
		while( qtyDiff[i] < -0.5 )
		{
			double maxtime = -1.0;
			int maxindex = -1;

			//Find the LEDs that have been on least.
			for( j = 0; j < led->total_leds; j++ )
			{
				if( led->led_note_attached[j] != i ) continue;
				if( !led->timebased ) { maxindex = j; break; }
				if( led->time_of_change[j] > maxtime )
				{
					maxtime = led->time_of_change[j];
					maxindex = j;
				}
			}
			if( maxindex >= 0 )
			{
				led->led_note_attached[maxindex] = -1;
				led->time_of_change[maxindex] = Now;
			}
			qtyDiff[i]++;
		}
	}

	//Throw LEDs back in.
	for( i = 0; i < totbins; i++ )
	{
		while( qtyDiff[i] > 0.5 )
		{
			double seltime = 1e20;
			int selindex = -1;

			
			//Find the LEDs that haven't been on in a long time.
			for( j = 0; j < led->total_leds; j++ )
			{
				if( led->led_note_attached[j] != -1 ) continue;
				if( !led->timebased ) { selindex = j; break; }

				if( led->snakey )
				{
					float bias = 0;
					float timeimp = 1;

					bias = (j - led->snakeyplace + led->total_leds) % led->total_leds;

					if( bias > led->total_leds / 2 ) bias = led->total_leds - bias + 1;
					timeimp = 0;

					float score = led->time_of_change[j] * timeimp + bias;
					if( score < seltime )
					{
						seltime = score;
						selindex = j;
					}
				}
				else
				{
					if( led->time_of_change[j] < seltime )
					{
						seltime = led->time_of_change[j];
						selindex = j;
					}
				}
			}
			if( selindex >= 0 )
			{
				led->led_note_attached[selindex] = i;
				led->time_of_change[selindex] = Now;
				led->snakeyplace = selindex;
			}
			qtyDiff[i]--;
		}
	}

	//Advance the LEDs to this position when outputting the values.
	for( i = 0; i < led->total_leds; i++ )	
	{
		int ia = led->led_note_attached[i];
		if( ia == -1 )
		{
			OutLEDs[i*3+0] = 0;
			OutLEDs[i*3+1] = 0;
			OutLEDs[i*3+2] = 0;
			continue;
		}
		float sat = binvals[ia] * led->satamp;
		float satQ = binvalsQ[ia] * led->satamp;
		if( satQ > 1 ) satQ = 1;
		float sendsat = (led->steady_bright?sat:satQ);
		if( sendsat > 1 ) sendsat = 1;
		int r = CCtoHEX( binpos[ia], 1.0, sendsat );

		OutLEDs[i*3+0] = r & 0xff;
		OutLEDs[i*3+1] = (r>>8) & 0xff;
		OutLEDs[i*3+2] = (r>>16) & 0xff;
	}

}

static void LEDParams(void * id )
{
	struct CellsOutDriver * led = (struct CellsOutDriver*)id;

	led->satamp = 2;		RegisterValue( "satamp", PAFLOAT, &led->satamp, sizeof( led->satamp ) );
	led->total_leds = 300;	RegisterValue( "leds", PAINT, &led->total_leds, sizeof( led->total_leds ) );
	led->steady_bright = 1;	RegisterValue( "seady_bright", PAINT, &led->steady_bright, sizeof( led->steady_bright ) );
	led->led_floor = .1;	RegisterValue( "led_floor", PAFLOAT, &led->led_floor, sizeof( led->led_floor ) );
	led->light_siding = 1.9;RegisterValue( "light_siding", PAFLOAT, &led->light_siding, sizeof( led->light_siding ) );
	led->qtyamp = 20;		RegisterValue( "qtyamp", PAFLOAT, &led->qtyamp, sizeof( led->qtyamp ) );
	led->timebased = 1;		RegisterValue( "timebased", PAINT, &led->timebased, sizeof( led->timebased ) );

	led->snakey = 0;		RegisterValue( "snakey", PAINT, &led->snakey, sizeof( led->snakey ) );

	led->snakeyplace = 0;

	printf( "Found LEDs for output.  leds=%d\n", led->total_leds );

}


static struct DriverInstances * OutputCells()
{
	int i;
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	memset( ret, 0, sizeof( struct DriverInstances ) );
	struct CellsOutDriver * led = ret->id = malloc( sizeof( struct CellsOutDriver ) );
	memset( led, 0, sizeof( struct CellsOutDriver ) );

	for( i = 0; i < MAX_LEDS; i++ )
	{
		led->led_note_attached[i] = -1;
	}
	ret->Func = LEDUpdate;
	ret->Params = LEDParams;
	LEDParams( led );
	return ret;

}

REGISTER_OUT_DRIVER(OutputCells);


