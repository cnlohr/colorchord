#ifndef _COLORCHORD_H
#define _COLORCHORD_H

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"

#define bins 120

void HandleProgressiveInt( int8_t sample1, int8_t sample2 );
void SetupConstants();

#endif


