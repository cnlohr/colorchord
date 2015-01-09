#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include "parameters.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "color.h"
#include "DrawFunctions.h"

//Uses: note_amplitudes2[note] for how many lights to use.
//Uses: note_amplitudes_out[note] for how bright it should be.

#define MAX_LEDS_PER_NOTE 512

extern short screenx, screeny;

struct DPODriver
{
	int xn;
	int yn;
	int zigzag;
};


static void DPOUpdate(void * id, struct NoteFinder*nf)
{
	int x,y;
	struct DPODriver * d = (struct DPODriver*)id;


	float cw = ((float)screenx) / d->xn;
	float ch = ((float)screeny) / d->yn;

	for( y = 0; y < d->yn; y++ )
	for( x = 0; x < d->xn; x++ )
	{
		int index = 0;
		if( d->zigzag )
		{
			if( y & 1 )
			{
				index = (d->xn-x-1)+y*d->xn;
			}
			else
			{
				index = x+y*d->xn;
			}
		}
		else
		{
			index = x+y*d->xn;
		}
		CNFGColor(  OutLEDs[index*3+0] | (OutLEDs[index*3+1] <<8)|(OutLEDs[index*3+2] <<16) );
		float dx = (x) * cw;
		float dy = (y) * ch;
		CNFGTackRectangle( dx, dy, dx+cw+.5, dy+ch+.5 );
	}
	CNFGColor( 0xffffff );
}

static void DPOParams(void * id )
{
	struct DPODriver * d = (struct DPODriver*)id;

	d->xn = 16;		RegisterValue(  "lightx", PINT, &d->xn, sizeof( d->xn ) );
	d->yn = 9;		RegisterValue(  "lighty", PINT, &d->yn, sizeof( d->yn ) );
	d->zigzag = 0;	RegisterValue(  "zigzag", PINT, &d->zigzag, sizeof( d->zigzag ) );

}

static struct DriverInstances * DisplayArray(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct DPODriver * d = ret->id = malloc( sizeof( struct DPODriver ) );
	memset( d, 0, sizeof( struct DPODriver ) );
	ret->Func = DPOUpdate;
	ret->Params = DPOParams;
	DPOParams( d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplayArray);


