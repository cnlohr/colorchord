#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define CNFGOGL
#define CNFG_IMPLEMENTATION
#include <rawdraw_sf.h>

#define CNFA_IMPLEMENTATION
#define PULSEAUDIO
#include <CNFA_sf.h>

#include "../../colorchord2/color.c"

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
int HandleDestroy() { return 0; }

struct CNFADriver * cnfa;

#define MAX_AUDIO_HISTORY 8192
short audioHistory[MAX_AUDIO_HISTORY];
int audioHead;


void SoundCB( struct CNFADriver *sd, short *out, short *in, int framesp, int framesr )
{
	int channelin = sd->channelsRec;
	int channelout = sd->channelsPlay;
	int i;

	for( i = 0; i < framesr; i++ )
	{
		audioHistory[audioHead] = in[i];
		audioHead = ( audioHead + 1 ) % MAX_AUDIO_HISTORY;
	}
}


float PhasorLerp( float from, float to, float alpha )
{
	float fDirection = to - from;
	if( fDirection > 3.14159 ) fDirection -= 3.14159 * 2.0;
	if( fDirection <-3.14159 ) fDirection += 3.14159 * 2.0;
	return from + fDirection * alpha;
}


int main()
{
	cnfa = CNFAInit( "PULSE", "colorchord", &SoundCB, 48000, 48000, 0, 1, 1024, "default", "@DEFAULT_MONITOR@", NULL );

	CNFGSetup( "NC Test", 1920, 1080 );
	while(CNFGHandleInput())
	{
		CNFGBGColor = 0x101010ff; //Dark Blue Background

		short w, h;
		CNFGClearFrame();
		CNFGGetDimensions( &w, &h );

		//Change color to white.
		CNFGColor( 0xffffffff ); 

		CNFGSetLineWidth(3);

		CNFGPenX = 1; CNFGPenY = h*1/8;
		CNFGDrawText( "ALIGNED WAVEFORM", 4 );
		CNFGPenX = 1; CNFGPenY = h*5/8-10;
		CNFGDrawText( "NC", 4 );
		CNFGPenX = 1; CNFGPenY = h*7/8-30;
		CNFGDrawText( "PHASE", 4 );
		//CNFGPenX = 1; CNFGPenY = h*7/8;
		//CNFGDrawText( "CENTER", 4 );


	// (5)  fLeft = fCenter - Fs/(2N) // fRight = fCenter + Fs/(2N)
	// Wdft = 2Wnc
	// (6)  Wnc = fRight - fLeft = Fs/N
	// (7)  N = round( round( 2*fCenter / Wnc ) * Fs/(2*fCenter)
	// (8)  max(0, −(Re′L × Re′R + Im′L × Im′R)).

	// Goal is make fLeft = one semitone down, fRight = one semitone up.

		int semitone = 0;

		const float fS = 48000;
		const float fBase = 55/2.0;
		const int nBinsPerOctave = 24;
		const int nOctaves = 6;
		const float fSemitonesWidth = 1.5;

		int headAtStart = audioHead;

		float fHighestAmp = 0;
		int nHighestSemitone = 1;

		float fNCs[nBinsPerOctave * nOctaves];
		float fPhases[nBinsPerOctave * nOctaves];

		// XXX TODO: Compute the DFT once, and extract NC and Phase from DFT.

		for( semitone = 0; semitone < nBinsPerOctave * nOctaves; semitone++ )
		{
			float fCenter = fBase * powf( 2, semitone / (float)nBinsPerOctave );
			float fLeft   = fBase * powf( 2, ( semitone - fSemitonesWidth ) / (float)nBinsPerOctave );
			float fRight  = fBase * powf( 2, ( semitone + fSemitonesWidth ) / (float)nBinsPerOctave );
			float Wnc = fRight - fLeft;
			int N = round( round( 2*fCenter / Wnc ) * fS/(2*fCenter) );
			//printf( "N: %f %d\n", fCenter, N );

			float fRLeft = 0;
			float fILeft = 0;
			float fRRight = 0;
			float fIRight = 0;
			float fRCenter = 0;
			float fICenter = 0;
			int i;

			int samp = headAtStart;
			float fOmegaLeft = 0;
			float fOmegaRight = 0;
			float fOmegaCenter = 0;
			for( i = 0; i < N; i++ )
			{
				samp--;
				if( samp < 0 ) samp += MAX_AUDIO_HISTORY;
				float v = audioHistory[samp] / 32768.0;
				fOmegaLeft   += fLeft   * 3.1415926 * 2.0 / fS;
				fOmegaRight  += fRight  * 3.1415926 * 2.0 / fS;
				fOmegaCenter += fCenter * 3.1415926 * 2.0 / fS;
				fRLeft   += cosf( fOmegaLeft   ) * v;
				fILeft   += sinf( fOmegaLeft   ) * v;
				fRRight  += cosf( fOmegaRight  ) * v;
				fIRight  += sinf( fOmegaRight  ) * v;
			}

			// Second part is the rescale
			float fRescale = 1.0/N * (fCenter + 4000)/300;

			fRLeft   *= fRescale;
			fILeft   *= fRescale;
			fRRight  *= fRescale;
			fIRight  *= fRescale;

			//	max(0, −(Re′L × Re′R + Im′L × Im′R)).
			float fNC = -(fRLeft * fRRight + fILeft * fIRight);
			fNCs[semitone] = fNC;

			if( fNC < 0 ) fNC = 0;
			int bS = (semitone+0) * (w-1) / (nBinsPerOctave * nOctaves);
			int bE = (semitone+1) * (w-1) / (nBinsPerOctave * nOctaves);
			int center = h*3/4;
			int y = center - (fNC) * center;

			CNFGColor( CCtoHEX( semitone/(float)nBinsPerOctave, 1.0, 1.0 ) ); 

			CNFGTackRectangle( bS, y, bE, center );

			float fReCenterPhase = atan2f( fRLeft - fRRight, fILeft - fIRight );
			fPhases[semitone] = fReCenterPhase;
			if( fNC > fHighestAmp && semitone > 0 && semitone < nBinsPerOctave * nOctaves - 1 )
			{
				fHighestAmp = fNC;
				nHighestSemitone = semitone;
			}

/*
			float fCenterAmp = sqrtf( fRCenter * fRCenter + fICenter * fICenter );
			y = h - fCenterAmp/2 * center/2;
			CNFGTackRectangle( bS, y, bE, h );
*/

			int cX = (bS + bE)/2;
			int cY = h * 7 / 8;
			fNC *= 100;
			CNFGTackSegment( 
				cX,
				cY, 
				cX + sin(fReCenterPhase) * fNC,
				cY + cos(fReCenterPhase) * fNC );

		}


		float fCenterDetune = 0;
		float fMaxPhase = 0;

		float fLeftPower = fNCs[nHighestSemitone-1];
		float fCenterPower = fNCs[nHighestSemitone];
		float fRightPower = fNCs[nHighestSemitone+1];

		float fPhaseLeft = fPhases[nHighestSemitone-1];
		float fPhaseCenter = fPhases[nHighestSemitone];
		float fPhaseRight = fPhases[nHighestSemitone+1];


		if( fLeftPower > fRightPower )
		{
			fCenterPower -= fRightPower;
			fLeftPower -= fRightPower;
			float alpha = 0.5 * fLeftPower / (fCenterPower + 0.0000001);
			fCenterDetune = -alpha;
			fMaxPhase = PhasorLerp( fPhaseCenter, fPhaseLeft, alpha );
		}
		else
		{
			fCenterPower -= fLeftPower;
			fRightPower -= fLeftPower;
			float alpha = 0.5 * fRightPower / (fCenterPower + 0.0000001);
			fCenterDetune = alpha;
			fMaxPhase = PhasorLerp( fPhaseCenter, fPhaseRight, alpha );
		}

		float fCenterMax = fBase * powf( 2, (nHighestSemitone + fCenterDetune) / (float)nBinsPerOctave );


		CNFGColor( 0xffffffff ); 
		int i;
		int samp = headAtStart-1;
		int samplesShift = ( -fMaxPhase + 3.14159) * fS / fCenterMax / 3.14159 / 2.0;
		samp -= samplesShift;

		for( i = w-1; i >= 0; i-- )
		{
			samp--;
			if( samp < 0 ) samp += MAX_AUDIO_HISTORY;
			float v = audioHistory[samp] / 32768.0;
			float vLast = audioHistory[(samp+1)&MAX_AUDIO_HISTORY] / 32768.0;
			int hsc = h/4;
			float aS = h/8;
			CNFGTackSegment( i, v * aS + hsc, i-1, v * aS + hsc );
		}



		//Draw a white pixel at 30, 30 
//		CNFGTackPixel( 30, 30 );         

		//Draw a line from 50,50 to 100,50
//		CNFGTackSegment( 50, 50, 100, 50 );

		//Dark Red Color Select
		CNFGColor( 0x800000FF );

		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();		
	}
	return 0;
}

