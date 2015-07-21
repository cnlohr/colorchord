#ifndef _SPI_2812_H
#define _SPI_2812_H

#define SPI2812_MAX_LEDS 300

//24 bits per LED, can fit two bits per byte of output.
#define SPI2812_BUFFSIZE (SPI2812_MAX_LEDS*24/2)


void InitSPI2812();
void SendSPI2812( unsigned char * lightarray, int leds ); //Need one R, G, B per element.

#endif


