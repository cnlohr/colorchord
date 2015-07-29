//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "hook.h"

struct KeyEvent
{
	void (*KeyE)( void * v, int key, int down );
	void * v;
} KeyEvents[MAX_KEY_EVENTS];

void KeyHappened( int key, int down )
{
	int i;
	for( i = 0; i < MAX_KEY_EVENTS; i++ )
	{
		if( KeyEvents[i].KeyE )
			KeyEvents[i].KeyE( KeyEvents[i].v, key, down );
	}
}

void HookKeyEvent(   void (*KeyE)( void * v, int key, int down ), void * v )
{
	int i;
	for( i = 0; i < MAX_KEY_EVENTS; i++ )
	{
		if( !KeyEvents[i].KeyE )
		{
			KeyEvents[i].KeyE = KeyE;
			KeyEvents[i].v = v;
			break;
		}
	}
}

void UnhookKeyEvent( void (*KeyE)( void * v, int key, int down ), void * v )
{
	int i;
	for( i = 0; i < MAX_KEY_EVENTS; i++ )
	{
		if( KeyEvents[i].KeyE == KeyE && KeyEvents[i].v == v )
		{
			KeyEvents[i].KeyE = 0;
			KeyEvents[i].v = 0;
		}
	}
}








struct SoundEvent
{
	void (*SoundE)( void * v, int samples, float * samps, int channel_ct );
	void * v;
};

struct SoundEvent SoundEvents[2][MAX_SOUND_EVENTS];


void SoundEventHappened( int samples, float * samps, int is_out, int channel_ct )
{
	int i;
	for( i = 0; i < MAX_SOUND_EVENTS; i++ )
	{
		if( SoundEvents[is_out][i].SoundE )
		{
			SoundEvents[is_out][i].SoundE( SoundEvents[is_out][i].v, samples, samps, channel_ct );
		}
	}
}

void HookSoundInEvent( void (*SoundE)( void * v, int samples, float * samps, int channel_ct ), void * v, int is_out )
{
	int i;
	for( i = 0; i < MAX_SOUND_EVENTS; i++ )
	{
		if( !SoundEvents[is_out][i].SoundE )
		{
			SoundEvents[is_out][i].SoundE = SoundE;
			SoundEvents[is_out][i].v = v;
			break;
		}
	}
}

void UnhookSoundInEvent( void (*SoundE)( void * v, int samples, float * samps, int channel_ct ), void * v, int is_out )
{
	int i;
	for( i = 0; i < MAX_SOUND_EVENTS; i++ )
	{
		if( SoundEvents[is_out][i].SoundE == SoundE && SoundEvents[is_out][i].v == v )
		{
			SoundEvents[is_out][i].SoundE = 0;
			SoundEvents[is_out][i].v = 0;
		}
	}
}



