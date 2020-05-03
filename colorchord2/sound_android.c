//Copyright 2019-2020 <>< Charles Lohr under the ColorChord License.
// This should be used with rawdrawandroid

#include "sound.h"
#include "os_generic.h"
#include <pthread.h> //Using android threads not os_generic threads.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NO_SOUND_PARAMETERS
#include "parameters.h"
#else
#define GetParameterI( x, y ) (y)
#define GetParameterS( x, y ) (y)
#endif


//based on https://github.com/android/ndk-samples/blob/master/native-audio/app/src/main/cpp/native-audio-jni.c

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <android_native_app_glue.h>
#include <android/log.h>
#include <jni.h>
#include <native_activity.h>

#define LOGI(...)  ((void)__android_log_print(ANDROID_LOG_INFO, APPNAME, __VA_ARGS__))
#define printf( x...) LOGI( x )

#define RECORDER_FRAMES 1024

#define BUFFERSETS 4

#define BLOCKING

//Across all sound systems.
static pthread_mutex_t  audioEngineLock = PTHREAD_MUTEX_INITIALIZER;

struct SoundDriverAndroid
{
	void (*CloseFn)( struct SoundDriverAndroid * object );
	int (*SoundStateFn)( struct SoundDriverAndroid * object );
	SoundCBType callback;
	SLObjectItf engineObject;
	SLEngineItf engineEngine;
	SLRecordItf recorderRecord;
	SLObjectItf recorderObject;
	SLAndroidSimpleBufferQueueItf recorderBufferQueue;
	unsigned recorderSize;

	short recorderBuffer[RECORDER_FRAMES];
};


void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	struct SoundDriverAndroid * r = (struct SoundDriverAndroid*)context;
	int samplesp = 0;
	float buffout[RECORDER_FRAMES];
	int i;
	short * rb = r->recorderBuffer;
	for( i = 0; i < RECORDER_FRAMES; i++ )	buffout[i] = (rb[i]+0.5)/32767.5;
	r->callback( 0, buffout, RECORDER_FRAMES, &samplesp, r );
	(*r->recorderBufferQueue)->Enqueue(r->recorderBufferQueue, r->recorderBuffer, sizeof(r->recorderBuffer));
}

static struct SoundDriverAndroid* InitAndroidSound( struct SoundDriverAndroid * r )
{
    SLresult result;
	LOGI( "Starting InitAndroidSound\n" );
    // create engine
    result = slCreateEngine(&r->engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the engine
    result = (*r->engineObject)->Realize(r->engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the engine interface, which is needed in order to create other objects
    result = (*r->engineObject)->GetInterface(r->engineObject, SL_IID_ENGINE, &r->engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;


	///////////////////////////////////////////////////////////////////////////////////////////////////////


    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};

	SLDataFormat_PCM format_pcm ={
		SL_DATAFORMAT_PCM,
		1, 
		SL_SAMPLINGRATE_16,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_SPEAKER_FRONT_CENTER,
		SL_BYTEORDER_LITTLEENDIAN,
	};

    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*r->engineEngine)->CreateAudioRecorder(r->engineEngine, &r->recorderObject, &audioSrc,
            &audioSnk, 1, id, req);
    if (SL_RESULT_SUCCESS != result) {
		LOGI( "CreateAudioRecorder failed\n" );
        return JNI_FALSE;
    }

    // realize the audio recorder
    result = (*r->recorderObject)->Realize(r->recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
		LOGI( "AudioRecorder Realize failed: %d\n", result );
        return JNI_FALSE;
    }

    // get the record interface
    result = (*r->recorderObject)->GetInterface(r->recorderObject, SL_IID_RECORD, &r->recorderRecord);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*r->recorderObject)->GetInterface(r->recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
            &r->recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // register callback on the buffer queue
    result = (*r->recorderBufferQueue)->RegisterCallback(r->recorderBufferQueue, bqRecorderCallback, r);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;


    assert( !pthread_mutex_trylock(&audioEngineLock));
    // in case already recording, stop recording and clear buffer queue
    result = (*r->recorderRecord)->SetRecordState(r->recorderRecord, SL_RECORDSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    result = (*r->recorderBufferQueue)->Clear(r->recorderBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // the buffer is not valid for playback yet
    r->recorderSize = 0;

    // enqueue an empty buffer to be filled by the recorder
    // (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
    result = (*r->recorderBufferQueue)->Enqueue(r->recorderBufferQueue, r->recorderBuffer, sizeof(r->recorderBuffer));
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // start recording
    result = (*r->recorderRecord)->SetRecordState(r->recorderRecord, SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

	LOGI( "Complete Init Sound Android\n" );
	return r;
}

void CloseSoundAndroid( struct SoundDriverAndroid * r );

int SoundStateAndroid( struct SoundDriverAndroid * soundobject )
{
	return ((soundobject->recorderObject)?1:0) | ((soundobject->recorderObject)?2:0);
}

void CloseSoundAndroid( struct SoundDriverAndroid * r )
{
    // destroy audio recorder object, and invalidate all associated interfaces
    if (r->recorderObject != NULL) {
        (*r->recorderObject)->Destroy(r->recorderObject);
        r->recorderObject = NULL;
        r->recorderRecord = NULL;
        r->recorderBufferQueue = NULL;
    }


    // destroy engine object, and invalidate all associated interfaces
    if (r->engineObject != NULL) {
        (*r->engineObject)->Destroy(r->engineObject);
        r->engineObject = NULL;
        r->engineEngine = NULL;
    }

}


int AndroidHasPermissions(const char* perm_name);
void AndroidRequestAppPermissions(const char * perm);


void * InitSoundAndroid( SoundCBType cb )
{
	int hasperm = AndroidHasPermissions( "RECORD_AUDIO" );
	if( !hasperm )
	{
		AndroidRequestAppPermissions( "RECORD_AUDIO" );
	}

	struct SoundDriverAndroid * r = (struct SoundDriverAndroid *)malloc( sizeof( struct SoundDriverAndroid ) );
	memset( r, 0, sizeof( *r) );
	r->CloseFn = CloseSoundAndroid;
	r->SoundStateFn = SoundStateAndroid;
	r->callback = cb;
	r->engineObject = 0;
	r->engineEngine = 0;
/*
	r->spsPlay = GetParameterI( "samplerate", 44100 );
	r->channelsPlay = GetParameterI( "channels", 2 );
	r->spsRec = r->spsPlay;
	r->channelsRec = r->channelsPlay;

	r->playback_handle = 0;
	r->record_handle = 0;
	r->buffer = GetParameterI( "buffer", 1024 );

	r->Android_fmt_s16le = 0;

*/
	return InitAndroidSound(r);
}

//Tricky: On Android, this can't actually run before main.  Have to manually execute it.

REGISTER_SOUND( AndroidSound, 10, "ANDROID", InitSoundAndroid );

