#ifndef _HPATIMER_H
#define _HPATIMER_H

#include <c_types.h>
//Using a system timer on the ESP to poll the ADC in at a regular interval...

//BUFFSIZE must be a power-of-two
#define HPABUFFSIZE 512
extern volatile uint8_t sounddata[HPABUFFSIZE];
extern volatile uint16_t soundhead;

void StartHPATimer();


#endif

