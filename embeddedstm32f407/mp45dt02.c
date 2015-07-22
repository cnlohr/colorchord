//Mostly stolen from:
//http://electronics.stackexchange.com/questions/98414/stm32f4-discovery-audio-sampling-above-16khz-not-working

#include "mp45dt02.h"
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_spi.h>
#include <pdm_filter.h>

//Warning: uncommenting this uses my PDM filter,
//which is pretty awful and slow.
#define ST_PDM

#ifdef ST_PDM
static PDMFilter_InitStruct pdm_filter;
#endif

void InitMP45DT02()
{

	//Have to enable this because the ST library expects it.
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);

#ifdef ST_PDM
	pdm_filter.LP_HZ=DFREQ*2; //??? This is wrong, but if I make it lower, it really cuts into my bandwidth.
	pdm_filter.HP_HZ=20;
	pdm_filter.Fs=FS;
	pdm_filter.Out_MicChannels=1;
	pdm_filter.In_MicChannels=1;
	PDM_Filter_Init(&pdm_filter);
#endif

    //MP45DT02 CLK-PB10
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin=GPIO_Pin_10;
    gpio.GPIO_Mode=GPIO_Mode_AF;
    gpio.GPIO_OType=GPIO_OType_PP;
    gpio.GPIO_PuPd=GPIO_PuPd_NOPULL;
    gpio.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_SPI2);

    //MP45DT02 DOUT-PC3
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    gpio.GPIO_Pin=GPIO_Pin_3;
    gpio.GPIO_Mode=GPIO_Mode_AF;
    gpio.GPIO_OType=GPIO_OType_PP;
    gpio.GPIO_PuPd=GPIO_PuPd_NOPULL;
    gpio.GPIO_Speed=GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &gpio);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_SPI2);

    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = SPI2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);


	RCC_PLLI2SCmd(ENABLE);

    //I2S2 config
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    I2S_InitTypeDef i2s;
    i2s.I2S_AudioFreq=I2S_FS;
    i2s.I2S_Standard=I2S_Standard_LSB;
    i2s.I2S_DataFormat=I2S_DataFormat_16b;
    i2s.I2S_CPOL=I2S_CPOL_High;
    i2s.I2S_Mode=I2S_Mode_MasterRx;
    i2s.I2S_MCLKOutput=I2S_MCLKOutput_Disable;
    I2S_Init(SPI2, &i2s);
    I2S_Cmd(SPI2, ENABLE);

	//Start
	SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_RXNE, ENABLE);

}


void SPI2_IRQHandler(void)
{
#ifdef ST_PDM
	static uint8_t  InternalBufferSize;
	static uint16_t InternalBuffer[MIC_IN_BUF_SIZE];
#else
	static int32_t  IIRBuffer;
	static int32_t  IIRLowpass;
	static int8_t  IIRPl;
#endif

    if(SPI_GetITStatus(SPI2, SPI_I2S_IT_RXNE))
	{
		uint16_t app = SPI_I2S_ReceiveData(SPI2);

#ifdef ST_PDM
		InternalBuffer[InternalBufferSize++] = HTONS(app);

		if (InternalBufferSize >= MIC_IN_BUF_SIZE)
		{
			int i;
			uint16_t audioRec[MIC_OUT_BUF_SIZE];
			InternalBufferSize = 0;
			//got 64 samples of 16bit => 16 samples of output
			PDM_Filter_64_LSB((uint8_t *)InternalBuffer,
				(uint16_t *)audioRec,
				VOLUME, (PDMFilter_InitStruct *)&pdm_filter);
			for( i = 0; i < MIC_OUT_BUF_SIZE; i++ )
			{
				GotSample( audioRec[i] );
			}
		}
#else
		int i;
		for( i = 0; i < 16; i++ )
		{
			IIRBuffer = IIRBuffer - (IIRBuffer/1024) + (((1<<i)&app)?1:-1)*256;
		}
		IIRPl++;
		if( IIRPl == 4 )
		{
			GotSample( IIRBuffer - IIRLowpass );
			IIRPl = 0;
		}
#endif

    }
}
