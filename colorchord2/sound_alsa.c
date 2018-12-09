//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "sound.h"
#include "os_generic.h"
#include "parameters.h"
#include <alsa/asoundlib.h>

#define BUFFERSETS 4

#define BLOCKING

struct SoundDriverAlsa
{
	void (*CloseFn)( struct SoundDriverAlsa * object );
	int (*SoundStateFn)( struct SoundDriverAlsa * object );
	SoundCBType callback;
	int channelsPlay;
	int spsPlay;
	int channelsRec;
	int spsRec;
	int alsa_fmt_s16le;

	snd_pcm_uframes_t buffer;
	og_thread_t thread;
	snd_pcm_t *playback_handle;
	snd_pcm_t *record_handle;

	//More fields may exist on a per-sound-driver basis
};

static struct SoundDriverAlsa* InitASound( struct SoundDriverAlsa * r );

void CloseSoundAlsa( struct SoundDriverAlsa * r );

int SoundStateAlsa( struct SoundDriverAlsa * soundobject )
{
	return ((soundobject->playback_handle)?1:0) | ((soundobject->record_handle)?2:0);
}

void CloseSoundAlsa( struct SoundDriverAlsa * r )
{
	if( r )
	{
		if( r->playback_handle ) snd_pcm_close (r->playback_handle);
		if( r->record_handle ) snd_pcm_close (r->record_handle);
#ifdef BLOCKING
		OGUSleep(2000);
		OGCancelThread( r->thread );
#endif
		free( r );
	}
}


