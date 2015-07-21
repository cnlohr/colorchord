#ifndef _SYSTEMS_H
#define _SYSTEMS_H


#ifdef STM32F30X 
#include <stm32f30x.h>
#include <stm32f30x_rcc.h>
#include <stm32f30x_gpio.h>

#define LEDPORT GPIOB
#define LEDPIN  8

#elif defined( STM32F40_41xxx )

#include <stm32f4xx.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_gpio.h>

#define LED_AHB_PORT RCC_AHB1Periph_GPIOD
#define LEDPORT GPIOD
#define LEDPIN  15
#else

#error Unsupported device.

#endif

void send_openocd_command(int command, void *message);
void send_text( const char * text );
void _delay_us(uint32_t us);

void ConfigureLED();


#define LED_TOGGLE {LEDPORT->ODR ^= (1<<LEDPIN);}

#ifdef STM32F30X 

#define LED_ON     {LEDPORT->BSRR = (1<<LEDPIN);}
#define LED_OFF    {LEDPORT->BRR = (1<<LEDPIN);}

#elif defined( STM32F40_41xxx )

#define LED_ON     {LEDPORT->BSRRL = (1<<LEDPIN);}
#define LED_OFF    {LEDPORT->BSRRH = (1<<LEDPIN);}

#endif


//General notes:

#endif

