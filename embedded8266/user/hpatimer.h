//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

//This is a tool to make the ESP8266 run its ADC and pipe the samples into 
//the sounddata fifo.

#ifndef _HPATIMER_H
#define _HPATIMER_H

#include <c_types.h>
#include "ccconfig.h" //For DFREQ

//Using a system timer on the ESP to poll the ADC in at a regular interval...

//BUFFSIZE must be a power-of-two
#define HPABUFFSIZE 512
extern volatile uint8_t sounddata[HPABUFFSIZE];
extern volatile uint16_t soundhead;

void StartHPATimer();

void ICACHE_FLASH_ATTR ContinueHPATimer();
void ICACHE_FLASH_ATTR PauseHPATimer();


#endif

