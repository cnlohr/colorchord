//Copyright 2015 <>< Charles Lohr under the ColorChord License.


#include "embeddedout.h"

//uint8_t ledArray[NUM_LIN_LEDS]; //Points to which notes correspond to these LEDs
uint8_t ledOut[NUM_LIN_LEDS*3];

uint16_t ledSpin;
uint16_t ledAmpOut[NUM_LIN_LEDS];
uint8_t ledFreqOut[NUM_LIN_LEDS];
uint8_t ledFreqOutOld[NUM_LIN_LEDS];

uint8_t RootNoteOffset;
uint32_t total_note_a_prev = 0;
int diff_a_prev = 0;
int rot_dir = 1; // initial rotation direction 1

void UpdateLinearLEDs()
{
	//Source material:
	/*
		extern uint8_t  note_peak_freqs[];
		extern uint16_t note_peak_amps[];  //[MAXNOTES] 
		extern uint16_t note_peak_amps2[];  //[MAXNOTES]  (Responds quicker)
		extern uint8_t  note_jumped_to[]; //[MAXNOTES] When a note combines into another one,
		extern uint8_t gCOLORCHORD_ADVANCE_SHIFT; // controls speed of shifting if 0 no shift
		extern int8_t gCOLORCHORD_SUPRESS_FLIP_DIR; //if non-zero will cause flipping shift on peaks, also controls speed
	*/

	//Goal: Make splotches of light that are porportional to the strength of notes.
	//Color them according to value in note_peak_amps2.

	uint8_t i;
	int8_t k;
	uint16_t j, l;
	uint32_t total_size_all_notes = 0;
	int32_t porpamps[MAXNOTES]; //LEDs for each corresponding note.
	uint8_t sorted_note_map[MAXNOTES]; //mapping from which note into the array of notes from the rest of the system.
	uint8_t sorted_map_count = 0;
	uint32_t note_nerf_a = 0;
	uint32_t total_note_a = 0;
	int diff_a = 0;
	int8_t jshift;

#if DEBUGPRINT
	printf( "Note Peak Freq: " );
        for( i = 0; i < MAXNOTES; i++ ) printf( " %5d /", note_peak_freqs[i]  );
	printf( "\n" );
        printf( "Note Peak Amps: " );
        for( i = 0; i < MAXNOTES; i++ ) printf( " %5d /", note_peak_amps[i]  );
	printf( "\n" );
        printf( "Note Peak Amp2: " );
        for( i = 0; i < MAXNOTES; i++ ) printf( " %5d /", note_peak_amps2[i]  );
	printf( "\n" );
        printf( "Note jumped to: " );
        for( i = 0; i < MAXNOTES; i++ ) printf( " %5d /", note_jumped_to[i]  );
	printf( "\n" );
#endif

	for( i = 0; i < MAXNOTES; i++ )
	{
		if( note_peak_freqs[i] == 255 ) continue;
		total_note_a += note_peak_amps[i];
	}

	diff_a = total_note_a_prev - total_note_a;

	note_nerf_a = ((total_note_a * NERF_NOTE_PORP)>>8);

	// ignore notes with amp too small or freq 255
	for( i = 0; i < MAXNOTES; i++ )
	{
		uint16_t ist = note_peak_amps[i];
		uint8_t nff = note_peak_freqs[i];
		if( nff == 255 )
		{
			continue;
		}
		if( ist < note_nerf_a )
		{
			continue;
		}
		sorted_note_map[sorted_map_count] = i;
		sorted_map_count++;
	}



	uint16_t local_peak_amps[MAXNOTES];
	uint16_t local_peak_amps2[MAXNOTES];
	uint16_t hold16;
	uint8_t hold8;
	uint8_t  local_peak_freq[MAXNOTES];
	uint8_t  local_note_jumped_to[MAXNOTES];
#if SORT_NOTES
	// can sort based on freq, amps etc - could mod to switch on a parameter
	uint16_t  *sort_key; // leaving untyped so can point to 16 or 8 bit, but may not best way
	switch(1) {
		case 1  :
			sort_key = local_peak_amps2;
		break;
		case 2  :
			sort_key = local_peak_amps;
		break;
		case 3  :
			sort_key = local_peak_freq;
		break;
	}
#endif

	//Make a copy of all of the variables into local ones so we don't have to keep double-dereferencing.
	//set sort key
	for( i = 0; i < sorted_map_count; i++ )
	{
		local_peak_amps[i] = note_peak_amps[sorted_note_map[i]] - note_nerf_a;
		local_peak_amps2[i] = note_peak_amps2[sorted_note_map[i]];
		local_peak_freq[i] = note_peak_freqs[sorted_note_map[i]];
		local_note_jumped_to[i] = note_jumped_to[sorted_note_map[i]];
	}

#if SORT_NOTES
	// note local_note_jumped_to still give original indices of notes (which may not even been inclued
	//    due to being eliminated as too small amplitude
	for( i = 0; i < sorted_map_count - 1; i++ )
	{
		for( j = i + 1; j < sorted_map_count; j++ )
		{
			//if (local_peak_freq[i] > local_peak_freq[j]) // inc sort on freq
			if (*(sort_key+i) < *(sort_key+j)) // dec sort
			{
				hold8 = local_peak_freq[j];
				local_peak_freq[j] = local_peak_freq[i];
				local_peak_freq[i] = hold8;
				hold8 = sorted_note_map[j];
				sorted_note_map[j] = sorted_note_map[i];
				sorted_note_map[i] = hold8;
				hold8 = local_note_jumped_to[j];
				local_note_jumped_to[j] = local_note_jumped_to[i];
				local_note_jumped_to[i] = hold8;
				hold16 = local_peak_amps[j];
				local_peak_amps[j] = local_peak_amps[i];
				local_peak_amps[i] = hold16;
				hold16 = local_peak_amps2[j];
				local_peak_amps2[j] = local_peak_amps2[i];
				local_peak_amps2[i] = hold16;
			}
		}
	}
#endif

	for( i = 0; i < sorted_map_count; i++ )
	{
		uint16_t ist = local_peak_amps[i];
		porpamps[i] = 0;
		total_size_all_notes += local_peak_amps[i];
	}

	if( total_size_all_notes == 0 )
	{
		for( j = 0; j < USE_NUM_LIN_LEDS * 3; j++ )
		{
			ledOut[j] = 0;
		}
		return;
	}

	uint32_t porportional = (uint32_t)(USE_NUM_LIN_LEDS<<16)/((uint32_t)total_size_all_notes);

	uint16_t total_accounted_leds = 0;

	for( i = 0; i < sorted_map_count; i++ )
	{
		porpamps[i] = (local_peak_amps[i] * porportional) >> 16;
		total_accounted_leds += porpamps[i];
	}

	int16_t total_unaccounted_leds = USE_NUM_LIN_LEDS - total_accounted_leds;

#if DEBUGPRINT
	printf( "note_nerf_a = %d,  total_size_all_notes =  %d, porportional = %d, total_accounted_leds = %d \n", note_nerf_a, total_size_all_notes, porportional,  total_accounted_leds );
	printf("snm: ");
        for( i = 0; i < sorted_map_count; i++ )  printf( "%d /", sorted_note_map[i]);
	printf( "\n" );

	printf("npf: ");
        for( i = 0; i < sorted_map_count; i++ )  printf( "%d /", note_peak_freqs[sorted_note_map[i]]);
	printf( "\n" );

	printf("lpf: ");
        for( i = 0; i < sorted_map_count; i++ )  printf( "%d /", local_peak_freq[i]);
	printf( "\n" );

	printf("npa: ");
        for( i = 0; i < sorted_map_count; i++ )  printf( "%d /", note_peak_amps[sorted_note_map[i]]);
	printf( "\n" );

	printf("lpa: ");
        for( i = 0; i < sorted_map_count; i++ )  printf( "%d /", local_peak_amps[i]);
	printf( "\n" );

	printf("lpa2: ");
        for( i = 0; i < sorted_map_count; i++ )  printf( "%d /", local_peak_amps2[i]);
	printf( "\n" );

	printf("porp: ");
        for( i = 0; i < sorted_map_count; i++ )  printf( "%d /", porpamps[i]);
	printf( "\n" );

	printf("lnjt: ");
        for( i = 0; i < sorted_map_count; i++ )  printf( "%d /", local_note_jumped_to[i]);
	printf( "\n" );

#endif


	int addedlast = 1;
	do
	{
		for( i = 0; i < sorted_map_count && total_unaccounted_leds; i++ )
		{
			porpamps[i]++; total_unaccounted_leds--;
			addedlast = 1;
		}
	} while( addedlast && total_unaccounted_leds );

	//Put the frequencies on a ring.
	j = 0;
	for( i = 0; i < sorted_map_count; i++ )
	{
		while( porpamps[i] > 0 )
		{
			ledFreqOut[j] = local_peak_freq[i];
			ledAmpOut[j] = (local_peak_amps2[i]*NOTE_FINAL_AMP)>>8;
			j++;
			porpamps[i]--;
		}
	}

	//This part totally can't run on an embedded system.
#if LIN_WRAPAROUND
        //printf("NOTERANGE: %d ", NOTERANGE); //192
	// bb this code finds an index ledSpin so that shifting the led display will have the minimum deviation
	//    from the previous display.
	//  this will cancel out shifting effects below
	uint16_t midx = 0;
	uint32_t mqty = 100000000;
	for( j = 0; j < USE_NUM_LIN_LEDS; j++ )
	{
		uint32_t dqty;
		uint16_t localj;

		dqty = 0;
		localj = j;
		for( l = 0; l < USE_NUM_LIN_LEDS; l++ )
		{
			int32_t d = (int32_t)ledFreqOut[localj] - (int32_t)ledFreqOutOld[l];
			if( d < 0 ) d *= -1;
			if( d > (NOTERANGE>>1) ) { d = NOTERANGE - d + 1; }
			dqty += ( d * d );

			localj++;
			if( localj == USE_NUM_LIN_LEDS ) localj = 0;
		}

		if( dqty < mqty )
		{
			mqty = dqty;
			midx = j;
		}
	}

	ledSpin = midx;
	printf("spin: %d, min deviation: %d\n", ledSpin, mqty); // bb

#else
	ledSpin = 0;
#endif
//TODO FIX if change gCOLORCHORD_SUPRESS_FLIP_DIR from non-zero to zero rot_dir will not reinitialize to 1
	// if option change direction on max peaks of total amplitude
	if (gCOLORCHORD_SUPRESS_FLIP_DIR == 0) {
		if (diff_a_prev < 0 && diff_a > 0) rot_dir *= -1;
	} else rot_dir = gCOLORCHORD_SUPRESS_FLIP_DIR;

        // want possible extra spin to relate to changes peak intensity
	// now every gCOLORCHORD_ADVANCE_SHIFT th frame
	if (gCOLORCHORD_ADVANCE_SHIFT != 0) {
//NOTE need - in front of rot_dir to make rotation direction consistent with UpdateRotationLEDs
		jshift = (ledSpin - rot_dir * framecount/gCOLORCHORD_ADVANCE_SHIFT ) % USE_NUM_LIN_LEDS; // neg % pos is neg so fix with
		if ( jshift < 0 ) jshift += USE_NUM_LIN_LEDS;
	        //printf("tnap tna %d %d dap da %d %d rot_dir %d, j shift %d\n",total_note_a_prev, total_note_a, diff_a_prev,  diff_a, rot_dir, j);
	} else {
		jshift = ledSpin;
	}

#if DEBUGPRINT
	printf("rot_dir %d, jshift %d\n", rot_dir, jshift);
	printf("leds: ");
#endif
	for( l = 0; l < USE_NUM_LIN_LEDS; l++, jshift++ )
	{
		if( jshift >= USE_NUM_LIN_LEDS ) j = 0;
// bb note:  lefFreqOutOld used only if wraparound
#if LIN_WRAPAROUND
		ledFreqOutOld[l] = ledFreqOut[jshift];
#endif
		uint16_t amp = ledAmpOut[jshift];
#if DEBUGPRINT
	        printf("%d:%d/", ledFreqOut[jshift], amp);
#endif
		if( amp > 255 ) amp = 255;
		uint32_t color = ECCtoHEX( (ledFreqOut[jshift]+RootNoteOffset)%NOTERANGE, 255, amp );
		ledOut[l*3+0] = ( color >> 0 ) & 0xff;
		ledOut[l*3+1] = ( color >> 8 ) & 0xff;
		ledOut[l*3+2] = ( color >>16 ) & 0xff;
	}
#if DEBUGPRINT
        printf( "\n" );
	printf("bytes: ");
        for( i = 0; i < USE_NUM_LIN_LEDS; i++ )  printf( "%02x%02x%02x-", ledOut[i*3+0], ledOut[i*3+1],ledOut[i*3+2]);
	printf( "\n\n" );
#endif
	total_note_a_prev = total_note_a;
	diff_a_prev = diff_a;
}

