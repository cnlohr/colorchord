//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "outdrivers.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "parameters.h"
#include "hook.h"


extern unsigned char	OutLEDs[MAX_LEDS*3];
extern int				UsedLEDs;


struct RecorderPlugin
{
	int is_recording;
	int sps;
	char In_Filename[PARAM_BUFF];
	char Out_Filename[PARAM_BUFF];

	int DunBoop;
	int BypassLength;
	int TimeSinceStart;

	FILE * fRec;
	FILE * fPlay;
};


void StopRecording( struct RecorderPlugin * rp )
{
	if( !rp->is_recording ) return;

	rp->TimeSinceStart = 0;
	rp->DunBoop = 0;
	if( rp->fRec )	fclose( rp->fRec );
	if( rp->fPlay)  fclose( rp->fPlay );
	rp->is_recording = 0;
	rp->fRec = 0;
	rp->fPlay = 0;
	printf( "Stopped.\n" );
}

void StartRecording( struct RecorderPlugin * rp )
{
	if( rp->is_recording ) return;

	rp->TimeSinceStart = 0;
	rp->DunBoop = 0;
	rp->is_recording = 1;

	printf( "Starting Recording: /%s/%s/\n", rp->In_Filename, rp->Out_Filename );

	if( rp->In_Filename[0] == 0 )
	{
		//Nothing
		rp->fPlay = 0;
	}
	else
	{
		rp->fPlay = fopen( rp->In_Filename, "rb" );
		if( !rp->fPlay )
		{
			fprintf( stderr, "Warning: Could not play filename: %s\n", rp->In_Filename );
		}
		printf( "Play file opened.\n" );
	}

	if( rp->Out_Filename[0] == 0 )
	{
		//Nothing
		rp->fRec = 0;
	}
	else
	{
		struct stat buf;
		char cts[1024];
		int i;
		for( i = 0; i < 999; i++ )
		{
			if( i == 0 )
			{
				snprintf( cts, 1023, "%s", rp->Out_Filename );
			}
			else
			{
				snprintf( cts, 1023, "%s.%03d", rp->Out_Filename, i );
			}

			if( stat( cts, &buf ) != 0 )
				break;
		}
		printf( "Found rec file %s\n", cts );
		rp->fRec = fopen( cts, "wb" );
		if( !rp->fRec )
		{
			fprintf( stderr, "Error: cannot start recording file \"%s\"\n", cts );
			return;
		}
		printf( "Recording...\n" );
	}
}


static void RecordEvent(void * v, int samples, float * samps, int channel_ct)
{
	struct RecorderPlugin * rp = (struct RecorderPlugin*)v;

	if( !rp->fRec || !rp->is_recording ) return;

	if( rp->DunBoop || !rp->fPlay )
	{
		int r = fwrite( samps, channel_ct * sizeof( float ), samples, rp->fRec );
		if( r != samples )
		{
			StopRecording( rp );
		}
	}
}

static void PlaybackEvent(void * v, int samples, float * samps, int channel_ct)
{
	struct RecorderPlugin * rp = (struct RecorderPlugin*)v;
	if( !rp->fPlay ) return;

	int r = fread( samps, channel_ct * sizeof( float ), samples, rp->fPlay );
	if( r != samples )
	{
		StopRecording( rp );
		return;
	}
	rp->TimeSinceStart += samples;

	if( rp->TimeSinceStart < rp->BypassLength )
	{
		if( rp->is_recording )
			force_white = 1;
		else
			force_white = 0;

		int r = fwrite( samps, channel_ct * sizeof( float ), samples, rp->fRec );
		if( r != samples )
		{
			StopRecording( rp );
		}
	}
	else
	{
		force_white = 0;
		rp->DunBoop = 1;
	}
}

static void MKeyEvent( void * v, int keycode, int down )
{
	struct RecorderPlugin * rp = (struct RecorderPlugin*)v;
	char c = toupper( keycode );
	if( c == 'R' && down && !rp->is_recording ) StartRecording( rp );
	if( c == 'S' && down &&  rp->is_recording ) StopRecording( rp );
}

static void DPOUpdate(void * id, struct NoteFinder*nf)
{
	//Do nothing, happens every frame.
}


static void DPOParams(void * id )
{
	struct RecorderPlugin * d = (struct RecorderPlugin*)id;
	d->is_recording = 0;
	d->fRec = 0;
	d->fPlay = 0;
	d->DunBoop = 0;
	d->TimeSinceStart = 0;

	memset( d->In_Filename, 0, PARAM_BUFF );	RegisterValue(  "player_filename", PABUFFER, d->In_Filename, PARAM_BUFF );
	memset( d->Out_Filename, 0, PARAM_BUFF );	RegisterValue(  "recorder_filename", PABUFFER, d->Out_Filename, PARAM_BUFF );
	
	d->sps = 0;		RegisterValue(  "samplerate", PAINT, &d->sps, sizeof( d->sps ) );
	d->BypassLength = 0;	RegisterValue(  "recorder_bypass", PAINT, &d->BypassLength, sizeof( d->BypassLength ) );

}

static struct DriverInstances * RecorderPlugin(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct RecorderPlugin * rp = ret->id = malloc( sizeof( struct RecorderPlugin ) );
	memset( rp, 0, sizeof( struct RecorderPlugin ) );
	ret->Func = DPOUpdate;
	ret->Params = DPOParams;
	DPOParams( rp );

	HookKeyEvent( MKeyEvent, rp );
	HookSoundInEvent( RecordEvent, rp, 0 );
	HookSoundInEvent( PlaybackEvent, rp, 1 );

	return ret;
}

REGISTER_OUT_DRIVER(RecorderPlugin);


