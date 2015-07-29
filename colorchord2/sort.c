//Copyright (Public Domain) 2015 <>< Charles Lohr, under the NewBSD License.
//This file may be used in whole or part in any way for any purpose by anyone
//without restriction.

#include "sort.h"
#include <string.h>
#include <stdio.h>

//Sort the indices of an array of floating point numbers
void SortFloats( int * indexouts, float * array, int count )
{
	int i;
	int j;
	unsigned char selected[count];

	memset( selected, 0, sizeof( selected ) );

	for( i = 0; i < count; i++ )
	{
		int leastindex = -1;
		float leastval = -1e200;

		for( j = 0; j < count; j++ )
		{
			if( !selected[j] && array[j] > leastval )
			{
				leastval = array[j];
				leastindex = j;
			}
		}

		if( leastindex < 0 )
		{
			fprintf( stderr, "ERROR: Sorting fault.\n" );
			goto fault;
		}

		selected[leastindex] = 1;
		indexouts[i] = leastindex;
	} 

	return;
fault:
	for( i = 0; i < count; i++ )
	{
		indexouts[i] = i;
	}
}



//Move the floats around according to the new index.
void RemapFloats( int * indexouts, float * array, int count )
{
	int i;
	float copyarray[count];
	memcpy( copyarray, array, sizeof( copyarray ) );
	for( i = 0; i < count; i++ )
	{
		array[i] = copyarray[indexouts[i]];;
	}
}

