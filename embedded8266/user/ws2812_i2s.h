#ifndef _I2S_TEST
#define _I2S_TEST

//Stuff that should be for the header:

#include <c_types.h>

//Parameters for the I2S DMA behaviour
//#define I2SDMABUFCNT (2)			//Number of buffers in the I2S circular buffer
//#define I2SDMABUFLEN (32*2)		//Length of one buffer, in 32-bit words.

//NOTE: Blocksize MUST be divisible by 4.  Cannot exceed 4092
//Each LED takes up 12 block bytes.
#define WS_BLOCKSIZE 4000

void ICACHE_FLASH_ATTR ws2812_init();
void ws2812_push( uint8_t * buffer, uint16_t buffersize ); //Buffersize = Nr LEDs * 3

#endif

