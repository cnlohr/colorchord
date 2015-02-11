#include "notefinder.h"
#include "parameters.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "util.h"
#include "dft.h"
#include "filter.h"
#include "decompose.h"
#include "sort.h"

struct NoteFinder * CreateNoteFinder( int spsRec )
{
	struct NoteFinder * ret = malloc( sizeof( struct NoteFinder ) );
	memset( ret, 0, sizeof( struct NoteFinder ) );
	ret->sps_rec = spsRec;

	//Set all default values before passing off to the parameter changer.
	ret->octaves = 5;
	ret->freqbins = 24;
	ret->base_hz = 55;
	ret->filter_strength = .5;
	ret->filter_iter = 1;
	ret->decompose_iterations = 1000;
	ret->dft_speedup = 300;
	ret->dft_q = 16;
	ret->slope = 0.0;
	ret->do_progressive_dft = 0;
	ret->default_sigma = 1.4;
	ret->note_jumpability = 2.5;
	ret->note_combine_distance = 0.5;
	ret->note_attach_freq_iir = 0.4;
	ret->note_attach_amp_iir = 0.2;
	ret->note_attach_amp_iir2 = 0.05;
	ret->note_min_amplitude = 0.001;
	ret->note_minimum_new_distribution_value = 0.02;
	ret->note_out_chop = 0.1;
	ret->dft_iir = 0.0;
	ret->current_note_id = 1;
	ret->amplify = 1;

	ret->ofreqs = 0; //force a re-init.

	RegisterValue( "octaves", PAINT, &ret->octaves, sizeof( ret->octaves ) );
	RegisterValue( "freqbins", PAINT, &ret->freqbins, sizeof( ret->freqbins ) );
	RegisterValue( "base_hz", PAFLOAT, &ret->base_hz, sizeof( ret->base_hz ) );
	RegisterValue( "filter_strength", PAFLOAT, &ret->filter_strength, sizeof( ret->filter_strength ) );
	RegisterValue( "filter_iter", PAINT, &ret->filter_iter, sizeof( ret->filter_iter ) );
	RegisterValue( "decompose_iterations", PAINT, &ret->decompose_iterations, sizeof( ret->decompose_iterations ) );
	RegisterValue( "amplify", PAFLOAT, &ret->amplify, sizeof( ret->amplify ) );
	RegisterValue( "dft_speedup", PAFLOAT, &ret->dft_speedup, sizeof( ret->dft_speedup ) );
	RegisterValue( "dft_q", PAFLOAT, &ret->dft_q, sizeof( ret->dft_q ) );
	RegisterValue( "default_sigma", PAFLOAT, &ret->default_sigma, sizeof( ret->default_sigma ) );
	RegisterValue( "note_jumpability", PAFLOAT, &ret->note_jumpability, sizeof( ret->note_jumpability ) );
	RegisterValue( "note_combine_distance", PAFLOAT, &ret->note_combine_distance, sizeof( ret->note_combine_distance ) );
	RegisterValue( "slope", PAFLOAT, &ret->slope, sizeof( ret->slope ) );
	RegisterValue( "note_attach_freq_iir", PAFLOAT, &ret->note_attach_freq_iir, sizeof( ret->note_attach_freq_iir ) );
	RegisterValue( "note_attach_amp_iir", PAFLOAT, &ret->note_attach_amp_iir, sizeof( ret->note_attach_amp_iir ) );
	RegisterValue( "note_attach_amp_iir2", PAFLOAT, &ret->note_attach_amp_iir2, sizeof( ret->note_attach_amp_iir2 ) );
	RegisterValue( "note_minimum_new_distribution_value", PAFLOAT, &ret->note_minimum_new_distribution_value, sizeof( ret->note_minimum_new_distribution_value ) );
	RegisterValue( "note_out_chop", PAFLOAT, &ret->note_out_chop, sizeof( ret->note_out_chop ) );
	RegisterValue( "dft_iir", PAFLOAT, &ret->dft_iir, sizeof( ret->dft_iir ) );
	RegisterValue( "do_progressive_dft", PAINT, &ret->do_progressive_dft, sizeof( ret->do_progressive_dft ) );

	AddCallback( "freqbins", ChangeNFParameters, ret );
	AddCallback( "octaves", ChangeNFParameters, ret );
	AddCallback( "base_hz", ChangeNFParameters, ret );

	ChangeNFParameters( ret );

	return ret;
}