static int SetHWParams( snd_pcm_t * handle, int * samplerate, int * channels, snd_pcm_uframes_t * buffer, struct SoundDriverAlsa * a )
{
	int err;
	snd_pcm_hw_params_t *hw_params;
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		goto fail;
	}

	if ((err = snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		goto fail;
	}

	if ((err = snd_pcm_hw_params_set_format (handle, hw_params, SND_PCM_FORMAT_FLOAT )) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));

		printf( "Trying backup: S16LE.\n" );	
		if ((err = snd_pcm_hw_params_set_format (handle, hw_params,  SND_PCM_FORMAT_S16_LE )) < 0) {
			fprintf (stderr, "cannot set sample format (%s)\n",
				 snd_strerror (err));
			goto fail;
		}

		a->alsa_fmt_s16le = 1;
	}

	if ((err = snd_pcm_hw_params_set_rate_near (handle, hw_params, (unsigned int*)samplerate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		goto fail;
	}

	if ((err = snd_pcm_hw_params_set_channels (handle, hw_params, *channels)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
			 snd_strerror (err));
		goto fail;
	}

	int dir = 0;
	if( (err = snd_pcm_hw_params_set_period_size_near(handle, hw_params, buffer, &dir)) < 0 )
	{
		fprintf( stderr, "cannot set period size. (%s)\n",
			snd_strerror(err) );
		goto fail;
	}


	if ((err = snd_pcm_hw_params (handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		goto fail;
	}

	snd_pcm_hw_params_free (hw_params);
	return 0;
fail:
	snd_pcm_hw_params_free (hw_params);
	return -2;
}


static int SetSWParams( snd_pcm_t * handle, int isrec )
{
	snd_pcm_sw_params_t *sw_params;
	int err;
	//Time for software parameters:

	if( !isrec )
	{
		if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) {
			fprintf (stderr, "cannot allocate software parameters structure (%s)\n",
				 snd_strerror (err));
			goto failhard;
		}
		if ((err = snd_pcm_sw_params_current (handle, sw_params)) < 0) {
			fprintf (stderr, "cannot initialize software parameters structure (%s) (%p)\n", 
				 snd_strerror (err), handle);
			goto fail;
		}
		if ((err = snd_pcm_sw_params_set_avail_min (handle, sw_params, GetParameterI( "minavailcount", 2048 ) )) < 0) {
			fprintf (stderr, "cannot set minimum available count (%s)\n",
				 snd_strerror (err));
			goto fail;
		}
		if ((err = snd_pcm_sw_params_set_stop_threshold(handle, sw_params, GetParameterI( "stopthresh", 512 ))) < 0) {
			fprintf (stderr, "cannot set minimum available count (%s)\n",
				 snd_strerror (err));
			goto fail;
		}
		if ((err = snd_pcm_sw_params_set_start_threshold(handle, sw_params, GetParameterI( "startthresh", 2048 ))) < 0) {
			fprintf (stderr, "cannot set minimum available count (%s)\n",
				 snd_strerror (err));
			goto fail;
		}
		if ((err = snd_pcm_sw_params (handle, sw_params)) < 0) {
			fprintf (stderr, "cannot set software parameters (%s)\n",
				 snd_strerror (err));
			goto fail;
		}

	}


	if ((err = snd_pcm_prepare (handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
			 snd_strerror (err));
		goto fail;
	}


	return 0;
fail:
	if( !isrec )
	{
		snd_pcm_sw_params_free (sw_params);
	}
failhard:
	return -1;
}

#ifdef BLOCKING
static void * SoundThread( void * v )
{
	int i;
	struct SoundDriverAlsa * a = (struct SoundDriverAlsa*)v;
	float * bufr[BUFFERSETS];
	float * bufp[BUFFERSETS];

	for(i = 0; i < BUFFERSETS; i++ )
	{
		bufr[i] = malloc( a->buffer * sizeof(float) * a->channelsRec );
		bufp[i] = malloc( a->buffer * sizeof(float) * a->channelsPlay  );
	}

	while( a->record_handle || a->playback_handle )
	{
		int err;

		i = (i+1)%BUFFERSETS;

		if( a->record_handle )
		{
			if( (err = snd_pcm_readi (a->record_handle, bufr[i], a->buffer)) != a->buffer)
			{
				fprintf (stderr, "read from audio interface failed (%s)\n",
					 snd_strerror (err));
				if( a->record_handle ) snd_pcm_close (a->record_handle);
				a->record_handle = 0;
			}
			else
			{
				//has_rec = 1;
			}
		}

		if( a->alsa_fmt_s16le )
		{
			//Hacky: Turns out data was s16le.
			int16_t * dat = (int16_t*)bufr[i];
			float *   dot = bufr[i];
			int i;
			int len = a->buffer;
			for( i = len-1; i >= 0; i-- )
			{
				dot[i] = dat[i]/32768.0;
			}
		}
		//Do our callback.
		int playbacksamples = 0;
		a->callback( bufp[i], bufr[i], a->buffer, &playbacksamples, (struct SoundDriver*)a );
		//playbacksamples *= sizeof(float) * a->channelsPlay;

		if( a->playback_handle )
		{
			if ((err = snd_pcm_writei (a->playback_handle, bufp[i], playbacksamples)) != playbacksamples)
			{
				fprintf (stderr, "write to audio interface failed (%s)\n",
					 snd_strerror (err));
				if( a->playback_handle ) snd_pcm_close (a->playback_handle);
					a->playback_handle = 0;
			}
		}
	}

	//Fault happened, re-initialize?
	InitASound( a );
	return 0;
}
#else

//Handle callback

static struct SoundDriverAlsa * reccb;
static int record_callback (snd_pcm_sframes_t nframes)
{
	int err;

//	printf ("playback callback called with %u frames\n", nframes);

	/* ... fill buf with data ... */

	if ((err = snd_pcm_writei (playback_handle, buf, nframes)) < 0) {
		fprintf (stderr, "write failed (%s)\n", snd_strerror (err));
	}

	return err;
}

#endif

static struct SoundDriverAlsa * InitASound( struct SoundDriverAlsa * r )
{
	int err;
	if( GetParameterI( "play", 0 ) )
	{
		if ((err = snd_pcm_open (&r->playback_handle, GetParameterS( "devplay", "default" ), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
			fprintf (stderr, "cannot open output audio device (%s)\n", 
				 snd_strerror (err));
			goto fail;
		}
	}

	if( GetParameterI( "record", 1 ) )
	{
		if ((err = snd_pcm_open (&r->record_handle, GetParameterS( "devrecord", "default" ), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
			fprintf (stderr, "cannot open input audio device (%s)\n", 
				 snd_strerror (err));
			goto fail;
		}
	}



	if( r->playback_handle )
	{
		if( SetHWParams( r->playback_handle, &r->spsPlay, &r->channelsPlay, &r->buffer, r ) < 0 ) 
			goto fail;
		if( SetSWParams( r->playback_handle, 0 ) < 0 )
			goto fail;
	}

	if( r->record_handle )
	{
		if( SetHWParams( r->record_handle, &r->spsRec, &r->channelsRec, &r->buffer, r ) < 0 )
			goto fail;
		if( SetSWParams( r->record_handle, 1 ) < 0 )
			goto fail;
	}

	if( r->playback_handle && r->record_handle )
	{
		snd_pcm_link ( r->playback_handle, r->record_handle );
	}

#ifdef BLOCKING
	r->thread = OGCreateThread( SoundThread, r );
#else
	reccb = r;
	//handle interrupt
#endif
	return r;

fail:
	if( r )
	{
		if( r->playback_handle ) snd_pcm_close (r->playback_handle);
		if( r->record_handle ) snd_pcm_close (r->record_handle);
		free( r );
	}
	return 0;
}



void * InitSoundAlsa( SoundCBType cb )
{
	struct SoundDriverAlsa * r = malloc( sizeof( struct SoundDriverAlsa ) );

	r->CloseFn = CloseSoundAlsa;
	r->SoundStateFn = SoundStateAlsa;
	r->callback = cb;

	r->spsPlay = GetParameterI( "samplerate", 44100 );
	r->channelsPlay = GetParameterI( "channels", 2 );
	r->spsRec = r->spsPlay;
	r->channelsRec = r->channelsPlay;

	r->playback_handle = 0;
	r->record_handle = 0;
	r->buffer = GetParameterI( "buffer", 1024 );

	r->alsa_fmt_s16le = 0;


	return InitASound(r);
}

REGISTER_SOUND( AlsaSound, 10, "ALSA", InitSoundAlsa );

