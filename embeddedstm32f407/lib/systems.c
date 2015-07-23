#include <stdint.h>
#include "systems.h"
#include <string.h>
#ifdef STM32F30X
#include <stm32f30x.h>
#include <stm32f30x_rcc.h>
#include <stm32f30x_gpio.h>
#elif defined( STM32F40_41xxx )
#include <stm32f4xx.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_gpio.h>
#endif
#include <core_cm4.h>
#include <core_cmFunc.h>

extern RCC_ClocksTypeDef RCC_Clocks;

//For precise timing.
volatile unsigned int *DWT_CYCCNT   = (volatile unsigned int *)0xE0001004; //address of the register
volatile unsigned int *SCB_DEMCR        = (volatile unsigned int *)0xE000EDFC; //address of the register
volatile unsigned int *DWT_CONTROL  = (volatile unsigned int *)0xE0001000; //address of the register

void send_openocd_command(int command, void *message)
{
#ifdef DEBUG
   asm("mov r0, %[cmd];"
       "mov r1, %[msg];"
       "bkpt #0xAB"
         :
         : [cmd] "r" (command), [msg] "r" (message)
         : "r0", "r1", "memory");
#endif
}


void send_text( const char * text )
{
	uint32_t m[] = { 2, (uint32_t)text, strlen(text) };
	send_openocd_command(0x05, m);
}

int __attribute__((used)) _write (int fd, const void *buf, size_t count)
{
	uint32_t m[] = { 2, (uint32_t)buf, count };
	send_openocd_command(0x05, m);
}

void __attribute__((used)) * _sbrk(int incr) {
    extern char _ebss; // Defined by the linker
    static char *heap_end;
    char *prev_heap_end;


    if (heap_end == 0) {
        heap_end = &_ebss;
    }
    prev_heap_end = heap_end;

	char * stack = (char*) __get_MSP();
     if (heap_end + incr >  stack)
     {
		return  (void*)(-1);
     }

    heap_end += incr;

    return (void*) prev_heap_end;
}

void _delay_us(uint32_t us) {
	if( us ) us--; //Approximate extra overhead time.
	us *= RCC_Clocks.HCLK_Frequency/1000000;
    *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
    *DWT_CYCCNT = 0; // reset the counter
    *DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counter
	while( *DWT_CYCCNT < us );
}

void ConfigureLED()
{
	ConfigureGPIO( LEDPIN, INOUT_OUT );
}

uint8_t GetGPIOFromString( const char * str )
{
	int mode = 0;
	int port = -1;
	int pin = -1;
	const char * st = str;
	for( ; *st; st++ )
	{
		char c = *st;
		if( mode == 0 )
		{
			if( c >= 'A' && c <= 'F' )
			{
				port = c - 'A';
				mode = 2;
			}
			else if( c >= 'a' && c <= 'f' )
			{
				port = c - 'a';
				mode = 2;
			}
		}
		else if( mode == 2 )
		{
			if( c >= '0' && c <= '9' )
			{
				pin = 0;
				mode = 3;
			}
		}

		if( mode == 3 )
		{
			if( c >= '0' && c <= '9' )
			{
				pin = pin * 10;
				pin+= c - '0';
			}
			else
			{
				break;
			}
		}
	}

	if( port > 0 && pin > 0 && port <= 6 && pin <= 15)
	{
		return (port<<4)|pin;
	}
	else
	{
		return 0xff;
	}
}


void ConfigureGPIO( uint8_t gpio, int parameters )
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	/* Enable the GPIO_LED Clock */
#ifdef STM32F30X
	RCC_AHBPeriphClockCmd( 1<<(17+(gpio>>4)), ENABLE);
#elif defined( STM32F40_41xxx )
	RCC_AHB1PeriphClockCmd( 1<<((gpio>>4)), ENABLE);
#endif

	if( parameters & DEFAULT_VALUE_FLAG )
	{
		GPIOOn( gpio );
	}
	else
	{
		GPIOOff( gpio );
	}

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = 1<<(gpio&0xf);
	GPIO_InitStructure.GPIO_Mode = (parameters&INOUT_FLAG)?GPIO_Mode_OUT:GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = (parameters&PUPD_FLAG)?( (parameters&PUPD_UP)?GPIO_PuPd_UP:GPIO_PuPd_DOWN ):GPIO_PuPd_NOPULL;

#ifdef STM32F30X
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
#elif defined( STM32F40_41xxx )
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
#endif

	GPIO_Init(GPIOOf(gpio), &GPIO_InitStructure);
}

