#ifndef _ADC_H
#define _ADC_H

#include <stdint.h>
#include "ccconfig.h"
#define ADCFS DFREQ
#define ADCOVERSAMP 4

void InitADC();
void ADCCallback( int16_t value );

#endif
