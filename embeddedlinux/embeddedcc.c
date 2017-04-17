//
// This is the teststrap for the Embedded ColorChord System.
// It is intended as a minimal scaffolding for testing Embedded ColorChord.
//
//
// Copyright 2015 <>< Charles Lohr, See LICENSE file for license info.


#include "embeddednf.h"
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include "embeddedout.h"

struct sockaddr_in servaddr;
int sock;

#define expected_lights NUM_LIN_LEDS
int toskip = 1;
int framecount = 0;
//TODO explore relation of NUM_LIN_LEDS and following two params
// ratio of gCOLORCHORD_SHIFT_INTERVAL / gCOLORCHORD_SHIFT_DISTANCE when less than 1 has interesting effect
uint8_t gCOLORCHORD_SHIFT_INTERVAL = 5;
uint8_t gCOLORCHORD_FLIP_ON_PEAK = 0; // non-zero will flip on peak total amp2
int8_t gCOLORCHORD_SHIFT_DISTANCE = 0; //distance of shift + anticlockwise, - clockwise, 0 no shift


void NewFrame()
{
	int i;
	char buffer[3000];
        framecount++;
	//printf("NEW FRAME %d ******\n", framecount);
	HandleFrameInfo();
	UpdateLinearLEDs();
	//UpdateAllSameLEDs();
	//UpdateRotatingLEDs();

	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;

	for( i = 0; i < expected_lights * 3; i++ )
	{
		buffer[i+toskip*3] = ledOut[i];
// printf("In NF: %d\n", ledOut[i]);

	}

	int r = send(sock,buffer,expected_lights*3+3,0);
}


int main( int argc, char ** argv )
{
	int wf = 0;
	int ci;
	// bb no [tool] argument expected or used?

	if( argc < 2 )
	{
		fprintf( stderr, "Error: usage: [tool] [ip address] [num to skip, default 0]\n" );
		return -1;
	}

	printf( "%d\n", argc );
        for (int i = 0; i < argc; i++)
        {     printf( "%s\n", argv[i] );
        }
	toskip = (argc > 2)?atoi(argv[2]):0;
        fprintf( stderr, "   toskip = %d \n", toskip );

	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	servaddr.sin_port=htons(7777);
	fprintf(stderr, "The IP address is %s\n", inet_ntoa(servaddr.sin_addr.s_addr));
	connect( sock, (struct sockaddr *)&servaddr, sizeof(servaddr) );

	InitColorChord(); // bb changed from Init() which does not seem to exist;

	while( ( ci = getchar() ) != EOF )
	{
		int cs = ci - 0x80;
//printf("byte: %d\n", cs);
// putchar(ci);
#ifdef USE_32DFT
		PushSample32( ((int8_t)cs)*32 );
#else
		Push8BitIntegerSkippy( (int8_t)cs );
#endif
		wf++;
		if( wf == 128 )
		{
			NewFrame();
			wf = 0; 
		}
	}
	return 0;
}

