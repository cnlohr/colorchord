#ifndef _CCCONFIG_H
#define _CCCONFIG_H

#include <c_types.h>

#define HPABUFFSIZE 512

#define CCEMBEDDED
#define NUM_LIN_LEDS 16
#define DFREQ 16000


#define memcpy ets_memcpy
#define memset ets_memset

extern uint8_t gDFTIIR; //=6
#define DFTIIR gDFTIIR

extern uint8_t gFUZZ_IIR_BITS; //=1
#define FUZZ_IIR_BITS  gFUZZ_IIR_BITS

#define MAXNOTES  12 //MAXNOTES cannot be changed dynamically.

extern uint8_t gFILTER_BLUR_PASSES; //=2
#define FILTER_BLUR_PASSES gFILTER_BLUR_PASSES

extern uint8_t gSEMIBITSPERBIN; //=3
#define SEMIBITSPERBIN gSEMIBITSPERBIN

extern uint8_t gMAX_JUMP_DISTANCE; //=4
#define MAX_JUMP_DISTANCE gMAX_JUMP_DISTANCE

extern uint8_t gMAX_COMBINE_DISTANCE; //=7
#define MAX_COMBINE_DISTANCE gMAX_COMBINE_DISTANCE

extern uint8_t gAMP_1_IIR_BITS; //=4
#define AMP_1_IIR_BITS gAMP_1_IIR_BITS

extern uint8_t gAMP_2_IIR_BITS; //=2
#define AMP_2_IIR_BITS gAMP_2_IIR_BITS

extern uint8_t gMIN_AMP_FOR_NOTE; //=80
#define MIN_AMP_FOR_NOTE gMIN_AMP_FOR_NOTE

extern uint8_t gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR; //=64
#define MINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR

extern uint8_t gNOTE_FINAL_AMP; //=12
#define NOTE_FINAL_AMP gNOTE_FINAL_AMP

extern uint8_t gNERF_NOTE_PORP; //=15
#define NERF_NOTE_PORP gNERF_NOTE_PORP

extern uint8_t gUSE_NUM_LIN_LEDS; // = NUM_LIN_LEDS
#define USE_NUM_LIN_LEDS gUSE_NUM_LIN_LEDS

//We are not enabling  for the ESP8266 port.
//#define LIN_WRAPAROUND 0
//but trying this now
//#define SORT_NOTES 1



#endif

