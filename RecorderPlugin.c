#include "outdrivers.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "parameters.h"
#include "hook.h"

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
	fclose( rp->fRec );
	fclose( rp->fPlay );
	rp->is_recording = 0;
	rp->fRec = 0;
	rp->fPlay = 0;
}

void StartRecording( struct RecorderPlugin * rp )
{
	if( rp->is_recording ) return;

	rp->TimeSinceStart = 0;
	rp->DunBoop = 0;
	rp->is_recording = 1;

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
	}

	if( rp->Out_Filename[0] == 0 )
	{
		//Nothing
		rp->fRec = 0;
	}
	else
	{
		int i;
		for( i = 0; i < 999; i++ )
		{
			char cts[1024];
			if( i == 0 )
			{
				snprintf( cts, 1023, "%s", rp->Out_Filename );
			}
			else
			{
				snprintf( cts, 1023, "%s.%03d", rp->Out_Filename, i );
			}

			rp->fRec = fopen( cts, "wb" );
			if( rp->fRec ) break;
		}
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

	if( rp->TimeSinceStart < rp->BypassLength )
	{
		int r = fread( samps, channel_ct * sizeof( float ), samples, rp->fPlay );
		if( r != samples )
		{
			StopRecording( rp );
		}

		rp->TimeSinceStart += samples;
		if( rp->TimeSinceStart > rp->BypassLength )
		{
			rp->DunBoop = 1;
		}
		else
		{
			int r = fwrite( samps, channel_ct * sizeof( float ), samples, rp->fRec );
			if( r != samples )
			{
				StopRecording( rp );
			}
		}
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
	d->sps = 0;		RegisterValue(  "samplerate", PAINT, &d->sps, sizeof( d->sps ) );
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


