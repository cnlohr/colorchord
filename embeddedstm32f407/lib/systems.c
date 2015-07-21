#include <stdint.h>
#include "systems.h"
#include <string.h>


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

int _write (int fd, const void *buf, size_t count)
{
	uint32_t m[] = { 2, (uint32_t)buf, count };
	send_openocd_command(0x05, m);
}

void * _sbrk(int incr) {
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
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* Enable the GPIO_LED Clock */

#ifdef STM32F30X
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; //| GPIO_Pin_15 (15 = CTS)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#elif defined( STM32F40_41xxx )

	RCC_AHB1PeriphClockCmd( LED_AHB_PORT, ENABLE);

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = (1<<LEDPIN);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(LEDPORT, &GPIO_InitStructure);
#endif

}


