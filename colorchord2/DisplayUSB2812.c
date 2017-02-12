//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include <string.h>
#include "parameters.h"
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include "color.h"
#include <math.h>
#include <unistd.h>

struct LEDOutDriver
{
	struct libusb_device_handle *devh;
	int did_init;
	int zigzag;
	int total_leds;
	int array;
	float outamp;
	uint8_t * last_leds;
	volatile int readyFlag;
	int xn;
	int yn;
	int rot90;
};


static void * LEDOutThread( void * v )
{
	struct LEDOutDriver * led = (struct LEDOutDriver*)v;
	while(1)
	{
		int total_bytes = (led->total_leds*3)+1;
		total_bytes = ((total_bytes-1)&0xffc0) + 0x40; //Round up.
		if( led->readyFlag )
		{
			int r = libusb_control_transfer( led->devh,
				0x40, //reqtype
				0xA5, //request
				0x0100, //wValue
				0x0000, //wIndex
				led->last_leds,
				total_bytes,
				1000 );
			if( r < 0 )
			{
				led->did_init = 0;
				printf( "Fault sending LEDs.\n" );
			}
			led->readyFlag = 0;
		}
		usleep(100);
	}
	return 0;
}

static void LEDUpdate(void * id, struct NoteFinder*nf)
{
	int i;
	struct LEDOutDriver * led = (struct LEDOutDriver*)id;

	if( !led->did_init )
	{
		led->did_init = 1;
		if( libusb_init(NULL) < 0 )
		{
			fprintf( stderr, "Error: Could not initialize libUSB\n" );
//			exit( -99 );
		}

		led->devh = libusb_open_device_with_vid_pid( NULL, 0xabcd, 0xf003 );

		if( !led->devh )
		{
			fprintf( stderr,  "Error: Cannot find device.\n" );
//			exit( -98 );
		}
	}

	while( led->readyFlag ) usleep(100);

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
		led->last_leds[i*3+0] = OutLEDs[source*3+1] * led->outamp;
		led->last_leds[i*3+1] = OutLEDs[source*3+0] * led->outamp; 
		led->last_leds[i*3+2] = OutLEDs[source*3+2] * led->outamp;
	}

	led->readyFlag = 1;
}

static void LEDParams(void * id )
{
	struct LEDOutDriver * led = (struct LEDOutDriver*)id;

	led->total_leds = GetParameterI( "leds", 300 );
	led->last_leds = malloc( led->total_leds * 3 + 1 );
	led->outamp = .1; RegisterValue(  "ledoutamp", PAFLOAT, &led->outamp, sizeof( led->outamp ) );
	led->zigzag = 0; RegisterValue(  "zigzag", PAINT, &led->zigzag, sizeof( led->zigzag ) );
	led->xn = 16;		RegisterValue(  "lightx", PAINT, &led->xn, sizeof( led->xn ) );
	led->yn = 9;		RegisterValue(  "lighty", PAINT, &led->yn, sizeof( led->yn ) );
	led->rot90 = 0;	RegisterValue(  "rot90", PAINT, &led->rot90, sizeof( led->rot90 ) );
	led->array = 0;	RegisterValue(  "ledarray", PAINT, &led->array, sizeof( led->array ) );

	led->did_init = 0;
}


static struct DriverInstances * DisplayUSB2812()
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	memset( ret, 0, sizeof( struct DriverInstances ) );
	struct LEDOutDriver * led = ret->id = malloc( sizeof( struct LEDOutDriver ) );
	ret->Func = LEDUpdate;
	ret->Params = LEDParams;
	OGCreateThread( LEDOutThread, led );
	led->readyFlag = 0;
	LEDParams( led );
	return ret;

}

REGISTER_OUT_DRIVER(DisplayUSB2812);


