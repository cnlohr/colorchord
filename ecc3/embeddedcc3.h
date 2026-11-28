#ifndef _EMBEDDEDCC3_H
#define _EMBEDDEDCC3_H

// TODO: Fix function names.
// TODO: Fix #define names.

// What if we do an IIR based falloff
// but, we were very careful about what that IIR window was.

// References:
//   [1] NC Method https://github.com/CaiB/ColorChord.NET/blob/master/Simulations/BinPeakLocation.m
//     (Originally) Kanatov, et. al https://www.mdpi.com/2079-9292/8/3/330
//   [2] Goertzels DFT https://github.com/cnlohr/cnlohr_tricks/#goertzels-dft
//   [3] Rotating output bins for NC-style collaboration between bins.
//		https://github.com/7ERr0r/sbsdft/blob/02b80708151af77cacfa4a8d2502139bd8ca93fe/src/spectrumapp/sbswdft.rs#L1474-L1496
//
// Useful tool I used:
//  https://audio.js.org/util/tone-generator/

#ifndef ECC3_BPERO
	#define ECC3_BPERO 24
#endif

#ifndef ECC3_OCTAVES
	#define ECC3_OCTAVES 6
#endif

#ifndef ECC3_FRAMETYPE
	#define ECC3_FRAMETYPE int16_t
#endif

#ifndef ECC3_MIN_AMP_FOR_NOTE
	#define ECC3_MIN_AMP_FOR_NOTE 80
#endif

#ifndef ECC3_MAXNOTES
	#define ECC3_MAXNOTES  12
#endif

#ifndef ECC3_MAX_JUMP_DISTANCE
	#define ECC3_MAX_JUMP_DISTANCE 4
#endif

#ifndef ECC3_AMP_1_IIR_BITS
	#define ECC3_AMP_1_IIR_BITS 3
#endif

#ifndef ECC3_AMP_2_IIR_BITS
	#define ECC3_AMP_2_IIR_BITS 1
#endif

#ifndef ECC3_MAX_COMBINE_DISTANCE
	#define ECC3_MAX_COMBINE_DISTANCE 7
#endif

#ifndef ECC3_MINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR
	#define ECC3_MINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR 256
#endif

#ifndef ECC3_NUM_LIN_LEDS
	#define ECC3_NUM_LIN_LEDS 64
#endif

#ifndef ECC3_USE_NUM_LIN_LEDS
	#define ECC3_USE_NUM_LIN_LEDS ECC3_NUM_LIN_LEDS
#endif

#ifndef ECC3_NERF_NOTE_PORP
	#define ECC3_NERF_NOTE_PORP 15
#endif

#ifndef ECC3_NOTE_FINAL_AMP
	#define ECC3_NOTE_FINAL_AMP  12   //Number from 0...255
#endif

#ifndef ECC3_SORT_NOTES
	#define ECC3_SORT_NOTES 0 // Only useful if you care to use the notes for "primary" "secondary" etc. color.
#endif

#ifndef ECC3_LIN_WRAPAROUND
	// this actually takes up a lot of resources and can be very slow, but is for things like a ring or loop of LEDs
	#define ECC3_LIN_WRAPAROUND 0
#endif

// Goal: Make all samples be approximately -4096 to 4095 - exceeding this can cause the frontend DFT to fail.
#ifndef ECC3_AUDIO_LEVEL
	#define ECC3_AUDIO_LEVEL(x) ((x)>>2)
#endif

#ifndef ECC3_BINS
#define ECC3_BINS ((ECC3_BPERO)*(ECC3_OCTAVES))
#endif

#define ECC3_SEMIBITSPERBIN 3 // 8 semitone levels.
#define ECC3_NOTERANGE ((1<<ECC3_SEMIBITSPERBIN)*ECC3_BPERO)

// Goertzels basically requires the coefficients to be 32-bits.
#ifndef ECC3_CONSTANTS_INCLUDED
	uint32_t ecc3_constants[ECC3_BINS];
	uint32_t ecc3_iirfalloff[ECC3_BINS];
	uint32_t ecc3_powerfalloff[ECC3_BINS];
	uint32_t ecc3_phasorsin[ECC3_BINS];
	uint32_t ecc3_phasorcos[ECC3_BINS];
	uint32_t ecc3_ncrotateC;
	uint32_t ecc3_ncrotateS;
