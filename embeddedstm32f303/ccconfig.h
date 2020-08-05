#ifndef _CCCONFIG_H
#define _CCCONFIG_H

#define CCEMBEDDED

#include <stdint.h>

extern uint8_t RootNoteOffset; //From embeddedout.c
extern int UseNumLinLeds;

#define NUM_LIN_LEDS 100
#define USE_NUM_LIN_LEDS UseNumLinLeds
#define DFREQ 12500 //Unsure.

#endif