void ChangeNFParameters( void * v )
{
	//int ofreqs = ret->freqbins * ret->octaves;
	int i;
	struct NoteFinder * ret = (struct NoteFinder*)v;
/*
	char t[128];
	ret->octaves = atoi_del( GetParamStr( parameters, "octaves", setstr( ret->octaves, t ) ) );
	ret->freqbins = atoi_del( GetParamStr( parameters, "freqbins", setstr( ret->freqbins, t ) ) );
	ret->base_hz = atof_del( GetParamStr( parameters, "base_hz", setstr( ret->base_hz, t ) ) );
	ret->filter_strength = atof_del( GetParamStr( parameters, "filter_strength", setstr( ret->filter_strength, t ) ) );
	ret->filter_iter = atoi_del( GetParamStr( parameters, "filter_iter", setstr( ret->filter_iter, t ) ) );
	ret->decompose_iterations = atoi_del( GetParamStr( parameters, "decompose_iterations", setstr( ret->decompose_iterations, t ) ) );
	ret->dft_speedup = atof_del( GetParamStr( parameters, "dft_speedup", setstr( ret->dft_speedup, t ) ) );
	ret->dft_q = atof_del( GetParamStr( parameters, "dft_q", setstr( ret->dft_q, t ) ) );
	ret->default_sigma = atof_del( GetParamStr( parameters, "default_sigma", setstr( ret->default_sigma, t ) ) );
	ret->note_jumpability = atof_del( GetParamStr( parameters, "note_jumpability", setstr( ret->note_jumpability, t ) ) );
	ret->note_combine_distance = atof_del( GetParamStr( parameters, "note_combine_distance", setstr( ret->note_combine_distance, t ) ) );
	ret->note_attach_freq_iir = atof_del( GetParamStr( parameters, "note_attach_freq_iir", setstr( ret->note_attach_freq_iir, t ) ) );
	ret->note_attach_amp_iir = atof_del( GetParamStr( parameters, "note_attach_amp_iir", setstr( ret->note_attach_amp_iir, t ) ) );
	ret->note_attach_amp_iir2 = atof_del( GetParamStr( parameters, "note_attach_amp_iir2", setstr( ret->note_attach_amp_iir2, t ) ) );
	ret->note_min_amplitude = atof_del( GetParamStr( parameters, "note_min_amplitude", setstr( ret->note_min_amplitude, t ) ) );
	ret->note_minimum_new_distribution_value = atof_del( GetParamStr( parameters, "note_minimum_new_distribution_value", setstr( ret->note_minimum_new_distribution_value, t ) ) );
	ret->note_out_chop = atof_del( GetParamStr( parameters, "note_out_chop", setstr( ret->note_out_chop, t ) ) );
	ret->dft_iir = atof_del( GetParamStr( parameters, "dft_iir", setstr( ret->dft_iir, t ) ) );
	ret->sps_rec = atof_del( GetParamStr( parameters, "sps_rec", setstr( ret->sps_rec, t ) ) );
	ret->amplify = atof_del( GetParamStr( parameters, "amplify", setstr( ret->amplify, t ) ) );
*/


printf( "%d %d %f %f %f\n", ret->freqbins, ret->octaves, ret->base_hz, ret->dft_q, ret->amplify );




	int freqs = ret->freqbins * ret->octaves;

	if( freqs != ret->ofreqs )
	{
		int note_peaks = ret->freqbins/2;
		int maxdists = ret->freqbins/2;
		ret->note_peaks = note_peaks;

		if( ret->note_positions ) free( ret->note_positions );
		ret->note_positions = calloc( 1, sizeof( float ) * note_peaks );

		if( ret->note_amplitudes ) free( ret->note_amplitudes );
		ret->note_amplitudes = calloc( 1, sizeof( float ) * note_peaks );

		if( ret->note_amplitudes_out ) free( ret->note_amplitudes_out );
		ret->note_amplitudes_out = calloc( 1, sizeof( float ) * note_peaks );

		if( ret->note_amplitudes2 ) free( ret->note_amplitudes2 );
		ret->note_amplitudes2 = calloc( 1, sizeof( float ) * note_peaks );

		if( ret->note_peaks_to_dists_mapping ) free( ret->note_peaks_to_dists_mapping );
		ret->note_peaks_to_dists_mapping = calloc( 1, sizeof( char ) * note_peaks );

		if( ret->enduring_note_id ) free( ret->enduring_note_id );
		ret->enduring_note_id = calloc( 1, sizeof( int ) * note_peaks );

		if( ret->note_founds ) free( ret->note_founds );
		ret->note_founds = calloc( 1, sizeof( unsigned char ) * note_peaks );

		if( ret->frequencies ) free( ret->frequencies );
		ret->frequencies = calloc( 1, sizeof( float ) * freqs );
		if( ret->outbins ) free( ret->outbins );
		ret->outbins = calloc( 1, sizeof( float ) * freqs );
		if( ret->folded_bins ) free( ret->folded_bins );
		ret->folded_bins = calloc( 1, sizeof( float ) * ret->freqbins );


		if( ret->dist_amps ) free( ret->dist_amps );
		ret->dist_amps = calloc( 1, sizeof( float ) * maxdists );

		if( ret->dist_means ) free( ret->dist_means );
		ret->dist_means = calloc( 1, sizeof( float ) * maxdists );

		if( ret->dist_sigmas ) free( ret->dist_sigmas );
		ret->dist_sigmas = calloc( 1, sizeof( float ) * maxdists );

		if( ret->dist_takens ) free( ret->dist_takens );
		ret->dist_takens = calloc( 1, sizeof( unsigned char ) * maxdists );

		ret->ofreqs = freqs;
	}

	for( i = 0; i < freqs; i++ )
	{
		ret->frequencies[i] = ( ret->sps_rec / ret->base_hz ) / pow( 2, (float)i / ret->freqbins );
	}

}

