//Copyright 2015-2020 <>< Charles Lohr under the ColorChord License.

#if defined(WINDOWS) || defined(USE_WINDOWS)\
 || defined(WIN32)   || defined(WIN64)      \
 || defined(_WIN32)  || defined(_WIN64)
#include <winsock2.h>
#include <windows.h>
#ifndef strdup
#define strdup _strdup
#endif
#endif 

#include <ctype.h>
#include "color.h"
#include <math.h>
#include <stdio.h>
#include "os_generic.h"
#include "dft.h"
#include "filter.h"
#include "decompose.h"
#include <stdlib.h>
#include <string.h>
#include "notefinder.h"
#include "outdrivers.h"
#include "parameters.h"
#include "hook.h"
#include "configs.h"


#define CNFG_IMPLEMENTATION
#include "CNFG.h"

#define CNFA_IMPLEMENTATION
#include "CNFA.h"



//Sound driver.
struct CNFADriver * sd;

int bQuitColorChord = 0;


#ifdef ANDROID
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <android/log.h>
#include <pthread.h>

void HandleDestroy()
{
	bQuitColorChord = 1;
	CNFAClose( sd );
}


#endif


#define GENLINEWIDTH 89
#define GENLINES 67

int genlinelen = 0;
char genlog[(GENLINEWIDTH+1)*(GENLINES+1)+2] = "log";
int genloglen;
int genloglines;
int firstnewline = -1;

void example_log_function( int readSize, char * buf )
{
	static og_mutex_t * mt;
	if( !mt ) mt = OGCreateMutex();
	OGLockMutex( mt );
	int i;
	for( i = 0; (readSize>=0)?(i <= readSize):buf[i]; i++ )
	{
		char c = buf[i];
		if( c == '\0' ) c = '\n';
		if( ( c != '\n' && genlinelen >= GENLINEWIDTH ) || c == '\n' )
		{
			int k;
			genloglines++;
			if( genloglines >= GENLINES )
			{
				genloglen -= firstnewline+1;
				int offset = firstnewline;
				firstnewline = -1;

				for( k = 0; k < genloglen; k++ )
				{
					if( ( genlog[k] = genlog[k+offset+1] ) == '\n' && firstnewline < 0)
					{
						firstnewline = k;
					}
				}
				genlog[k] = 0;
				genloglines--;
			}
			genlinelen = 0;
			if( c != '\n' )
			{
				genlog[genloglen+1] = 0;
				genlog[genloglen++] = '\n';
			}
			if( firstnewline < 0 ) firstnewline = genloglen;
		}
		genlog[genloglen+1] = 0;
		genlog[genloglen++] = c;
		if( c != '\n' ) genlinelen++;
	}

	OGUnlockMutex( mt );
}




#if defined(WIN32) || defined(USE_WINDOWS)

#define ESCAPE_KEY 0x1B

void HandleDestroy()
{
	CNFAClose( sd );
}


#else

#define ESCAPE_KEY 65307

#endif

int is_suspended = 0;

float DeltaFrameTime = 0;
double Now = 0;

int lastfps;
short screenx, screeny;

struct DriverInstances * outdriver[MAX_OUT_DRIVERS];

int headless = 0;		REGISTER_PARAM( headless, PAINT );
int set_screenx = 640;	REGISTER_PARAM( set_screenx, PAINT );
int set_screeny = 480;	REGISTER_PARAM( set_screeny, PAINT );
char sound_source[16]; 	REGISTER_PARAM( sound_source, PABUFFER );
int cpu_autolimit = 1; 	REGISTER_PARAM( cpu_autolimit, PAINT );
float cpu_autolimit_interval = 0.016; REGISTER_PARAM( cpu_autolimit_interval, PAFLOAT );
int sample_channel = -1;			  REGISTER_PARAM( sample_channel, PAINT );
int showfps = 1;        			  REGISTER_PARAM( showfps, PAINT );

#if defined(ANDROID) || defined( __android__ )
float in_amplitude = 2;
#else
float in_amplitude = 1;
#endif
REGISTER_PARAM( in_amplitude, PAFLOAT );

struct NoteFinder * nf;

//Sound circular buffer
#define SOUNDCBSIZE 8096
#define MAX_CHANNELS 2

