//Copyright 2015 <>< Charles Lohr under the ColorChord License.

//This file is really rough.  Full duplex doesn't seem to work hardly at all.


#include "sound.h"
#include "os_generic.h"
#include "parameters.h"
#include <stdlib.h>

#include <pulse/simple.h>
#include <pulse/pulseaudio.h>
#include <pulse/error.h>
#include <stdio.h>
#include <string.h>

#define BUFFERSETS 3


//from http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncPlayback/
//also http://maemo.org/api_refs/5.0/5.0-final/pulseaudio/pacat_8c-example.html


struct SoundDriverPulse
{
	void (*CloseFn)( struct SoundDriverPulse * object );
	int (*SoundStateFn)( struct SoundDriverPulse * object );
	SoundCBType callback;
	int channelsPlay;
	int spsPlay;
	int channelsRec;
	int spsRec;

	const char * sourceName;
	og_thread_t thread;
 	pa_stream *  	play;
 	pa_stream *  	rec;
	pa_context *  pa_ctx;
	pa_mainloop *pa_ml;
	int pa_ready;
	int buffer;
	//More fields may exist on a per-sound-driver basis
};



void CloseSoundPulse( struct SoundDriverPulse * r );

int SoundStatePulse( struct SoundDriverPulse * soundobject )
{
	return ((soundobject->play)?1:0) | ((soundobject->rec)?2:0);
}

void CloseSoundPulse( struct SoundDriverPulse * r )
{
	if( r )
	{
		if( r->play )
		{
			pa_stream_unref (r->play);
			r->play = 0;
		}

		if( r->rec )
		{
			pa_stream_unref (r->rec);
			r->rec = 0;
		}
		OGUSleep(2000);
		OGCancelThread( r->thread );
		free( r );
	}
}

static void * SoundThread( void * v )
{
	struct SoundDriverPulse * r = (struct SoundDriverPulse*)v;
	while(1)
	{
		pa_mainloop_iterate( r->pa_ml, 1, NULL );
	}
	return 0;
}
/*
	int i;
	int error;
	struct SoundDriverPulse * r = (struct SoundDriverPulse*)v;

	float * bufr[BUFFERSETS];
	float * bufp[BUFFERSETS];

	for(i = 0; i < BUFFERSETS; i++ )
	{
		bufr[i] = malloc( r->buffer * sizeof(float) * r->channelsRec );
		bufp[i] = malloc( r->buffer * sizeof(float) * r->channelsPlay  );
	}

	while( r->play || r->rec )
	{
		i = (i+1)%BUFFERSETS;

		if( r->rec )
		{
			if (pa_stream_read(r->rec, bufr[i], r->buffer * sizeof(float) * r->channelsRec, &error) < 0) {
				fprintf(stderr, __FILE__": pa_stream_write() failed: %s\n", pa_strerror(error));
				pa_stream_unref( r->play );
				r->rec = 0;
			}
		}

		int playbacksamples = 0;
		r->callback( bufp[i], bufr[i], r->buffer, &playbacksamples, (struct SoundDriver*)r );
		playbacksamples *= sizeof( float ) * r->channelsPlay;

		if( r->play )
		{
			if (pa_stream_write(r->play, bufp[i], playbacksamples, NULL, 0LL, PA_SEEK_RELATIVE) < 0) {
				fprintf(stderr, __FILE__": pa_stream_write() failed: %s\n", pa_strerror(error));
				pa_stream_unref( r->play );
				r->play = 0;
			}
		}
	}

}*/

