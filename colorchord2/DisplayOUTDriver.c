//Copyright 2015 <>< Charles Lohr under the ColorChord License.

//XXX This needs to be re-worked to only output LEDs so DisplayArray can take it.
//XXX CONSIDER DUMPING

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "outdrivers.h"
#include "DrawFunctions.h"

#include "color.h"

//Uses: note_amplitudes2[note] for how many lights to use.
//Uses: note_amplitudes_out[note] for how bright it should be.

#define MAX_LEDS_PER_NOTE 1024

extern short screenx, screeny;

struct LINote
{
	float ledexp;
	float lednrm;  //How many LEDs should we have?
	int nrleds;    //How many LEDs do we have?
	int ledids[MAX_LEDS_PER_NOTE];
};

struct DPODriver
{
	int xn;
	int yn;
	float cutoff;
	float satamp;
	float pow;

	int note_peaks;
	struct LINote * notes;
	int numallocleds;
	int unallocleds[MAX_LEDS_PER_NOTE];
};

int GetAnLED( struct DPODriver * d )
{
	if( d->numallocleds )
	{
		return d->unallocleds[--d->numallocleds];
	}

	int i;
	//Okay, now we have to go search for one.
	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * n = &d->notes[i];
		if( n->lednrm < n->nrleds && n->nrleds > 0 )
		{
			return n->ledids[--n->nrleds];
		}
	}
	return -1;
}

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

		d->numallocleds = tleds;
		for( i = 0; i < d->numallocleds; i++ )
			d->unallocleds[i] = i;

	}


	float totalexp = 0;

	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * l = &d->notes[i];
		l->ledexp = powf( nf->note_amplitudes2[i], d->pow ) - d->cutoff;
		if( l->ledexp < 0 ) l->ledexp = 0;
		totalexp += l->ledexp;
	}

	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * l = &d->notes[i];
		l->lednrm = l->ledexp * tleds / totalexp;
	}

	//Relinquish LEDs
	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * l = &d->notes[i];
		while( l->lednrm < l->nrleds && l->nrleds > 0 )
		{
			d->unallocleds[d->numallocleds++] = l->ledids[--l->nrleds];
		}
	}

	//Add LEDs
	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * l = &d->notes[i];
		while( l->lednrm > l->nrleds )
		{
			int led = GetAnLED( d );
			if( led < 0 ) break;
			l->ledids[l->nrleds++] = led;
		}
	}

/*	float cw = ((float)screenx) / d->xn;
	float ch = ((float)screeny) / d->yn;

	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * l = &d->notes[i];
		int j;
		float sat = nf->note_amplitudes_out[i] * d->satamp;
		if( sat > 1 ) sat = 1;
		CNFGDialogColor = CCtoHEX( nf->note_positions[i] / nf->freqbins, 1.0, sat );
		for( j = 0; j < l->nrleds; j++ )
		{
			int id = l->ledids[j];
			float x = (id % d->xn) * cw;
			float y = (id / d->xn) * ch;

			CNFGDrawBox( x, y, x+cw, y+ch );
		}
	}*/

	int led = 0;
	for( i = 0; i < d->note_peaks; i++ )
	{
		struct LINote * l = &d->notes[i];
		int j;
		float sat = nf->note_amplitudes_out[i] * d->satamp;
		if( sat > 1 ) sat = 1;
		uint32_t color = CCtoHEX( nf->note_positions[i] / nf->freqbins, 1.0, sat );

		OutLEDs[led*3+0] = color & 0xff;
		OutLEDs[led*3+1] = ( color >> 8 ) & 0xff;
		OutLEDs[led*3+2] = ( color >> 16 ) & 0xff;
		led++;

	}
	
}

static void DPOParams(void * id )
{
	struct DPODriver * d = (struct DPODriver*)id;
	d->xn = GetParameterI( "lightx", 16 );
	d->yn = GetParameterI( "lighty", 9 );
	d->cutoff = GetParameterF( "cutoff", .05 );
	d->satamp = GetParameterF( "satamp", 3 );
	d->pow = GetParameterF( "pow", 2.1 );
	d->note_peaks = 0;
	if( d->xn * d->yn >= MAX_LEDS_PER_NOTE )
	{
		fprintf( stderr, "ERROR: Cannot have more lights than %d\n", MAX_LEDS_PER_NOTE );
		exit (-1);
	}
}

static struct DriverInstances * DisplayOutDriver(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct DPODriver * d = ret->id = malloc( sizeof( struct DPODriver ) );
	memset( d, 0, sizeof( struct DPODriver ) );
	ret->Func = DPOUpdate;
	ret->Params = DPOParams;
	DPOParams( d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplayOutDriver);