double VisTimeEnd, VisTimeStart;
int soundhead = 0;
float sound[SOUNDCBSIZE];
int show_debug = 0;
int show_debug_basic = 1;

int gKey = 0;
extern int force_white;

void RecalcBaseHz()
{
	nf->base_hz = 55 * pow( 2, gKey / 12.0 ); ChangeNFParameters( nf );
}

void HandleKey( int keycode, int bDown )
{
	char c = toupper( keycode );
#ifdef ANDROID
	if( keycode == 4 && bDown )
	{
		//Back button.
		printf( "Back button pressed\n" );
		AndroidSendToBack( 0 );
		return;
	}
#endif
	if( c == 'D' && bDown ) show_debug = !show_debug;
	if( c == 'W' ) force_white = bDown;
	if( c == '9' && bDown ) { gKey--; 		RecalcBaseHz(); }
	if( c == '-' && bDown ) { gKey++; 		RecalcBaseHz(); }
	if( c == '0' && bDown ) { gKey = 0;		RecalcBaseHz(); }
	if( c == 'E' && bDown ) show_debug_basic = !show_debug_basic;
	if( c == 'K' && bDown ) DumpParameters();
	if( keycode == ESCAPE_KEY ) exit( 0 );
	printf( "Key: %d -> %d\n", keycode, bDown );
	KeyHappened( keycode, bDown );
}

//On Android we want a really basic GUI
void HandleButton( int x, int y, int button, int bDown )
{
	printf( "Button: %d,%d (%d) -> %d\n", x, y, button, bDown );
	if( bDown )
	{
		if( y < 800 )
		{
			if( x < screenx/3 )
			{
				gKey --;
			}
			else if( x < (screenx*2/3) )
			{
				gKey = 0;
			}
			else
			{
				gKey++;
			}
			printf( "KEY: %d\n", gKey );
			RecalcBaseHz();
		}
	}
}

void HandleMotion( int x, int y, int mask )
{
}

