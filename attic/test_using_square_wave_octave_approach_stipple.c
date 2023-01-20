/*
	An experiment in very, very low-spec ColorChord.  This technique foregoes
	multiplies.

	Approach 1:

	This approach uses a table, like several colorchord algorithms to only
	process one octave each cycle.  It then uses a table to decide how often
	to process each bin.  It performs that bin at 4x the bin's sampling
	frequency, in quadrature.  So that it performs the +real, +imag, -real,
	-imag operations over each cycle. 

	You can observe an overtone at the current bin - 1.5 octaves! This is
	expected, since, it's the inverse of what the DFT of a square wave would
	be.

	That is a minor drawback, but the **major** drawback is that any DC
	offset, OR lower frequencies present when computing higher frequencies will
	induce a significant ground flutter, and make results really inaccurate.

	This will need to be addressed before the algorithm is ready for prime-time.

	NOTE: To explore:
		1) Consider SAMPLE_Q to possibly use all 4 cycles - though this will
			add latency, it will be more "accurate" --> YES! This helps!
		2) Use the cursor to move left and right to sweep tone and up and down
			to change DC-bias.  This exhibits this algo's shortcoming.
		TODO: Can we somehow zero out the DC offset?


	DISCOVERY: This approach (octave-step in octave) is very sensitive FSPS

	Theories:
	 * If we force all 4 quadrature value to be the same number of
			integration cycles, does that solve it?  --> NO! IT GETS MESSY (see Approach 1B)
*/

#include <stdio.h>
#include <math.h>

#define CNFG_IMPLEMENTATION
#define CNFGOGL
#include "rawdraw_sf.h"

#include "os_generic.h"


uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val );

int lastx = 200;
int lasty = 1000;

