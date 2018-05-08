//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _SOUND_H
#define _SOUND_H

#define MAX_SOUND_DRIVERS 10

struct SoundDriver;

typedef void(*SoundCBType)( float * out, float * in, int samplesr, int * samplesp, struct SoundDriver * sd );
typedef void*(SoundInitFn)( SoundCBType cb );

struct SoundDriver
{
	void (*CloseFn)( void * object );
	int (*SoundStateFn)( struct SoundDriver * object );
	SoundCBType callback;
	int channelsPlay;
	int spsPlay;
	int channelsRec;
	int spsRec;

	//More fields may exist on a per-sound-driver basis
};

//Accepts:
// samplerate=44100;channels=2;devplay=default;devrecord=default;record=1;play=1;minavailcount=4096;stopthresh=1024;startthresh=4096;buffer=1024
// buffer is in samples
//If DriverName = 0 or empty, will try to find best driver.
struct SoundDriver * InitSound( const char * driver_name, SoundCBType cb );
int SoundState( struct SoundDriver * soundobject ); //returns 0 if okay, negative if faulted.
void CloseSound( struct SoundDriver * soundobject );

//Called by various sound drivers.  Notice priority must be greater than 0.  Priority of 0 or less will not register.
void RegSound( int priority, const char * name, SoundInitFn * fn );

#define REGISTER_SOUND( sounddriver, priority, name, function ) \
	void __attribute__((constructor)) REGISTER##sounddriver() { RegSound( priority, name, function ); }

#endif

