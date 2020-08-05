#include <stdint.h>
#include <stm32f30x_rcc.h>
#include <stm32f30x_gpio.h>
#include <stm32f30x_dbgmcu.h>
#include <stdio.h>
#include <math.h>
#include <systems.h>
#include <DFT32.h>
#include <embeddednf.h>
#include <embeddedout.h>
#include "spi2812.h"
#include "adc.h"

gpio freepin;

RCC_ClocksTypeDef RCC_Clocks;

volatile int adcer;
volatile int hit = 0;

#define CIRCBUFSIZE 256
volatile int last_samp_pos;
int16_t sampbuff[CIRCBUFSIZE];
volatile int samples;




int UseNumLinLeds = 20;
int Gain = 10;

void ADCCallback( int16_t adcval )
{
#ifdef TQFP32
	sampbuff[last_samp_pos] = adcval*Gain;
#else
	sampbuff[last_samp_pos] = adcval;
#endif
	last_samp_pos = ((last_samp_pos+1)%CIRCBUFSIZE);
	samples++;
}


//Call this once we've stacked together one full colorchord frame.
void NewFrame()
{
	uint8_t led_outs[NUM_LIN_LEDS*3];
	int i;
	HandleFrameInfo();
	UpdateLinearLEDs();

	SendSPI2812( ledOut, NUM_LIN_LEDS );
}


int main(void)
{  
	uint32_t i = 0;

	send_text( "TEST1\n" );

	RCC_GetClocksFreq( &RCC_Clocks );


	GPIO_InitTypeDef  GPIO_InitStructure;

	//Turn B10 (TX) on, so we can have something positive to bias the ADC with.
#ifndef TQFP32
	ConfigureLED(); LED_OFF;
	ConfigureGPIO( GetGPIOFromString( "PB10" ), INOUT_OUT | DEFAULT_ON );
#endif

	/* SysTick end of count event each 10ms */
	SysTick_Config( RCC_Clocks.HCLK_Frequency/100 ); /// 100);

	float fv = RCC_Clocks.HCLK_Frequency/1000000.0;

	InitSPI2812();
	InitADC();
	InitColorChord(); //Colorchord

//	printf( "Operating at %.3fMHz\n", fv );


#ifndef TQFP32
	freepin = GetGPIOFromString( "PB11" );
	ConfigureGPIO( freepin, INOUT_OUT | DEFAULT_ON );
#endif

	int this_samp = 0;
	int wf = 0;

#ifndef TQFP32
	LED_ON;
#endif

	while(1)
	{
		if( this_samp != last_samp_pos )
		{
#ifndef TQFP32
			GPIOOn( freepin );
#endif
			PushSample32( sampbuff[this_samp] ); //Can't put in full volume.

			this_samp = (this_samp+1)%CIRCBUFSIZE;

			wf++;
			if( wf == 128 )
			{
				NewFrame();
				wf = 0; 
			}
#ifndef TQFP32
			GPIOOff( freepin );
#endif
		}
	}
}

void TimingDelay_Decrement()
{
#ifndef TQFP32
	LED_TOGGLE;
#endif
}


