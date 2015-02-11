#ifndef _DFT_H
#define _DFT_H


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

//Everything the integer one buys, except it only calculates 2 octaves worth of notes per audio frame.
//This is sort of working, but still have some quality issues.
//It would theoretically be fast enough to work on an AVR.
void DoDFTProgressiveIntegerSkippy( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );

#endif

