// Copyright 2015-2020 <>< Charles Lohr under the ColorChord License.

#if defined( WINDOWS ) || defined( USE_WINDOWS ) || defined( WIN32 ) || defined( WIN64 ) || \
	defined( _WIN32 ) || defined( _WIN64 )
#ifdef TCC
#include <winsock2.h> 
#endif
#ifndef strdup
#define strdup _strdup
#endif
#endif 

#include "color.h"
#include "configs.h"
#include "decompose.h"
#include "dft.h"
#include "filter.h"
#include "hook.h"
#include "notefinder.h"
#include "os_generic.h"
#include "outdrivers.h"
#include "parameters.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define CNFG_IMPLEMENTATION
#include "CNFG.h"

#define CNFA_IMPLEMENTATION
#include "CNFA.h"


// Sound driver.
struct CNFADriver *sd;

int bQuitColorChord = 0;


#ifdef ANDROID
#include <android/log.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void HandleDestroy()
{
	bQuitColorChord = 1;
	CNFAClose( sd );
}


#endif


#define GENLINEWIDTH 89
#define GENLINES     67

char genlog[ ( GENLINEWIDTH + 1 ) * ( GENLINES + 1 ) + 2 ] = "log";
int genloglen;
int genloglines;
int genlinelen   = 0;
int firstnewline = -1;


// Define application colors RGBA format
#define BACKGROUND_COLOR 0x000080ff
#define LINE_COLOR 0xffffffff
#define TEXT_COLOR 0xffffffff
// Text colors for the debug options at the bottom of the screen
#define ENABLED_COLOR 0xffffffff
#define DISABLED_COLOR 0x800000ff

void example_log_function( int readSize, char *buf )
{
	static og_mutex_t *mt;
	if ( !mt ) mt = OGCreateMutex();
	OGLockMutex( mt );
	for ( int i = 0; readSize && i <= readSize && buf[ i ]; i++ )
	{
		char c = buf[ i ];
		if ( c == '\0' ) c = '\n';
		if ( ( c != '\n' && genlinelen >= GENLINEWIDTH ) || c == '\n' )
		{
			genloglines++;
			if ( genloglines >= GENLINES )
			{
				genloglen   -= firstnewline + 1;
				int offset   = firstnewline;
				firstnewline = -1;
				int k;
				for ( k = 0; k < genloglen; k++ )
				{
					if ( ( genlog[ k ] = genlog[ k + offset + 1 ] ) == '\n' && firstnewline < 0 )
						firstnewline = k;
				}
				genlog[ k ] = 0;
				genloglines--;
			}
			genlinelen = 0;
			if ( c != '\n' )
			{
				genlog[ genloglen + 1 ] = 0;
				genlog[ genloglen++ ]   = '\n';
			}
			if ( firstnewline < 0 ) firstnewline = genloglen;
		}
		genlog[ genloglen + 1 ] = 0;
		genlog[ genloglen++ ]   = c;
		if ( c != '\n' ) genlinelen++;
	}

	OGUnlockMutex( mt );
}


#if defined( WIN32 ) || defined( USE_WINDOWS )
#define ESCAPE_KEY 0x1B

void HandleDestroy()
{
	CNFAClose( sd );
}
#else
#define ESCAPE_KEY 65307
// Stub function for Linux
void HandleDestroy()
{
}

#endif

float DeltaFrameTime = 0;
double Now           = 0;
int is_suspended     = 0;
int lastfps;
short screenx, screeny;

struct DriverInstances *outdriver[ MAX_OUT_DRIVERS ];
int headless    = 0;                  REGISTER_PARAM( headless, PAINT );
int set_screenx = 640;                REGISTER_PARAM( set_screenx, PAINT );
int set_screeny = 480;                REGISTER_PARAM( set_screeny, PAINT );
char sound_source[ 16 ];              REGISTER_PARAM( sound_source, PABUFFER );
int cpu_autolimit = 1;                REGISTER_PARAM( cpu_autolimit, PAINT );
float cpu_autolimit_interval = 0.016; REGISTER_PARAM( cpu_autolimit_interval, PAFLOAT );
int sample_channel = -1;              REGISTER_PARAM( sample_channel, PAINT );
int showfps = 1;                      REGISTER_PARAM( showfps, PAINT );

