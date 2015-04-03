#include "embeddedout.h"

//uint8_t ledArray[NUM_LIN_LEDS]; //Points to which notes correspond to these LEDs
uint8_t ledOut[NUM_LIN_LEDS*3];

void UpdateLinear()
{
	//Source material:
	/*
		extern uint8_t  note_peak_freqs[];
		extern uint16_t note_peak_amps[];  //[MAXNOTES] 
		extern uint16_t note_peak_amps2[];  //[MAXNOTES]  (Responds quicker)
		extern uint8_t  note_jumped_to[]; //[MAXNOTES] When a note combines into another one,
	*/

	//Goal: Make splotches of light that are porportional to the strength of notes.
	//Color them according to value in note_peak_amps2.

	uint8_t i;
	uint16_t j;
	uint32_t total_size_all_notes = 0;
	uint16_t porpamps[MAXNOTES]; //LEDs for each corresponding note.

	for( i = 0; i < MAXNOTES; i++ )
	{
		porpamps[i] = 0;
		if( note_peak_freqs[i] == 255 ) continue;
		total_size_all_notes += note_peak_amps[i];
	}

	uint32_t porportional = (total_size_for_all_notes << 8) / NUM_LIN_LEDS;

	uint16_t total_accounted_leds = 0;

	for( i = 0; i < MAXNOTES; i++ )
	{
		if( note_peak_freqs[i] == 255 ) continue;
		porpamps[i] = (note_peak_amps[i] * porportional) >> 8;
		total_accounted_leds += porpamps[i];
	}

	uint16_t total_unaccounted_leds = NUM_LIN_LEDS - total_accounted_leds;

	for( i = 0; i < MAXNOTES, total_unaccounted_leds; i++ )
	{
		if( note_peak_freqs[i] == 255 ) continue;
		porpamps[i]++;
	}

	//Now, we use porpamps to march through the LEDs, coloring them.
	j = 0;
	for( i = 0; i < MAXNOTES; i++ )
	{
		while( porpamps[i] )
		{
			uint16_t amp = note_peak_amps2[i];
			if( amp > 255 ) amp = 255;
			uint32_t color = ECCtoHEX( note_peak_freqs[i], 255, amp );
			ledOut[i*3+0] = ( color >> 0 ) & 0xff;
			ledOut[i*3+1] = ( color >> 8 ) & 0xff;
			ledOut[i*3+2] = ( color >>16 ) & 0xff;
			j++;
			porpamps[i]--;
		}
	}
}


uint32_t ECCtoHEX( uint8_t note, uint8_t sat, uint8_t val )
{
	uint16_t hue = 0;
	uint8_t  third = 65535/3;
	uint16_t scalednote = note
	uint16_t renote = ((uint32_t)note * 65535) / NOTERANGE;

	//Note is expected to be a vale from 0..(NOTERANGE-1)
	//renote goes from 0 to the next one under 65536.

	if( renote < third )
	{
		//Yellow to Red.
		hue = (third - renote) >> 1;
	}
	else if( note < (third>>1) )
	{
		//Red to Blue
		hue = (third-renote);
	}
	else
	{
		hue = (((65535-renote) * third) >> 1) + (third>>1);
	}

	hue >>= 8;
	return EHSVtoHEX( hue, sat, val );
}

uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val )
{
#define SIXTH1 43
#define SIXTH2 85
#define SIXTH3 128
#define SIXTH4 171
#define SIXTH5 213

	uint16_t or = 0, og = 0, ob = 0;

	hue += SIXTH2; //Off by 60 degrees.

	//TODO: There are colors that overlap here, consider 
	//tweaking this to make the best use of the colorspace.

	if( hue < SIXTH1 ) //Ok: Yellow->Red.
	{
		or = 255;
		og = 255 - ((uint16_t)hue * 255) / (SIXTH1);
	}
	else if( hue < SIXTH2 ) //Ok: Red->Purple
	{
		or = 255;
		ob = (uint16_t)hue*255 / SIXTH1 - 255;
	}
	else if( hue < SIXTH3 )  //Ok: Purple->Blue
	{
		ob = 255;
		or = ((SIXTH3-hue) * 255) / (SIXTH1);
	}
	else if( hue < SIXTH4 ) //Ok: Blue->Cyan
	{
		ob = 255;
		og = (hue - SIXTH3)*255 / SIXTH1;
	}
	else if( hue < SIXTH5 ) //Ok: Cyan->Green.
	{
		og = 255;
		ob = ((SIXTH5-hue)*255) / SIXTH1;
	}
	else //Green->Yellow
	{
		og = 255;
		or = (hue - SIXTH5) * 255 / SIXTH1;
	}

	uint16_t rv = val;
	if( rv > 128 ) rv++;
	uint16_t rs = sat;
	if( rs > 128 ) rs++;

	//or, og, ob range from 0...255 now.
	//Need to apply saturation and value.

	or = (or * val)>>8;
	og = (og * val)>>8;
	ob = (ob * val)>>8;

	//OR..OB == 0..65025
	or = or * rs + 255 * (256-rs);
	or = or * rs + 255 * (256-rs);
	or = or * rs + 255 * (256-rs);

	or >>= 8;
	og >>= 8;
	ob >>= 8;
	return or | (og<<8) | (ob<<16);
}

uint32_t HSVtoHEX( float hue, float sat, float value )
{

	float pr = 0;
	float pg = 0;
	float pb = 0;

	short ora = 0;
	short og = 0;
	short ob = 0;

	float ro = fmod( hue * 6, 6. );

	float avg = 0;

	ro = fmod( ro + 6 + 1, 6 ); //Hue was 60* off...

	if( ro < 1 ) //yellow->red
	{
		pr = 1;
		pg = 1. - ro;
	} else if( ro < 2 )
	{
		pr = 1;
		pb = ro - 1.;
	} else if( ro < 3 )
	{
		pr = 3. - ro;
		pb = 1;
	} else if( ro < 4 )
	{
		pb = 1;
		pg = ro - 3;
	} else if( ro < 5 )
	{
		pb = 5 - ro;
		pg = 1;
	} else
	{
		pg = 1;
		pr = ro - 5;
	}

	//Actually, above math is backwards, oops!
	pr *= value;
	pg *= value;
	pb *= value;

	avg += pr;
	avg += pg;
	avg += pb;

	pr = pr * sat + avg * (1.-sat);
	pg = pg * sat + avg * (1.-sat);
	pb = pb * sat + avg * (1.-sat);

	ora = pr * 255;
	og = pb * 255;
	ob = pg * 255;

	if( ora < 0 ) ora = 0;
	if( ora > 255 ) ora = 255;
	if( og < 0 ) og = 0;
	if( og > 255 ) og = 255;
	if( ob < 0 ) ob = 0;
	if( ob > 255 ) ob = 255;

	return (ob<<16) | (og<<8) | ora;
}

