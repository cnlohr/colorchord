//ColorChordEmbedded implementation on the STM32F407 for the stm32f4-discovery.
//Uses on-board microphone and outputs WS2812 signal for configurable number of LEDs
//on PB5 via DMA+SPI

#include <stdint.h>
#include <stdio.h>
#include <systems.h>
#include <math.h>
#include <stm32f4xx_exti.h>
#include <DFT32.h>
#include <embeddednf.h>
#include <embeddedout.h>
#include "mp45dt02.h"

volatile int wasclicked = 0; //Used for if the root change button was clicked.

RCC_ClocksTypeDef RCC_Clocks;


//Circular buffer for incoming data so we don't spend much time servicing the interrupt and we can handle colorchord in the main thread.
#define CIRCBUFSIZE 256
volatile int last_samp_pos;
int16_t sampbuff[CIRCBUFSIZE];
volatile int samples;

//This gets called by the ADC/Microphone
void GotSample( int samp )
{
	sampbuff[last_samp_pos] = samp;
	last_samp_pos = ((last_samp_pos+1)%CIRCBUFSIZE);
	samples++;
}

//Call this once we've stacked together one full colorchord frame.
void NewFrame()
{
//	uint8_t led_outs[NUM_LIN_LEDS*3];
	int i;
	HandleFrameInfo();

//	UpdateLinearLEDs();
	UpdateAllSameLEDs();

	SendSPI2812( ledOut, NUM_LIN_LEDS );
}


void Configure_PA0(void) {
    /* Set variables used */
    GPIO_InitTypeDef GPIO_InitStruct;
    EXTI_InitTypeDef EXTI_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;
    
    /* Enable clock for GPIOD */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    /* Enable clock for SYSCFG */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    
    /* Set pin as input */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Tell system that you will use PA0 for EXTI_Line0 */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0);
    
    /* PA0 is connected to EXTI_Line0 */
    EXTI_InitStruct.EXTI_Line = EXTI_Line0;
    /* Enable interrupt */
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    /* Interrupt mode */
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    /* Triggers on rising and falling edge */
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    /* Add to EXTI */
    EXTI_Init(&EXTI_InitStruct);
 
    /* Add IRQ vector to NVIC */
    /* PA0 is connected to EXTI_Line0, which has EXTI0_IRQn vector */
    NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;   
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00; /* Set priority */
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;     /* Set sub priority */
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;   /* Enable interrupt */
    NVIC_Init(&NVIC_InitStruct);     /* Add to NVIC */
}

//Handle button press on PA0.
void EXTI0_IRQHandler(void)
{
	static int rootoffset;

    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
	{
		if( wasclicked == 0 )
			wasclicked = 10;
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

int main(void)
{  
	uint32_t i = 0;

	RCC_GetClocksFreq( &RCC_Clocks );

	ConfigureLED(); 

	LED_OFF;

	// SysTick end of count event each 10ms
	SysTick_Config( RCC_Clocks.HCLK_Frequency / 100);

	float fv = RCC_Clocks.HCLK_Frequency / 1000000.0f;
//	We can use printf to print back through the debugging interface, but that's slow and
//  it also takes up a bunch of space.  No printf = no space wasted in printf.
//	printf( "Operating at %.3fMHz\n", fv );

	InitColorChord();
	Configure_PA0();
	InitMP45DT02();
	InitSPI2812();

	int this_samp = 0;
	int wf = 0;

	while(1)
	{

		if( this_samp != last_samp_pos )
		{
			LED_OFF; //Use led on the board to show us how much CPU we're using. (You can also probe PB15)

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
	static int rootoffset;

	//A way of making sure we debounce the button.
	if( wasclicked )
	{
		if( wasclicked == 10 )
		{
			if( rootoffset++ >= 12 ) rootoffset = 0;
			RootNoteOffset = (rootoffset * NOTERANGE) / 12;
		}

		wasclicked--;
	}

}