#endif

typedef struct embeddedcc3_t
{
	int32_t spreva2[ECC3_BINS][2];

	// Internal state, that is not used between phases. 
	// These could live on the stack, but some users may find them
	// useful.
	uint32_t nc_values[ECC3_BINS];
	uint32_t folded_dft_nc[ECC3_BPERO];

	uint8_t  note_peak_freqs[ECC3_MAXNOTES];
	uint16_t note_peak_amps[ECC3_MAXNOTES];
	uint16_t note_peak_amps2[ECC3_MAXNOTES];
	uint8_t  note_jumped_to[ECC3_MAXNOTES];

	uint8_t ledOut[ECC3_NUM_LIN_LEDS*3];
	uint16_t ledAmpOut[ECC3_NUM_LIN_LEDS];
	uint8_t ledFreqOut[ECC3_NUM_LIN_LEDS];
	uint8_t ledFreqOutOld[ECC3_NUM_LIN_LEDS];

	uint16_t ledSpin;
	uint8_t RootNoteOffset;

} embeddedcc3;

static void ECC3Reset( embeddedcc3 * ecc ) { memset( ecc, 0, sizeof(*ecc) ); }
static void ECC3Update( embeddedcc3 * ecc, ECC3_FRAMETYPE * frames, int nrframes );
static void ECC3ComputeNotes( embeddedcc3 * ecc );

static void ECC3UpdateLinearLEDs(embeddedcc3 * ecc);
static void ECC3UpdateAllSameLEDs( embeddedcc3 * ecc );

// Also need "cells"


// Utility
static uint32_t ECC3toHEX( uint8_t note, uint8_t sat, uint8_t val );
static uint32_t ECC3HSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val );
static int ECC3_apsqrt( uint32_t i );

static void ECC3Update( embeddedcc3 * ecc, ECC3_FRAMETYPE * frames, int nrframes )
{
	int i;
	int32_t * sprevaptr = ecc->spreva2[0];
	for( i = 0; i < ECC3_BINS; i++ )
	{
		int32_t sprev = sprevaptr[0];
		int32_t sprev2 = sprevaptr[1];
		uint32_t coeff = ecc3_constants[i];
		uint32_t iircons = ecc3_iirfalloff[i];
		for( int j = 0; j < nrframes; j++ )
		{
			ECC3_FRAMETYPE v = ECC3_AUDIO_LEVEL(frames[j]);
			//
			// Update phasor. The way this is written, it turns into:
			//  mulhsu, mulhsu, slli, add, sub (RISC-V)
			// or
			//  (asr,umull,mla)x2, add, add, sub (ARMv7)
			// for ARM, we could re-formulate coefficients as int32_t, 
			//  but it would require more fixup.  It offers no benefit.
			//
			// Part 1: Actually sprev * coeff, *2 here because >>31
			//  would be a slower operation.  For floating-point
			//  formulations, do that here.
			//
			// Part 2: Traditionally, goertzels would use sprev2*1
			//  here over the window, but we are trying windowless.
			//  so we need to re-tune coefficient.
			int32_t s = 
				v 
				+ 2*((sprev *(uint64_t)coeff  )>>32)
				-   ((sprev2*(uint64_t)iircons)>>32);

			sprev2 = sprev;
			sprev = s;
		}
//printf( "%d %d\n", i, sprev );
		sprevaptr[0] = sprev;
		sprevaptr[1] = sprev2;
		sprevaptr += 2;
	}
}

