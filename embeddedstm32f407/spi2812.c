//Parts stolen from:
// https://javakys.wordpress.com/2014/09/04/how-to-implement-full-duplex-spi-communication-using-spi-dma-mode-on-stm32f2xx-or-stm32f4xx/
//Timing from:
// https://cpldcpu.wordpress.com/2014/01/14/light_ws2812-library-v2-0-part-i-understanding-the-ws2812/
//Also...
// https://my.st.com/public/STe2ecommunities/mcu/Lists/STM32Discovery/Flat.aspx?RootFolder=https%3a%2f%2fmy.st.com%2fpublic%2fSTe2ecommunities%2fmcu%2fLists%2fSTM32Discovery%2fSTM32F3%20SPI%20slave%20receive%20with%20DMA&FolderCTID=0x01200200770978C69A1141439FE559EB459D75800084C20D8867EAD444A5987D47BE638E0F&currentviews=1336
// For the '407... https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=https%3a%2f%2fmy.st.com%2fpublic%2fSTe2ecommunities%2fmcu%2fLists%2fcortex_mx_stm32%2fSPI1%20in%20DMA2%20on%20STM32F4&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=2447
// Also for the '407... http://www.micromouseonline.com/2012/03/11/adding-dma-to-the-spi-driver-with-the-stm32f4/


#include <string.h>
#include <stdint.h>

#include "spi2812.h"

//Number of 0 segments to transmit ahead of time to reset string.
#define ZERO_BUFFER 6

uint8_t MyBuffer[SPI2812_BUFFSIZE+ZERO_BUFFER];



#ifdef STM32F30X
#include <stm32f30x_rcc.h>
#include <stm32f30x_gpio.h>
#include <stm32f30x_spi.h>
#include <stm32f30x_dma.h>
#include <stm32f30x_misc.h>

	//Currently, the 30X version does not have configurable ports.


#define SPI_PORT                  SPI2
#define SPI_PORT_CLOCK            RCC_APB1Periph_SPI2
#define SPI_PORT_CLOCK_INIT       RCC_APB1PeriphClockCmd
#define SPI_MOSI_PIN             GPIO_Pin_15
#define SPI_MOSI_GPIO_PORT       GPIOB
#define SPI_MOSI_GPIO_CLK			RCC_AHBPeriph_GPIOB
#define SPI_MOSI_SOURCE          GPIO_PinSource15
#define SPI_MOSI_AF 			GPIO_AF_5
#define SPI_PORT_DR_ADDRESS                SPI_PORT->DR
#define SPI_PORT_DMA                       DMA1
#define SPI_PORT_DMAx_CLK                  RCC_AHBPeriph_DMA1
#define SPI_PORT_TX_DMA_CHANNEL            DMA1_Channel5
#define SPI_PORT_DMA_TX_IRQn               DMA1_Channel5_IRQn

#define DMA_HANDLER_IRQFN							DMA1_Channel5_IRQHandler
#define DMA_FLAG_C					DMA1_FLAG_TC5
#define DMA_FLAG_E					DMA1_FLAG_TE5
/*
#define SPI_PORT_TX_DMA_STREAM             DMA1_Stream4
#define SPI_PORT_TX_DMA_FLAG_FEIF          DMA_FLAG_FEIF4
#define SPI_PORT_TX_DMA_FLAG_DMEIF         DMA_FLAG_DMEIF4
#define SPI_PORT_TX_DMA_FLAG_TEIF          DMA_FLAG_TEIF4
#define SPI_PORT_TX_DMA_FLAG_HTIF          DMA_FLAG_HTIF4
#define SPI_PORT_TX_DMA_FLAG_TCIF          DMA_FLAG_TCIF4
#define SPI_PORT_DMA_TX_IRQn               DMA1_Stream4_IRQn
#define SPI_PORT_DMA_TX_IRQHandler         DMA1_Stream4_IRQHandler
*/

#elif defined( STM32F40_41xxx )

#include <stm32f4xx_rcc.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_spi.h>
#include <stm32f4xx_dma.h>
#include <misc.h>

#define SPI_PORT                  SPI1
#define SPI_PORT_CLOCK            RCC_APB2Periph_SPI1
#define SPI_PORT_CLOCK_INIT       RCC_APB2PeriphClockCmd
#define SPI_MOSI_PIN             GPIO_Pin_5
#define SPI_MOSI_GPIO_PORT       GPIOB
#define SPI_MOSI_GPIO_CLK        RCC_AHB1Periph_GPIOB
#define SPI_MOSI_SOURCE          GPIO_PinSource5
#define SPI_MOSI_AF              GPIO_AF_SPI1
#define SPI_PORT_DR_ADDRESS                SPI_PORT->DR
#define SPI_PORT_DMA                       DMA2
#define SPI_PORT_DMAx_CLK                  RCC_AHB1Periph_DMA2
#define SPI_PORT_TX_DMA_CHANNEL            DMA_Channel_3
#define SPI_PORT_TX_DMA_STREAM             DMA2_Stream3
#define SPI_PORT_TX_DMA_FLAG_FEIF          DMA_FLAG_FEIF3
#define SPI_PORT_TX_DMA_FLAG_DMEIF         DMA_FLAG_DMEIF3
#define SPI_PORT_TX_DMA_FLAG_TEIF          DMA_FLAG_TEIF3
#define SPI_PORT_TX_DMA_FLAG_HTIF          DMA_FLAG_HTIF3
#define SPI_PORT_TX_DMA_FLAG_TCIF          DMA_FLAG_TCIF3
#define SPI_PORT_DMA_TX_IRQn               DMA2_Stream3_IRQn
#define SPI_PORT_DMA_TX_IRQHandler         DMA2_Stream3_IRQHandler
#define DMA_FLAGE							DMA_IT_TCIF3