static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {
//	pa_usec_t usec;

	struct SoundDriverPulse * r = (struct SoundDriverPulse*)userdata;
	if( r->rec )
	{
		return;
	}

/*
	//Neat: You might want this:

	pa_stream_get_latency(s,&usec,&neg);

	if (sampleoffs*2 + length > sizeof(sampledata))
	{
		sampleoffs = 0;
	}

	if (length > sizeof(sampledata)) {
		length = sizeof(sampledata);
	}

*/


//	pa_stream_write(s, &sampledata[sampleoffs], length, NULL, 0LL, PA_SEEK_RELATIVE);
	int playbacksamples = 0;
	float bufr[r->buffer*r->channelsRec];
	float bufp[r->buffer*r->channelsPlay];

	r->callback( bufp, bufr, r->buffer, &playbacksamples, (struct SoundDriver*)r );
	//playbacksamples *= sizeof( float ) * r->channelsPlay;

	pa_stream_write(r->play, &bufp, playbacksamples, NULL, 0LL, PA_SEEK_RELATIVE);

}


static void stream_record_cb(pa_stream *s, size_t length, void *userdata) {
//	pa_usec_t usec;
//	int neg;

	struct SoundDriverPulse * r = (struct SoundDriverPulse*)userdata;

/*	pa_stream_get_latency(s,&usec,&neg);
	printf("  latency %8d us\n",(int)usec);

	if (sampleoffs*2 + length > sizeof(sampledata))
	{
		sampleoffs = 0;
	}

	if (length > sizeof(sampledata)) {
		length = sizeof(sampledata);
	}*/

	int playbacksamples = 0;
	float * bufr;

    if (pa_stream_peek(r->rec, (void*)&bufr, &length) < 0) {
        fprintf(stderr, ("pa_stream_peek() failed: %s\n"), pa_strerror(pa_context_errno(r->pa_ctx)));
        return;
    }

	float * buffer;
    buffer = pa_xmalloc(length);
    memcpy(buffer, bufr, length);
	pa_stream_drop(r->rec);

	float bufp[length*r->channelsPlay];
	r->callback( bufp, buffer, length/sizeof(float)/r->channelsRec, &playbacksamples, (struct SoundDriver*)r );
	//playbacksamples *= sizeof( float ) * r->channelsPlay;
	pa_xfree( buffer );
	if( r->play )
		pa_stream_write(r->play, &bufp, playbacksamples*sizeof(float)*r->channelsPlay, NULL, 0LL, PA_SEEK_RELATIVE);
}



static void stream_underflow_cb(pa_stream *s, void *userdata) {
  // We increase the latency by 50% if we get 6 underflows and latency is under 2s
  // This is very useful for over the network playback that can't handle low latencies
  printf("underflow\n");
//  underflows++;
/*  if (underflows >= 6 && latency < 2000000) {
    latency = (latency*3)/2;
    bufattr.maxlength = pa_usec_to_bytes(latency,&ss);
    bufattr.tlength = pa_usec_to_bytes(latency,&ss);  
    pa_stream_set_buffer_attr(s, &bufattr, NULL, NULL);
    underflows = 0;
    printf("latency increased to %d\n", latency);
  }*/
}


void pa_state_cb(pa_context *c, void *userdata) {
	pa_context_state_t state;
	int *pa_ready = userdata;
	state = pa_context_get_state(c);
	switch  (state) {
		// These are just here for reference
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		default:
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			*pa_ready = 2;
			break;
		case PA_CONTEXT_READY:
			*pa_ready = 1;
		break;
	}
}


