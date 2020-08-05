//Mostly from: http://www.pezzino.ch/stm32-adc-voltage-monitor/
//Also: http://www.micromouseonline.com/2009/05/26/simple-adc-use-on-the-stm32/

#include "adc.h"
#include <stm32f30x_rcc.h>
#include <stm32f30x_gpio.h>
#include <stm32f30x_adc.h>
#include <stm32f30x_tim.h>
#include <stm32f30x_misc.h>

static int calibration_value;
extern RCC_ClocksTypeDef RCC_Clocks;

#ifdef TQFP32
#define GPIOADCPORT GPIOA
#define GPIOADCNUM  GPIO_Pin_0
#define ADCPLLCLK   RCC_ADC12PLLCLK_Div2
#define ADCPERIPHEN RCC_AHBPeriph_ADC12
#define ADCPORTAHB  RCC_AHBPeriph_GPIOA
#define ADCNUM		ADC1
#define ADCCHAN ADC_Channel_1
#else
#define GPIOADCPORT GPIOB
#define GPIOADCNUM  GPIO_Pin_12
#define ADCPLLCLK   RCC_ADC34PLLCLK_Div2
#define ADCPERIPHEN RCC_AHBPeriph_ADC34
#define ADCPORTAHB  RCC_AHBPeriph_GPIOB
#define ADCNUM		ADC4
#define ADCCHAN ADC_Channel_3
#endif

void InitADC()
{
	ADC_InitTypeDef       ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	GPIO_InitTypeDef      GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* Configure the ADC clock */
	RCC_ADCCLKConfig( ADCPLLCLK );

	/* Enable ADC1 clock */
	RCC_AHBPeriphClockCmd( ADCPERIPHEN, ENABLE );

	/* ADC Channel configuration */
	/* GPIOC Periph clock enable */
	RCC_AHBPeriphClockCmd( ADCPORTAHB, ENABLE );

	/* Configure ADC Channel7 as analog input */
	GPIO_InitStructure.GPIO_Pin = GPIOADCNUM;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init( GPIOADCPORT, &GPIO_InitStructure );

	ADC_StructInit( &ADC_InitStructure );

	/* Calibration procedure */
	ADC_VoltageRegulatorCmd( ADCNUM, ENABLE );

	/* Insert delay equal to 10 µs */
	_delay_us( 10 );

	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Clock = ADC_Clock_AsynClkMode;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_DMAMode = ADC_DMAMode_OneShot;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = 0;
	ADC_CommonInit( ADCNUM, &ADC_CommonInitStructure );

	ADC_InitStructure.ADC_ContinuousConvMode = ADC_ContinuousConvMode_Enable;
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ExternalTrigConvEvent = ADC_ExternalTrigConvEvent_0;
	ADC_InitStructure.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_OverrunMode = ADC_OverrunMode_Disable;
	ADC_InitStructure.ADC_AutoInjMode = ADC_AutoInjec_Disable;
	ADC_InitStructure.ADC_NbrOfRegChannel = 1;
	ADC_Init( ADCNUM, &ADC_InitStructure );

	/* ADC4 regular channel3 configuration */
	ADC_RegularChannelConfig( ADCNUM, ADCCHAN, 1, ADC_SampleTime_181Cycles5 );

	/* Enable ADC4 */
	ADC_Cmd( ADCNUM, ENABLE );

	/* wait for ADRDY */
	while( !ADC_GetFlagStatus( ADCNUM, ADC_FLAG_RDY ) );


	NVIC_InitTypeDef NVIC_InitStructure;
	/* Enable the TIM2 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init (&NVIC_InitStructure);

	/* TIM2 clock enable */
	RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM2, ENABLE);
	/* Time base configuration */
	RCC->CFGR |= 0X1400;
	TIM_TimeBaseStructure.TIM_Period = (RCC_Clocks.HCLK_Frequency/(ADCFS*ADCOVERSAMP)) - 1; 
	TIM_TimeBaseStructure.TIM_Prescaler = 1 - 1; // Operate at clock frequency
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit (TIM2, &TIM_TimeBaseStructure);
	/* TIM IT enable */
	TIM_ITConfig (TIM2, TIM_IT_Update, ENABLE);
	/* TIM2 enable counter */
	TIM_Cmd (TIM2, ENABLE);

	/* Start ADC4 Software Conversion */
	ADC_StartConversion( ADCNUM );
}


void TIM2_IRQHandler (void)
{
	static uint8_t oversamp = 0;
	static int32_t average = 0;
	static int32_t oversampout = 0;

	if (TIM_GetITStatus (TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit (TIM2, TIM_IT_Update);
		int16_t value = ADC_GetConversionValue(ADCNUM);
		ADC_StartConversion( ADCNUM );

		oversampout += value;
		oversamp++;

		if( oversamp >= ADCOVERSAMP )
		{
			value = oversampout / ADCOVERSAMP;
			average = ((average*1023) + (value*1024))/1024;
			value = value-(average/1024);
			oversamp = 0;
			ADCCallback( value );
			oversampout = 0;
		}
	}
}


