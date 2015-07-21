#ifndef _ADC_H
#define _ADC_H

#include <stdint.h>

#define ADCFS DFREQ

void InitADC();
void ADCCallback( int16_t value );

#endif
