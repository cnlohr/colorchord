//Copyright (Public Domain) 2015 <>< Charles Lohr, under the NewBSD License.
//This file may be used in whole or part in any way for any purpose by anyone
//without restriction.

#include <math.h>
#include "util.h"
#include <stdlib.h>

//Take the absolute distance between two points on a torus.
float fabsloop( float a, float b, float modl )
{
	float fa = fabsf( a - b );

	fa = fmodf( fa, modl );

	if( fa > modl/2.0 )
	{
		fa = modl - fa;
	}

	return fa;
}

//Get the weighted - average of two points on a torus.
float avgloop( float pta, float ampa, float ptb, float ampb, float modl )
{
	float amptot = ampa + ampb;

	//Determine if it should go linearly, or around the edge.
	if( fabsf( pta - ptb ) > modl/2.0 )
	{
		//Loop around the outside.
		if( pta < ptb )
		{
			pta += modl;
		}
		else
		{
			ptb += modl;
		}
	}

	float modmid = (pta * ampa + ptb * ampb)/amptot;

	return fmodf( modmid, modl );
}

int atoi_del( char * data )
{
	int ret = atoi( data );
	free( data );
	return ret;
}

float atof_del( char * data )
{
	float ret = atof( data );
	free( data );
	return ret;
}


