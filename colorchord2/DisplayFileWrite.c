//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>

#include "outdrivers.h"
#include "DrawFunctions.h"

#include "color.h"

extern struct NoteFinder * nf;

#ifndef O_DIRECT
	#define O_DIRECT	00040000
#endif

struct FileWriteDriver
{
	int lights_file;
	int total_leds;
	int inflate_to_u32;
	int file_thread_usleep;
	int asynchronous;
	uint32_t pass_buffer[MAX_LEDS];
	og_thread_t  rt_thread;
	og_sema_t rt_sema;
};

static void * LightsWrite( void * v )
{
	struct FileWriteDriver * d = (struct FileWriteDriver *)v;
	while(1)
	{
		OGLockSema( d->rt_sema );
		if( d->lights_file > 0 )
		{
			int btos = ((d->inflate_to_u32)?4:3)*d->total_leds;
			int r = write( d->lights_file, d->pass_buffer, btos );
			if( r != btos ) goto fail_write;
		}
		else
		{
			const char * lightsfile = GetParameterS( "lightsfile", 0 );
			if( lightsfile )
			{
				d->lights_file = open( lightsfile, O_WRONLY  );
				if( d->lights_file <= 0 )
				{
					fprintf( stderr, "Error: Can't open \"%s\" (%d)\n", lightsfile, d->lights_file );
				}
				else
				{
					fprintf( stderr, "File %s opened OK\n", lightsfile );
				}
			}
		}
		continue;
	fail_write:
		fprintf( stderr, "File writing fault\n" );
		close( d->lights_file );
		d->lights_file = 0;
	}
	return 0;
}

static void FileWriteUpdate(void * id, struct NoteFinder*nf)
{
	struct FileWriteDriver * d = (struct FileWriteDriver*)id;

	if( OGGetSema( d->rt_sema ) > 0 ) return;
	if( !d->inflate_to_u32 )
	{
		memcpy( d->pass_buffer, OutLEDs, d->total_leds*3 );
	}
	else
	{
		int i;
		for( i = 0; i < d->total_leds; i++ )
		{
			uint8_t * ol = &OutLEDs[i*3];
			d->pass_buffer[i] = ol[0] | (ol[1]<<8) | (ol[2]<<16) |  0xff000000;
		}
	}
    OGUnlockSema( d->rt_sema );

}

static void FileWriteParams(void * id )
{
	struct FileWriteDriver * d = (struct FileWriteDriver*)id;

	d->total_leds = 300;	RegisterValue( "leds", PAINT, &d->total_leds, sizeof( d->total_leds ));
	d->inflate_to_u32 = 1;  RegisterValue( "inflate_to_u32", PAINT, &d->inflate_to_u32, sizeof( d->inflate_to_u32 ));
	d->asynchronous = 1;	RegisterValue( "file_async", PAINT, &d->asynchronous, sizeof( d->asynchronous ));
	d->file_thread_usleep = 10000;	RegisterValue( "file_thread_usleep", PAINT, &d->file_thread_usleep, sizeof( d->file_thread_usleep ));
}

static struct DriverInstances * DisplayFileWrite(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct FileWriteDriver * d = ret->id = malloc( sizeof( struct FileWriteDriver ) );
	memset( d, 0, sizeof( struct FileWriteDriver ) );
	ret->Func = FileWriteUpdate;
	ret->Params = FileWriteParams;
	FileWriteParams( d );
	printf( "Loaded DisplayFileWrite\n" );
    d->rt_sema = OGCreateSema();
	d->rt_thread = OGCreateThread( LightsWrite, d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplayFileWrite);


