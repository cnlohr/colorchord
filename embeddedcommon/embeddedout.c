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
		extern uint8_t gCOLORCHORD_SHIFT_INTERVAL; // controls speed of shifting if 0 no shift
		extern uint8_t gCOLORCHORD_FLIP_ON_PEAK; //if non-zero gives flipping at peaks of shift direction, 0 no flip
		extern int8_t gCOLORCHORD_SHIFT_DISTANCE; //distance of shift
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
	int8_t shift_dist = 0;
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
	uint16_t  local_peak_freq[MAXNOTES]; // using 16 bit so can easily choose amps, amps2 or freq as key
	uint8_t  local_note_jumped_to[MAXNOTES];
#if SORT_NOTES
	// can sort based on freq, amps etc - could mod to switch on a parameter
	uint16_t  *sort_key;
	uint16_t  *non_key[2];
	switch(3) {
		case 1  :
			sort_key = local_peak_amps2;
			non_key[0] = local_peak_amps;
			non_key[1] = local_peak_freq;
		break;
		case 2  :
			sort_key = local_peak_amps;
			non_key[0] = local_peak_amps2;
			non_key[1] = local_peak_freq;
		break;
		case 3  :
			sort_key = local_peak_freq;
			non_key[0] = local_peak_amps;
			non_key[1] = local_peak_amps2;
		break;
	}
#endif
	//Make a copy of all of the variables into local ones so we don't have to keep double-dereferencing.
	for( i = 0; i < sorted_map_count; i++ )
	{
		local_peak_amps[i] = note_peak_amps[sorted_note_map[i]] - note_nerf_a;
		local_peak_amps2[i] = note_peak_amps2[sorted_note_map[i]];
		local_peak_freq[i] = (uint16_t) note_peak_freqs[sorted_note_map[i]];
		local_note_jumped_to[i] = note_jumped_to[sorted_note_map[i]];
	}


