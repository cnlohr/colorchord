#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include "parameters.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "color.h"
#include "DrawFunctions.h"
#include <unistd.h>

#define MAX_BUFFER 1487

struct DPODriver
{
	int leds;
	int skipfirst;
	int firstval;
	int port;
	int oldport;
	char address[PARAM_BUFF];
	char oldaddress[PARAM_BUFF];
	struct sockaddr_in servaddr;
	int socket;
};


static void DPOUpdate(void * id, struct NoteFinder*nf)
{
	struct DPODriver * d = (struct DPODriver*)id;
	int i, j;

	if( strcmp( d->oldaddress, d->address ) != 0 || d->socket == -1 || d->oldport != d->port )
	{
		d->socket = socket(AF_INET,SOCK_DGRAM,0);


		struct hostent *hname;
		hname = gethostbyname(d->address);

		if( hname )
		{
			bzero(&d->servaddr, sizeof(d->servaddr));
			d->servaddr.sin_family = hname->h_addrtype;
			d->servaddr.sin_port = htons( d->port );
			d->servaddr.sin_addr.s_addr = *(long*)hname->h_addr;

			if( d->socket >= 0 )
			{
				d->oldport = d->port;
				memcpy( d->oldaddress, d->address, PARAM_BUFF );
			}
			else
			{
				fprintf( stderr, "Socket Error.\n" );
			}
		}
		else
		{
			fprintf( stderr, "Error: Cannot find host \"%s\":%d\n", d->address, d->port );
		}
	}

	if( d->socket > 0 )
	{
		char buffer[MAX_BUFFER];
		i = 0;
		while( i < d->skipfirst )
			buffer[i++] = d->firstval;

		if( d->leds * 3 + i >= MAX_BUFFER )
			d->leds = (MAX_BUFFER-1)/3;

		for( j = 0; j < d->leds; j++ )
		{
			buffer[i++] = OutLEDs[j*3+0];  //RED
			buffer[i++] = OutLEDs[j*3+2];  //BLUE
			buffer[i++] = OutLEDs[j*3+1];  //GREEN
		}
		int r = sendto( d->socket, buffer, i, MSG_NOSIGNAL, &d->servaddr, sizeof( d->servaddr ) );
		if( r < 0 )
		{
			fprintf( stderr, "Send fault.\n" );
			close( d->socket );
			d->socket = -1;
		}
	}

}

static void DPOParams(void * id )
{
	struct DPODriver * d = (struct DPODriver*)id;
	strcpy( d->address, "localhost" );

	d->leds = 10;		RegisterValue(  "leds", PINT, &d->leds, sizeof( d->leds ) );
	d->skipfirst = 1;	RegisterValue(  "skipfirst", PINT, &d->skipfirst, sizeof( d->skipfirst ) );
	d->port = 7777;		RegisterValue(  "port", PINT, &d->port, sizeof( d->port ) );
	d->firstval = 0;	RegisterValue(  "firstval", PINT, &d->firstval, sizeof( d->firstval ) );
						RegisterValue(  "address", PBUFFER, d->address, sizeof( d->address ) );
	d->socket = -1;
	d->oldaddress[0] = 0;
}

static struct DriverInstances * DisplayNetwork(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct DPODriver * d = ret->id = malloc( sizeof( struct DPODriver ) );
	memset( d, 0, sizeof( struct DPODriver ) );
	ret->Func = DPOUpdate;
	ret->Params = DPOParams;
	DPOParams( d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplayNetwork);