#endif




#ifdef STM32F30X

void DMA_HANDLER_IRQFN() 
{
    if ( DMA_GetFlagStatus(DMA_FLAG_C) == SET )
    {
        DMA_ClearITPendingBit(DMA_FLAG_C);
    }

    /* Transfer Error */
    if(DMA_GetITStatus(DMA_FLAG_E) == SET)
	{
	    if (DMA_GetFlagStatus(DMA_FLAG_E) != RESET)
	    {
	        DMA_ClearITPendingBit(DMA_FLAG_E);
		}
	}
}

#elif defined( STM32F40_41xxx )

void SPI_PORT_DMA_TX_IRQHandler(){

  // Test if DMA Stream Transfer Complete interrupt
  if (DMA_GetITStatus(SPI_PORT_TX_DMA_STREAM, DMA_FLAGE)) {
    DMA_ClearITPendingBit(SPI_PORT_TX_DMA_STREAM, DMA_FLAGE);
  }


}
 
#endif

void InitSPI2812()
{
	SPI_InitTypeDef SPI_InitStructure;
	DMA_InitTypeDef dma_init_struct;
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef nvic_init_struct;

#ifdef STM32F30X

	//On SPI2, PORT B.15

	RCC_AHBPeriphClockCmd(SPI_MOSI_GPIO_CLK, ENABLE);
	RCC_AHBPeriphClockCmd(SPI_PORT_DMAx_CLK, ENABLE);
	SPI_PORT_CLOCK_INIT(SPI_PORT_CLOCK, ENABLE);
	SPI_I2S_DeInit(SPI_PORT);

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = SPI_MOSI_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

	GPIO_PinAFConfig(SPI_MOSI_GPIO_PORT, SPI_MOSI_SOURCE, SPI_MOSI_AF); //MOSI  //GPIO_AF_5??? GPIO_AF_6??? TODO 

	/* SPI Config */
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;  //TODO: Remove this???

	SPI_CalculateCRC(SPI_PORT, DISABLE);
	SPI_Init(SPI_PORT, &SPI_InitStructure);
    RCC_I2SCLKConfig( RCC_I2S2CLKSource_SYSCLK);
	SPI_Cmd(SPI_PORT, ENABLE);

	//We're on DMA1, channel 5.

	memset( MyBuffer, 0xAA, 128 );

    DMA_DeInit(SPI_PORT_TX_DMA_CHANNEL);
    DMA_StructInit(&dma_init_struct);
    dma_init_struct.DMA_PeripheralBaseAddr = (uint32_t) &SPI_PORT->DR;
    dma_init_struct.DMA_MemoryBaseAddr = (uint32_t)MyBuffer;  //REmove this? somehow?
    dma_init_struct.DMA_DIR = DMA_DIR_PeripheralDST;
    dma_init_struct.DMA_BufferSize = 128;
    dma_init_struct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init_struct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init_struct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init_struct.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
    dma_init_struct.DMA_Mode = DMA_Mode_Normal;
	dma_init_struct.DMA_Priority = DMA_Priority_VeryHigh;
	dma_init_struct.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(SPI_PORT_TX_DMA_CHANNEL, &dma_init_struct);

	/* Configure the Priority Group */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	/* Initialize NVIC Struct for DMA */
	nvic_init_struct.NVIC_IRQChannel = SPI_PORT_DMA_TX_IRQn;
	nvic_init_struct.NVIC_IRQChannelPreemptionPriority = 1;
	nvic_init_struct.NVIC_IRQChannelSubPriority = 0;
	nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init_struct);

	SPI_I2S_DMACmd(SPI_PORT, SPI_I2S_DMAReq_Tx, ENABLE);

