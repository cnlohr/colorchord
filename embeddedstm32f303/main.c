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


gpio freepin;

RCC_ClocksTypeDef RCC_Clocks;

volatile int adcer;
volatile int hit = 0;

#define CIRCBUFSIZE 256
volatile int last_samp_pos;
int16_t sampbuff[CIRCBUFSIZE];
volatile int samples;


void ADCCallback( uint16_t adcval )
{
	sampbuff[last_samp_pos] = adcval;
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

	RCC_GetClocksFreq( &RCC_Clocks );

	ConfigureLED(); LED_OFF;

	GPIO_InitTypeDef  GPIO_InitStructure;

	//Turn B10 (TX) on, so we can have something positive to bias the ADC with.
	ConfigureGPIO( GetGPIOFromString( "PB10" ), INOUT_OUT | DEFAULT_ON );

	/* SysTick end of count event each 10ms */
	SysTick_Config( RCC_Clocks.HCLK_Frequency/100 ); /// 100);

	float fv = RCC_Clocks.HCLK_Frequency/1000000.0;

	InitSPI2812();
	InitADC();
	Init(); //Colorchord

//	printf( "Operating at %.3fMHz\n", fv );

	freepin = GetGPIOFromString( "PB11" );
	ConfigureGPIO( freepin, INOUT_OUT | DEFAULT_ON );

	int this_samp = 0;
	int wf = 0;

	LED_ON;

	while(1)
	{
		if( this_samp != last_samp_pos )
		{
			GPIOOn( freepin );
			PushSample32( sampbuff[this_samp] ); //Can't put in full volume.

			this_samp = (this_samp+1)%CIRCBUFSIZE;

			wf++;
			if( wf == 128 )
			{
				NewFrame();
				wf = 0; 
			}

			GPIOOff( freepin );
		}
	}
}

void TimingDelay_Decrement()
{
	LED_TOGGLE;
}


