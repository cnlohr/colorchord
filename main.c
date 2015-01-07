#include <ctype.h>
#include "color.h"
#include <math.h>
#include <stdio.h>
#include "sound.h"
#include "os_generic.h"
#include "DrawFunctions.h"
#include "dft.h"
#include "filter.h"
#include "decompose.h"
#include <stdlib.h>
#include <string.h>
#include "notefinder.h"
#include "outdrivers.h"
#include "parameters.h"

short screenx, screeny;
int gargc;
char ** gargv;

int set_screenx = 640;	REGISTER_PARAM( set_screenx, PINT );
int set_screeny = 480;	REGISTER_PARAM( set_screeny, PINT );
char sound_source[16]; 	REGISTER_PARAM( sound_source, PBUFFER );
int cpu_autolimit = 1; 	REGISTER_PARAM( cpu_autolimit, PINT );
int sample_channel = -1;REGISTER_PARAM( sample_channel, PINT );

struct NoteFinder * nf;

//Sound circular buffer
#define SOUNDCBSIZE 65536
#define MAX_CHANNELS 2

double VisTimeEnd, VisTimeStart;
int soundhead = 0;
float sound[SOUNDCBSIZE];
int show_debug = 0;
int show_debug_basic = 1;

int gKey = 0;

void HandleKey( int keycode, int bDown )
{
	char c = toupper( keycode );
	if( c == 'D' && bDown ) show_debug = !show_debug;
	if( c == '9' && bDown ) { gKey--; 		nf->base_hz = 55 * pow( 2, gKey / 12.0 ); ChangeNFParameters( nf ); }
	if( c == '-' && bDown ) { gKey++; 		nf->base_hz = 55 * pow( 2, gKey / 12.0 ); ChangeNFParameters( nf ); }
	if( c == '0' && bDown ) { gKey = 0;		nf->base_hz = 55 * pow( 2, gKey / 12.0 ); ChangeNFParameters( nf ); }
	if( c == 'E' && bDown ) show_debug_basic = !show_debug_basic;
	if( c == 'K' && bDown ) DumpParameters();
	if( keycode == 65307 ) exit( 0 );
	printf( "Key: %d -> %d\n", keycode, bDown );
}

void HandleButton( int x, int y, int button, int bDown )
{
	printf( "Button: %d,%d (%d) -> %d\n", x, y, button, bDown );
}

void HandleMotion( int x, int y, int mask )
{
}

void SoundCB( float * out, float * in, int samplesr, int * samplesp, struct SoundDriver * sd )
{
	int channelin = sd->channelsRec;
//	int channelout = sd->channelsPlay;

	//Load the samples into a ring buffer.  Split the channels from interleved to one per buffer.
	*samplesp = 0;

	int process_channels = (MAX_CHANNELS < channelin)?MAX_CHANNELS:channelin;

	int i;
	int j;
	for( i = 0; i < samplesr; i++ )
	{
		if( sample_channel < 0 )
		{
			float fo = 0;
			for( j = 0; j < process_channels; j++ )
			{
				float f = in[i*channelin+j];
				if( f < -1 || f > 1 ) continue;
				fo += f;
			}
			fo /= process_channels;
			sound[soundhead] = fo;
			soundhead = (soundhead+1)%SOUNDCBSIZE;
		
		}
		else
		{
			float f = in[i*channelin+sample_channel];
			if( f < -1 || f > 1 ) continue;
			sound[soundhead] = f;
			soundhead = (soundhead+1)%SOUNDCBSIZE;
		}
	}
}


void LoadFile( const char * filename )
{
	char * buffer;
	int r;
	int i;

	FILE * f = fopen( filename, "rb" );
	if( !f )
	{
		fprintf( stderr, "Warning: cannot open %s.\n", filename );
	}
	else
	{
		fseek( f, 0, SEEK_END );
		int size = ftell( f );
		fseek( f, 0, SEEK_SET );
		buffer = malloc( size + 1 );
		r = fread( buffer, size, 1, f );
		fclose( f );
		buffer[size] = 0;
		if( r != 1 )
		{
			fprintf( stderr, "Warning: %d bytes read.  Expected: %d from file %s\n", r, size, filename );
		}
		else
		{
			SetParametersFromString( buffer );
		}
		free( buffer );
	}


	if( gargc > 2 )
	{
		for( i = 2; i < gargc; i++ )
		{
			printf( "AP: %s\n", gargv[i] );
			SetParametersFromString( gargv[i] );
		}
	}
}