void SoundCB( struct CNFADriver * sd, short * out, short * in, int framesp, int framesr )
{
	int channelin = sd->channelsRec;
	int channelout = sd->channelsPlay;
	//*samplesp = 0;
//	int process_channels = (MAX_CHANNELS < channelin)?MAX_CHANNELS:channelin;

	//Load the samples into a ring buffer.  Split the channels from interleved to one per buffer.

	int i;
	int j;

	if( in )
	{
		for( i = 0; i < framesr; i++ )
		{
			if( sample_channel < 0 )
			{
				float fo = 0;
				for( j = 0; j < channelin; j++ )
				{
					float f = in[i*channelin+j] / 32767.;
					if( f >= -1 && f <= 1 )
					{
						fo += f;
					}
					else
					{
						fo += (f>0)?1:-1;
	//					printf( "Sound fault A %d/%d %d/%d %f\n", j, channelin, i, samplesr, f );
					}
				}
				fo /= channelin;
				sound[soundhead] = fo*in_amplitude;
				soundhead = (soundhead+1)%SOUNDCBSIZE;
			}
			else
			{
				float f = in[i*channelin+sample_channel] / 32767.;

				if( f > 1 || f < -1 )
				{ 	
					f = (f>0)?1:-1;
				}


				//printf( "Sound fault B %d/%d\n", i, samplesr );
				sound[soundhead] = f*in_amplitude;
				soundhead = (soundhead+1)%SOUNDCBSIZE;

			}
		}
		SoundEventHappened( framesr, in, 0, channelin );
	}


	if( out )
	{
		for( j = 0; j < framesp * channelout; j++ )
		{
			out[j] = 0;
		}
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
void RegisterConstructorFunctions(){

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
	//REGISTERDisplayDMX();
	//REGISTERDisplayFileWrite();
	REGISTERDisplayHIDAPI();
	REGISTERDisplayNetwork();
	REGISTERDisplayOutDriver();
	REGISTERDisplayPie();
	//REGISTERDisplaySHM();

	// Output stuff
	//REGISTERDisplayUSB2812();
	REGISTEROutputCells();
	REGISTEROutputLinear();
	REGISTEROutputProminent();
	REGISTEROutputVoronoi();
	//REGISTERRecorderPlugin();

	//void ManuallyRegisterDevices();
	//ManuallyRegisterDevices();
}
#endif

int main(int argc, char ** argv)
{
	int i;

#ifdef TCC
	RegisterConstructorFunctions();
#endif


	printf( "Output Drivers:\n" );
	for( i = 0; i < MAX_OUT_DRIVERS; i++ )
	{
		if( ODList[i].Name ) printf( "\t%s\n", ODList[i].Name );
	}

#if defined(WIN32) || defined(USE_WINDOWS)
	//In case something needs network access.
    WSADATA wsaData;
    WSAStartup(0x202, &wsaData);
#elif defined( ANDROID )
	int hasperm = AndroidHasPermissions( "READ_EXTERNAL_STORAGE" );
	if( !hasperm )
	{
		AndroidRequestAppPermissions( "READ_EXTERNAL_STORAGE" );
	}
	int haspermInternet = AndroidHasPermissions( "INTERNET" );
	if( !haspermInternet )
	{
		AndroidRequestAppPermissions( "INTERNET" );
	}
#else
	//Linux
#endif

	gargc = argc;
	gargv = argv;

	SetupConfigs();

	//Initialize Rawdraw
	int frames = 0;
	double ThisTime;
	double LastFPSTime = OGGetAbsoluteTime();
	double LastFrameTime = OGGetAbsoluteTime();
	double SecToWait;
	CNFGBGColor = 0x800000;
	CNFGDialogColor = 0x444444;

	char title[1024];
	char * tp = title;

	memcpy( tp, "ColorChord ", strlen( "ColorChord " ) );
	tp += strlen( "ColorChord " );

	for( i = 1; i < argc; i++ )
	{
		memcpy( tp, argv[i], strlen( argv[i] ) );
		tp += strlen( argv[i] );
		*tp = ' ';
		tp++;
	}
	*tp = 0;
	if( !headless )
		CNFGSetup( title, set_screenx, set_screeny );


	char * OutDriverNames = strdup( GetParameterS( "outdrivers", "null" ) );
	char * ThisDriver = OutDriverNames;
	char * TDStart;
	for( i = 0; i < MAX_OUT_DRIVERS; i++ )
	{
		while( *ThisDriver == ' ' || *ThisDriver == '\t' ) ThisDriver++;
		if( !*ThisDriver ) break;

		TDStart = ThisDriver;

		while( *ThisDriver != 0 && *ThisDriver != ',' )
		{
			if( *ThisDriver == '\t' || *ThisDriver == ' ' ) *ThisDriver = 0;
			ThisDriver++;
		}

		if( *ThisDriver )
		{
			*ThisDriver = 0;
			ThisDriver++;
		}
	
		printf( "Loading: %s\n", TDStart );
		outdriver[i] = SetupOutDriver( TDStart );
	}
	free(OutDriverNames);


	do
	{
		//Initialize Sound
		sd = CNFAInit( sound_source, "colorchord", &SoundCB, 
			GetParameterI( "samplerate", 44100 ), GetParameterI( "samplerate", 44100 ),
			GetParameterI( "channels", 2 ), GetParameterI( "channels", 2 ), 
			GetParameterI( "buffer", 1024 ),
			GetParameterS( "devplay", 0 ), GetParameterS( "devrecord", 0 ), NULL );

		if( sd ) break;
			
		CNFGColor( 0xffffff );
		CNFGPenX = 10; CNFGPenY = 100;
		CNFGHandleInput();
		CNFGClearFrame();
		CNFGDrawText( "Colorchord must be used with sound. Sound not available.", 10 );
		CNFGSwapBuffers();
		OGSleep(1);
	} while( 1 );

	nf = CreateNoteFinder( sd->spsRec );

	//Once everything was reinitialized, re-read the ini files.
	SetEnvValues( 1 );

	printf( "================================================= Set Up\n" );

	Now = OGGetAbsoluteTime();
	double Last = Now;
	#ifdef ANDROID
	while(!bQuitColorChord)
	#else
	while(!bQuitColorChord)
	#endif
	{
		char stt[1024];
		//Handle Rawdraw frame swappign

		Now = OGGetAbsoluteTime();
		DeltaFrameTime = Now - Last;

		if( !headless )
		{
			CNFGHandleInput();
			CNFGClearFrame();
			CNFGColor( 0xFFFFFF );
			CNFGGetDimensions( &screenx, &screeny );
		}

		RunNoteFinder( nf, sound, (soundhead-1+SOUNDCBSIZE)%SOUNDCBSIZE, SOUNDCBSIZE );
		//Done all ColorChord work.


		VisTimeStart = OGGetAbsoluteTime();

		for( i = 0; i < MAX_OUT_DRIVERS; i++ )
		{

			if( force_white )
			{
				memset( OutLEDs, 0x7f, MAX_LEDS*3 );
			}

			if( outdriver[i] )
				outdriver[i]->Func( outdriver[i]->id, nf );
		}

		VisTimeEnd = OGGetAbsoluteTime();


		if( !headless )
		{
			//Handle outputs.
			int freqbins = nf->freqbins;
			int note_peaks = freqbins/2;
			int freqs = freqbins * nf->octaves;
			//int maxdists = freqbins/2;

			//Do a bunch of debugging.
			if( show_debug_basic && !is_suspended )
			{
				//char sttdebug[1024];
				//char * sttend = sttdebug;

				for( i = 0; i < nf->dists_count; i++ )
				{
					CNFGPenX = (nf->dists[i].mean + 0.5) / freqbins * screenx;  //Move over 0.5 for visual purposes.  The means is correct.
					CNFGPenY = 400-nf->dists[i].amp * 150.0 / nf->dists[i].sigma;
					//printf( "%f %f\n", dists[i].mean, dists[i].amp );
					sprintf( stt, "%f\n%f\n", nf->dists[i].mean, nf->dists[i].amp );
//					sttend += sprintf( sttend, "%f/%f ",nf->dists[i].mean, nf->dists[i].amp );
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
					float note = (float) nf->note_positions[i] / freqbins;
					CNFGDialogColor = CCtoHEX( note, 1.0, 1.0 );
					CNFGDrawBox( ((float)i / note_peaks) * screenx, 480 - nf->note_amplitudes_out[i] * 100, ((float)(i+1) / note_peaks) * screenx, 480 );
					CNFGPenX = ((float)(i+.4) / note_peaks) * screenx;
					CNFGPenY = screeny - 30;
					sprintf( stt, "%d\n%0.0f", nf->enduring_note_id[i], nf->note_amplitudes2[i]*1000.0 );
					//sttend += sprintf( sttend, "%5d/%5.0f ", nf->enduring_note_id[i], nf->note_amplitudes2[i]*1000.0 );
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
				//puts( sttdebug );
			}

			//Extra debugging?
			if( show_debug && !is_suspended )
			{
				//Draw the histogram
				float lasthistval;
				CNFGColor( 0xffffff );

				for( i = -1; i < screenx; i++ )
				{
					float thishistval = CalcHistAt( (float)i/(float)screenx*freqbins-0.5, nf->freqbins, nf->dists, nf->dists_count );
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

			if( !is_suspended )
			{
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

				CNFGColor( 0xffffff );
				CNFGPenX = 440; CNFGPenY = screeny-10;
				sprintf( stt, "FPS: %d", lastfps );
				CNFGDrawText( stt, 2 );

#ifdef ANDROID
				CNFGColor( 0xffffff );
				CNFGPenX = 10; CNFGPenY = 600;
				CNFGDrawText( genlog, 3 );
#endif
				CNFGSwapBuffers();
			}
		}

		//Finish Rawdraw with FPS counter, and a nice delay loop.
		frames++;

		ThisTime = OGGetAbsoluteTime();
		if( ThisTime > LastFPSTime + 1 && showfps )
		{
#ifndef ANDROID
			printf( "FPS: %d\n", frames );
#endif
			lastfps = frames;
			frames = 0;
			LastFPSTime+=1;
		}

		if( cpu_autolimit )
		{
			SecToWait = cpu_autolimit_interval - ( ThisTime - LastFrameTime );
			LastFrameTime += cpu_autolimit_interval;
			if( SecToWait < -.1 ) LastFrameTime = ThisTime - .1;
			if( SecToWait > 0 )
				OGUSleep( (int)( SecToWait * 1000000 ) );
		}

		if( !is_suspended )
			SetEnvValues( 0 );

		Last = Now;
	}

}
