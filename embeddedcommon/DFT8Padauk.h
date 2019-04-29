#ifndef _DFT8PADAUK_H
#define _DFT8PADAUK_H

/* Note: Frequencies must be precompiled. */

void DoDFT8BitPadauk( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup );

#endif

