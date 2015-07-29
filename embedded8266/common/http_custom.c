//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "http.h"
#include "mystuff.h"
#include "commonservices.h"

static ICACHE_FLASH_ATTR void huge()
{
	uint8_t i = 0;

	START_PACK;
	do
	{
		PushByte( 0 );
		PushByte( i );
	} while( ++i ); //Tricky:  this will roll-over to 0, and thus only execute 256 times.

	EndTCPWrite( curhttp->socket );
}


static ICACHE_FLASH_ATTR void echo()
{
	char mydat[128];
	int len = URLDecode( mydat, 128, curhttp->pathbuffer+8 );

	START_PACK;
	PushBlob( mydat, len );
	EndTCPWrite( curhttp->socket );

	curhttp->state = HTTP_WAIT_CLOSE;
}

static ICACHE_FLASH_ATTR void issue()
{
	char mydat[512];
	int len = URLDecode( mydat, 512, curhttp->pathbuffer+9 );

	issue_command(curhttp->socket, mydat, len );

	curhttp->state = HTTP_WAIT_CLOSE;
}


void ICACHE_FLASH_ATTR HTTPCustomStart( )
{
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/huge", 7 ) == 0 )
	{
		curhttp->rcb = (void(*)())&huge;
		curhttp->bytesleft = 0xffffffff;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/echo?", 8 ) == 0 )
	{
		curhttp->rcb = (void(*)())&echo;
		curhttp->bytesleft = 0xfffffffe;
	}
	else
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/issue?", 9 ) == 0 )
	{
		curhttp->rcb = (void(*)())&issue;
		curhttp->bytesleft = 0xfffffffe;
	}
	else
	{
		curhttp->rcb = 0;
		curhttp->bytesleft = 0;
	}
	curhttp->isfirst = 1;
	HTTPHandleInternalCallback();
}


void ICACHE_FLASH_ATTR HTTPCustomCallback( )
{
	if( curhttp->rcb )
		((void(*)())curhttp->rcb)();
	else
		curhttp->isdone = 1;
}

void ICACHE_FLASH_ATTR WebSocketTick()
{
}

void ICACHE_FLASH_ATTR WebSocketData( uint8_t * data, int len )
{
	char stout[128];
	int stl = ets_sprintf( stout, "output.innerHTML = %d; doSend('x' );\n", curhttp->bytessofar++ );
	WebSocketSend( stout, stl );
}

void ICACHE_FLASH_ATTR NewWebSocket()
{
}

