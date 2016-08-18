//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "hpatimer.h"
#include "adc.h"
#include "osapi.h"
#include "mem.h"
#include "eagle_soc.h"
#include "user_interface.h"
#include "ets_sys.h"

//BUFFSIZE must be a power-of-two
volatile uint8_t sounddata[HPABUFFSIZE];
volatile uint16_t soundhead;


#define FRC1_ENABLE_TIMER  BIT7

typedef enum {
    DIVDED_BY_1 = 0,
    DIVDED_BY_16 = 4,
    DIVDED_BY_256 = 8,
} TIMER_PREDIVED_MODE;

typedef enum {
    TM_LEVEL_INT = 1,
    TM_EDGE_INT   = 0,
} TIMER_INT_MODE;

#define FRC1_AUTO_RELOAD 64

static void timerhandle( void * v )
{
    RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
	uint16_t r = hs_adc_read();
	sounddata[soundhead] = r>>6;
	soundhead = (soundhead+1)&(HPABUFFSIZE-1);
}

void ICACHE_FLASH_ATTR StartHPATimer()
{

    RTC_REG_WRITE(FRC1_CTRL_ADDRESS,  FRC1_AUTO_RELOAD|
                  DIVDED_BY_16 | //5MHz main clock.
                  FRC1_ENABLE_TIMER |
                  TM_EDGE_INT );

    RTC_REG_WRITE(FRC1_LOAD_ADDRESS,  5000000/DFREQ);
    RTC_REG_WRITE(FRC1_COUNT_ADDRESS, 5000000/DFREQ);

    //pwm_set_freq_duty(freq, duty);
    //pwm_start();
//    RTC_REG_WRITE(FRC1_LOAD_ADDRESS, local_single[0].h_time);
    ETS_FRC_TIMER1_INTR_ATTACH(timerhandle, NULL);
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
	system_timer_reinit();
	hs_adc_start();
}

void ICACHE_FLASH_ATTR PauseHPATimer()
{
    TM1_EDGE_INT_DISABLE();
    ETS_FRC1_INTR_DISABLE();
	system_timer_reinit();
}

void ICACHE_FLASH_ATTR ContinueHPATimer()
{
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
	system_timer_reinit();
	hs_adc_start();
}



