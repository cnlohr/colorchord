#ifndef _SYSTEMS_H
#define _SYSTEMS_H

void send_openocd_command(int command, void *message);
void send_text( const char * text );
void _delay_us(uint32_t us);


typedef uint8_t gpio;

gpio GetGPIOFromString( const char * str );


#define DEFAULT_VALUE_FLAG	0x00000001
#define DEFAULT_ON 			0x00000001
#define DEFAULT_OFF			0x00000000

#define INOUT_FLAG			0x00000002
#define INOUT_OUT 			0x00000002
#define INOUT_IN			0x00000000

#define PUPD_FLAG			0x0000000C
#define PUPD_NONE 			0x00000000
#define PUPD_UP 			0x00000004
#define PUPD_DOWN			0x00000008

void ConfigureGPIO( gpio gpio, int parameters );

#define GPIOOf(x)  ((GPIO_TypeDef *) ((((x)>>4)<=6)?(AHB2PERIPH_BASE+0x400*((x)>>4)):0x60000000) )
#define GPIOPin(x)   ((1<<((x)&0x0f)))
#define GPIOLatch(x) GPIOOf(x)->ODR
#define GPIOOff(x)  GPIOOf(x)->BRR = (1<<((x)&0x0f));
#define GPIOOn(x)  GPIOOf(x)->BSRR = (1<<((x)&0x0f));


void ConfigureLED();
#define LED_TOGGLE {GPIOB->ODR ^= GPIO_Pin_8;}
#define LED_ON     {GPIOB->BSRR ^= GPIO_Pin_8;}
#define LED_OFF    {GPIOB->BRR ^= GPIO_Pin_8;}
//General notes:

#endif

