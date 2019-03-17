//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "outdrivers.h"

#include "color.h"
#include "hidapi.h"

struct HIDAPIOutDriver
{
	hid_device *devh;
	int did_init;
	int zigzag;
	int total_leds;
	int array;
	float outamp;
	uint8_t * last_leds;
	int buffersize;
	volatile int readyFlag;
	int xn;
	int yn;
	int rot90;
	int is_rgby;
	int bank_size[4];
	int bank_id[4];
};


static void * LEDOutThread( void * v )
{
	struct HIDAPIOutDriver * led = (struct HIDAPIOutDriver*)v;
	while(1)
	{
		int total_bytes = led->buffersize;
		total_bytes = ((total_bytes-2)&0xffc0) + 0x41; //Round up.
		if( led->readyFlag )
		{
/*
			int i;
			printf( "%d -- ", total_bytes );
			for( i = 0; i < total_bytes; i++ )
			{
				if( !( i & 0x0f ) ) printf( "\n" );
				printf( "%02x ", led->last_leds[i] );
			}
			printf( "\n" );
*/
			int r = hid_send_feature_report( led->devh, led->last_leds, total_bytes );
			if( r < 0 )
			{
				led->did_init = 0;
				printf( "Fault sending LEDs.\n" );
			}
			led->readyFlag = 0;
		}
		OGUSleep(100);
	}
	return 0;
}

static void LEDUpdate(void * id, struct NoteFinder*nf)
{
	int i;
	struct HIDAPIOutDriver * led = (struct HIDAPIOutDriver*)id;


	if( !led->did_init )
	{
		led->did_init = 1;
		hid_init();
		
		led->devh = hid_open( 0xabcd, 0xf104, 0 );

		if( !led->devh )
		{
			fprintf( stderr,  "Error: Cannot find device.\n" );
//			exit( -98 );
		}
	}

	int leds_this_round = 0;
	int ledbid = 0;

	while( led->readyFlag ) OGUSleep(100);

	int lastledplace = 1;
	led->last_leds[0] = 0;

	int k = 0;

	//Advance the LEDs to this position when outputting the values.
	for( i = 0; i < led->total_leds; i++ )	
	{
		int source = i;
		if( !led->array )
		{
			int sx, sy;
			if( led->rot90 )
			{
				sy = i % led->yn;
				sx = i / led->yn;
			}
			else
			{
				sx = i % led->xn;
				sy = i / led->xn;
			}

			if( led->zigzag )
			{
				if( led->rot90 )
				{
					if( sx & 1 )
					{
						sy = led->yn - sy - 1;
					}
				}
				else
				{
					if( sy & 1 )
					{
						sx = led->xn - sx - 1;
					}
				}
			}

			if( led->rot90 )
			{
				source = sx + sy * led->xn;
			}
			else
			{
				source = sx + sy * led->yn;
			}
		}


		if( led->is_rgby )
		{
			int r = OutLEDs[k++];
			int g = OutLEDs[k++];
			int b = OutLEDs[k++];
			int y = 0;
			int rg_common;
			if( r/2 > g ) rg_common = g;
			else        rg_common = r/2;

			if( rg_common > 255 ) rg_common = 255;
			y = rg_common;
			r -= rg_common;
			g -= rg_common;
			if( r < 0 ) r = 0;
			if( g < 0 ) g = 0;


			if( (lastledplace % 64) == 1 ) led->last_leds[lastledplace++] = led->bank_id[ledbid];
			led->last_leds[lastledplace++] = g * led->outamp;
			if( (lastledplace % 64) == 1 ) led->last_leds[lastledplace++] = led->bank_id[ledbid];
			led->last_leds[lastledplace++] = r * led->outamp; 
			if( (lastledplace % 64) == 1 ) led->last_leds[lastledplace++] = led->bank_id[ledbid];
			led->last_leds[lastledplace++] = b * led->outamp;
			if( (lastledplace % 64) == 1 ) led->last_leds[lastledplace++] = led->bank_id[ledbid];
			led->last_leds[lastledplace++] = y * led->outamp;
		}
		else
		{
			if( (lastledplace % 64) == 1 ) led->last_leds[lastledplace++] = led->bank_id[ledbid];
			led->last_leds[lastledplace++] = OutLEDs[source*3+1] * led->outamp;
			if( (lastledplace % 64) == 1 ) led->last_leds[lastledplace++] = led->bank_id[ledbid];
			led->last_leds[lastledplace++] = OutLEDs[source*3+0] * led->outamp; 
			if( (lastledplace % 64) == 1 ) led->last_leds[lastledplace++] = led->bank_id[ledbid];
			led->last_leds[lastledplace++] = OutLEDs[source*3+2] * led->outamp;
		}
		//printf( "%3d -- %3d [%3d] == %d %d %d\n", ledbid, source, lastledplace, OutLEDs[source*3+1], OutLEDs[source*3+0], OutLEDs[source*3+2] );
		leds_this_round++;
		while( leds_this_round >= led->bank_size[ledbid] )
		{
			while( (lastledplace % 64) != 1 ) 	led->last_leds[lastledplace++] = 0;
			ledbid++;
				if( ledbid > 3 ) break;
			if(  led->bank_size[ledbid] != 0 )
				led->last_leds[lastledplace++] = led->bank_id[ledbid];
			else
				continue;
			leds_this_round = 0;
		}
		if( ledbid > 3 ) break;
	}
	led->buffersize = lastledplace;

	led->readyFlag = 1;
}

