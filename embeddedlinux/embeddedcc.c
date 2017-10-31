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
#include <stdlib.h>   // Added by [olel] for atoi
#include "embeddedout.h"

struct sockaddr_in servaddr;
int sock;

#define expected_lights NUM_LIN_LEDS

int toskip = 1;

void NewFrame()
{
	int i;
	char buffer[3000];

	HandleFrameInfo();
	UpdateLinearLEDs();

	buffer[0] = 0;
	buffer[1] = 0;
	buffer[2] = 0;

	for( i = 0; i < expected_lights * 3; i++ )
	{
		buffer[i+toskip*3] = ledOut[i];
	}

	int r = send(sock,buffer,expected_lights*3+3,0);
}


int main( int argc, char ** argv )
{
	int wf = 0;
	int ci;

	if( argc < 2 )
	{
		fprintf( stderr, "Error: usage: [tool] [ip address] [num to skip, default 0]\n" );
		return -1;
	}

	printf( "%d\n", argc );
	toskip = (argc > 2)?atoi(argv[2]):0;

	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	servaddr.sin_port=htons(7777);

	connect( sock, (struct sockaddr *)&servaddr, sizeof(servaddr) );

	InitColorChord();  // Changed by [olel] cause this was changed in embeddednf

	while( ( ci = getchar() ) != EOF )
	{
		int cs = ci - 0x80;
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