#if defined( ANDROID ) || defined( __android__ )
float in_amplitude = 2;
#else
float in_amplitude = 1;
#endif
REGISTER_PARAM( in_amplitude, PAFLOAT );

struct NoteFinder *nf;

// Sound circular buffer
#define SOUNDCBSIZE  8096
#define MAX_CHANNELS 2

double VisTimeEnd, VisTimeStart;
float sound[ SOUNDCBSIZE ];
int soundhead        = 0;
int show_debug       = 0;
int show_debug_basic = 1;

int gKey = 0;
extern int force_white;

void RecalcBaseHz()
{
	nf->base_hz = 55 * pow( 2, gKey / 12.0 );
	ChangeNFParameters( nf );
}

void HandleKey( int keycode, int bDown )
{
	char c = toupper( keycode );
#ifdef ANDROID
	if ( keycode == 4 && bDown )
	{
		// Back button.
		printf( "Back button pressed\n" );
		AndroidSendToBack( 0 );
		return;
	}
#endif
	if( keycode == ESCAPE_KEY ) exit( 0 );
	if( c == 'W' )              force_white = bDown;
	if( c == 'D' && bDown )     show_debug = !show_debug;
	if( c == '9' && bDown )     { gKey--;   RecalcBaseHz(); }
	if( c == '-' && bDown )     { gKey++;   RecalcBaseHz(); }
	if( c == '0' && bDown )     { gKey = 0; RecalcBaseHz(); }
	if( c == 'E' && bDown )     show_debug_basic = !show_debug_basic;
	if( c == 'K' && bDown )     DumpParameters();
	printf( "Key: %d -> %d\n", keycode, bDown );
	KeyHappened( keycode, bDown );
}

// On Android we want a really basic GUI
void HandleButton( int x, int y, int button, int bDown )
{
	printf( "Button: %d,%d (%d) -> %d\n", x, y, button, bDown );
	if ( bDown )
	{
		if ( y < 800 )
		{
			if ( x < screenx / 3 )
				gKey--;
			else if ( x < ( screenx * 2 / 3 ) )
				gKey = 0;
			else
				gKey++;
			printf( "KEY: %d\n", gKey );
			RecalcBaseHz();
		}
	}
}

void HandleMotion( int x, int y, int mask )
{
}

void SoundCB( struct CNFADriver *sd, short *out, short *in, int framesp, int framesr )
{
	int channelin = sd->channelsRec;
	int channelout = sd->channelsPlay;

	// Load the samples into a ring buffer.  Split the channels from interleved to one per buffer.
	if ( in )
	{
		for ( int i = 0; i < framesr; i++ )
		{
			if ( sample_channel < 0 )
			{
				float fo = 0;
				for ( int j = 0; j < channelin; j++ )
				{
					float f = in[ i * channelin + j ] / 32767.;
					if ( f >= -1 && f <= 1 )
						fo += f;
					else
						fo += ( f > 0 ) ? 1 : -1;
				}
				fo /= channelin;
				sound[ soundhead ] = fo * in_amplitude;
				soundhead = ( soundhead + 1 ) % SOUNDCBSIZE;
			}
			else
			{
				float f = in[ i * channelin + sample_channel ] / 32767.;
				if ( f > 1 || f < -1 ) f = ( f > 0 ) ? 1 : -1;
				sound[ soundhead ] = f * in_amplitude;
				soundhead = ( soundhead + 1 ) % SOUNDCBSIZE;
			}
		}
		SoundEventHappened( framesr, in, 0, channelin );
	}


	if ( out )
	{
		memset( out, 0, framesp * channelout );
		SoundEventHappened( framesp, out, 1, channelout );
	}
}

#ifdef ANDROID
void HandleSuspend()
{
	is_suspended = 1;
}

void HandleResume()
{
	is_suspended = 0;
}
#endif

