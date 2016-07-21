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
		r = fread( buffer, size, 1, f );
		fclose( f );
		buffer[size] = 0;
		if( r != 1 )
		{
			fprintf( stderr, "Warning: %d bytes read.  Expected: %d from file %s\n", r, size, filename );
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
	int i;
	int hits = 0;
	for( i = 0; i < InitialFileCount; i++ )
	{
		double ft = OGGetFileTime( InitialFile[i] );
		if( FileTimes[i] != ft )
		{
			FileTimes[i] = ft;
			hits++;
		}
	}

	if( !hits && !force ) return;

	//Otherwise, something changed.

	LoadFile( InitialFile[0] );

	for( i = 1; i < gargc; i++ )
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

	InitialFile[0] = "default.conf";

	ProcessArgs();

}
