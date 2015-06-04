#include "ws2812.h"
#include "ets_sys.h"
#include "mystuff.h"
#include "osapi.h"

#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
	gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)


//I just used a scope to figure out the right time periods.

void SEND_WS_0()
{
	uint8_t time = 8;
	WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	while(time--)
	{
		WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );
	}

}

void SEND_WS_1()
{
	uint8_t time = 9;
	while(time--)
	{
		WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	}
	time = 3;
	while(time--)
	{
		WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );
	}

}


void WS2812OutBuffer( uint8_t * buffer, uint16_t length )
{
	uint16_t i;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(WSGPIO), 0);
	for( i = 0; i < length; i++ )
	{
		uint8_t byte = buffer[i];
		if( byte & 0x80 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x40 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x20 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x10 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x08 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x04 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x02 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x01 ) SEND_WS_1(); else SEND_WS_0();
	}
	//reset will happen when it's low long enough.
	//(don't call this function twice within 10us)
}



