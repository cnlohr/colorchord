//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include "parameters.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "color.h"
#include "DrawFunctions.h"

#if defined(WIN32) || defined(WINDOWS)
#include <windows.h>
#ifdef TCC
#include <winsock2.h>
#endif
#define MSG_NOSIGNAL 0
#else
#define closesocket( x ) close( x )
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#define MAX_BUFFER 1487*2

struct DPODriver
{
	int leds;
	int skipfirst;
	int fliprg;
	int firstval;
	int port;
	int is_rgby;
	int oldport;
	int skittlequantity; //When LEDs are in a ring, backwards and forwards  This is the number of LEDs in the ring.
	char address[PARAM_BUFF];
	char oldaddress[PARAM_BUFF];
	struct sockaddr_in servaddr;
	int socket;
};


static void DPOUpdate(void * id, struct NoteFinder*nf)
{
	struct DPODriver * d = (struct DPODriver*)id;
#ifdef WIN32
	static int wsa_did_start;
	if( !wsa_did_start )
	{
		
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;
		wVersionRequested = MAKEWORD(2, 2);
		err = WSAStartup(wVersionRequested, &wsaData);
	}
#endif

	int i, j;

	if( strcmp( d->oldaddress, d->address ) != 0 || d->socket == -1 || d->oldport != d->port )
	{
		d->socket = socket(AF_INET,SOCK_DGRAM,0);


		struct hostent *hname;
		hname = gethostbyname(d->address);

		if( hname )
		{
			memset(&d->servaddr, 0, sizeof(d->servaddr));
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
		uint8_t buffer[MAX_BUFFER];
		uint8_t lbuff[MAX_BUFFER];

		d->firstval = 0;
		i = 0;
		while( i < d->skipfirst )
		{
			lbuff[i] = d->firstval;
			buffer[i++] = d->firstval;
		}

		if( d->is_rgby )
		{
			i = d->skipfirst;
			int k = 0;
			if( d->leds * 4 + i >= MAX_BUFFER )
				d->leds = (MAX_BUFFER-1)/4;

			//Copy from OutLEDs[] into buffer, with size i.
			for( j = 0; j < d->leds; j++ )
			{
				int r = OutLEDs[k++];
				int g = OutLEDs[k++];
				int b = OutLEDs[k++];
				int y = 0;
				int rg_common;
				if( r/2 > g ) rg_common = g;
				else        rg_common = r/2;

				if( rg_common > 255 ) rg_common = 255;
				y = rg_common;
				r -= rg_common;
				g -= rg_common;
				if( r < 0 ) r = 0;
				if( g < 0 ) g = 0;

				//Conversion from RGB to RAGB.  Consider: A is shifted toward RED.
				buffer[i++] = g; //Green
				buffer[i++] = r; //Red
				buffer[i++] = b; //Blue
				buffer[i++] = y; //Amber
			}
		}
		else
		{
			if( d->fliprg )
			{
				for( j = 0; j < d->leds; j++ )
				{
					lbuff[i++] = OutLEDs[j*3+1]; //GREEN??
					lbuff[i++] = OutLEDs[j*3+0]; //RED??
					lbuff[i++] = OutLEDs[j*3+2]; //BLUE
				}
			}
			else
			{
				for( j = 0; j < d->leds; j++ )
				{
					lbuff[i++] = OutLEDs[j*3+0];  //RED
					lbuff[i++] = OutLEDs[j*3+2];  //BLUE
					lbuff[i++] = OutLEDs[j*3+1];  //GREEN
				}
			}

			if( d->skittlequantity )
			{
				i = d->skipfirst;
				for( j = 0; j < d->leds; j++ )
				{
					int ledw = j;
					int ledpor = ledw % d->skittlequantity;
					int ol;

					if( ledw >= d->skittlequantity )
					{
						ol = ledpor*2-1;
						ol = d->skittlequantity*2 - ol -2;
					}
					else
					{
						ol = ledpor*2;
					}

					buffer[i++] = lbuff[ol*3+0+d->skipfirst];
					buffer[i++] = lbuff[ol*3+1+d->skipfirst];
					buffer[i++] = lbuff[ol*3+2+d->skipfirst];
				}
			}
			else
			{
				memcpy( buffer, lbuff, i );
			}
		}

		int r = sendto( d->socket, buffer, i, MSG_NOSIGNAL,(const struct sockaddr *) &d->servaddr, sizeof( d->servaddr ) );
		if( r < 0 )
		{
			fprintf( stderr, "Send fault.\n" );
			closesocket( d->socket );
			d->socket = -1;
		}
	}

}

static void DPOParams(void * id )
{
	struct DPODriver * d = (struct DPODriver*)id;
	strcpy( d->address, "localhost" );

	d->leds = 10;		RegisterValue(  "leds", PAINT, &d->leds, sizeof( d->leds ) );
	d->skipfirst = 1;	RegisterValue(  "skipfirst", PAINT, &d->skipfirst, sizeof( d->skipfirst ) );
	d->port = 7777;		RegisterValue(  "port", PAINT, &d->port, sizeof( d->port ) );
	d->firstval = 0;	RegisterValue(  "firstval", PAINT, &d->firstval, sizeof( d->firstval ) );
						RegisterValue(  "address", PABUFFER, d->address, sizeof( d->address ) );
	d->fliprg = 0;		RegisterValue(  "fliprg", PAINT, &d->fliprg, sizeof( d->fliprg ) );
	d->is_rgby = 0;		RegisterValue(  "rgby", PAINT, &d->is_rgby, sizeof( d->is_rgby ) );
	d->skittlequantity=0;RegisterValue(  "skittlequantity", PAINT, &d->skittlequantity, sizeof( d->skittlequantity ) );
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


