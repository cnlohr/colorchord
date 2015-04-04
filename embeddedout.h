#ifndef _EMBEDDEDOUT_H
#define _EMBEDDEDOUT_H

#include "embeddednf.h"


//Controls brightness
#define NOTE_FINAL_AMP  255   //Number from 0...255

//Controls, basically, the minimum size of the splotches.
#define NERF_NOTE_SIZE_VALUE 1

#define NUM_LIN_LEDS 296

#define LIN_WRAPAROUND 1 //Whether the output lights wrap around.

extern uint8_t ledArray[];
extern uint8_t ledOut[]; //[NUM_LIN_LEDS*3]
void UpdateLinearLEDs();

uint32_t ECCtoHEX( uint8_t note, uint8_t sat, uint8_t val );
uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val ); //hue = 0..255 // TODO: TEST ME!!!


#endif

