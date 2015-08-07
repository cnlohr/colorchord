//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#ifndef _COMMON_H
#define _COMMON_H

#include "c_types.h"

//Returns nr bytes to return.  You must allocate retdata.
//It MUST be at least 1,300 bytes large and it MUST be 32-bit aligned.
//NOTE: It is SAFE to use pusrdata and retdata as the same buffer.
int ICACHE_FLASH_ATTR issue_command(char * retdata, int retsize, char *pusrdata, unsigned short len);

//Includes UDP Control + HTTP Interfaces
void ICACHE_FLASH_ATTR CSPreInit();
void ICACHE_FLASH_ATTR CSInit();
void ICACHE_FLASH_ATTR CSTick( int slowtick );

//You must provide:
//Critical should not lock interrupts, just disable services that have problems
//with double-interrupt faults.  I.e. turn off/on any really fast timer interrupts.
//These generally only get called when doing serious operations like reflashing.
void EnterCritical();
void ExitCritical();

//If we receive a command that's not F, E or W (Flash Echo Wifi)
int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len);

#endif

