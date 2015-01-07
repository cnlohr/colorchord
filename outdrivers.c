#include "outdrivers.h"
#include <string.h>
#include "parameters.h"
#include "os_generic.h"
#include <stdio.h>
#include <stdlib.h>

struct OutDriverListElem ODList[MAX_OUT_DRIVERS];
const char OutDriverParameters[MAX_OUT_DRIVER_STRING];

void NullUpdate(void * id, struct NoteFinder*nf)
{
}

void NullParams(void * id )
{
}

struct DriverInstances * null( )
{
	printf( "Null lights driver initialized.\n" );
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	ret->id = 0;
	ret->Func = NullUpdate;
	ret->Params = NullParams;
	return ret;
}
REGISTER_OUT_DRIVER(null);


struct DriverInstances * SetupOutDriver( )
{
	int i;
	const char * p = GetParameterS( "displayname", "null" );
	for( i = 0; i < MAX_OUT_DRIVERS; i++ )
	{
		if( ODList[i].Name && strcmp( p, ODList[i].Name ) == 0 )
		{
			printf( "Found: %s %p\n", ODList[i].Name, ODList[i].Init );
			return ODList[i].Init(  );
			break;
		}
	}
	if( i == MAX_OUT_DRIVERS )
	{
		fprintf( stderr, "Error: Could not find outdriver.\n" );
	}
	return 0;
}

void RegOutDriver( const char * ron, struct DriverInstances * (*Init)( ) )
{
	int i;
	for( i = 0; i < MAX_OUT_DRIVERS; i++ )
	{
		if( ODList[i].Name == 0 )
		{
			ODList[i].Name = strdup( ron );
			ODList[i].Init = Init;
			break;
		}
	}
	if( i == MAX_OUT_DRIVERS )
	{
		fprintf( stderr, "Error: Too many outdrivers registered.\n" );
		exit( -55 );
	}
}


