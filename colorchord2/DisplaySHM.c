//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <stdio.h>
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "outdrivers.h"
#include "DrawFunctions.h"

#include "color.h"

extern struct NoteFinder * nf;


struct SHMDriver
{
	int lights_file;
	int dft_file;
	int notes_file;

	uint8_t * dft_ptr;
	uint8_t * lights_ptr;
	uint8_t * notes_ptr;

	int total_dft;
	int total_leds;
};


static void SHMUpdate(void * id, struct NoteFinder*nf)
{
	struct SHMDriver * d = (struct SHMDriver*)id;

	if( !d->lights_file )
	{
		const char * shmname = GetParameterS( "shm_lights", 0 );
		if( shmname )
		{
			d->lights_file = shm_open(shmname, O_CREAT | O_RDWR, 0644);
			ftruncate( d->lights_file, 16384 );
			d->lights_ptr = mmap(0,16384, PROT_READ | PROT_WRITE, MAP_SHARED, d->lights_file, 0);
		}
	}


	if( !d->dft_file )
	{
		const char * shmname = GetParameterS( "shm_dft", 0 );
		if( shmname )
		{
			d->dft_file = shm_open(shmname, O_CREAT | O_RDWR, 0644);
			ftruncate( d->dft_file, 16384 );
			d->dft_ptr = mmap(0,16384, PROT_READ | PROT_WRITE, MAP_SHARED, d->dft_file, 0);
		}
	}


	if( !d->notes_file )
	{
		const char * shmname = GetParameterS( "shm_notes", 0 );
		if( shmname )
		{
			d->notes_file = shm_open(shmname, O_CREAT | O_RDWR, 0644);
			ftruncate( d->notes_file, 16384 );
			d->notes_ptr = mmap(0,16384, PROT_READ | PROT_WRITE, MAP_SHARED, d->notes_file, 0);
		}
	}


	if( d->dft_ptr )
	{

		d->total_dft = nf->octaves * nf->freqbins;
		memcpy( d->dft_ptr+0, &nf->octaves, 4 );
		memcpy( d->dft_ptr+4, &nf->freqbins, 4 );
		memcpy( d->dft_ptr+8, nf->folded_bins, nf->freqbins * sizeof(float) );
		memcpy( d->dft_ptr+8+nf->freqbins * sizeof(float),
			 nf->outbins, d->total_dft * sizeof(float) );
	}

	if( d->lights_ptr )
	{
		memcpy( d->lights_ptr, &d->total_leds, 4 );
		memcpy( d->lights_ptr + 4, OutLEDs, d->total_leds*3 ); 
	}


	if( d->notes_ptr )
	{
		memcpy( d->notes_ptr, &nf->dists_count, 4 );
		memcpy( d->notes_ptr+4, nf->dists, sizeof( nf->dists[0] ) * nf->dists_count );
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


