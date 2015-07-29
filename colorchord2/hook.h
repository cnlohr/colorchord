//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _KEYHOOK_H
#define _KEYHOOK_H

#define MAX_KEY_EVENTS 16
#define MAX_SOUND_EVENTS 16

void KeyHappened( int key, int down );
void HookKeyEvent( void (*KeyEvent)( void * v, int key, int down ), void * v );
void UnhookKeyEvent( void (*KeyEvent)( void * v, int key, int down ), void * v );


void SoundEventHappened( int samples, float * samps, int channel_ct, int is_out );
void HookSoundInEvent( void (*SoundE)( void * v, int samples, float * samps, int channel_ct ), void * v, int is_out );
void UnhookSoundInEvent( void (*SoundE)( void * v, int samples, float * samps, int channel_ct ), void * v, int is_out );


#endif