static void ECC3ComputeNotes( embeddedcc3 * ecc )
{
	int32_t realimag[ECC3_BINS][2];

	int i;
	for( i = 0; i < ECC3_BINS; i++ )
	{
		int32_t sprev = ecc->spreva2[i][0];
		int32_t sprev2 = ecc->spreva2[i][1];

		int32_t real = ( ( sprev * (uint64_t)ecc3_phasorcos[i]) >> 32) - sprev2;
		int32_t imag = ( ( sprev * (uint64_t)ecc3_phasorsin[i]) >> 32);

		realimag[i][0] = real;
		realimag[i][1] = imag;
	}

	int n;


	uint32_t folded_dft_nc_unfiltered[ECC3_BPERO];

	for( n = 0; n < ECC3_BPERO; n++ )
	{
		int32_t powervalue = 0;
		int sum = 0;
		for( int o = 0; o < ECC3_OCTAVES; o++ )
		{
			int i = n + o * ECC3_BPERO;
			int32_t rp = realimag[i][0];
			int32_t ip = realimag[i][1];
			if( i == ECC3_BINS-1 ) break;
			int32_t rn = realimag[i+1][0];
			int32_t in = realimag[i+1][1];

			int32_t rotrn = ((rn * (uint64_t)ecc3_ncrotateC)>>32) - ((in * (uint64_t)ecc3_ncrotateS)>>32);
			int32_t rotin = ((rn * (uint64_t)ecc3_ncrotateS)>>32) + ((in * (uint64_t)ecc3_ncrotateC)>>32);

			// It's algebraically more work to do the scaling here, but we want to do it here so
			// we don't lose precision in the NC step.

			uint32_t powerfalloff = ecc3_powerfalloff[i];
			int noteinoctave = i%ECC3_BPERO;

			// Feather start and end.
			if( o == 0 )
			{
				powerfalloff = powerfalloff / ECC3_BPERO * (n+1);
			}
			else if( o == ECC3_OCTAVES-1 )
			{
				// Taper ending to zero to exclude last element, since it is unusable.
				powerfalloff = powerfalloff / ECC3_BPERO * (ECC3_BPERO - n - 1) ;
			}
			else
			{
				// powerfalloff unaffected.
			}

			rp = (rp * (uint64_t)powerfalloff)>>32;
			ip = (ip * (uint64_t)powerfalloff)>>32;

			rotrn = (rotrn * (uint64_t)powerfalloff)>>32;
			rotin = (rotin * (uint64_t)powerfalloff)>>32;

			// 8 Picked somewhat arbitrarily, but to help keep accuracy and minimize overflows.
			#define ECC3_LOCAL_INASCALE 8
			int v =  -(( (rp * (int64_t)rotrn)>>ECC3_LOCAL_INASCALE) + ((ip * (int64_t)rotin)>>ECC3_LOCAL_INASCALE));

			int negative = 0;
			if( v < 0 ) continue;
			v = (ECC3_apsqrt(v));

			ecc->nc_values[i] = v;

			powervalue += v;
		}
		folded_dft_nc_unfiltered[n] = powervalue;
		// Right now, this tops out around 22193.  
		// Something else is wrong before this chunk that causes that.
	}

	// folded_dft_nc is now populated.
	// Next, find the peaks.
	// The next part is wholesale-lifted from colorchord/embeddedcommon (for colorchord2)

	uint32_t * folded_bins = ecc->folded_dft_nc;

#if 1
	// Need to take the unfiltered bins and filter them into filtered.
	for( i = 0; i < ECC3_BPERO; i++ )
	{
		int ppbv = folded_dft_nc_unfiltered[(i-2)>=0?(i-2):(ECC3_BPERO+i-2)];
		int pbv = folded_dft_nc_unfiltered[(i-1)>=0?(i-1):(ECC3_BPERO+i-1)];
		int tbv = folded_dft_nc_unfiltered[i];
		int nbv = folded_dft_nc_unfiltered[(i+1)<ECC3_BPERO?(i+1):(i+1-ECC3_BPERO)];
		int nnbv = folded_dft_nc_unfiltered[(i+2)<ECC3_BPERO?(i+2):(i+2-ECC3_BPERO)];

		folded_bins[i] = ((ppbv >> 2) + (pbv>>1) + tbv + (nbv>>1) + (nnbv>>2)) >> 1;
	}
#else
	// Raw
	memcpy( folded_bins, folded_dft_nc_unfiltered, sizeof(folded_dft_nc_unfiltered) );
#endif


	uint8_t hitnotes[ECC3_MAXNOTES] = { 0 };

	//Next, we have to find the peaks, this is what "decompose" does in our
	//normal tool.  NOTE: We CANNOT accept bins > 32767 here.
	{
		int adjLeft = ECC3_BPERO-1;
		int adjRight = 1;
		for( i = 0; i < ECC3_BPERO; i++ )
		{
			int32_t prevBin = folded_bins[adjLeft];
			int32_t nextBin = folded_bins[adjRight];
			int32_t thisBin = folded_bins[i];
			int thisfreq = i<<ECC3_SEMIBITSPERBIN;
			int offset;
			adjLeft++; if( adjLeft == ECC3_BPERO ) adjLeft = 0;
			adjRight++; if( adjRight == ECC3_BPERO ) adjRight = 0;
			if( thisBin < ECC3_MIN_AMP_FOR_NOTE ) continue;
			if( prevBin > thisBin || nextBin > thisBin ) continue;
			if( prevBin == thisBin && nextBin == thisBin ) continue;

			//i is at a peak... 
			int32_t totaldiff = (( thisBin - prevBin ) + ( thisBin - nextBin ));
			int32_t porpdiffP = ((thisBin-prevBin)<<16) / totaldiff; //close to 0 =
					//closer to this side, 32768 = in the middle, 65535 away.
			int32_t porpdiffN = ((thisBin-nextBin)<<16) / totaldiff;

			if( porpdiffP < porpdiffN )
			{
				//Closer to prev.
				offset = -(32768 - porpdiffP);
			}
			else
			{
				//Closer to next
				offset = (32768 - porpdiffN);
			}

			//Need to round.  That's what that extra +(15.. is in the center.
			thisfreq += (offset+(1<<(15-ECC3_SEMIBITSPERBIN)))>>(16-ECC3_SEMIBITSPERBIN);

			//In the event we went 'below zero' need to wrap to the top.
			if( thisfreq > 255-(1<<ECC3_SEMIBITSPERBIN) )
				thisfreq = (1<<ECC3_SEMIBITSPERBIN)*ECC3_BPERO - (256-thisfreq);

			//Okay, we have a peak, and a frequency. Now, we need to search
			//through the existing notes to see if we have any matches.
			//If we have a note that's close enough, we will try to pull it
			//closer to us and boost it.
			int8_t lowest_found_free_note = -1;
			int8_t closest_note_id = -1;
			int16_t closest_note_distance = 32767;
			
			for( int j = 0; j < ECC3_MAXNOTES; j++ )
			{
				uint8_t nf = ecc->note_peak_freqs[j];

				if( nf == 255 )
				{
					if( lowest_found_free_note == -1 )
						lowest_found_free_note = j;
					continue;
				}

				int16_t distance = thisfreq - nf;

				if( distance < 0 ) distance = -distance;

				//Make sure that if we've wrapped around the right side of the
				//array, we can detect it and loop it back.
				if( distance > ((1<<(ECC3_SEMIBITSPERBIN-1))*ECC3_BPERO) )
				{
					distance = ((1<<(ECC3_SEMIBITSPERBIN))*ECC3_BPERO) - distance;
				}

				//If we find a note closer to where we are than any of the 
				//others, we can mark it as our desired note.
				if( distance < closest_note_distance )
				{	
					closest_note_id = j;
					closest_note_distance = distance;
				}
			}

			int8_t marked_note = -1;

			if( closest_note_distance <= ECC3_MAX_JUMP_DISTANCE )
			{
				//We found the note we need to augment.
				ecc->note_peak_freqs[closest_note_id] = thisfreq;
				marked_note = closest_note_id;
			}

			//The note was not found.
			else if( lowest_found_free_note != -1 )
			{
				ecc->note_peak_freqs[lowest_found_free_note] = thisfreq;
				marked_note = lowest_found_free_note;
			}

			//If we found a note to attach to, we have to use the IIR to
			//increase the strength of the note, but we can't exactly snap
			//it to the new strength.
			if( marked_note != -1 )
			{
				hitnotes[marked_note] = 1;

				ecc->note_peak_amps[marked_note] =
					ecc->note_peak_amps[marked_note] -
					(ecc->note_peak_amps[marked_note]>>ECC3_AMP_1_IIR_BITS) +
					(thisBin>>(ECC3_AMP_1_IIR_BITS-3));

				ecc->note_peak_amps2[marked_note] =
					ecc->note_peak_amps2[marked_note] -
					(ecc->note_peak_amps2[marked_note]>>ECC3_AMP_2_IIR_BITS) +
					((thisBin<<3)>>(ECC3_AMP_2_IIR_BITS));
			}
		}
	}

#if 0
	for( i = 0; i < MAXNOTES; i++ )
	{
		if( note_peak_freqs[i] == 255 ) continue;
		printf( "%d / ", note_peak_amps[i] );
	}
	printf( "\n" );
#endif

	//Now we need to handle combining notes.
	for( i = 0; i < ECC3_MAXNOTES; i++ )
	for( int j = 0; j < i; j++ )
	{
		//We'd be combining nf2 (j) into nf1 (i) if they're close enough.
		uint8_t nf1 = ecc->note_peak_freqs[i];
		uint8_t nf2 = ecc->note_peak_freqs[j];
		int16_t distance = nf1 - nf2;

		if( nf1 == 255 || nf2 == 255 ) continue;

		if( distance < 0 ) distance = -distance;

		//If it wraps around above the halfway point, then we're closer to it
		//on the other side. 
		if( distance > ((1<<(ECC3_SEMIBITSPERBIN-1))*ECC3_BPERO) )
		{
			distance = ((1<<(ECC3_SEMIBITSPERBIN))*ECC3_BPERO) - distance;
		}

		if( distance > ECC3_MAX_COMBINE_DISTANCE )
		{
			continue;
		}

		int into;
		int from;

		if( ecc->note_peak_amps[i] > ecc->note_peak_amps[j] )
		{
			into = i;
			from = j;
		}
		else
		{
			into = j;
			from = i;
		}

		//We need to combine the notes.  We need to move the new note freq
		//towards the stronger of the two notes.  
		int16_t amp1 = ecc->note_peak_amps[into];
		int16_t amp2 = ecc->note_peak_amps[from];

		//0 to 32768 porportional to how much of amp1 we want.
		uint32_t porp = (amp1<<15) / (amp1+amp2+1);  
		uint16_t newnote = (nf1 * porp + nf2 * (32768-porp))>>15;

		//When combining notes, we have to use the stronger amplitude note.
		//trying to average or combine the power of the notes looks awful.
		ecc->note_peak_freqs[into] = newnote;
		ecc->note_peak_amps[into] = (ecc->note_peak_amps[into]>ecc->note_peak_amps[from])?
				ecc->note_peak_amps[into]:ecc->note_peak_amps[j];
		ecc->note_peak_amps2[into] = (ecc->note_peak_amps2[into]>ecc->note_peak_amps2[from])?
				ecc->note_peak_amps2[into]:ecc->note_peak_amps2[j];

		ecc->note_peak_freqs[from] = 255;
		ecc->note_peak_amps[from] = 0;
		ecc->note_jumped_to[from] = i;
	}

	//For all of the notes that have not been hit, we have to allow them to
	//to decay.  We only do this for notes that have not found a peak.
	for( i = 0; i < ECC3_MAXNOTES; i++ )
	{
		if( ecc->note_peak_freqs[i] == 255 || hitnotes[i] ) continue;

		ecc->note_peak_amps[i] -= ecc->note_peak_amps[i]>>ECC3_AMP_1_IIR_BITS;
		ecc->note_peak_amps2[i] -= ecc->note_peak_amps2[i]>>ECC3_AMP_2_IIR_BITS;

		//In the event a note is not strong enough anymore, it is to be
		//returned back into the great pool of unused notes.
		if( ecc->note_peak_amps[i] < ECC3_MINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR )
		{
			ecc->note_peak_freqs[i] = 255;
			ecc->note_peak_amps[i] = 0;
			ecc->note_peak_amps2[i] = 0;
		}
	}
}

