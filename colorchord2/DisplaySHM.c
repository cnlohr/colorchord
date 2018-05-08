//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include "parameters.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "color.h"
#include "DrawFunctions.h"
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>

struct SHMDriver
{
	int lights_file;
	//int dft_file; //Not available.
	//uint8_t * dft_ptr;
	uint8_t * lights_ptr;
	//int total_dft;
	int total_leds;
};


static void SHMUpdate(void * id, struct NoteFinder*nf)
{
	struct SHMDriver * d = (struct SHMDriver*)id;


	if( !d->lights_file )
	{
		const char * shm_lights = GetParameterS( "shm_lights", 0 );
		//const char * shm_dft    = GetParameterS( "shm_dft", 0 ); // Not available.

		if( shm_lights )
		{
			d->lights_file = shm_open(shm_lights, O_CREAT | O_RDWR, 0644);
			ftruncate( d->lights_file, 16384 );
			d->lights_ptr = mmap(0,16384, PROT_READ | PROT_WRITE, MAP_SHARED, d->lights_file, 0);
		}

		printf( "Got SHM: %s->%d [%p]\n", shm_lights, d->lights_file, d->lights_ptr );
	}


	if( d->lights_ptr )
	{
		memcpy( d->lights_ptr, &d->total_leds, 4 );
		memcpy( d->lights_ptr + 4, OutLEDs, d->total_leds*3 ); 
	}

}

static void SHMParams(void * id )
{
	struct SHMDriver * d = (struct SHMDriver*)id;

	d->total_leds = 300;	RegisterValue( "leds", PAINT, &d->total_leds, sizeof( d->total_leds ));
}

static struct DriverInstances * DisplaySHM(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct SHMDriver * d = ret->id = malloc( sizeof( struct SHMDriver ) );
	memset( d, 0, sizeof( struct SHMDriver ) );
	ret->Func = SHMUpdate;
	ret->Params = SHMParams;
	SHMParams( d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplaySHM);


