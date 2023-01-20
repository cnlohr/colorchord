/*
	An experiment in very, very low-spec ColorChord.  This technique foregoes
	multiplies.

	Approach 2:

	Similar approach to Approach 1, in that this uses square waves and quarter
	wavelength segments to quadrature encode, but instead of using an octave
	at a time, it instead creates a heap to work through every sample.

	That way, the error induced by sample stutter is minimized and the square
	waves are as accurate as possible.

	WARNING: With this approach, operations can 'bunch up' so that you may
	need to clear many, many ops in a single cycle, so it is not at all
	appropirate for being run in an interrupt.

	Another benefit:  If sample rate is large, no time is spent working on
	samples that don't need work.  This is better for a sparse set of ops.


	TODO: Can we do this approach, but with a fixed table to instruct when to
	perform every bin?

	GENERAL OBSERVATION FOR ALL VERSIONS: (applicableto all) If we integrate
		only bumps for sin/cos, it seems to have different noise properties.
		May be beneficial!
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

#define FSPS  12000
#define OCTAVES 6
#define BPERO 24
#define BASE_FREQ 22.5
#define QUADRATURE_STEP_DENOMINATOR 16384

// Careful:  It makes a lot of sense to explore these relationships.
#define SAMPLE_Q 4
#define MAG_IIR 0
#define COMPLEX_IIR 2
#define RUNNING_IIR 31

#define TEST_SAMPLES 256

int32_t next_heap_events[OCTAVES*BPERO*2] = { 0 };

int sineshape;
// This will sort the head node back down into the heap, so the heap will
// remain a min-heap.  This is done in log(n) time.  But, with our data, it
// experimentally only needs to run for 6.47 iterations per call on average
// assuming 24 BPERO and 6 OCTAVES.

int heapsteps = 0;
int reheaps = 0;
void PercolateHeap( int32_t current_time )
{
	reheaps++;
	int this_index = 0;
	int this_val = next_heap_events[0];
	do
	{
		heapsteps++;
		int left = (this_index * 2 + 1);
		int right = (this_index * 2 + 2);

		// At end.  WARNING: This heap algorithm is only useful if it's an even number of things.
		if( right >= OCTAVES*BPERO ) return;

		int leftval = next_heap_events[left*2];
		int rightval = next_heap_events[right*2];
		int diffleft  = leftval - this_val;
		int diffright  = rightval - this_val;
		//printf( "RESORT ID %d / INDEX %d / [%d %d] %d %d %d %d\n", next_heap_events[this_index*2+1], this_index, diffleft, diffright, leftval, rightval, left, right ); 
		if( diffleft > 0 && diffright > 0 )
		{
			// The heap is sorted.  We're done.
			return;
		}

		// Otherwise we have to pick an edge to sort on.
		if( diffleft <= diffright )
		{
			//printf( "LEFT %d / %d\n", left, this_val );
			int swapevent = next_heap_events[left*2+1];
			next_heap_events[left*2+1] = next_heap_events[this_index*2+1];
			next_heap_events[this_index*2+1] = swapevent;

			next_heap_events[this_index*2+0] = leftval;
			next_heap_events[left*2+0] = this_val;

			this_index = left;
		}
		else
		{
			//printf( "RIGHT %d\n", right );

			int swapevent = next_heap_events[right*2+1];
			next_heap_events[right*2+1] = next_heap_events[this_index*2+1];
			next_heap_events[this_index*2+1] = swapevent;

			next_heap_events[this_index*2+0] = rightval;
			next_heap_events[right*2+0] = this_val;

			this_index = right;
		}
	} while( 1 );
}


int main()
{


	int16_t samples[TEST_SAMPLES];
	int i;


	CNFGSetup( "Example App", 1024, 768 );

	// Make a running counter to count up by this amount every cycle.
	// If the new number > 2048, then perform a quadrature step.
	int32_t flipdistance[BPERO*OCTAVES];
	for( i = 0; i < BPERO*OCTAVES; i++ )
	{
		double freq = pow( 2, (double)i / (double)BPERO ) * (BASE_FREQ/2.0);
		double pfreq = freq;
		double spacing = (FSPS / 2) / pfreq / 4;
		
		flipdistance[i] = QUADRATURE_STEP_DENOMINATOR * spacing;
		// Spacing = "quadrature every X samples"

		next_heap_events[i*2+1] = i;
	}

	// This is for timing.  Not accumulated data.
	uint8_t quadrature_state[BPERO*OCTAVES] = { 0 };
	
	uint32_t last_accumulated_value[BPERO*OCTAVES*2] = { 0 };
	
	int32_t real_imaginary_running[BPERO*OCTAVES*2] = { 0 };
	
	uint32_t sample_accumulator = 0;
	int32_t  time_accumulator = 0;
	
	int32_t qcount[BPERO*OCTAVES] = { 0 };
	int32_t magsum[BPERO*OCTAVES] = { 0 };

	int frameno = 0;
	double dLT = OGGetAbsoluteTime();

	double ToneOmega = 0;
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
		sprintf( cts, "%f %d %f %f SINESHAPE: %d", freq, time_accumulator, (double)heapsteps / (double)reheaps, (double)reheaps/(time_accumulator/QUADRATURE_STEP_DENOMINATOR), sineshape );
		CNFGColor( 0xffffffff );
		CNFGPenX = 2;
		CNFGPenY = 2;
		CNFGDrawText( cts, 2 );
		
		while( OGGetAbsoluteTime() < dLT + TEST_SAMPLES / (double)FSPS );
		dLT += TEST_SAMPLES / (double)FSPS;
		

		#define WATCHBIN -1

		for( i = 0; i < TEST_SAMPLES; i++ )
		{
			sample_accumulator += samples[i];

			time_accumulator += QUADRATURE_STEP_DENOMINATOR;

			while( (time_accumulator - next_heap_events[0]) > 0 )
			{
				// Event has occurred.
				int binno = next_heap_events[1];
				//printf( "%d %d\n", binno, next_heap_events[0] );
				next_heap_events[0] += flipdistance[binno];
				PercolateHeap( time_accumulator );

				//int j;
				//for( j = 0; j < OCTAVES*BPERO; j++ ) printf( "[%d %d]", next_heap_events[j*2+0], next_heap_events[j*2+1] );
				//printf( "\n" );
	
				int qstate = quadrature_state[binno] = ( quadrature_state[binno] + 1 ) % 4;

				int last_q_bin = (binno * 2) + ( qstate & 1 );
				int delta;
				if( !sineshape )
				{
					delta = sample_accumulator - last_accumulated_value[last_q_bin];
					last_accumulated_value[last_q_bin] = sample_accumulator;
				}
				else
				{
					// TESTING: Sine Shape - this only integrates bumps for sin/cos
					// instead of full triangle waves.
					// Observation: For higher frequency bins, this seems to help
					// a lot. 
					// side-benefit, this takes less RAM.
					// BUT BUT! It messes with lower frequencies, making them uglier.
					delta = sample_accumulator - last_accumulated_value[binno];
					last_accumulated_value[binno] = sample_accumulator;
					delta *= 1.4;  // Just to normalize the results of the test (not for production)
				}				
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





void HandleKey( int keycode, int bDown ) { if( keycode  == 'a' && bDown ) sineshape = !sineshape; }
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

