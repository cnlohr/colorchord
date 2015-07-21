#include <stdint.h>
#include <stdio.h>
#include <systems.h>
#include <math.h>
#include "mp45dt02.h"

#include <DFT32.h>
#include <embeddednf.h>
#include <embeddedout.h>

RCC_ClocksTypeDef RCC_Clocks;

#define CIRCBUFSIZE 256

volatile int last_samp_pos;
int16_t sampbuff[CIRCBUFSIZE];
volatile int samples;

void GotSample( int samp )
{
//	lastsamp = samp;
	sampbuff[last_samp_pos] = samp;
	last_samp_pos = ((last_samp_pos+1)%CIRCBUFSIZE);
	samples++;
}



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

	ConfigureLED(); 

	LED_OFF;

	/* SysTick end of count event each 10ms */
	SysTick_Config( RCC_Clocks.HCLK_Frequency / 100);

	float fv = RCC_Clocks.HCLK_Frequency / 1000000.0f;
//	printf( "Operating at %.3fMHz\n", fv );

	Init();

	InitMP45DT02();
	InitSPI2812();

	int this_samp = 0;
	int wf = 0;

	while(1)
	{

		if( this_samp != last_samp_pos )
		{
			LED_OFF;

			PushSample32( sampbuff[this_samp]/2 ); //Can't put in full volume.

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
	static int i;
	static int k;
	int j;

/*	uint8_t obuf[16*3];
	for( j = 0; j < 16; j++ )
	{
		obuf[j*3+0] = (sampbuff[j]>0)?sampbuff[j]/256:0 ;//(sampbuff[0]&(1<<j))?0xf:0x00;//lastsamppos+10;
		obuf[j*3+1] = (sampbuff[j]<0)?sampbuff[j]/-256:0;
		obuf[j*3+2] = 0;
	}
	k++;
	SendSPI2812( obuf, 16 );*/

}


