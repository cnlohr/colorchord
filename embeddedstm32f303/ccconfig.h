#ifndef _CCCONFIG_H
#define _CCCONFIG_H

#define CCEMBEDDED

#ifdef TQFP32
#define NUM_LIN_LEDS 20
#define USE_NUM_LIN_LEDS 20
#define DFREQ 12500 //XXX Incorrect.
#else
#define NUM_LIN_LEDS 24
#define USE_NUM_LIN_LEDS 24
#define DFREQ 12500
#endif

#endif