void UpdateAllSameLEDs()
{
	int i;
	int8_t j;
	uint8_t freq = 0;
	uint16_t amp = 0;


	for( i = 0; i < MAXNOTES; i++ )
	{
		uint16_t ist = note_peak_amps2[i];
		uint8_t ifrq = note_peak_freqs[i];
		if( ifrq != 255 && ist > amp  )
		{
			freq = ifrq;
			amp = ist;
		}
	}
	amp = (((uint32_t)(amp))*NOTE_FINAL_AMP)>>8; //bb was 10

	if( amp > 255 ) amp = 255;
	uint32_t color = ECCtoHEX( (freq+RootNoteOffset)%NOTERANGE, 255, amp );
	for( i = 0; i < USE_NUM_LIN_LEDS; i++ )
	{
		ledOut[i*3+0] = ( color >> 0 ) & 0xff;
		ledOut[i*3+1] = ( color >> 8 ) & 0xff;
		ledOut[i*3+2] = ( color >>16 ) & 0xff;
	}
}

void UpdateRotatingLEDs()
{
	int i;
	int8_t jshift;
	uint8_t freq = 0;
	uint16_t amp = 0;
	uint32_t note_nerf_a = 0;
	uint32_t total_note_a = 0;
	int diff_a = 0;

	for( i = 0; i < MAXNOTES; i++ )
	{
		uint16_t ist = note_peak_amps2[i];
		uint8_t ifrq = note_peak_freqs[i];
		if( ifrq != 255 )
		{
			if( ist > amp ) {
				freq = ifrq;
				amp = ist;
			}
			total_note_a += note_peak_amps[i];
		}
	}
	diff_a = total_note_a_prev - total_note_a;
	amp = (((uint32_t)(amp))*NOTE_FINAL_AMP)>>8; //bb was 10

	if( amp > 255 ) amp = 255;
	uint32_t color = ECCtoHEX( (freq+RootNoteOffset)%NOTERANGE, 255, amp );
	// if option change direction on max peaks of total amplitude


	if (gCOLORCHORD_SUPRESS_FLIP_DIR == 0) {
		if (diff_a_prev < 0 && diff_a > 0) rot_dir *= -1;
	} else rot_dir = gCOLORCHORD_SUPRESS_FLIP_DIR;

	if (gCOLORCHORD_ADVANCE_SHIFT != 0) {
		// now every gCOLORCHORD_ADVANCE_SHIFT/rot_dir th frame
		jshift = ((rot_dir)*framecount/gCOLORCHORD_ADVANCE_SHIFT) % USE_NUM_LIN_LEDS; // neg % pos is neg so fix
		if( jshift < 0 ) jshift +=  USE_NUM_LIN_LEDS;
		//printf("tnap tna %d %d dap da %d %d rot_dir %d, jshift %d \n",total_note_a_prev, total_note_a, diff_a_prev,  diff_a, rot_dir, jshift);
	} else {q
		jshift = 0;
	}

	//for( i = 0; i < USE_NUM_LIN_LEDS; i++ )
	for( i = 0; i < 1; i++, jshift++ )
	{
		if( jshift >= USE_NUM_LIN_LEDS ) jshift = 0;
		ledOut[jshift*3+0] = ( color >> 0 ) & 0xff;
		ledOut[jshift*3+1] = ( color >> 8 ) & 0xff;
		ledOut[jshift*3+2] = ( color >>16 ) & 0xff;
	}
	total_note_a_prev = total_note_a;
	diff_a_prev = diff_a;

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