int main(int argc, char ** argv)
{
//	const char * OutDriver = "name=LEDOutDriver;leds=512;light_siding=1.9";
	const char * InitialFile = "default.conf";
	int i;
	double LastFileTime;

	gargc = argc;
	gargv = argv;

	if( argc > 1 )
	{
		InitialFile = argv[1];
	}

	{
		LastFileTime = OGGetFileTime( InitialFile );
		LoadFile( InitialFile );
	}


	//Initialize Rawdraw
	int frames = 0;
	double ThisTime;
	double LastFPSTime = OGGetAbsoluteTime();
	double LastFrameTime = OGGetAbsoluteTime();
	double SecToWait;
	CNFGBGColor = 0x800000;
	CNFGDialogColor = 0x444444;
	CNFGSetup( "ColorChord Test", set_screenx, set_screeny );
	struct DriverInstances * outdriver = SetupOutDriver( );

	//Initialize Sound
	struct SoundDriver * sd = InitSound( sound_source, &SoundCB );

	if( !sd )
	{
		fprintf( stderr, "ERROR: Failed to initialize sound output device\n" );
		return -1;
	}

	nf = CreateNoteFinder( sd->spsRec );


	while(1)
	{
		char stt[1024];
		//Handle Rawdraw frame swappign
		CNFGHandleInput();
		CNFGClearFrame();
		CNFGColor( 0xFFFFFF );
		CNFGGetDimensions( &screenx, &screeny );

		RunNoteFinder( nf, sound, (soundhead-1+SOUNDCBSIZE)%SOUNDCBSIZE, SOUNDCBSIZE );
		//Done all ColorChord work.

		VisTimeStart = OGGetAbsoluteTime();
		if( outdriver )
			outdriver->Func( outdriver->id, nf );
		VisTimeEnd = OGGetAbsoluteTime();

		//Handle outputs.
		int freqbins = nf->freqbins;
		int note_peaks = freqbins/2;
		int freqs = freqbins * nf->octaves;
		//int maxdists = freqbins/2;

		//Do a bunch of debugging.
		if( show_debug_basic )
		{
			for( i = 0; i < nf->dists; i++ )
			{
				CNFGPenX = (nf->dist_means[i] + 0.5) / freqbins * screenx;  //Move over 0.5 for visual purposes.  The means is correct.
				CNFGPenY = 400-nf->dist_amps[i] * 150.0 / nf->dist_sigmas[i];
				//printf( "%f %f\n", dist_means[i], dist_amps[i] );
				sprintf( stt, "%f\n%f\n", nf->dist_means[i], nf->dist_amps[i] );
				CNFGDrawText( stt, 2 );
			}
			CNFGColor( 0xffffff );

			//Draw the folded bins
			for( i = 0; i < freqbins; i++ )
			{
				float x0 = i / (float)freqbins * (float)screenx;
				float x1 = (i+1) / (float)freqbins * (float)screenx;
				float amp = nf->folded_bins[i] * 250.0;
				CNFGDialogColor = CCtoHEX( ((float)(i+0.5) / freqbins), 1.0, 1.0 );
				CNFGDrawBox( x0, 400-amp, x1, 400 );
			}
			CNFGDialogColor = 0xf0f000;

			for( i = 0; i < note_peaks; i++ )
			{
				//printf( "%f %f /", note_positions[i], note_amplitudes[i] );
				if( nf->note_amplitudes_out[i] < 0 ) continue;
				CNFGDialogColor = CCtoHEX( (nf->note_positions[i] / freqbins), 1.0, 1.0 );
				CNFGDrawBox( ((float)i / note_peaks) * screenx, 480 - nf->note_amplitudes_out[i] * 100, ((float)(i+1) / note_peaks) * screenx, 480 );
				CNFGPenX = ((float)(i+.4) / note_peaks) * screenx;
				CNFGPenY = screeny - 30;
				sprintf( stt, "%d\n%0.0f", nf->enduring_note_id[i], nf->note_amplitudes2[i]*1000.0 );
				CNFGDrawText( stt, 2 );

			}

			//Let's draw the o-scope.
			int thissoundhead = soundhead;
			thissoundhead = (thissoundhead-1+SOUNDCBSIZE)%SOUNDCBSIZE;
			int lasty = sound[thissoundhead] * 128 + 128; thissoundhead = (thissoundhead-1+SOUNDCBSIZE)%SOUNDCBSIZE;
			int thisy = sound[thissoundhead] * 128 + 128; thissoundhead = (thissoundhead-1+SOUNDCBSIZE)%SOUNDCBSIZE;
			for( i = 0; i < screenx; i++ )
			{
				CNFGTackSegment( i, lasty, i+1, thisy );
				lasty = thisy;
				thisy = sound[thissoundhead] * 128 + 128; thissoundhead = (thissoundhead-1+SOUNDCBSIZE)%SOUNDCBSIZE;
			}
		}

		//Extra debugging?
		if( show_debug )
		{
			//Draw the histogram
			float lasthistval;
			CNFGColor( 0xffffff );

			for( i = -1; i < screenx; i++ )
			{
				float thishistval = CalcHistAt( (float)i/(float)screenx*freqbins-0.5, nf->freqbins, nf->dist_means, nf->dist_amps, nf->dist_sigmas, nf->dists );
				if( i >= 0 )
					CNFGTackSegment( i, 400-lasthistval * 250.0, i+1, 400-thishistval * 250.0 );
				lasthistval = thishistval;
			}

			CNFGColor( 0xffffff );

			//Draw the bins
			for( i = 0; i < freqs; i++ )
			{
				float x0 = i / (float)freqs * (float)screenx;
				float x1 = (i+1) / (float)freqs * (float)screenx;
				float amp = nf->outbins[i] * 250.0;
				CNFGDialogColor = CCtoHEX( ((float)i / freqbins), 1.0, 1.0 );
				CNFGDrawBox( x0, 0, x1, amp );
			}
			CNFGDialogColor = 0x0f0f0f;

			char stdebug[1024];
			sprintf( stdebug, "DFT:%8.2fms\nFLT:%8.2f\nDEC:%8.2f\nFNL:%8.2f\nDPY:%8.2f",
				(nf->DFTTime - nf->StartTime)*1000,
				(nf->FilterTime - nf->DFTTime)*1000,
				(nf->DecomposeTime - nf->FilterTime)*1000,
				(nf->FinalizeTime - nf->DecomposeTime)*1000,
				(VisTimeEnd - VisTimeStart)*1000 );
			CNFGPenX = 50;
			CNFGPenY = 50;
			CNFGDrawText( stdebug, 2 );
		}

		CNFGColor( show_debug?0xffffff:0x000000 );
		CNFGPenX = 0; CNFGPenY = screeny-10;
		CNFGDrawText( "Extra Debug (D)", 2 );

		CNFGColor( show_debug_basic?0xffffff:0x000000 );
		CNFGPenX = 120; CNFGPenY = screeny-10;
		CNFGDrawText( "Basic Debug (E)", 2 );

		CNFGColor( show_debug_basic?0xffffff:0x000000 );
		CNFGPenX = 240; CNFGPenY = screeny-10;
		sprintf( stt, "[9] Key: %d [0] (%3.1f) [-]", gKey, nf->base_hz );
		CNFGDrawText( stt, 2 );

		//Finish Rawdraw with FPS counter, and a nice delay loop.
		frames++;
		CNFGSwapBuffers();
		ThisTime = OGGetAbsoluteTime();
		if( ThisTime > LastFPSTime + 1 )
		{
			printf( "FPS: %d\n", frames );
			frames = 0;
			LastFPSTime+=1;
		}

		if( cpu_autolimit )
		{
			SecToWait = .016 - ( ThisTime - LastFrameTime );
			LastFrameTime += .016;
			if( SecToWait < -.1 ) LastFrameTime = ThisTime - .1;
			if( SecToWait > 0 )
				OGUSleep( (int)( SecToWait * 1000000 ) );
		}

		if( OGGetFileTime( InitialFile ) != LastFileTime )
		{
			LastFileTime = OGGetFileTime( InitialFile );
			LoadFile( InitialFile );
		}

	}

}

