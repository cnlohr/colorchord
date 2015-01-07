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
	int total_leds;
	float light_siding;
	float * last_led_pos;
	float * last_led_amp;
	float led_floor;
	int led_bins;
	float ampoff;
	uint8_t * last_leds;
	volatile int readyFlag;
};


static void * LEDOutThread( void * v )
{
	struct LEDOutDriver * led = (struct LEDOutDriver*)v;
	while(1)
	{
		if( led->readyFlag )
		{
			int r = libusb_control_transfer( led->devh,
				0x40, //reqtype
				0xA5, //request
				0x0100, //wValue
				0x0000, //wIndex
				led->last_leds,
				(led->total_leds*3)+1,
				1000 );
			if( r < 0 ) led->did_init = 0;
			led->readyFlag = 0;
		}
		usleep(100);
	}
	return 0;
}

static void LEDUpdate(void * id, struct NoteFinder*nf)
{
	struct LEDOutDriver * led = (struct LEDOutDriver*)id;

	if( !led->did_init )
	{
		led->did_init = 1;
		if( libusb_init(NULL) < 0 )
		{
			fprintf( stderr, "Error: Could not initialize libUSB\n" );
			exit( -99 );
		}

		led->devh = libusb_open_device_with_vid_pid( NULL, 0xabcd, 0xf003 );

		if( !led->devh )
		{
			fprintf( stderr,  "Error: Cannot find device.\n" );
			exit( -98 );
		}
	}




	//Step 1: Calculate the quantity of all the LEDs we'll want.
	int totbins = nf->dists;
	int i, j;
	float binvals[totbins];
	float binpos[totbins];
	float totalbinval = 0;

//	if( totbins > led_bins ) totbins = led_bins;



	for( i = 0; i < totbins; i++ )
	{
		binpos[i] = nf->note_positions[i] / nf->freqbins;
		binvals[i] = pow( nf->note_amplitudes[i], led->light_siding );
		totalbinval += binvals[i];
	}

	float rledpos[led->total_leds];
	float rledamp[led->total_leds];
	int rbinout = 0;

	for( i = 0; i < totbins; i++ )
	{
		int nrleds = (int)((binvals[i] / totalbinval) * led->total_leds);
		if( nrleds < 40 ) nrleds = 0;
		for( j = 0; j < nrleds && rbinout < led->total_leds; j++ )
		{
			rledpos[rbinout] = binpos[i];
			rledamp[rbinout] = binvals[i];
			rbinout++;
		}
	}

	for( ; rbinout < led->total_leds; rbinout++ )
	{
		rledpos[rbinout] = rledpos[rbinout-1];
		rledamp[rbinout] = rledamp[rbinout-1];
	}

	//Now we have to minimize "advance".
	int minadvance = 0;
//	float mindiff = 1e20;
/*
	//Uncomment this for a rotationally continuous surface.
	for( i = 0; i < total_leds; i++ )
	{
		float diff = 0;
		diff = 0;
		for( j = 0; j < total_leds; j+=4 )
		{
			int r = (j + i) % total_leds;
			diff += (last_led_pos[j] - rledpos[r])*(last_led_pos[j] - rledpos[r]);
		}
		if( diff < mindiff )
		{
			mindiff = diff;
			minadvance = i;
		}
	}
*/
//	printf( "MA: %d %f\n", minadvance, mindiff );

	while( led->readyFlag ) usleep(100);

	//Advance the LEDs to this position when outputting the values.
	for( i = 0; i < led->total_leds; i++ )	
	{
		int ia = ( i + minadvance + led->total_leds ) % led->total_leds;
		led->last_led_pos[i] = rledpos[ia];
		led->last_led_amp[i] = rledamp[ia] * led->ampoff;
		int r = CCtoHEX( led->last_led_pos[i], 1.0, led->last_led_amp[i] );
		led->last_leds[i*3+0] = (r>>8) & 0xff;
		led->last_leds[i*3+1] = r & 0xff;
		led->last_leds[i*3+2] = (r>>16) & 0xff;
	}

	led->readyFlag = 1;
}

static void LEDParams(void * id )
{
	struct LEDOutDriver * led = (struct LEDOutDriver*)id;

	led->total_leds = GetParameterI( "leds", 300 );
	led->ampoff = GetParameterF( "amp", 1.0 );
	led->light_siding = GetParameterF( "light_siding", 1.4 );

	printf( "Found LEDs for output.  leds=%d\n", led->total_leds );

	if( led->last_led_pos ) free( led->last_led_pos );
	led->last_led_pos = malloc( sizeof( float ) * led->total_leds );

	if( led->last_led_amp ) free( led->last_led_amp );
	led->last_led_amp = malloc( sizeof( float ) * led->total_leds );

	led->led_floor = GetParameterF( "led_floor", .1 );
	led->led_bins = GetParameterF( "led_bins", 12 );

	if( led->last_leds ) free( led->last_leds );
	led->last_leds = malloc( led->total_leds * 3 );

}


static struct DriverInstances * LEDOutDriver()
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

REGISTER_OUT_DRIVER(LEDOutDriver);