void * InitSoundPulse( SoundCBType cb )
{
	static pa_buffer_attr bufattr;
	static pa_sample_spec ss;
	int error;
	pa_mainloop_api *pa_mlapi;
	struct SoundDriverPulse * r = malloc( sizeof( struct SoundDriverPulse ) );

	r->pa_ml = pa_mainloop_new();
	pa_mlapi = pa_mainloop_get_api(r->pa_ml);
	const char * title = GetParameterS( "title", "PA Test" );
	r->pa_ctx = pa_context_new(pa_mlapi, title );
	pa_context_connect(r->pa_ctx, NULL, 0, NULL);

	//TODO: pa_context_set_state_callback

	r->CloseFn = CloseSoundPulse;
	r->SoundStateFn = SoundStatePulse;
	r->callback = cb;

	r->spsPlay = GetParameterI( "samplerate", 44100 );
	r->channelsPlay = GetParameterI( "channels", 2 );
	r->spsRec = r->spsPlay;
	r->channelsRec = r->channelsPlay;
	r->sourceName = GetParameterS( "sourcename", NULL );

	if( strcmp( r->sourceName, "default" ) == 0  )
	{
		r->sourceName = 0;
	}

	r->play = 0;
	r->rec = 0;
	r->buffer = GetParameterI( "buffer", 1024 );
	printf ("Pulse: from: %s (%s) / %dx%d (%d)\n", r->sourceName, title, r->spsPlay, r->channelsPlay, r->buffer );

	memset( &ss, 0, sizeof( ss ) );

	ss.format = PA_SAMPLE_FLOAT32NE;
	ss.rate = r->spsPlay;
	ss.channels = r->channelsPlay;

	r->pa_ready = 0;
	pa_context_set_state_callback(r->pa_ctx, pa_state_cb, &r->pa_ready);

	while (r->pa_ready == 0)
	{
		pa_mainloop_iterate(r->pa_ml, 1, NULL);
	}

	int bufbytes = r->buffer * sizeof(float) * r->channelsRec;

	if( GetParameterI( "play", 1 ) )
	{
		if (!(r->play = pa_stream_new(r->pa_ctx, "Play", &ss, NULL))) {
			error = -3; //XXX ??? TODO
			fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
			goto fail;
		}

		pa_stream_set_underflow_callback(r->play, stream_underflow_cb, NULL);
		pa_stream_set_write_callback(r->play, stream_request_cb, r );

		bufattr.fragsize = (uint32_t)-1;
		bufattr.maxlength = bufbytes*3; //XXX TODO Consider making this -1
		bufattr.minreq = 0;
		bufattr.prebuf =  (uint32_t)-1;
		bufattr.tlength = bufbytes*3;
		int ret = pa_stream_connect_playback(r->play, NULL, &bufattr,
				                    // PA_STREAM_INTERPOLATE_TIMING
				                    // |PA_STREAM_ADJUST_LATENCY //Some servers don't like the adjust_latency flag.
				                    // |PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
					0, NULL, NULL );
		printf( "Play stream.\n" );
		if( ret < 0 )
		{
			fprintf(stderr, __FILE__": (PLAY) pa_stream_connect_playback() failed: %s\n", pa_strerror(ret));
			goto fail;
		}
	}

	if( GetParameterI( "rec", 1 ) )
	{
		if (!(r->rec = pa_stream_new(r->pa_ctx, "Record", &ss, NULL))) {
			error = -3; //XXX ??? TODO
			fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
			goto fail;
		}

		pa_stream_set_read_callback(r->rec, stream_record_cb, r );

		bufattr.fragsize = bufbytes;
		bufattr.maxlength = (uint32_t)-1;//(uint32_t)-1; //XXX: Todo, should this be low?
		bufattr.minreq = bufbytes;
		bufattr.prebuf = (uint32_t)-1;
		bufattr.tlength = bufbytes*3;
		printf( "Source: %s\n", r->sourceName );
		int ret = pa_stream_connect_record(r->rec, r->sourceName, &bufattr, 0
//							       |PA_STREAM_INTERPOLATE_TIMING
			                       |PA_STREAM_ADJUST_LATENCY  //Some servers don't like the adjust_latency flag.
//		                     	|PA_STREAM_AUTO_TIMING_UPDATE
//				0 
				);

		printf( "Got handle: %d\n", ret );
		if( ret < 0 )
		{
			fprintf(stderr, __FILE__": (REC) pa_stream_connect_playback() failed: %s\n", pa_strerror(ret));
			goto fail;
		}
	}

	printf( "Pulse initialized.\n" );


//	SoundThread( r );
	r->thread = OGCreateThread( SoundThread, r );
	return r;

fail:
	if( r )
	{
		if( r->play ) pa_xfree (r->play);
		if( r->rec ) pa_xfree (r->rec);
		free( r );
	}
	return 0;
}



REGISTER_SOUND( PulseSound, 11, "PULSE", InitSoundPulse );