static void ECC3UpdateLinearLEDs(embeddedcc3 * ecc)
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
	int8_t k;
	uint16_t j, l;
	uint32_t total_size_all_notes = 0;
	int32_t porpamps[ECC3_MAXNOTES]; //LEDs for each corresponding note.
	uint8_t sorted_note_map[ECC3_MAXNOTES]; //mapping from which note into the array of notes from the rest of the system.
	uint8_t sorted_map_count = 0;
	uint32_t note_nerf_a = 0;

	for( i = 0; i < ECC3_MAXNOTES; i++ )
	{
		if( ecc->note_peak_freqs[i] == 255 ) continue;
		note_nerf_a += ecc->note_peak_amps[i];
	}

	note_nerf_a = ((note_nerf_a * ECC3_NERF_NOTE_PORP)>>8);


	for( i = 0; i < ECC3_MAXNOTES; i++ )
	{
		uint16_t ist = ecc->note_peak_amps[i];
		uint8_t nff = ecc->note_peak_freqs[i];
		if( nff == 255 )
		{
			continue;
		}
		if( ist < note_nerf_a )
		{
			continue;
		}

#if ECC3_SORT_NOTES
		for( j = 0; j < sorted_map_count; j++ )
		{
			if( ecc->note_peak_freqs[ sorted_note_map[j] ] > nff )
			{
				break; // so j is correct place to insert
			}
		}
		for( k = sorted_map_count; k > j; k-- ) // make room
		{
			ecc->sorted_note_map[k] = sorted_note_map[k-1];
		}
		ecc->sorted_note_map[j] = i; // insert in correct place
#else
		sorted_note_map[sorted_map_count] = i; // insert at end
#endif
		sorted_map_count++;
	}

