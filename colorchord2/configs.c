#include "configs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_generic.h>
#include "parameters.h"

int gargc;
char ** gargv;


const char * InitialFile[NRDEFFILES];
double FileTimes[NRDEFFILES];
int InitialFileCount = 1;


void LoadFile( const char * filename )
{
	char * buffer;
	int r;

	printf( "Loading file: %s\n", filename );
	FILE * f = fopen( filename, "rb" );
	if( !f )
	{
		fprintf( stderr, "Warning: cannot open %s.\n", filename );
	}
	else
	{
		fseek( f, 0, SEEK_END );
		int size = ftell( f );
		fseek( f, 0, SEEK_SET );
		buffer = malloc( size + 1 );
		r = fread( buffer, 1, size, f);
		fclose( f );
		buffer[size] = 0;
		if( r != size )
		{
			fprintf( stderr, "Warning: %d bytes read. Expected: %d from file %s\n", r, size, filename );
		}
		else
		{
			SetParametersFromString( buffer );
		}
		free( buffer );
	}
}

void SetEnvValues( int force )
{
	static int ifcheck;
	int hits = 0;

	if( InitialFileCount )
	{
		//Only check one location per frame.
		double ft = OGGetFileTime( InitialFile[ifcheck] );
		if( FileTimes[ifcheck] != ft )
		{
			FileTimes[ifcheck] = ft;
			hits++;
		}
		ifcheck = ( ifcheck + 1 ) % InitialFileCount;
	}

	if( !hits && !force ) return;

	//Otherwise, something changed.
#ifdef ANDROID
	SetParametersFromString( "cpu_autolimit=1" );
	SetParametersFromString( "set_screenx=720" );
	SetParametersFromString( "set_screeny=480" );
	SetParametersFromString( "buffer=384" );
	SetParametersFromString( "play=0" );
	SetParametersFromString( "rec=1" );
	SetParametersFromString( "channels=2" );
	SetParametersFromString( "samplerate=44100" );
	SetParametersFromString( "sourcename=default" );
	SetParametersFromString( "amplify=2.0" );
	SetParametersFromString( "base_hz=55" );

	SetParametersFromString( "dft_iir=0.6" );
	SetParametersFromString( "dft_q=20.0000" );
	SetParametersFromString( "dft_speedup=1000.0000" );
	SetParametersFromString( "octaves=5" );

	SetParametersFromString( "do_progressive_dft=4" );

	SetParametersFromString( "filter_iter=2" );
	SetParametersFromString( "filter_strength=.5" );
	SetParametersFromString( "freqbins = 24" );
	SetParametersFromString( "do_progressive_dft=4" );
	SetParametersFromString( "note_attach_amp_iir=0.3500" );
	SetParametersFromString( "note_attach_amp_iir2=0.250" );

	SetParametersFromString( "note_combine_distance=0.5000" );
	SetParametersFromString( "note_jumpability=1.8000" );
	SetParametersFromString( "note_minimum_new_distribution_value=0.0200" );
	SetParametersFromString( "note_out_chop=0.05000" );
	SetParametersFromString( "outdrivers=OutputVoronoi,DisplayArray" );
	SetParametersFromString( "note_attach_amp_iir2=0.250" );

	SetParametersFromString( "lightx=32" );
	SetParametersFromString( "lighty=60" );
	SetParametersFromString( "fromsides=1" );
	SetParametersFromString( "shape_cutoff=0.03" );

	SetParametersFromString( "satamp=5.000" );
	SetParametersFromString( "amppow=2.510" );
	SetParametersFromString( "distpow=1.500" );

	printf( "On Android, looking for configuration file in: %s\n", InitialFile[0] );
#endif

	int i;
	for( i = 0; i < InitialFileCount; i++ )
	{
		LoadFile( InitialFile[i] );
	}
	for( ; i < gargc; i++ )
	{
		if( strchr( gargv[i], '=' ) != 0 )
		{
			printf( "AP: %s\n", gargv[i] );
			SetParametersFromString( gargv[i] );
		}
		else
		{
			printf( "LF: %s\n", gargv[i] );
			LoadFile( gargv[i] );
		}
	}
}

void ProcessArgs()
{
	int i;
	for( i = 1; i < gargc; i++ )
	{
		if( strchr( gargv[i], '=' ) != 0 )
		{
			//A value setting operation
		}
		else
		{
			InitialFile[InitialFileCount++] = gargv[i];
		}
	}

	SetEnvValues( 1 );
}


void SetupConfigs()
{
#ifdef ANDROID
	InitialFile[0] = "/sdcard/colorchord-android.txt";
	InitialFile[1] = "/storage/emulated/0/colorchord-android.txt";
	InitialFile[2] = "/sdcard/colorchord-android-overlay.txt";
	InitialFile[3] = "/storage/emulated/0/colorchord-android-overlay.txt";
	InitialFileCount = 4;
#else
	InitialFile[0] = "default.conf";
#endif

	ProcessArgs();
}
