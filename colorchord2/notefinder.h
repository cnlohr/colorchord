#ifndef _NOTEFINDER_H
#define _NOTEFINDER_H

#include "os_generic.h"


struct NoteFinder
{
	//Setup DFT Bins
	int ofreqs;
	float slope;// = 0
	int octaves;// = 5;
	int freqbins;// = 24;
	int note_peaks; //Calculated from freqbins (not configurable)
	float base_hz;// = 55;
	float filter_strength;// = .5; //0=Disabled.
	int filter_iter;// = 1;
	int decompose_iterations;// = 1000;
	float amplify; // =1 (amplify input across the board)
	int do_progressive_dft; //= 1

	//at 300, there is still some minimal aliasing at higher frequencies.  Increase this for less low-end distortion
	//NOTE: This /should/ get fixed, as we /should/ be decimating the input data intelligently with lower octaves.
	float dft_speedup;// = 300; 

	//The "tightness" of the curve, or how many samples back to look?
	float dft_q;// = 16;

	float dft_iir; //IIR to impose the output of the IIR.

	//This controls the expected shape of the normal distributions.  I am not sure how to calculate this from samplerate, Q and bins.
	float default_sigma;// = 1.4;//freqbins/dft_q/1.6; //Guess? This happens to work out well?

	float note_jumpability;// = 2.5; //How far established notes are allowed to "jump" in order to attach themselves to a new "peak"
	float note_combine_distance;// = 0.5; //How close established notes need to be to each other before they can be "combined" into a single note.
	float note_attach_freq_iir;// = 0.2;
	float note_attach_amp_iir;// = 0.2;
	float note_attach_amp_iir2;// = 0.1;
	float note_min_amplitude;// = 0.02;  //What is considered a "dead" light?
	float note_minimum_new_distribution_value;// = 0.02; //A distribution must be /this/ big otherwise, it will be discarded.

	float note_out_chop;// = .1; (How much to decimate the output notes to reduce spurious noise)

	float sps_rec; //samples per second

	//For the "note_" section, the arrays are of size (freqbins/2)
	//OUTPUTS: You probably want these; the amplitude and frequency of each note the system found.
	float * note_positions;			//Position of note, in 0..freqbins frequency.  [note_peaks]
	float * note_amplitudes_out;	//Amplitude of note (after "chop") [note_peaks]
	float * note_amplitudes2;	//Amplitude of note (after "chop") [note_peaks] (using iir2)

	//Other note informations
	float * note_amplitudes;		//Amplitude of note (before "chop") [note_peaks]
	unsigned char * note_founds;	//Array of whether or note a note is taken by a frequency normal distribution [note_peaks]

	//Utility to search from [note_peaks] as index -> nf->dists as value.
	//This makes it possible to read dist_amps[note_peaks_to_dists_mapping[note]].
	char * note_peaks_to_dists_mapping;

	//Enduring note id:  From frame to frame, this will keep values the same.  That way you can know if a note has changed.
	int * enduring_note_id; //If value is 0, it is not in use.  [note_peaks]
	int current_note_id;

	//What frequency each one of the unfolded bins are (in 1/sps's)
	float * frequencies;

	//The unfolded spectrum.
	float * outbins;

	//The folded output of the unfolded spectrum.
	float * folded_bins;


	//Dists: These things are the distributions that are found, they are very fickle and change frequently.
	int dists; //# of distributions (these are the precursors to notes - it is trying to find the normal distributions.)
	float * dist_amps;   //Amplitude of normal distribution... [nf->dists]
	float * dist_means;  //Mean of normal distribution... [nf->dists]
	float * dist_sigmas; //Sigma of normal distribution... [nf->dists]
	unsigned char * dist_takens; //Which distributions are now associated with notes


	//For profiling, all in absolute time in seconds.
	double StartTime;
	double DFTTime;
	double FilterTime;
	double DecomposeTime;
	double FinalizeTime;
};

struct NoteFinder * CreateNoteFinder( int spsRec );  //spsRec = 44100, etc.
void ChangeNFParameters( void * v );
void RunNoteFinder( struct NoteFinder * nf, const float * audio_stream, int head, int buffersize );

#endif

