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

struct DMXOutDriver
{
	struct libusb_device_handle *devh;
	int did_init;
	int byte_offset;
	int total_leds;
	float outamp;
	uint8_t * last_leds;
	volatile int readyFlag;
};


static void * DMXOutThread( void * v )
{
	struct DMXOutDriver * led = (struct DMXOutDriver*)v;
	while(1)
	{
		if( led->readyFlag && led->devh )
		{
/*			int r = libusb_control_transfer( led->devh,
				0x40, //reqtype
				0xA5, //request
				0x0100, //wValue
				0x0000, //wIndex
				led->last_leds,
				(led->total_leds*3)+1,
				1000 );*/
			int r = libusb_control_transfer( led->devh, 0, 0xc6, 0x100, 0,
				led->last_leds,
				(led->total_leds*3)+led->byte_offset+1,
				2000 );

			if( r < 0 )
			{
				led->did_init = 0;
				printf( "Fault sending DMXs.\n" );
			}
		}
		led->readyFlag = 0;
		usleep(100);
	}
	return 0;
}

static void DMXUpdate(void * id, struct NoteFinder*nf)
{
	int i;
	struct DMXOutDriver * led = (struct DMXOutDriver*)id;

	if( !led->did_init )
	{
		led->did_init = 1;
		if( libusb_init(NULL) < 0 )
		{
			fprintf( stderr, "Error: Could not initialize libUSB\n" );
//			exit( -99 );
		}

		led->devh = libusb_open_device_with_vid_pid( NULL, 0x16c0, 0x27dd );
//		led->devh = libusb_open_device_with_vid_pid( NULL, 0x1d6b, 0x0002 );


		if( !led->devh )
		{
			led->did_init = 0;
			fprintf( stderr,  "Error: Cannot find device.\n" );
//			exit( -98 );
		}
	}

	while( led->readyFlag ) usleep(100);

	//Advance the DMXs to this position when outputting the values.
	for( i = 0; i < led->total_leds; i++ )	
	{
		int source = i;
		led->last_leds[i*3+0 + led->byte_offset] = OutLEDs[source*3+0] * led->outamp;
		led->last_leds[i*3+1 + led->byte_offset] = OutLEDs[source*3+1] * led->outamp; 
		led->last_leds[i*3+2 + led->byte_offset] = OutLEDs[source*3+2] * led->outamp;
	}

	led->readyFlag = 1;
}

static void DMXParams(void * id )
{
	struct DMXOutDriver * led = (struct DMXOutDriver*)id;

	led->total_leds = GetParameterI( "leds", 10 );
	led->byte_offset = 32; RegisterValue(  "byte_offset", PAINT, &led->byte_offset, sizeof( led->byte_offset ) );
	led->last_leds = malloc( 1026 );
	led->outamp = .1; RegisterValue(  "ledoutamp", PAFLOAT, &led->outamp, sizeof( led->outamp ) );

	led->did_init = 0;
}


static struct DriverInstances * DisplayDMX()
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	memset( ret, 0, sizeof( struct DriverInstances ) );
	struct DMXOutDriver * led = ret->id = malloc( sizeof( struct DMXOutDriver ) );
	ret->Func = DMXUpdate;
	ret->Params = DMXParams;
	OGCreateThread( DMXOutThread, led );
	led->readyFlag = 0;
	DMXParams( led );
	return ret;

}

REGISTER_OUT_DRIVER(DisplayDMX);


