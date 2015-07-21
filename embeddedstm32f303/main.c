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

	ConfigureLED(); LED_ON;

	//Notes:
	// * CTS pin is connected to PB15 (SPI2_MOSI).
	// * SPI2 is connected to DMA1, on Channel 5.

	//Alternatively, try using the crazy timer-based DMA?
	//This would be good for driving parallel strings, but bad for memory density on one.

	GPIO_InitTypeDef  GPIO_InitStructure;


	//Turn B10 (TX) on, so we can have something to bias the ADC with.
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIOB->ODR |= GPIO_Pin_10;


	/* SysTick end of count event each 10ms */
	SysTick_Config( RCC_Clocks.HCLK_Frequency/100 ); /// 100);

	float fv = RCC_Clocks.HCLK_Frequency/1000000.0;

	InitSPI2812();
	InitADC();
	Init(); //Colorchord

	//printf( "Operating at %.3fMHz\n", fv );

	int this_samp = 0;
	int wf = 0;

	while(1)
	{
		if( this_samp != last_samp_pos )
		{
			LED_OFF; //Use led on the board to show us how much CPU we're using. (You can also probe PB15)

			PushSample32( sampbuff[this_samp] ); //Can't put in full volume.

			this_samp = (this_samp+1)%CIRCBUFSIZE;

			wf++;
			if( wf == 128 )
			{
				NewFrame();
				wf = 0; 
			}
			LED_ON;
		}
		LED_ON; //Take up a little more time to make sure we don't miss this.
	}
}

void TimingDelay_Decrement()
{
/*	static int i;
	static int k;
	int j;
	i++;

	if( i == 100 )
	{
		printf( "%d\n", hit );
		i = hit = 0;
	}

	#define LEDS 20
	uint8_t obuf[LEDS*3];
	for( j = 0; j < LEDS; j++ )
	{
		obuf[j*3+0] = sin((k+j*2)/10.0)*100;
		obuf[j*3+1] = sin((k+j*2+20)/10.0)*100;
		obuf[j*3+2] = sin((k+j*2+40)/10.0)*100;
	}
	k++;
	SendSPI2812( obuf, LEDS );
	LED_ON;
*/
}