int main()
{
	#define FSPS  22100  // Careful, this matters!  It is selected to avoid periodic peaks encountered with DC offsets.
	#define OCTAVES 6
	#define BPERO 24
	#define BASE_FREQ 22.5
	#define QUADRATURE_STEP_DENOMINATOR 16384
	
	// Careful:  It makes a lot of sense to explore these relationships.
	#define SAMPLE_Q 4
	#define MAG_IIR 0
	#define RUNNING_IIR 31
	#define COMPLEX_IIR 2
	
	#define TEST_SAMPLES 256


	int16_t samples[TEST_SAMPLES];
	int i;


	CNFGSetup( "Example App", 1024, 768 );
	
	// Precomputed Tables
	int8_t whichoctave[2<<OCTAVES];
	for( i = 0; i < (2<<OCTAVES); i++ )
	{
		int j;
		for( j = 0; j < OCTAVES; j++ )
		{
			if( i & (1<<j) ) break;
		}
		if( j == OCTAVES )
			whichoctave[i] = -1;
		else
			whichoctave[i] = OCTAVES - j - 1;
	}
	// Make a running counter to count up by this amount every cycle.
	// If the new number > 2048, then perform a quadrature step.
	int32_t flipdistance[BPERO];
	for( i = 0; i < BPERO; i++ )
	{
		double freq = pow( 2, (double)i / (double)BPERO ) * (BASE_FREQ/2.0);
		double pfreq = pow( 2, OCTAVES ) * freq;
		double spacing = (FSPS / 2) / pfreq / 4;
		
		flipdistance[i] = QUADRATURE_STEP_DENOMINATOR * spacing;
		// Spacing = "quadrature every X samples"
		//printf( "%f %d\n", spacing, flipdistance[i] );
	}

	// This is for timing.  Not accumulated data.
	int32_t quadrature_timing_last[BPERO*OCTAVES] = { 0 };
	uint8_t  quadrature_state[BPERO*OCTAVES] = { 0 };
	
	uint32_t last_accumulated_value[BPERO*OCTAVES*2] = { 0 };
	
	uint32_t octave_timing[OCTAVES] = { 0 };
	
	int32_t real_imaginary_running[BPERO*OCTAVES*2] = { 0 };
	
	uint32_t sample_accumulator = 0;
	
	int32_t qcount[BPERO*OCTAVES] = { 0 };
	int32_t magsum[BPERO*OCTAVES] = { 0 };

	int frameno = 0;
	double dLT = OGGetAbsoluteTime();
	int samplenoIn = 0;
	int sampleno = 0;
	double ToneOmega = 0;
	int ops;
	while( CNFGHandleInput() )
	{
		CNFGClearFrame();
		
		frameno++;
		float freq = 
			//pow( 2, (frameno%600)/100.0 ) * 25;
			pow( 2, (lastx)/100.0 ) * lastx;
			//101;
			
		for( i = 0; i < TEST_SAMPLES; i++ )
		{
			samples[i] = lasty/5 + sin( ToneOmega ) * 127;// + (rand()%128)-64;
			ToneOmega += 1 / (double)FSPS * (double)freq * 3.14159 * 2.0;
		}
		char cts[1024];
		sprintf( cts, "%f %d %f", freq, sampleno, ops/(double)sampleno );
		CNFGColor( 0xffffffff );
		CNFGPenX = 2;
		CNFGPenY = 2;
		CNFGDrawText( cts, 2 );
		
		while( OGGetAbsoluteTime() < dLT + TEST_SAMPLES / (double)FSPS );
		dLT += TEST_SAMPLES / (double)FSPS;

		if( 0 )
		{
			memset( real_imaginary_running, 0, sizeof( real_imaginary_running ) );
			memset( last_accumulated_value, 0, sizeof( last_accumulated_value ) );
			memset( quadrature_timing_last, 0, sizeof( quadrature_timing_last ) );
			memset( quadrature_state, 0, sizeof( quadrature_state ) );
			memset( octave_timing, 0, sizeof( octave_timing ) );
			sample_accumulator = 0;
		}
		
		for( i = 0; i < TEST_SAMPLES; i++ )
		{
			sample_accumulator += samples[i];
			int octave = whichoctave[(sampleno)&((2<<OCTAVES)-1)];
			sampleno++;
			if( octave < 0 )
			{
				// A "free cycle" this happens every 1/2^octaves
				continue;
			}
			
			#define WATCHBIN 1
			int b;
			int binno = octave * BPERO;
			int ocative_time = octave_timing[octave] += QUADRATURE_STEP_DENOMINATOR;
			for( b = 0; b < BPERO; b++, binno++ )
			{
				if( binno == WATCHBIN )
				{
					printf( "%d %d %d %6d %6d %6d\n", ocative_time, quadrature_timing_last[binno], quadrature_state[0], real_imaginary_running[0], real_imaginary_running[1], magsum[0] );
				}
				if( ocative_time - quadrature_timing_last[binno] > 0 )
				{
					ops++;
					quadrature_timing_last[binno] += flipdistance[b];
					// This code will get appropriately executed every quadrature update.

					int qstate = quadrature_state[binno] = ( quadrature_state[binno] + 1 ) % 4;

					int last_q_bin = (binno * 2) + ( qstate & 1 );
					int delta = sample_accumulator - last_accumulated_value[last_q_bin];
					last_accumulated_value[last_q_bin] = sample_accumulator;
					
					if( binno == WATCHBIN )
						printf( "Delta: %d\n", delta );
					
					// Qstate = 
					//   (0) = +Cos, (1) = +Sin, (2) = -Cos, (3) = -Sin
					if( qstate & 2 ) delta *= -1;

					// Update real and imaginary components with delta.
					int running = real_imaginary_running[last_q_bin];
					running = running - (running>>RUNNING_IIR) + delta;
					real_imaginary_running[last_q_bin] = running;
					
					int q = ++qcount[binno];
					if( q == SAMPLE_Q ) // Effective Q factor.
					{
						qcount[binno] = 0;
						int newmagR = real_imaginary_running[(binno * 2)];
						int newmagI = real_imaginary_running[(binno * 2)+1];

						real_imaginary_running[(binno * 2)] = newmagR - (newmagR>>COMPLEX_IIR);
						real_imaginary_running[(binno * 2)+1] = newmagI - (newmagI>>COMPLEX_IIR);

						// Super-cheap, non-multiply, approximate complex vector magnitude calculation.
						newmagR = (newmagR<0)?-newmagR:newmagR;
						newmagI = (newmagI<0)?-newmagI:newmagI;
						int newmag = 
							//sqrt(newmagR*newmagR + newmagI*newmagI );
							newmagR > newmagI ? newmagR + (newmagI>>1) : newmagI + (newmagR>>1);

						int lastmag = magsum[binno];
						magsum[binno] = lastmag - (lastmag>>MAG_IIR) + newmag;
					}
				}
			}
		}
		
		int lx, ly;
		for( i = 0; i < BPERO*OCTAVES; i++ )
		{
			CNFGColor( (EHSVtoHEX( (i * 256 / BPERO)&0xff, 255, 255 ) << 8) | 0xff );
			float real = real_imaginary_running[i*2+0];
			float imag = real_imaginary_running[i*2+1];
			float mag = sqrt( real * real + imag * imag );
			
			mag = (float)magsum[i] * pow( 2, i / (double)BPERO );
			int y = 768 - ((int)(mag / FSPS * 10) >> MAG_IIR);
			if( i ) CNFGTackSegment( i*4, y, lx*4, ly );
			lx = i; ly= y;
			//printf( "%d %d\n", real_imaginary_running[i*2+0], real_imaginary_running[i*2+1] );
		}
		
		CNFGSwapBuffers();		
	}
}





void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { lastx = x; lasty = y; }
void HandleDestroy() { }


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

