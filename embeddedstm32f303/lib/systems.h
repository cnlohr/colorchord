#ifndef _SYSTEMS_H
#define _SYSTEMS_H

void send_openocd_command(int command, void *message);
void send_text( const char * text );
void _delay_us(uint32_t us);

void ConfigureLED();
#define LED_TOGGLE {GPIOB->ODR ^= GPIO_Pin_8;}
#define LED_ON     {GPIOB->BSRR ^= GPIO_Pin_8;}
#define LED_OFF    {GPIOB->BRR ^= GPIO_Pin_8;}
//General notes:

#endif

