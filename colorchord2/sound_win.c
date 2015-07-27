#include <windows.h>
#include "parameters.h"
#include "sound.h"
#include "os_generic.h"
#include <mmsystem.h>
#include <stdio.h>
#include <stdint.h>

#if defined(WIN32)
#pragma comment(lib,"winmm.lib")
#endif

#define BUFFS 2

struct SoundDriverWin
{
	void (*CloseFn)( struct SoundDriverWin * object );
	int (*SoundStateFn)( struct SoundDriverWin * object );
	SoundCBType callback;
	int channelsPlay;
	int spsPlay;
	int channelsRec;
	int spsRec;


	int buffer;
	int isEnding;
	int GOBUFF;

	int recording;

	HWAVEIN hMyWave;
	WAVEHDR WavBuff[BUFFS];
};

static struct SoundDriverWin * w;

void CloseSoundWin( struct SoundDriverWin * r )
{
	int i;

	if( r )
	{
		waveInStop(r->hMyWave);
		waveInReset(r->hMyWave);

		for ( i=0;i<BUFFS;i++)
		{
			waveInUnprepareHeader(r->hMyWave,&(r->WavBuff[i]),sizeof(WAVEHDR));
			free ((r->WavBuff[i]).lpData);
		}
		waveInClose(r->hMyWave);
		free( r );
	}
}

int SoundStateWin( struct SoundDriverWin * soundobject )
{
	return soundobject->recording;
}

void CALLBACK HANDLEMIC(HWAVEIN hwi,UINT umsg, DWORD dwi, DWORD hdr, DWORD dwparm)
{
	int ctr;
	int ob;
	long cValue;
	unsigned int maxWave=0;

	float buffer[w->buffer*w->channelsRec];

	if (w->isEnding) return;

	switch (umsg)
	{
	case MM_WIM_OPEN:
		printf( "Mic Open.\n" );
		w->recording = 1;
		break;

	case MM_WIM_DATA:
//		printf( "Mic Data.\n");
		ob = (w->GOBUFF+(BUFFS))%BUFFS;
//		waveInPrepareHeader(w->hMyWave,&(w->WavBuff[w->Cbuff]),sizeof(WAVEHDR));

		for (ctr=0;ctr<w->buffer * w->channelsRec;ctr++) {
			float cv = (uint16_t)(((uint8_t)w->WavBuff[ob].lpData[ctr*2+1])*256+((uint8_t)w->WavBuff[ob].lpData[ctr*2])+32768)-32768;
			cv /= 32768;
//			if( ctr < 3 ) cv = -1;
//			buffer[(w->buffer * w->channelsRec)-ctr-1] = cv;
			buffer[ctr] = cv;
		}

		waveInAddBuffer(w->hMyWave,&(w->WavBuff[w->GOBUFF]),sizeof(WAVEHDR));
		w->GOBUFF = ( w->GOBUFF + 1 ) % BUFFS;

		int playbacksamples; //Unused
		w->callback( 0, buffer, w->buffer, &playbacksamples, (struct SoundDriver*)w );

	}
}


static struct SoundDriverWin * InitWinSound( struct SoundDriverWin * r )
{
	int i;
	WAVEFORMATEX wfmt;

	if( GetParameterI( "play", 0 )  )
	{
		fprintf( stderr, "Error: This Windows Sound Driver does not support playback.\n" );
		exit( -1 );
	}

	w = r;


	printf( "WFMT: %d %d %d\n", r->channelsRec, r->spsRec,
		r->spsRec * r->channelsRec );

	wfmt.wFormatTag = WAVE_FORMAT_PCM;
	wfmt.nChannels = r->channelsRec;
	wfmt.nSamplesPerSec = r->spsRec;
	wfmt.nAvgBytesPerSec = r->spsRec * r->channelsRec;
	wfmt.nBlockAlign = r->channelsRec * 2;
	wfmt.wBitsPerSample = 16;
	wfmt.cbSize = 0;

	long dwdevice;
	dwdevice = GetParameterI( "wininput", WAVE_MAPPER );

	printf( "Wave Devs: %d; WAVE_MAPPER: %d; Selected Input: %d\n", waveInGetNumDevs(), WAVE_MAPPER, dwdevice );

	printf( "waveInOpen: %p, %p\n", &r->hMyWave, &wfmt );

	int p = waveInOpen(&r->hMyWave, dwdevice, &wfmt,(DWORD)(void*)(&HANDLEMIC) , 0, CALLBACK_FUNCTION);

	printf( "WIO: %d\n", p );  //On real windows, returns 11

	for ( i=0;i<BUFFS;i++)
	{
		memset( &(r->WavBuff[i]), 0, sizeof(r->WavBuff[i]) );
		(r->WavBuff[i]).dwBufferLength = r->buffer*2*r->channelsRec;
		(r->WavBuff[i]).dwLoops = 1;
		(r->WavBuff[i]).lpData=(char*) malloc(r->buffer*r->channelsRec*2);
		waveInPrepareHeader(r->hMyWave,&(r->WavBuff[i]),sizeof(WAVEHDR));
		waveInAddBuffer(r->hMyWave,&(r->WavBuff[i]),sizeof(WAVEHDR));
	}

	p = waveInStart(r->hMyWave);

	printf( "WIS: %d\n", p ); //On real windows returns 5.

	return r;
}



void * InitSoundWin( SoundCBType cb )
{
	struct SoundDriverWin * r = malloc( sizeof( struct SoundDriverWin ) );

	r->CloseFn = CloseSoundWin;
	r->SoundStateFn = SoundStateWin;
	r->callback = cb;

	r->spsRec = GetParameterI( "samplerate", 44100 );
	r->channelsRec = GetParameterI( "channels", 2 );
	r->buffer = GetParameterI( "buffer", 384 );
	r->recording = 0;
	r->isEnding = 0;
	printf( "Buffer: %d\n", r->buffer );

	r->GOBUFF=0;

	return InitWinSound(r);
}

EXECUTE_AT_BOOT( WinSoundReg, RegSound( 10, "WIN", InitSoundWin ) );