#if SORT_NOTES
	// note local_note_jumped_to still give original indices of notes (which may not even been inclued
	//    due to being eliminated as too small amplitude
	// TODO bubble sort on key and reorder sorted_note_map
	for( i = 0; i < sorted_map_count - 1; i++ )
	{
		for( j = i + 1; j < sorted_map_count; j++ )
		{
			if (*(sort_key+i) < *(sort_key+j)) // dec sort
			{
				hold16 = *(sort_key+j);
				*(sort_key+j) = *(sort_key+i);
				*(sort_key+i) = hold16;
				hold16 = *(non_key[0]+j);
				*(non_key[0]+j) = *(non_key[0]+i);
				*(non_key[0]+i) = hold16;
				hold16 = *(non_key[1]+j);
				*(non_key[1]+j) = *(non_key[1]+i);
				*(non_key[1]+i) = hold16;
				hold8 = sorted_note_map[j];
				sorted_note_map[j] = sorted_note_map[i];
				sorted_note_map[i] = hold8;
				hold8 = local_note_jumped_to[j];
				local_note_jumped_to[j] = local_note_jumped_to[i];
				local_note_jumped_to[i] = hold8;
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
	//printf("spin: %d, min deviation: %d\n", ledSpin, mqty);

#else
	ledSpin = 0;
#endif
	// if option change direction on max peaks of total amplitude
	if (gCOLORCHORD_FLIP_ON_PEAK ) {
		if (diff_a_prev < 0 && diff_a > 0) {
			rot_dir *= -1;
			shift_dist = rot_dir * gCOLORCHORD_SHIFT_DISTANCE;
		}
	} else shift_dist = gCOLORCHORD_SHIFT_DISTANCE;

        // want possible extra spin to relate to changes peak intensity
	// now every gCOLORCHORD_SHIFT_INTERVAL th frame
	if (gCOLORCHORD_SHIFT_INTERVAL != 0) {
		//NOTE need - in front of rot_dir to make rotation direction consistent with UpdateRotationLEDs
		jshift = (ledSpin - shift_dist * framecount/gCOLORCHORD_SHIFT_INTERVAL ) % USE_NUM_LIN_LEDS; // neg % pos is neg so fix with
		if ( jshift < 0 ) jshift += USE_NUM_LIN_LEDS;
	        //printf("tnap tna %d %d dap da %d %d rot_dir %d, j shift %d\n",total_note_a_prev, total_note_a, diff_a_prev,  diff_a, rot_dir, j);
	} else {
		jshift = ledSpin;
	}

#if DEBUGPRINT
	printf("rot_dir %d, shift_dist %d, jshift %d\n", rot_dir, shift_dist, jshift);
	printf("leds: ");
#endif
	for( l = 0; l < USE_NUM_LIN_LEDS; l++, jshift++ )
	{
		if( jshift >= USE_NUM_LIN_LEDS ) jshift = 0;
//note:  lefFreqOutOld used only if wraparound
#if LIN_WRAPAROUND
		if( ledSpin >= USE_NUM_LIN_LEDS ) ledSpin = 0;
		ledFreqOutOld[l] = ledFreqOut[ledSpin];
		ledSpin++;
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
	amp = (((uint32_t)(amp))*NOTE_FINAL_AMP)>>10;

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
	int8_t shift_dist;
	uint8_t freq = 0;
	uint16_t amp = 0;
	uint16_t amp2 = 0;
	uint32_t note_nerf_a = 0;
	uint32_t total_note_a = 0;
	//uint32_t total_note_a2 = 0;
	int diff_a = 0;
	int32_t led_arc_len;

	for( i = 0; i < MAXNOTES; i++ )
	{
		uint16_t ist = note_peak_amps2[i];
		uint8_t ifrq = note_peak_freqs[i];
		if( ifrq != 255 )
		{
			if( ist > amp2 ) {
				freq = ifrq;
				amp2 = ist;
			}
			total_note_a += note_peak_amps[i];
			//total_note_a2 += ist;
		}
	}

	diff_a = total_note_a_prev - total_note_a;

	// can set color intensity using amp2
	amp = (((uint32_t)(amp2))*NOTE_FINAL_AMP)>>10;
	if( amp > 255 ) amp = 255;
	//uint32_t color = ECCtoHEX( (freq+RootNoteOffset)%NOTERANGE, 255, amp );
	uint32_t color = ECCtoHEX( (freq+RootNoteOffset)%NOTERANGE, 255, 255 );

	// can have led_arc_len a fixed size or proportional to amp2
	//led_arc_len = 5;
	led_arc_len = (amp * USE_NUM_LIN_LEDS) >> 8;
	//printf("test %d %d \n", amp2, led_arc_len);

        // want possible extra spin to relate to changes peak intensity
	if (gCOLORCHORD_FLIP_ON_PEAK ) {
		if (diff_a_prev < 0 && diff_a > 0) {
			rot_dir *= -1;
			shift_dist = rot_dir * gCOLORCHORD_SHIFT_DISTANCE;
		}
	} else shift_dist = gCOLORCHORD_SHIFT_DISTANCE;

	// now every gCOLORCHORD_SHIFT_INTERVAL th frame
	if (gCOLORCHORD_SHIFT_INTERVAL != 0) {
		// On each gCOLORCHORD_SHIFT_INTERVAL/rot_dir-th frame make rotational shift
		jshift = (shift_dist * framecount/gCOLORCHORD_SHIFT_INTERVAL - led_arc_len/2) % USE_NUM_LIN_LEDS; // neg % pos is neg so fix
		if( jshift < 0 ) jshift +=  USE_NUM_LIN_LEDS;
		//printf("tnap tna %d %d dap da %d %d rot_dir %d, jshift %d \n",total_note_a_prev, total_note_a, diff_a_prev,  diff_a, rot_dir, jshift);
	} else {
		jshift = (0 - led_arc_len/2) % USE_NUM_LIN_LEDS; // neg % pos is neg so fix
		if( jshift < 0 ) jshift +=  USE_NUM_LIN_LEDS;
	}

	for( i = 0; i < led_arc_len; i++, jshift++ )
	{
		// even if led_arc_len exceeds USE+NUM_LIN_LEDS using jshift will prevent over running ledOut
		if( jshift >= USE_NUM_LIN_LEDS ) jshift = 0;
		ledOut[jshift*3+0] = ( color >> 0 ) & 0xff;
		ledOut[jshift*3+1] = ( color >> 8 ) & 0xff;
		ledOut[jshift*3+2] = ( color >>16 ) & 0xff;
	}

	for( i = led_arc_len; i < USE_NUM_LIN_LEDS; i++, jshift++ )
	{
		if( jshift >= USE_NUM_LIN_LEDS ) jshift = 0;
		ledOut[jshift*3+0] = 0x0;
		ledOut[jshift*3+1] = 0x0;
		ledOut[jshift*3+2] = 0x0;
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