#if 0
	for( i = 0; i < sorted_map_count; i++ )
	{
		printf( "%d: %d: %d /", sorted_note_map[i],  note_peak_freqs[sorted_note_map[i]], note_peak_amps[sorted_note_map[i]] );
	}
	printf( "\n" );
#endif

	uint16_t local_peak_amps[ECC3_MAXNOTES];
	uint16_t local_peak_amps2[ECC3_MAXNOTES];
	uint8_t  local_peak_freq[ECC3_MAXNOTES];

	//Make a copy of all of the variables into local ones so we don't have to keep double-dereferencing.
	for( i = 0; i < sorted_map_count; i++ )
	{
		//printf( "%5d ", local_peak_amps[i] );
		local_peak_amps[i] = ecc->note_peak_amps[sorted_note_map[i]] - note_nerf_a;
		local_peak_amps2[i] = ecc->note_peak_amps2[sorted_note_map[i]];
		local_peak_freq[i] = ecc->note_peak_freqs[sorted_note_map[i]];
		//printf( "%5d ", local_peak_amps[i] );
	}
	//printf( "\n" );

	int minimum_amp_to_contribute =  local_peak_amps[0]>>2; // If less than 1/8 the max, it doesn't contribute.
	for( i = 0; i < sorted_map_count; i++ )
	{
		if( local_peak_amps[i] < minimum_amp_to_contribute )
		{
			local_peak_amps[i] = 0;
			local_peak_amps2[i] = 0;
		}
	}	

	for( i = 0; i < sorted_map_count; i++ )
	{
		uint16_t ist = local_peak_amps[i];
		porpamps[i] = 0;
		total_size_all_notes += local_peak_amps[i];
	}

	if( total_size_all_notes == 0 )
	{
		for( j = 0; j < ECC3_USE_NUM_LIN_LEDS * 3; j++ )
		{
			ecc->ledOut[j] = 0;
		}
		return;
	}

	// WARNING: Unsigned divide here.  This has to be a divide.  RIP the ch32v002.  It is guaranteed not to be 0.
	uint32_t porportional = (uint32_t)(ECC3_USE_NUM_LIN_LEDS<<16)/((uint32_t)total_size_all_notes);
	uint16_t total_accounted_leds = 0;

	for( i = 0; i < sorted_map_count; i++ )
	{
		porpamps[i] = (local_peak_amps[i] * porportional) >> 16;
		total_accounted_leds += porpamps[i];
	}

	int16_t total_unaccounted_leds = ECC3_USE_NUM_LIN_LEDS - total_accounted_leds;

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
			ecc->ledFreqOut[j] = local_peak_freq[i];
			ecc->ledAmpOut[j] = (local_peak_amps2[i]*ECC3_NOTE_FINAL_AMP)>>8;
			j++;
			porpamps[i]--;
		}
	}

	//This part totally can't run on an embedded system.
	if( ECC3_LIN_WRAPAROUND )
	{
		uint16_t midx = 0;
		uint32_t mqty = 100000000;
		for( j = 0; j < ECC3_USE_NUM_LIN_LEDS; j++ )
		{
			uint32_t dqty;
			uint16_t localj;

			dqty = 0;
			localj = j;
			for( l = 0; l < ECC3_USE_NUM_LIN_LEDS; l++ )
			{
				int32_t d = (int32_t)ecc->ledFreqOut[localj] - (int32_t)ecc->ledFreqOutOld[l];
				if( d < 0 ) d *= -1;
				if( d > (ECC3_NOTERANGE>>1) ) { d = ECC3_NOTERANGE - d + 1; }
				dqty += ( d * d );

				localj++;
				if( localj == ECC3_USE_NUM_LIN_LEDS ) localj = 0;
			}

			if( dqty < mqty )
			{
				mqty = dqty;
				midx = j;
			}
		}

		ecc->ledSpin = midx;
	}
	else
	{
		ecc->ledSpin = 0;
	}

	j = ecc->ledSpin;
	for( l = 0; l < ECC3_USE_NUM_LIN_LEDS; l++, j++ )
	{
		if( j >= ECC3_USE_NUM_LIN_LEDS ) j = 0;
		ecc->ledFreqOutOld[l] = ecc->ledFreqOut[j];

		uint16_t amp = ecc->ledAmpOut[j];
		if( amp > 255 ) amp = 255;
		uint32_t color = ECC3toHEX( (ecc->ledFreqOut[j]+ecc->RootNoteOffset)%ECC3_NOTERANGE, 255, amp );

		ecc->ledOut[l*3+0] = ( color >> 0 ) & 0xff;
		ecc->ledOut[l*3+1] = ( color >> 8 ) & 0xff;
		ecc->ledOut[l*3+2] = ( color >>16 ) & 0xff;
	}
}




