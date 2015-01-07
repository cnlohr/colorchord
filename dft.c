#include "dft.h"
#include <math.h>
#include <stdio.h>


void DoDFT( float * outbins, float * frequencies, int bins, float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q )
{
	int i, j;
	for( i = 0; i < bins; i++ )
	{
		float freq = frequencies[i];
		float phi = 0;
		int sampleplace = place_in_data_buffer;
		float advance = 3.14159*2.0/freq;

		float binqtys = 0;
		float binqtyc = 0;

		for( j = 0; j <= freq * q; j++ )
		{
			float sample = databuffer[sampleplace];
			sampleplace = (sampleplace-1+size_of_data_buffer)%size_of_data_buffer;
//printf( "%d\n", sampleplace );
			float sv = sin( phi ) * sample;
			float cv = cos( phi ) * sample;

			binqtys += sv;
			binqtyc += cv;

			phi += advance;
		}

		float amp = sqrtf( binqtys * binqtys + binqtyc * binqtyc );
		outbins[i] = amp / freq / q;
	}
}

void DoDFTQuick( float * outbins, float * frequencies, int bins, const float * databuffer, int place_in_data_buffer, int size_of_data_buffer, float q, float speedup )
{
	int i, j;

	for( i = 0; i < bins; i++ )
	{
		float freq = frequencies[i];
		float phi = 0;
		int ftq = freq * q; 
		int sampleplace = place_in_data_buffer;
		float advance = 3.14159*2.0/freq;

		float binqtys = 0;
		float binqtyc = 0;

		int skip = floor( ftq / speedup );
		if( skip == 0 ) skip = 1;
		advance *= skip;

		for( j = 0; j <= ftq; j += skip )
		{
			float sample = databuffer[sampleplace];
			sampleplace = (sampleplace-skip+size_of_data_buffer)%size_of_data_buffer;
//printf( "%d\n", sampleplace );
			float sv = sinf( phi ) * sample;
			float cv = cosf( phi ) * sample;

			binqtys += sv;
			binqtyc += cv;

			phi += advance;
		}

		float amp = sqrtf( binqtys * binqtys + binqtyc * binqtyc );
		outbins[i] = amp / freq / q * skip;
	}
}

