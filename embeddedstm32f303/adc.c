//TODO: Consider oversampling the ADC using DMA.
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

void InitADC()
{
	ADC_InitTypeDef       ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	GPIO_InitTypeDef      GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* Configure the ADC clock */
	RCC_ADCCLKConfig( RCC_ADC34PLLCLK_Div2 );

	/* Enable ADC1 clock */
	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_ADC34, ENABLE );

	/* ADC Channel configuration */
	/* GPIOC Periph clock enable */
	RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOB, ENABLE );

	/* Configure ADC Channel7 as analog input */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init( GPIOB, &GPIO_InitStructure );

	ADC_StructInit( &ADC_InitStructure );

	/* Calibration procedure */
	ADC_VoltageRegulatorCmd( ADC4, ENABLE );

	/* Insert delay equal to 10 µs */
	//vTaskDelay(5);
	_delay_us( 10 );

//	ADC_SelectCalibrationMode( ADC4, ADC_CalibrationMode_Single );
//	ADC_StartCalibration( ADC4 );
//	while ( ADC_GetCalibrationStatus( ADC4 ) != RESET );
//	calibration_value = ADC_GetCalibrationValue( ADC4 );

	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Clock = ADC_Clock_AsynClkMode;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_DMAMode = ADC_DMAMode_OneShot;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = 0;
	ADC_CommonInit( ADC4, &ADC_CommonInitStructure );

	ADC_InitStructure.ADC_ContinuousConvMode = ADC_ContinuousConvMode_Enable;
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ExternalTrigConvEvent = ADC_ExternalTrigConvEvent_0;
	ADC_InitStructure.ADC_ExternalTrigEventEdge = ADC_ExternalTrigEventEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_OverrunMode = ADC_OverrunMode_Disable;
	ADC_InitStructure.ADC_AutoInjMode = ADC_AutoInjec_Disable;
	ADC_InitStructure.ADC_NbrOfRegChannel = 1;
	ADC_Init( ADC4, &ADC_InitStructure );

	/* ADC4 regular channel3 configuration */
	ADC_RegularChannelConfig( ADC4, ADC_Channel_3, 1, ADC_SampleTime_181Cycles5 );

	/* Enable ADC4 */
	ADC_Cmd( ADC4, ENABLE );

	/* wait for ADRDY */
	while( !ADC_GetFlagStatus( ADC4, ADC_FLAG_RDY ) );


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
	TIM_TimeBaseStructure.TIM_Period = (RCC_Clocks.HCLK_Frequency/ADCFS) - 1; 
	TIM_TimeBaseStructure.TIM_Prescaler = 1 - 1; // Operate at clock frequency
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit (TIM2, &TIM_TimeBaseStructure);
	/* TIM IT enable */
	TIM_ITConfig (TIM2, TIM_IT_Update, ENABLE);
	/* TIM2 enable counter */
	TIM_Cmd (TIM2, ENABLE);


	/* Start ADC4 Software Conversion */
	ADC_StartConversion( ADC4 );
}


void TIM2_IRQHandler (void) //Timer Interupt for sending data
{
	static int32_t average = 0;
	if (TIM_GetITStatus (TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit (TIM2, TIM_IT_Update);
		int16_t value = ADC_GetConversionValue(ADC4);
		average = ((average*1023) + (value*1024))/1024;
		ADC_StartConversion( ADC4 );
		ADCCallback( value-(average/1024));
	}
}