static void LEDParams(void * id )
{
	struct HIDAPIOutDriver * led = (struct HIDAPIOutDriver*)id;

	led->total_leds = GetParameterI( "leds", 300 );
	led->last_leds = malloc( led->total_leds * 5 + 4 );
	led->outamp = .1;    RegisterValue(  "ledoutamp", PAFLOAT, &led->outamp, sizeof( led->outamp ) );
	led->zigzag = 0;     RegisterValue(  "zigzag", PAINT, &led->zigzag, sizeof( led->zigzag ) );
	led->xn = 16;        RegisterValue(  "lightx", PAINT, &led->xn, sizeof( led->xn ) );
	led->yn = 9;         RegisterValue(  "lighty", PAINT, &led->yn, sizeof( led->yn ) );
	led->rot90 = 0;      RegisterValue(  "rot90", PAINT, &led->rot90, sizeof( led->rot90 ) );
	led->array = 0;      RegisterValue(  "ledarray", PAINT, &led->array, sizeof( led->array ) );
	led->is_rgby = 0;    RegisterValue(  "ledisrgby", PAINT, &led->is_rgby, sizeof( led->is_rgby ) );

	led->bank_size[0] = 0;  RegisterValue(  "bank1_size", PAINT, &led->bank_size[0], sizeof( led->bank_size[0] ) );
	led->bank_size[1] = 0;  RegisterValue(  "bank2_size", PAINT, &led->bank_size[1], sizeof( led->bank_size[1] ) );
	led->bank_size[2] = 0;  RegisterValue(  "bank3_size", PAINT, &led->bank_size[2], sizeof( led->bank_size[2] ) );
	led->bank_size[3] = 0;  RegisterValue(  "bank4_size", PAINT, &led->bank_size[3], sizeof( led->bank_size[3] ) );
	led->bank_id[0] = 0;    RegisterValue(  "bank1_id", PAINT, &led->bank_id[0], sizeof( led->bank_id[0] ) );
	led->bank_id[1] = 0;    RegisterValue(  "bank2_id", PAINT, &led->bank_id[1], sizeof( led->bank_id[1] ) );
	led->bank_id[2] = 0;    RegisterValue(  "bank3_id", PAINT, &led->bank_id[2], sizeof( led->bank_id[2] ) );
	led->bank_id[3] = 0;    RegisterValue(  "bank4_id", PAINT, &led->bank_id[3], sizeof( led->bank_id[3] ) );

	led->did_init = 0;
}


static struct DriverInstances * DisplayHIDAPI()
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	memset( ret, 0, sizeof( struct DriverInstances ) );
	struct HIDAPIOutDriver * led = ret->id = malloc( sizeof( struct HIDAPIOutDriver ) );
	ret->Func = LEDUpdate;
	ret->Params = LEDParams;
	OGCreateThread( LEDOutThread, led );
	led->readyFlag = 0;
	LEDParams( led );
	return ret;

}

REGISTER_OUT_DRIVER(DisplayHIDAPI);


