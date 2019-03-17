//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "outdrivers.h"
#include "DrawFunctions.h"

#include "color.h"

//Uses: note_amplitudes2[note] for how many lights to use.
//Uses: note_amplitudes_out[note] for how bright it should be.

#define MAX_LEDS_PER_NOTE 512

extern short screenx, screeny;

struct DPODriver
{
	int leds;
	float pie_min;
	float pie_max;
};


static void DPOUpdate(void * id, struct NoteFinder*nf)
{
	struct DPODriver * d = (struct DPODriver*)id;
	int i;
	float cw = ((float)(screenx)) / 2.0;
	float ch = ((float)(screeny)) / 2.0;
	RDPoint pts[4];
	float sizeA = sqrtf( screenx * screenx + screeny * screeny ) * d->pie_min;
	float sizeB = sqrtf( screenx * screenx + screeny * screeny ) * d->pie_max;
	for( i = 0; i < d->leds; i++ )
	{
		float angA = 6.28318 * (float)i / (float)d->leds;
		float angB = 6.28318 * (float)(i+1) / (float)d->leds + .002;
		pts[0].x = cw + cos(angA) * sizeA;
		pts[0].y = ch + sin(angA) * sizeA;
		pts[1].x = cw + cos(angA) * sizeB;
		pts[1].y = ch + sin(angA) * sizeB;
		pts[2].x = cw + cos(angB) * sizeB;
		pts[2].y = ch + sin(angB) * sizeB;
		pts[3].x = cw + cos(angB) * sizeA;
		pts[3].y = ch + sin(angB) * sizeA;

		CNFGColor(  OutLEDs[i*3+0] | (OutLEDs[i*3+1] <<8)|(OutLEDs[i*3+2] <<16) );
		CNFGTackPoly( pts, 4 );
	}


	CNFGColor( 0xffffff );
}

static void DPOParams(void * id )
{
	struct DPODriver * d = (struct DPODriver*)id;

	d->leds = 9;		RegisterValue(  "leds", PAINT, &d->leds, sizeof( d->leds ) );
	d->pie_min = .18;	RegisterValue(  "pie_min", PAFLOAT, &d->pie_min, sizeof( d->pie_min ) );
	d->pie_max = .3;	RegisterValue(  "pie_max", PAFLOAT, &d->pie_max, sizeof( d->pie_max ) );

}

static struct DriverInstances * DisplayPie(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct DPODriver * d = ret->id = malloc( sizeof( struct DPODriver ) );
	memset( d, 0, sizeof( struct DPODriver ) );
	ret->Func = DPOUpdate;
	ret->Params = DPOParams;
	DPOParams( d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplayPie);


