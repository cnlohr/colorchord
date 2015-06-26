#ifndef _WS2812_H
#define _WS2812_H

#define WSGPIO 0

#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "gpio.h"

//You will have to 	os_intr_lock();  	os_intr_unlock();

void WS2812OutBuffer( uint8_t * buffer, uint16_t length );

#endif