// function for calling initilization functions if we are using TCC
#ifdef TCC
void RegisterConstructorFunctions()
{

	// Basic Window stuff
	REGISTERheadless();
	REGISTERset_screenx();
	REGISTERset_screeny();
	REGISTERsound_source();
	REGISTERcpu_autolimit();
	REGISTERcpu_autolimit_interval();
	REGISTERsample_channel();
	REGISTERshowfps();
	REGISTERin_amplitude();

	// Audio stuff
	REGISTERNullCNFA();
	REGISTERWinCNFA();
	REGISTERcnfa_wasapi();

	// Video Stuff
	REGISTERnull();
	REGISTERDisplayArray();
	// REGISTERDisplayDMX();
	// REGISTERDisplayFileWrite();
	REGISTERDisplayHIDAPI();
	REGISTERDisplayNetwork();
	REGISTERDisplayOutDriver();
	REGISTERDisplayPie();
	// REGISTERDisplaySHM();

	// Output stuff
	// REGISTERDisplayUSB2812();
	REGISTEROutputCells();
	REGISTEROutputLinear();
	REGISTEROutputProminent();
	REGISTEROutputVoronoi();
}
#endif

int main( int argc, char **argv )
{
#ifdef TCC
	RegisterConstructorFunctions();
#endif


	printf( "Output Drivers:\n" );
	for ( int i = 0; i < MAX_OUT_DRIVERS; i++ )
	{
		if ( ODList[ i ].Name ) printf( "\t%s\n", ODList[ i ].Name );
	}

#if defined( WIN32 ) || defined( USE_WINDOWS )
	// In case something needs network access.
	WSADATA wsaData;
	WSAStartup( 0x202, &wsaData );
#elif defined( ANDROID )
	int hasperm = AndroidHasPermissions( "READ_EXTERNAL_STORAGE" );
	int haspermInternet = AndroidHasPermissions( "INTERNET" );
	if ( !hasperm ) AndroidRequestAppPermissions( "READ_EXTERNAL_STORAGE" );
	if ( !haspermInternet ) AndroidRequestAppPermissions( "INTERNET" );
#else
	// Linux
#endif

	gargc = argc;
	gargv = argv;

	SetupConfigs();

	// Initialize Rawdraw
	int frames = 0;
	double ThisTime;
	double SecToWait;
	double LastFPSTime = OGGetAbsoluteTime();
	double LastFrameTime = OGGetAbsoluteTime();
	CNFGBGColor = BACKGROUND_COLOR;

	// Generate the window title
	char title[ 1024 ];
	strcpy( title, "Colorchord " );
	for ( int i = 1; i < argc; i++ )
	{
		strcat( title, argv[ i ] );
		strcat( title, " " );
	}
	if ( !headless ) CNFGSetup( title, set_screenx, set_screeny );

	char *OutDriverNames = strdup( GetParameterS( "outdrivers", "null" ) );
	char *ThisDriver = OutDriverNames;
	char *TDStart;
	for ( int i = 0; i < MAX_OUT_DRIVERS; i++ )
	{
		while ( *ThisDriver == ' ' || *ThisDriver == '\t' ) ThisDriver++;
		if ( !*ThisDriver ) break;

		TDStart = ThisDriver;

		while ( *ThisDriver != 0 && *ThisDriver != ',' )
		{
			if ( *ThisDriver == '\t' || *ThisDriver == ' ' ) *ThisDriver = 0;
			ThisDriver++;
		}

		if ( *ThisDriver )
		{
			*ThisDriver = 0;
			ThisDriver++;
		}

		printf( "Loading: %s\n", TDStart );
		outdriver[ i ] = SetupOutDriver( TDStart );
	}
	free( OutDriverNames );


	do {
		// Initialize Sound
		sd = CNFAInit( sound_source, "colorchord", &SoundCB, GetParameterI( "samplerate", 44100 ),
			GetParameterI( "samplerate", 44100 ), GetParameterI( "channels", 2 ),
			GetParameterI( "channels", 2 ), GetParameterI( "buffer", 1024 ),
			GetParameterS( "devplay", 0 ), GetParameterS( "devrecord", 0 ), NULL );

		if ( sd ) break;

		CNFGColor( LINE_COLOR );
		CNFGPenX = 10;
		CNFGPenY = 100;
		CNFGHandleInput();
		CNFGClearFrame();
		CNFGDrawText( "Colorchord must be used with sound. Sound not available.", 10 );
		CNFGSwapBuffers();
		OGSleep( 1 );
	} while ( 1 );
	nf = CreateNoteFinder( sd->spsRec );

	// Once everything was reinitialized, re-read the ini files.
	SetEnvValues( 1 );

	printf( "================================================= Set Up\n" );

	Now = OGGetAbsoluteTime();
	double Last = Now;
	while ( !bQuitColorChord )
	{
		char stt[ 1024 ];
		// Handle Rawdraw frame swappign

		Now = OGGetAbsoluteTime();
		DeltaFrameTime = Now - Last;

		if ( !headless )
		{
			CNFGHandleInput();
			CNFGClearFrame();
			CNFGColor( LINE_COLOR );
			CNFGGetDimensions( &screenx, &screeny );
		}

		RunNoteFinder( nf, sound, ( soundhead - 1 + SOUNDCBSIZE ) % SOUNDCBSIZE, SOUNDCBSIZE );
		// Done all ColorChord work.


		VisTimeStart = OGGetAbsoluteTime();

		// call the output drivers with the updated note finder data
		for ( int i = 0; i < MAX_OUT_DRIVERS; i++ )
		{
			if ( force_white ) memset( OutLEDs, 0x7f, MAX_LEDS * 3 );
			if ( outdriver[ i ] ) outdriver[ i ]->Func( outdriver[ i ]->id, nf );
		}

		VisTimeEnd = OGGetAbsoluteTime();


		if ( !headless )
		{
			// Handle outputs.
			int freqbins = nf->freqbins;
			int note_peaks = freqbins / 2;
			int freqs = freqbins * nf->octaves;

			// Do a bunch of debugging.
			if ( show_debug_basic && !is_suspended )
			{
				CNFGColor( TEXT_COLOR );
				for ( int i = 0; i < nf->dists_count; i++ )
				{
					// Move over 0.5 for visual purposes.  The means is correct.
					CNFGPenX = ( nf->dists[ i ].mean + 0.5 ) / freqbins * screenx;
					CNFGPenY = 400 - nf->dists[ i ].amp * 150.0 / nf->dists[ i ].sigma;
					sprintf( stt, "%f\n%f\n", nf->dists[ i ].mean, nf->dists[ i ].amp );
					CNFGDrawText( stt, 2 );
				}

				CNFGColor( LINE_COLOR );
				// Draw the folded bins
				for ( int bin = 0; bin < freqbins; bin++ )
				{
					const float x0 = bin / (float)freqbins * (float)screenx;
					const float x1 = ( bin + 1 ) / (float)freqbins * (float)screenx;
					const float amp = nf->folded_bins[ bin ] * 250.0;
					const float note = (float)( bin + 0.5 ) / freqbins;
					CNFGDialogColor = CCtoHEX( note, 1.0, 1.0 );
					CNFGDrawBox( x0, 400 - amp, x1, 400 );
				}

				// Draw the note peaks
				for ( int peak = 0; peak < note_peaks; peak++ )
				{
					if ( nf->note_amplitudes_out[ peak ] < 0 ) continue;
					float note = (float)nf->note_positions[ peak ] / freqbins;
					CNFGDialogColor = CCtoHEX( note, 1.0, 1.0 );
					const int x1 = ( (float)peak / note_peaks ) * screenx;
					const int x2 = ( (float)( peak + 1 ) / note_peaks ) * screenx;
					const int y1 = 480 - nf->note_amplitudes_out[ peak ] * 100;
					const int y2 = 480;
					CNFGColor( LINE_COLOR );
					CNFGDrawBox( x1, y1, x2, y2 );

					CNFGPenX = ( (float)( peak + .4 ) / note_peaks ) * screenx;
					CNFGPenY = screeny - 30;
					sprintf( stt, "%d\n%0.0f", nf->enduring_note_id[ peak ],
						nf->note_amplitudes2[ peak ] * 1000.0 );

					CNFGColor( TEXT_COLOR );
					CNFGDrawText( stt, 2 );
				}

				CNFGColor( LINE_COLOR );
				// Let's draw the o-scope.
				int thissoundhead = ( soundhead - 1 + SOUNDCBSIZE ) % SOUNDCBSIZE;
				int lasty;
				int thisy = sound[ thissoundhead ] * -128 + 128;
				for ( int i = screenx - 1; i > 0; i-- )
				{
					lasty = thisy;
					thisy = sound[ thissoundhead ] * -128 + 128;
					thissoundhead = ( thissoundhead - 1 + SOUNDCBSIZE ) % SOUNDCBSIZE;
					CNFGTackSegment( i, lasty, i - 1, thisy );
				}
			}

			// Extra debugging?
			if ( show_debug && !is_suspended )
			{
				// Draw the histogram
				float lasthistval;
				CNFGColor( LINE_COLOR );
				for ( int x_val = -1; x_val < screenx; x_val++ )
				{
					// Calculate the value of the histogram at the current screen position
					float hist_point = (float)x_val / (float)screenx * freqbins - 0.5;
					float thishistval =
						CalcHistAt( hist_point, nf->freqbins, nf->dists, nf->dists_count );

					// Display the value on the screen
					const short y = 400 - lasthistval * 250.0;
					if ( x_val >= 0 ) CNFGTackSegment( x_val, y, x_val + 1, y );
					lasthistval = thishistval;
				}

				CNFGColor( LINE_COLOR );
				// Draw the bins
				for ( int bin = 0; bin < freqs; bin++ )
				{
					float x0 = bin / (float)freqs * (float)screenx;
					float x1 = ( bin + 1 ) / (float)freqs * (float)screenx;
					float amp = nf->outbins[ bin ] * 250.0;
					float note = (float)bin / freqbins;
					CNFGDialogColor = CCtoHEX( note, 1.0, 1.0 );
					CNFGDrawBox( x0, 0, x1, amp );
				}

				CNFGColor( TEXT_COLOR );
				char stdebug[ 1024 ];
				sprintf( stdebug, "DFT:%8.2fms\nFLT:%8.2f\nDEC:%8.2f\nFNL:%8.2f\nDPY:%8.2f",
					( nf->DFTTime - nf->StartTime ) * 1000, ( nf->FilterTime - nf->DFTTime ) * 1000,
					( nf->DecomposeTime - nf->FilterTime ) * 1000,
					( nf->FinalizeTime - nf->DecomposeTime ) * 1000,
					( VisTimeEnd - VisTimeStart ) * 1000 );
				CNFGPenX = 50;
				CNFGPenY = 50;
				CNFGDrawText( stdebug, 2 );
			}

			if ( !is_suspended )
			{
				CNFGColor( show_debug ? ENABLED_COLOR : DISABLED_COLOR );
				CNFGPenX = 0;
				CNFGPenY = screeny - 10;
				CNFGDrawText( "Extra Debug (D)", 2 );

				CNFGColor( show_debug_basic ? ENABLED_COLOR : DISABLED_COLOR );
				CNFGPenX = 120;
				CNFGPenY = screeny - 10;
				CNFGDrawText( "Basic Debug (E)", 2 );

				CNFGColor( show_debug_basic ? ENABLED_COLOR : DISABLED_COLOR );
				CNFGPenX = 240;
				CNFGPenY = screeny - 10;
				sprintf( stt, "[9] Key: %d [0] (%3.1f) [-]", gKey, nf->base_hz );
				CNFGDrawText( stt, 2 );

				CNFGColor( TEXT_COLOR );
				CNFGPenX = 440;
				CNFGPenY = screeny - 10;
				sprintf( stt, "FPS: %d", lastfps );
				CNFGDrawText( stt, 2 );

#ifdef ANDROID
				CNFGColor( TEXT_COLOR );
				CNFGPenX = 10;
				CNFGPenY = 600;
				CNFGDrawText( genlog, 3 );
#endif
				CNFGSwapBuffers();
			}
		}

		// Finish Rawdraw with FPS counter, and a nice delay loop.
		frames++;

		ThisTime = OGGetAbsoluteTime();
		if ( ThisTime > LastFPSTime + 1 && showfps )
		{
#ifndef ANDROID
			printf( "FPS: %d\n", frames );
#endif
			lastfps = frames;
			frames = 0;
			LastFPSTime += 1;
		}

		if ( cpu_autolimit )
		{
			SecToWait = cpu_autolimit_interval - ( ThisTime - LastFrameTime );
			LastFrameTime += cpu_autolimit_interval;
			if ( SecToWait < -.1 ) LastFrameTime = ThisTime - .1;
			if ( SecToWait > 0 ) OGUSleep( (int)( SecToWait * 1000000 ) );
		}

		if ( !is_suspended ) SetEnvValues( 0 );

		Last = Now;
	}
}