void RunNoteFinder( struct NoteFinder * nf, const float * audio_stream, int head, int buffersize )
{
	int i, j;
	int freqbins = nf->freqbins;
	int note_peaks = freqbins/2;
	int freqs = freqbins * nf->octaves;
	int maxdists = freqbins/2;
	float dftbins[freqs];

	//Now, march onto the DFT, this pulls out the bins we're after.
	//This DFT function does not wavelet or anything.
	nf->StartTime = OGGetAbsoluteTime();

	switch( nf->do_progressive_dft )
	{
	case 0:
		DoDFTQuick( dftbins, nf->frequencies, freqs, audio_stream, head, buffersize, nf->dft_q, nf->dft_speedup );
		break;
	case 1:
		DoDFTProgressive( dftbins, nf->frequencies, freqs, audio_stream, head, buffersize, nf->dft_q, nf->dft_speedup );
		break;
	case 2:
		DoDFTProgressiveInteger( dftbins, nf->frequencies, freqs, audio_stream, head, buffersize, nf->dft_q, nf->dft_speedup );
		break;
	case 3:
		DoDFTProgressiveIntegerSkippy( dftbins, nf->frequencies, freqs, audio_stream, head, buffersize, nf->dft_q, nf->dft_speedup );
		break;
	default:
		fprintf( stderr, "Error: No DFT Seleced\n" );
	}
	nf->DFTTime = OGGetAbsoluteTime();

	for( i = 0; i < freqs; i++ )
	{
		nf->outbins[i] = (nf->outbins[i] * (nf->dft_iir) + (dftbins[i] * (1.-nf->dft_iir) * nf->amplify *  ( 1. + nf->slope * i )));
	}
	

	//Taper the first and last octaves.
	for( i = 0; i < freqbins; i++ )
	{
		nf->outbins[i] *= (i+1.0)/nf->freqbins;
	}

	for( i = 0; i < freqbins; i++ )
	{
		nf->outbins[freqs-i-1] *= (i+1.0)/nf->freqbins;
	}

	//Combine the bins into folded bins.
	for( i = 0; i < freqbins; i++ )
	{
		float amp = 0;
		for( j = 0; j < nf->octaves; j++ )
		{
			amp += nf->outbins[i+j*freqbins];
		}
		nf->folded_bins[i] = amp;
	}

	//This is here to reduce the number of false-positive hits.  It helps remove peaks that are meaningless.
	FilterFoldedBinsBlob( nf->folded_bins, freqbins, nf->filter_strength, nf->filter_iter );

	nf->FilterTime = OGGetAbsoluteTime();

	memset( nf->dist_takens, 0, sizeof( unsigned char ) * maxdists  );
	nf->dists = DecomposeHistogram( nf->folded_bins, freqbins, nf->dist_means, nf->dist_amps, nf->dist_sigmas, maxdists, nf->default_sigma, nf->decompose_iterations );
	{
		int dist_sorts[nf->dists];
		SortFloats( dist_sorts, nf->dist_amps, nf->dists );
		RemapFloats( dist_sorts, nf->dist_amps, nf->dists );
		RemapFloats( dist_sorts, nf->dist_means, nf->dists );
		RemapFloats( dist_sorts, nf->dist_sigmas, nf->dists );
	}
	nf->DecomposeTime = OGGetAbsoluteTime();


	//We now have the positions and amplitudes of the normal distributions that comprise our spectrum.  IN SORTED ORDER!
	//dists = # of distributions
	//dist_amps[] = amplitudes of the normal distributions
	//dist_means[] = positions of the normal distributions

	//We need to use this in a filtered manner to obtain the "note" peaks
	//note_peaks = total number of peaks.
	//note_positions[] = position of the note on the scale.
	//note_amplitudes[] = amplitudes of these note peaks.
	memset( nf->note_founds, 0, sizeof( unsigned char ) * note_peaks );

	//First try to find any close peaks.
	for( i = 0; i < note_peaks; i++ )
	{
		for( j = 0; j < nf->dists; j++ )
		{ 
			if( !nf->dist_takens[j] && !nf->note_founds[i] && fabsloop( nf->note_positions[i], nf->dist_means[j], freqbins ) < nf->note_jumpability && nf->dist_amps[j] > 0.00001 ) //0.00001 for stability.
			{
				//Attach ourselves to this bin.
				nf->note_peaks_to_dists_mapping[i] = j;
				nf->dist_takens[j] = 1;
				if( nf->enduring_note_id[i] == 0 )
					nf->enduring_note_id[i] = nf->current_note_id++;	
				nf->note_founds[i] = 1;

				nf->note_positions[i] = avgloop( nf->note_positions[i], (1.-nf->note_attach_freq_iir), nf->dist_means[j], nf->note_attach_freq_iir, nf->freqbins);

				//I guess you can't IIR this like normal.
				////note_positions[i] * (1.-note_attach_freq_iir) + dist_means[j] * note_attach_freq_iir;

				nf->note_amplitudes[i] = nf->note_amplitudes[i] * (1.-nf->note_attach_amp_iir) + nf->dist_amps[j] * nf->note_attach_amp_iir;  
				//XXX TODO: Consider: Always boost power, never reduce?
//					if( dist_amps[i] > note_amplitudes[i] )
//						note_amplitudes[i] = dist_amps[i];
			}
		}
	}

	//Combine like-notes.
	for( i = 0; i < note_peaks; i++ )
	{
//		printf( "%f %f %d\n", nf->note_amplitudes[i], nf->note_positions[i], nf->enduring_note_id[i] );
		for( j = 0; j < note_peaks; j++ )
		{
			if( i == j ) continue;
			if( fabsloop( nf->note_positions[i], nf->note_positions[j], nf->freqbins  ) < nf->note_combine_distance &&
				nf->note_amplitudes[i] > 0.0 &&
				nf->note_amplitudes[j] > 0.0 )
			{
				int a;
				int b;
				if( nf->note_amplitudes[i] > nf->note_amplitudes[j] )
				{
					a = i;
					b = j;
				}
				else
				{
					b = i;
					a = j;
				}
				float newp = avgloop( nf->note_positions[a], nf->note_amplitudes[a], nf->note_positions[b], nf->note_amplitudes[b], freqbins );

				//Combine B into A.
				nf->note_amplitudes[a] += nf->note_amplitudes[b];
				nf->note_positions[a] = newp;
				nf->note_amplitudes[b] = 0;
				nf->note_positions[b] = -100;
				nf->enduring_note_id[b] = 0;
			}
		}
	}

	//Assign dead or decayed notes to new  peaks.
	for( i = 0; i < note_peaks; i++ )
	{
		if( nf->note_amplitudes[i] < nf->note_min_amplitude )
		{
			nf->enduring_note_id[i] = 0;

			//Find a new peak for this note.
			for( j = 0; j < nf->dists; j++ )
			{
				if( !nf->dist_takens[j] && nf->dist_amps[j] > nf->note_minimum_new_distribution_value ) 
				{
					nf->enduring_note_id[i] = nf->current_note_id++;
					nf->dist_takens[j] = 1;
					nf->note_amplitudes[i] = nf->dist_amps[j];//min_note_amplitude + dist_amps[j] * note_attach_amp_iir; //TODO: Should this jump?
					nf->note_positions[i] = nf->dist_means[j];
					nf->note_founds[i] = 1;
					nf->dist_takens[j] = 1;
				}
			}
		}
	}

	//Any remaining notes that could not find a peak good enough must be decayed to oblivion.
	for( i = 0; i < note_peaks; i++ )
	{
		if( !nf->note_founds[i] )
		{
			nf->note_amplitudes[i] = nf->note_amplitudes[i] * (1.-nf->note_attach_amp_iir);
		}

		nf->note_amplitudes2[i] = nf->note_amplitudes2[i] * (1.-nf->note_attach_amp_iir2) + nf->note_amplitudes[i] * nf->note_attach_amp_iir2;

		if( nf->note_amplitudes2[i] < nf->note_min_amplitude )
		{
			nf->note_amplitudes[i] = 0;
			nf->note_amplitudes2[i] = 0;
		}
	}

	for( i = 0; i < note_peaks; i++ )
	{
		nf->note_amplitudes_out[i] = nf->note_amplitudes[i] - nf->note_out_chop;
		if( nf->note_amplitudes_out[i] < 0 )
		{
			nf->note_amplitudes_out[i] = 0;
		}
	}

	//We now have our "notes"
	//Notes are made of "note_amplitudes" and "note_positions" in the scale to 0..freqbins
	//They are stable but in an arbitrary order.

	
	nf->FinalizeTime = OGGetAbsoluteTime();
}


