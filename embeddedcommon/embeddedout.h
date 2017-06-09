//Copyright 2015 <>< Charles Lohr under the ColorChord License.



#ifndef _EMBEDDEDOUT_H
#define _EMBEDDEDOUT_H

#include "embeddednf.h"

extern int gFRAMECOUNT_MOD_SHIFT_INTERVAL;
extern int gROTATIONSHIFT; //Amount of spinning of pattern around a LED ring
extern uint8_t gCOLORCHORD_SHIFT_INTERVAL; //how frame interval between shifts. if 0 no shift
extern uint8_t gCOLORCHORD_FLIP_ON_PEAK; //if non-zero will cause flipping shift on peaks
extern int8_t gCOLORCHORD_SHIFT_DISTANCE; //distance of shift
extern uint8_t gCOLORCHORD_SORT_NOTES; // 0 no sort, 1 inc freq, 2 dec amps, 3 dec amps2
extern uint8_t gCOLORCHORD_LIN_WRAPAROUND; // 0 no adjusting, else current led display has minimum deviation to prev


//TODO fix
// print debug info wont work on esp8266 need debug to go to usb there
#ifndef DEBUGPRINT
#define DEBUGPRINT 0
#endif

//Controls brightness
#ifndef NOTE_FINAL_AMP
#define NOTE_FINAL_AMP  150   //Number from 0...255
#endif

//Controls, basically, the minimum size of the splotches.
#ifndef NERF_NOTE_PORP
#define NERF_NOTE_PORP 15 //value from 0 to 255
#endif

#ifndef NUM_LIN_LEDS
#define NUM_LIN_LEDS 32
#endif

#ifndef USE_NUM_LIN_LEDS
#define USE_NUM_LIN_LEDS NUM_LIN_LEDS
#endif

/*
#ifndef LIN_WRAPAROUND
//Whether the output lights wrap around.
//(Can't easily run on embedded systems)
//(might be ok on ESP8266)
#define LIN_WRAPAROUND 0
#endif
*/
/*
#ifndef SORT_NOTES
#define SORT_NOTES 1     //FIXED 0 no sort, 1 sort on inc freq, 2 on dec amps, 3 on dec amps2, default inc freq
#endif
*/
extern uint8_t ledArray[];
extern uint8_t ledOut[]; //[NUM_LIN_LEDS*3]
extern uint8_t RootNoteOffset; //Set to define what the root note is.  0 = A.

//For doing the nice linear strip LED updates
//Also added rotation direction changing at peak total amp2 LEDs
void UpdateLinearLEDs();

//For making all the LEDs the same and quickest.  Good for solo instruments?
void UpdateAllSameLEDs();

//For using dominant note as in UpdateAllSameLEDs but display one light and rotate with direction changing at peak total amp2 LEDs
void UpdateRotatingLEDs();


uint32_t ECCtoHEX( uint8_t note, uint8_t sat, uint8_t val );
uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val ); //hue = 0..255 // TODO: TEST ME!!!


#endif

