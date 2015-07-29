//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#ifndef _HTTP_H
#define _HTTP_H

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "espconn.h"
#include "mfs.h"
#include "mystuff.h"

#define HTTP_SERVER_TIMEOUT 500
#define HTTP_CONNECTIONS 8
#define MAX_PATHLEN 80

//You must provide:
void ICACHE_FLASH_ATTR HTTPCustomStart( );
void ICACHE_FLASH_ATTR HTTPCustomCallback( );  //called when we can send more data

void ICACHE_FLASH_ATTR WebSocketData( uint8_t * data, int len );
void ICACHE_FLASH_ATTR WebSocketTick( );
void ICACHE_FLASH_ATTR WebSocketNew();

extern struct HTTPConnection * curhttp;
extern uint8 * curdata;
extern uint16  curlen;

#define POP (*curdata++)

#define HTTP_STATE_NONE        0
#define HTTP_STATE_WAIT_METHOD 1
#define HTTP_STATE_WAIT_PATH   2
#define HTTP_STATE_WAIT_PROTO  3

#define HTTP_STATE_WAIT_FLAG   4
#define HTTP_STATE_WAIT_INFLAG 5
#define HTTP_STATE_DATA_XFER   7
#define HTTP_STATE_DATA_WEBSOCKET   8

#define HTTP_WAIT_CLOSE        15


struct HTTPConnection
{
	uint8_t  state:4;
	uint8_t  state_deets;
	uint8_t  pathbuffer[MAX_PATHLEN]; //Also used for temporary and handshake information when using websockets.
	uint8_t  is_dynamic:1;
	uint16_t timeout;

	union data_t
	{
		struct MFSFileInfo filedescriptor;
		struct UserData { uint16_t a, b, c; } user;
	} data;

	void * rcb;

	uint32_t bytesleft;
	uint32_t bytessofar;

	uint8_t  is404:1;
	uint8_t  isdone:1;
	uint8_t  isfirst:1;
	uint8_t  need_resend:1;

	struct espconn * socket;
} HTTPConnections[HTTP_CONNECTIONS];



void ICACHE_FLASH_ATTR httpserver_connectcb(void *arg);

void HTTPTick( uint8_t timedtick ); 
//you can call this a LOT if you want fast transfers, but be sure only to call it with a 1 at the update tick rate.

int ICACHE_FLASH_ATTR URLDecode( char * decodeinto, int maxlen, const char * buf );

void ICACHE_FLASH_ATTR WebSocketGotData( uint8_t c );
void ICACHE_FLASH_ATTR WebSocketTickInternal();

void ICACHE_FLASH_ATTR WebSocketSend( uint8_t * data, int size );


#endif

