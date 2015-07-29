//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _DFT_H
#define _DFT_H

#include <stdint.h>

//Warning: Most ColorChords are not available for ColorChord Embedded.
#ifndef CCEMBEDDED

//There are several options here, the last few are selectable by modifying the do_progressive_dft flag.

//Do a DFT on a live audio ring buffer.  It assumes new samples are added on in the + direction, older samples go negative.
//Frequencies are as a function of the samplerate, for example, a frequency of 22050 is actually 2 Hz @ 44100 SPS
//bins = number of frequencies to check against.
void DoDFT( float * outbins, float * frequencies, int bins, float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q );

//Skip many of the samples on lower frequencies.
//Speedup = target number of data points
void DoDFTQuick( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );

//An unusual tool to do a "progressive" DFT, using data from previous rounds.
void DoDFTProgressive( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );

//A progressive DFT that's done using only low-bit integer math.
//This is almost fast enough to work on an AVR, with two AVRs, it's likely that it could be powerful enough.
//This is fast enough to run on an ESP8266
void DoDFTProgressiveInteger( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );

#endif

//Everything the integer one buys, except it only calculates 2 octaves worth of notes per audio frame.
//This is sort of working, but still have some quality issues.
//It would theoretically be fast enough to work on an AVR.
//NOTE: This is the only DFT available to the embedded port of ColorChord
void DoDFTProgressiveIntegerSkippy( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );

//It's actually split into a few functions, which you can call on your own:
void SetupDFTProgressiveIntegerSkippy();  //Call at start.

#ifndef CCEMBEDDED
void UpdateBinsForProgressiveIntegerSkippy( const float * frequencies ); //Update the frequencies
#endif

void UpdateBinsForProgressiveIntegerSkippyInt( const uint16_t * frequencies );
void Push8BitIntegerSkippy( int8_t dat ); //Call this to push on new frames of sound.


//You can # define these to be other things.
#ifndef OCTAVES
#define OCTAVES  5
#endif

#ifndef FIXBPERO
#define FIXBPERO 24
#endif

#ifndef FIXBINS
#define FIXBINS  (FIXBPERO*OCTAVES)
#endif

#ifndef BINCYCLE
#define BINCYCLE (1<<OCTAVES)
#endif

//This variable determins how much to nerf the current sample of the DFT.
//I've found issues when this is smaller, but bigger values do have a negative
//impact on quality.  We should strongly consider using 32-bit accumulators.
#ifndef SHIFT_ADD_DETAIL
#define SHIFT_ADD_DETAIL 5
#endif

//Whenever you need to read the bins, you can do it from here.
extern uint16_t Sdatspace[];  //(advances,places,isses,icses)
extern uint16_t embeddedbins[]; //This is updated every time the DFT hits the octavecount, or every BINCYCLE updates.

#endif

