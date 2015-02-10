#ifndef _DFT_H
#define _DFT_H

//XXX WARNING: TODO: the last two parameters are a double due to a compiler bug.

//Do a DFT on a live audio ring buffer.  It assumes new samples are added on in the + direction, older samples go negative.
//Frequencies are as a function of the samplerate, for example, a frequency of 22050 is actually 2 Hz @ 44100 SPS
//bins = number of frequencies to check against.
void DoDFT( float * outbins, float * frequencies, int bins, float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q );

//Skip many of the samples on lower frequencies; TODO: Need to fix the nyquist problem where high frequencies show low-frequency components.
//Speedup = target number of data points
void DoDFTQuick( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );

//An unusual tool to do a "progressive" DFT, using data from previous rounds.
void DoDFTProgressive( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );

//A progressive DFT that's done using only low-bit integer math.
void DoDFTProgressiveInteger( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );


#endif

