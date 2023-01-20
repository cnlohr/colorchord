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
int binmode = 0;
int additionaltone = 0;
int lastx = 200;
int lasty = 1000;

// Keep notimes as small as possible
static int timul( int adder, int notimes )
{
	int sum = 0;
	while( notimes )
	{
		if ( notimes & 1) sum += adder;
		adder <<= 1;
		notimes >>= 1;
	}
	return sum;
}

int main()
{
	#define FSPS  22100  // Careful, this matters!  It is selected to avoid periodic peaks encountered with DC offsets.
	#define OCTAVES 6
	#define BPERO 32
	#define BASE_FREQ 22.5
	#define QUADRATURE_STEP_DENOMINATOR 16384
	
	// This ultimately determines smoothness + q.
	#define COMPLEX_IIR 3

	#define TEST_SAMPLES 1024


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


	struct bindata
	{
		int32_t quadrature_timing_last;
		uint32_t last_accumulated_value[2];
		int32_t real_imaginary_running[2];
		uint32_t magsum;
		uint8_t quadrature_state;
		uint8_t reserved1, reserved2, reserved3;
	} bins[BPERO*OCTAVES] = { 0 };
	// This is for timing.  Not accumulated data.

	uint32_t octave_timing[OCTAVES] = { 0 };

	uint32_t sample_accumulator = 0;
	
	int frameno = 0;
	double dLT = OGGetAbsoluteTime();
	int samplenoIn = 0;
	int sampleno = 0;
	double ToneOmega = 0;
	double ToneOmega2 = 0;
	int ops;
	while( CNFGHandleInput() )
	{
		short w, h;
		CNFGGetDimensions( &w, &h );
		CNFGClearFrame();
		
		frameno++;
		float freq = 
			//pow( 2, (frameno%600)/100.0 ) * 25;
			pow( 2, (lastx)/100.0 ) * 22.5;
			//101;
		float freq2 = 
			//pow( 2, (frameno%600)/100.0 ) * 25;
			pow( 2, (200)/100.0 ) * 22.5;
			//101;
			
		for( i = 0; i < TEST_SAMPLES; i++ )
		{
			samples[i] = lasty/5 + sin( ToneOmega ) * 127 + (additionaltone?(sin(ToneOmega2)*128):0);// + (rand()%128)-64;
			ToneOmega += 1 / (double)FSPS * (double)freq * 3.14159 * 2.0;
			ToneOmega2 += 1 / (double)FSPS * (double)freq2 * 3.14159 * 2.0;
		}
		char cts[1024];
		sprintf( cts, "%f %d %f binmode: %d", freq, sampleno, ops/(double)sampleno, binmode );
		CNFGColor( 0xffffffff );
		CNFGPenX = 2;
		CNFGPenY = 2;
		CNFGDrawText( cts, 2 );
		
		while( OGGetAbsoluteTime() < dLT + TEST_SAMPLES / (double)FSPS );
		dLT += TEST_SAMPLES / (double)FSPS;

		
		for( i = 0; i < TEST_SAMPLES; i++ )
		{
			sample_accumulator += samples[i];
			int octave = whichoctave[(sampleno)&((2<<OCTAVES)-1)];
			sampleno++;
			if( octave < 0 )
			{
				// A "free cycle" this happens every 1/(2^octaves)
				continue;
			}
			
			#define WATCHBIN 1
			int b;
			int binno = octave * BPERO;
			int ocative_time = octave_timing[octave] += QUADRATURE_STEP_DENOMINATOR;
			for( b = 0; b < BPERO; b++, binno++ )
			{
				ops+=5;
				struct bindata * thisbin = &bins[binno];
				if( ocative_time - thisbin->quadrature_timing_last > 0 )
				{
					ops+=20;
					thisbin->quadrature_timing_last += flipdistance[b];

					// This code will get appropriately executed every quadrature update.

					int qstate = thisbin->quadrature_state = ( thisbin->quadrature_state + 1 ) % 4;

					int last_q_bin = ( qstate & 1 );

					//int delta = sample_accumulator - last_accumulated_value[binmode?(binno*2):last_q_bin];
					//last_accumulated_value[binmode?(binno*2):last_q_bin] = sample_accumulator * (binmode?1.8:1.0);

					int delta = sample_accumulator - thisbin->last_accumulated_value[last_q_bin];
					thisbin->last_accumulated_value[last_q_bin] = sample_accumulator;
					
					// Qstate = 
					//   (0) = +Cos, (1) = +Sin, (2) = -Cos, (3) = -Sin
					if( qstate & 2 ) delta *= -1;

					// Update real and imaginary components with delta.
					int running = thisbin->real_imaginary_running[last_q_bin];
					running = running + delta;
					thisbin->real_imaginary_running[last_q_bin] = running;
					
					if( qstate == 0 )
					{
						ops+=20;
						int newmagR = thisbin->real_imaginary_running[0];
						int newmagI = thisbin->real_imaginary_running[1];

						thisbin->real_imaginary_running[0] = newmagR - (newmagR>>COMPLEX_IIR);
						thisbin->real_imaginary_running[1] = newmagI - (newmagI>>COMPLEX_IIR);

						// Super-cheap, non-multiply, approximate complex vector magnitude calculation.
						newmagR = (newmagR<0)?-newmagR:newmagR;
						newmagI = (newmagI<0)?-newmagI:newmagI;
						int newmag = 
							//sqrt(newmagR*newmagR + newmagI*newmagI );
							newmagR > newmagI ? newmagR + (newmagI>>1) : newmagI + (newmagR>>1);
						thisbin->magsum = newmag >> (OCTAVES-octave);
					}
				}
			}
		}

		int folded_bins[BPERO];

		// Taper and fold (happens when we want to update lights)
		for( i = 0; i < BPERO; i++ )
		{
			int b = i;
			int running;
			running = bins[b].magsum * i / BPERO;
			b+=BPERO;
			int j;
			for( j = 1; j < OCTAVES-1; j++ )
			{
				running += bins[b].magsum;
				b+=BPERO;
			}
			running += bins[b].magsum * ( BPERO - i ) / BPERO;
			folded_bins[i] = running * pow( 2, (i%BPERO) / (double)BPERO );  //XXX TODO: Make this fixed.
		}

		int lx, ly;
#if 0
		// Boxcar filter 
		int fuzzed_bins[BPERO];
		int fuzzed_bins2[BPERO];

		// Taper and fold (happens when we want to update lights)
		for( i = 0; i < BPERO; i++ )
		{
			fuzzed_bins[i] = (folded_bins[i] + (folded_bins[(i+1)%BPERO]>>0) + (folded_bins[(i+BPERO-1)%BPERO]>>0))>>1;
		}

		// Taper and fold (happens when we want to update lights)
		for( i = 0; i < BPERO; i++ )
		{
			fuzzed_bins2[i] = (fuzzed_bins[i] + (folded_bins[(i+1)%BPERO]>>0) + (fuzzed_bins[(i+BPERO-1)%BPERO]>>0))>>1;
		}

		// Filter agian.
		for( i = 0; i < BPERO; i++ )
		{
			fuzzed_bins[i] = (fuzzed_bins2[i] + (fuzzed_bins2[(i+1)%BPERO]>>0) + (fuzzed_bins2[(i+BPERO-1)%BPERO]>>0))>>1;
		}

		// Taper and fold (happens when we want to update lights)
		for( i = 0; i < BPERO; i++ )
		{
			fuzzed_bins2[i] = (fuzzed_bins[i] + (folded_bins[(i+1)%BPERO]>>0) + (fuzzed_bins[(i+BPERO-1)%BPERO]>>0))>>1;

			fuzzed_bins2[i] = fuzzed_bins2[i]>>1;
		}

#else
		int fuzzed_bins2[BPERO];
		int fziir = folded_bins[i];
		// Forward IIR
		int j;

		#define FUZZ_IIR 2
		for( j = 0; j < 4; j++ )
		for( i = 0; i < BPERO; i++ )
		{
			// Initiate IIR
			fziir = fziir - (fziir>>FUZZ_IIR) + folded_bins[i];
		}
		for( i = 0; i < BPERO; i++ )
		{
			fziir = fziir - (fziir>>FUZZ_IIR) + folded_bins[i];;
			fuzzed_bins2[i] = fziir;
		}

		// reverse IIR.
		for( j = 0; j < 4; j++ )
		for( i = BPERO-1; i >= 0; i-- )
		{
			// Initiate IIR
			fziir = fziir - (fziir>>FUZZ_IIR) + fuzzed_bins2[i];
		}
		for( i = BPERO-1; i >= 0; i-- )
		{
			fziir = fziir - (fziir>>FUZZ_IIR) + fuzzed_bins2[i];
			fuzzed_bins2[i] = fziir;
		}

		int minfuzz = fziir;
		for( i = 0; i < BPERO; i++ )
		{
			if( minfuzz > fuzzed_bins2[i] ) minfuzz = fuzzed_bins2[i];
		}
		for( i = 0; i < BPERO; i++ )
		{
			fuzzed_bins2[i] = ( fuzzed_bins2[i] - minfuzz ) >> FUZZ_IIR;
		}
#endif
		for( i = 0; i < BPERO*OCTAVES; i++ )
		{
			CNFGColor( (EHSVtoHEX( (i * 256 / BPERO)&0xff, 255, 255 ) << 8) | 0xff );
			float mag = (float)bins[i].magsum * pow( 2, (i%BPERO) / (double)BPERO );
			int y = 768 - mag/20;
			if( i ) CNFGTackSegment( i*4, y, lx*4, ly );
			lx = i; ly= y;
		}


		for( i = 0; i < BPERO*2; i++ )
		{
			CNFGColor( (EHSVtoHEX( (i * 256 / BPERO)&0xff, 255, 255 ) << 8) | 0xff );
			float mag = (float)folded_bins[i%BPERO];
			int y = 400 - mag/100;
			if( i ) CNFGTackSegment( i*8, y, lx*8, ly );
			lx = i; ly= y;
		}

		for( i = 0; i < BPERO*2; i++ )
		{
			CNFGColor( (EHSVtoHEX( (i * 256 / BPERO)&0xff, 255, 255 ) << 8) | 0xff );
			float mag = (float)fuzzed_bins2[i%BPERO];
			int y = 500 - mag/100;
			if( i ) CNFGTackSegment( i*8, y, lx*8, ly );
			lx = i; ly= y;
		}


		// Fol

		
		CNFGSwapBuffers();		
	}
}





void HandleKey( int keycode, int bDown ) { if( bDown ) { if( keycode == 'a' ) binmode =!binmode; if( keycode == 'b' ) additionaltone = ! additionaltone; } }
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

