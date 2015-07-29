//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#ifndef _COMMON_H
#define _COMMON_H

#include "c_types.h"

//Includes UDP Control + HTTP Interfaces
void ICACHE_FLASH_ATTR CSInit();
void ICACHE_FLASH_ATTR CSTick( int slowtick );

//You must provide:
void EnterCritical();
void ExitCritical();

#endif