static void ECC3UpdateAllSameLEDs( embeddedcc3 * ecc )
{
	int i;
	uint8_t freq = 0;
	uint16_t amp = 0;

	for( i = 0; i < ECC3_MAXNOTES; i++ )
	{
		uint16_t ist = ecc->note_peak_amps2[i];
		uint8_t ifrq = ecc->note_peak_freqs[i];
		if( ist > amp && ifrq != 255 )
		{
			freq = ifrq;
			amp = ist;
		}
	}

	amp = (((uint32_t)(amp))*ECC3_NOTE_FINAL_AMP)>>8;

	if( amp > 255 ) amp = 255;
	uint32_t color = ECC3toHEX( (freq+ecc->RootNoteOffset)%ECC3_NOTERANGE, 255, amp );

	for( i = 0; i < ECC3_USE_NUM_LIN_LEDS; i++ )
	{
		ecc->ledOut[i*3+0] = ( color >> 0 ) & 0xff;
		ecc->ledOut[i*3+1] = ( color >> 8 ) & 0xff;
		ecc->ledOut[i*3+2] = ( color >>16 ) & 0xff;
	}
}





static uint32_t ECC3HSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val )
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


static uint32_t ECC3toHEX( uint8_t note, uint8_t sat, uint8_t val )
{
	uint16_t hue = 0;
	uint16_t third = 65535/3;
	uint16_t scalednote = note;
	uint32_t renote = ((uint32_t)note * 65536) / ECC3_NOTERANGE;

	//Note is expected to be a vale from 0..(ECC3_NOTERANGE-1)
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

	return ECC3HSVtoHEX( hue, sat, val );
}

static int ECC3_apsqrt( uint32_t i )
{
	// Fast approximate square root, accurate to about 1-2 code values across integer space.
	if( i == 0 ) return 0;
	int x = 1<<( ( 32 - __builtin_clz(i) )/2);
	x = (x + i/x)/2;
	x = (x + i/x)/2;
	x = (x + i/x)/2;
	return x;
}


#endif


