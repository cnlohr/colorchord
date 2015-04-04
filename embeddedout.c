#include "embeddedout.h"

//uint8_t ledArray[NUM_LIN_LEDS]; //Points to which notes correspond to these LEDs
uint8_t ledOut[NUM_LIN_LEDS*3];

void UpdateLinearLEDs()
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
	int32_t porpamps[MAXNOTES]; //LEDs for each corresponding note.

	uint8_t sorted_note_map[MAXNOTES]; //mapping from which note into the array of notes from the rest of the system.

	uint16_t local_peak_amps[MAXNOTES];
	uint16_t local_peak_amps2[MAXNOTES];
	uint8_t  local_note_freq[MAXNOTES];

	uint8_t sorted_map_count = 0;

	for( i = 0; i < MAXNOTES; i++ )
	{
		uint16_t ist = note_peak_amps[i];
		if( note_peak_freqs[i] == 255 || ist <= NERF_NOTE_SIZE_VALUE )
		{
			local_peak_amps[i] = 0;
			continue;
		}

		for( j = 0; j < sorted_map_count; j++ )
		{
			//TODO SORT ME
		}
		sorted_map_count++;
	}

	for( i = 0; i < MAXNOTES; i++ )
	{
		uint16_t ist = note_peak_amps[i];
		porpamps[i] = 0;
		if( note_peak_freqs[i] == 255 || ist <= NERF_NOTE_SIZE_VALUE )
		{
			local_peak_amps[i] = 0;
			continue;
		}
		local_peak_amps[i] = ist - NERF_NOTE_SIZE_VALUE;
		total_size_all_notes += local_peak_amps[i];
	}

	if( total_size_all_notes == 0 )
	{
		for( j = 0; j < NUM_LIN_LEDS * 3; j++ )
		{
			ledOut[j] = 0;
		}
		return;
	}

	uint32_t porportional = (uint32_t)(NUM_LIN_LEDS<<8)/((uint32_t)total_size_all_notes);
	uint16_t total_accounted_leds = 0;

	for( i = 0; i < MAXNOTES; i++ )
	{
		if( note_peak_freqs[i] == 255 ) continue;
		porpamps[i] = (local_peak_amps[i] * porportional) >> 8;
		total_accounted_leds += porpamps[i];
	}

	int16_t total_unaccounted_leds = NUM_LIN_LEDS - total_accounted_leds;

	for( i = 0; i < MAXNOTES && total_unaccounted_leds; i++ )
	{
		if( note_peak_freqs[i] == 255 ) continue;
		porpamps[i]++; total_unaccounted_leds--;
	}

	//Now, we use porpamps to march through the LEDs, coloring them.
	j = 0;
	for( i = 0; i < MAXNOTES; i++ )
	{
		while( porpamps[i] > 0 )
		{
			uint16_t amp = ((uint32_t)note_peak_amps2[i] * NOTE_FINAL_AMP) >> 8;
			if( amp > 255 ) amp = 255;
			uint32_t color = ECCtoHEX( note_peak_freqs[i], 255, amp );
			ledOut[j*3+0] = ( color >> 0 ) & 0xff;
			ledOut[j*3+1] = ( color >> 8 ) & 0xff;
			ledOut[j*3+2] = ( color >>16 ) & 0xff;

			j++;
			porpamps[i]--;
		}
	}
}


uint32_t ECCtoHEX( uint8_t note, uint8_t sat, uint8_t val )
{
	uint16_t hue = 0;
	uint16_t third = 65535/3;
	uint16_t scalednote = note;
	uint32_t renote = ((uint32_t)note * 65536) / NOTERANGE;

	//Note is expected to be a vale from 0..(NOTERANGE-1)
	//renote goes from 0 to the next one under 65536.


	if( renote < third )
	{
		//Yellow to Red.
		hue = (third - renote) >> 1;
	}
	else if( renote < (third<<1) )
	{
		//Red to Blue
		hue = (third-renote);
	}
	else
	{
		//hue = ((((65535-renote)>>8) * (uint32_t)(third>>8)) >> 1) + (third<<1);
		hue = (uint16_t)(((uint32_t)(65536-renote)<<16) / (third<<1)) + (third>>1); // ((((65535-renote)>>8) * (uint32_t)(third>>8)) >> 1) + (third<<1);
	}
	hue >>= 8;
//	printf( "%d;", hue );

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

	hue -= SIXTH1; //Off by 60 degrees.

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
	og = og * rs + 255 * (256-rs);
	ob = ob * rs + 255 * (256-rs);
//printf( "__%d %d %d =-> %d\n", or, og, ob, rs );

	or >>= 8;
	og >>= 8;
	ob >>= 8;

	return or | (og<<8) | ((uint32_t)ob<<16);
}
/*
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
*/