#elif defined( STM32F40_41xxx )

	// enable the SPI peripheral clock
	SPI_PORT_CLOCK_INIT(SPI_PORT_CLOCK, ENABLE);
	// enable the peripheral GPIO port clocks
	RCC_AHB1PeriphClockCmd(SPI_MOSI_GPIO_CLK, ENABLE);
	// Connect SPI pins to AF5 - see section 3, Table 6 in the device datasheet
	GPIO_PinAFConfig(SPI_MOSI_GPIO_PORT, SPI_MOSI_SOURCE, SPI_MOSI_AF);
	// now configure the pins themselves
	// they are all going to be fast push-pull outputs
	// but the SPI pins use the alternate function
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = SPI_MOSI_PIN;
	GPIO_Init(SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
	// now we can set up the SPI peripheral
	// Assume the target is write only and we look after the chip select ourselves
	// SPI clock rate will be system frequency/4/prescaler
	// so here we will go for 72/4/8 = 2.25MHz
	SPI_I2S_DeInit(SPI_PORT);
	SPI_StructInit(&SPI_InitStructure);
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; //On F104, assume 100MHz.
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_Init(SPI_PORT, &SPI_InitStructure);
	// Enable the SPI port
	SPI_Cmd(SPI_PORT, ENABLE);


	// first enable the clock
	RCC_AHB1PeriphClockCmd(SPI_PORT_DMAx_CLK, ENABLE);
	// start with a blank DMA configuration just to be sure
	DMA_DeInit(SPI_PORT_TX_DMA_STREAM);

	while (DMA_GetCmdStatus (SPI_PORT_TX_DMA_STREAM) != DISABLE);

	// Configure DMA controller to manage TX DMA requests
	// first make sure we are using the default values
	DMA_StructInit(&dma_init_struct);
	// these are the only parameters that change from the defaults
	dma_init_struct.DMA_PeripheralBaseAddr = (uint32_t) & (SPI_PORT->DR);
	dma_init_struct.DMA_Channel = SPI_PORT_TX_DMA_CHANNEL;
	dma_init_struct.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	dma_init_struct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	/*
	* It is not possible to call DMA_Init without values for the source
	* address and non-zero size even though a transfer is not done here.
	* These are checked only when the assert macro are active though.
	*/
	dma_init_struct.DMA_Memory0BaseAddr = (uint32_t)MyBuffer;
	dma_init_struct.DMA_BufferSize = 1;
	DMA_Init(SPI_PORT_TX_DMA_STREAM, &dma_init_struct);
	// Enable the DMA transfer complete interrupt
	DMA_ITConfig(SPI_PORT_TX_DMA_STREAM, DMA_IT_TC, ENABLE);


	DMA_ITConfig (SPI_PORT_TX_DMA_STREAM, DMA_IT_TC, ENABLE);
	// enable the interrupt in the NVIC
	nvic_init_struct.NVIC_IRQChannel = SPI_PORT_DMA_TX_IRQn;
	nvic_init_struct.NVIC_IRQChannelPreemptionPriority = 0;
	nvic_init_struct.NVIC_IRQChannelSubPriority = 1;
	nvic_init_struct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init (&nvic_init_struct);
	// Enable dma tx request.
	SPI_I2S_DMACmd (SPI_PORT, SPI_I2S_DMAReq_Tx, ENABLE);

#endif

//These would fire it off...
//    DMA_Cmd(DMA1_Channel5, ENABLE);
//    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
//    DMA_ITConfig(DMA1_Channel5, DMA_IT_TE, ENABLE);

}

void SendSPI2812( unsigned char * lightarray, int length )
{
	static const uint8_t aoarray[4] = { 0b10001000, 0b10001110, 0b11101000, 0b11101110 };

	int i;
	if( length > SPI2812_MAX_LEDS ) length = SPI2812_MAX_LEDS;

	MyBuffer[0] = 0;
	for( i = 0; i < length; i++ )
	{
		uint8_t * colorbase = &lightarray[i*3];
		uint8_t * buffbase = &MyBuffer[i*24/2+ZERO_BUFFER];

		int j;
		for( j = 0; j < 3; j++ )
		{
			uint8_t c = colorbase[j];// (j==0)?1:(j==1)?0:2 ]; //Flip R and G.

			*(buffbase++) = aoarray[(c>>6)&3];
			*(buffbase++) = aoarray[(c>>4)&3];
			*(buffbase++) = aoarray[(c>>2)&3];
			*(buffbase++) = aoarray[(c>>0)&3];
		}
	}

	for( i = 0; i < ZERO_BUFFER; i++ )
		MyBuffer[i] = 0;

	length *= 24/2;
	length += ZERO_BUFFER;

#ifdef STM32F30X

    DMA_Cmd(SPI_PORT_TX_DMA_CHANNEL, DISABLE);
    SPI_PORT_TX_DMA_CHANNEL->CMAR = (uint32_t) MyBuffer;
    SPI_PORT_TX_DMA_CHANNEL->CNDTR = (uint16_t) length;
    DMA_Cmd(SPI_PORT_TX_DMA_CHANNEL, ENABLE);
    DMA_ITConfig(SPI_PORT_TX_DMA_CHANNEL, DMA_IT_TC, ENABLE);
    DMA_ITConfig(SPI_PORT_TX_DMA_CHANNEL, DMA_IT_TE, ENABLE);

#elif defined( STM32F40_41xxx )

  SPI_PORT_TX_DMA_STREAM->NDTR = (uint32_t) length;
  SPI_PORT_TX_DMA_STREAM->M0AR = (uint32_t) MyBuffer;
  DMA_Cmd (SPI_PORT_TX_DMA_STREAM, ENABLE);

#endif

}




