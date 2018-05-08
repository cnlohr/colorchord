//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "sound.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static SoundInitFn * SoundDrivers[MAX_SOUND_DRIVERS];
static char * SoundDriverNames[MAX_SOUND_DRIVERS];  //XXX: There's a bug in my compiler, this should be 'static'
static int SoundDriverPriorities[MAX_SOUND_DRIVERS];
/*
void CleanupSound() __attribute__((destructor));
void CleanupSound()
{
	int i;
	for( i = 0; i < MAX_SOUND_DRIVERS; i++ )
	{
		if( SoundDriverNames[i] )
		{
			free( SoundDriverNames[i] );
		}
	}
}
*/

void RegSound( int priority, const char * name, SoundInitFn * fn )
{
	int j;

	if( priority <= 0 )
	{
		return;
	}

	for( j = MAX_SOUND_DRIVERS-1; j >= 0; j-- )
	{
		//Cruise along, find location to insert
		if( j > 0 && ( !SoundDrivers[j-1] || SoundDriverPriorities[j-1] < priority ) )
		{
			SoundDrivers[j] = SoundDrivers[j-1];
			SoundDriverNames[j] = SoundDriverNames[j-1];
			SoundDriverPriorities[j] = SoundDriverPriorities[j-1];
		}
		else
		{
			SoundDrivers[j] = fn;
			SoundDriverNames[j] = strdup( name );
			SoundDriverPriorities[j] = priority;
			break;
		}
	}
}

struct SoundDriver * InitSound( const char * driver_name, SoundCBType cb )
{
	int i;
	struct SoundDriver * ret = 0;
	if( driver_name == 0 || strlen( driver_name ) == 0 )
	{
		//Search for a driver.
		for( i = 0; i < MAX_SOUND_DRIVERS; i++ )
		{
			if( SoundDrivers[i] == 0 )
			{
				return 0;
			}
			ret = SoundDrivers[i]( cb );
			if( ret )
			{
				return ret;
			}
		}
	}
	else
	{
		printf( "Initializing sound.  Recommended driver: %s\n", driver_name );
		for( i = 0; i < MAX_SOUND_DRIVERS; i++ )
		{
			if( SoundDrivers[i] == 0 )
			{
				return 0;
			}
			if( strcmp( SoundDriverNames[i], driver_name ) == 0 )
			{
				return SoundDrivers[i]( cb );
			}
		}
	}
	return 0;
}

int SoundState( struct SoundDriver * soundobject )
{
	if( soundobject )
	{
		return soundobject->SoundStateFn( soundobject );
	}
	return -1;
}

void CloseSound( struct SoundDriver * soundobject )
{
	if( soundobject )
	{
		soundobject->CloseFn( soundobject );
	}
}

