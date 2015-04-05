//
// This is the teststrap for the Embedded ColorChord System.
// It is intended as a minimal scaffolding for testing Embedded ColorChord.
//


#include "embeddednf.h"
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include "embeddedout.h"

struct sockaddr_in servaddr;
int sock;

#define expected_lights 296

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
		buffer[i+3] = ledOut[i];
	}

	int r = send(sock,buffer,expected_lights*3+3,0);
}


int main()
{
	int wf = 0;
	int ci;

	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	printf( "%d\n", sock );

	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("192.168.0.245");
	servaddr.sin_port=htons(7777);

	connect( sock, (struct sockaddr *)&servaddr, sizeof(servaddr) );

	Init();

	while( ( ci = getchar() ) != EOF )
	{
		int cs = ci - 0x80;
#ifdef USE_32DFT
		PushSample32( ((int8_t)cs)*32 );
#else
		Push8BitIntegerSkippy( (int8_t)cs );
#endif
		wf++;
		if( wf == 64 )
		{
			NewFrame();
			wf = 0; 
		}
	}
	return 0;
}

