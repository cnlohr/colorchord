#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include "parameters.h"
#include <stdlib.h>
#include <string.h>
#include "color.h"
#include <math.h>
#include "DrawFunctions.h"

//Uses: note_amplitudes2[note] for how many lights to use.
//Uses: note_amplitudes_out[note] for how bright it should be.

#define MAX_LEDS_PER_NOTE 512

extern short screenx, screeny;

struct LINote
{
	float x, y;   //In screen space.
	float ledexp;
	float lednrm;  //How many LEDs should we have?
};

struct DPODriver
{
	int xn;
	int yn;
	float cutoff;
	float satamp;
	float amppow;  //For amplitudes
	float distpow; //for distances
	int note_peaks;
	int from_sides;

	struct LINote * notes;
};

static void DPOUpdate(void * id, struct NoteFinder*nf)
{
	int i;
	struct DPODriver * d = (struct DPODriver*)id;

	int tleds = d->xn * d->yn;

	if( d->note_peaks != nf->note_peaks )
	{
		d->note_peaks = nf->note_peaks;
		if( d->notes ) free( d->notes );
		d->notes = malloc( sizeof( struct LINote ) * nf->note_peaks );
		memset( d->notes, 0, sizeof( struct LINote ) * nf->note_peaks );
	}


	float totalexp = 0;

	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * l = &d->notes[i];
		l->ledexp = pow( nf->note_amplitudes2[i], d->amppow ) - d->cutoff;
		if( l->ledexp < 0 ) l->ledexp = 0;
		totalexp += l->ledexp;
		if( d->from_sides )
		{
			float angle = nf->note_positions[i] / nf->freqbins * 6.28318;
//			float angle = nf->enduring_note_id[i];
			float cx = screenx/2;
			float cy = screeny/2;
			float tx = sin( angle ) * cx + cx;
			float ty = cos( angle ) * cy + cy;
			l->x = l->x * .9 + tx * .1;
			l->y = l->y * .9 + ty * .1;
			if( nf->enduring_note_id[i] == 0 )
			{
				l->x = cx;
				l->y = cy;
			}
		}
		else
		{
			srand( nf->enduring_note_id[i] );
			l->x = rand()%screenx;
			l->y = rand()%screenx;
		}
	}

	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * l = &d->notes[i];
		l->lednrm = l->ledexp * tleds / totalexp;
	}



	float cw = ((float)screenx) / d->xn;
	float ch = ((float)screeny) / d->yn;

	int x, y;
	for( y = 0; y < d->yn; y++ )
	for( x = 0; x < d->xn; x++ )
	{
		float lx = (x+.5) * cw;
		float ly = (y+.5) * ch;
		float kx = (x) * cw;
		float ky = (y) * ch;

		int bestmatch = -1;
		float bestmatchval = 0;
		for( i = 0; i < d->note_peaks; i++ )
		{
			struct LINote * l = &d->notes[i];
			float distsq = (lx-l->x)*(lx-l->x)+(ly-l->y)*(ly-l->y);
			float dist;
			if( d->distpow == 1.0 )
				dist = distsq;
			else if( d->distpow == 2.0 )
				dist = sqrtf(distsq);
			else
				dist = powf(distsq,1.0);

			float match = l->ledexp / dist;
			if( match > bestmatchval ) 
			{
				bestmatch = i;
				bestmatchval = match;
			}
		}

		if( bestmatch != -1 )
		{
			float sat = nf->note_amplitudes_out[bestmatch] * d->satamp;
			if( sat > 1.0 ) sat = 1.0;
			CNFGColor ( CCtoHEX( nf->note_positions[bestmatch] / nf->freqbins, 1.0, sat ) );
		}
		else
		{
			CNFGColor ( 0 );
		}
		CNFGTackRectangle( kx, ky, kx+cw, ky+ch );

	}
	CNFGColor ( 0xffffff );
}

static void DPOParams(void * id )
{
	struct DPODriver * d = (struct DPODriver*)id;
	d->xn = 160;		RegisterValue( "lightx", PINT, &d->xn, sizeof( d->xn ) ); printf( "XN: %d\n", d->xn );
	d->yn = 90;			RegisterValue( "lighty", PINT, &d->yn, sizeof( d->yn ) );
	d->cutoff = .01; 	RegisterValue( "shape_cutoff", PFLOAT, &d->cutoff, sizeof( d->cutoff ) );
	d->satamp = 5;		RegisterValue( "satamp", PFLOAT, &d->satamp, sizeof( d->satamp ) );

	d->amppow = 2.51;	RegisterValue( "amppow", PFLOAT, &d->amppow, sizeof( d->amppow ) );
	d->distpow = 1.5;	RegisterValue( "distpow", PFLOAT, &d->distpow, sizeof( d->distpow ) );

	d->from_sides = 1.5;RegisterValue( "fromsides", PINT, &d->from_sides, sizeof( d->from_sides ) );

	d->note_peaks = 0;
}

static struct DriverInstances * DisplayShapeDriver(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct DPODriver * d = ret->id = malloc( sizeof( struct DPODriver ) );
	memset( d, 0, sizeof( struct DPODriver ) );
	ret->Func = DPOUpdate;
	ret->Params = DPOParams;
	DPOParams( d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplayShapeDriver);


