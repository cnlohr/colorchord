//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "hpatimer.h"
#include "adc.h"
#include "osapi.h"
#include "mem.h"
#include "eagle_soc.h"
#include "user_interface.h"
#include "ets_sys.h"

typedef enum {
    FRC1_SOURCE = 0,
    NMI_SOURCE = 1,
} FRC1_TIMER_SOURCE_TYPE;

//BUFFSIZE must be a power-of-two
volatile uint8_t sounddata[HPABUFFSIZE] = {0};
volatile uint16_t soundhead = 0;
volatile uint16_t soundtail = 0;

bool hpaRunning = false;

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

/**
 * Timer callback function, registered by ETS_FRC_TIMER1_INTR_ATTACH() in StartHPATimer()
 * Calls hs_adc_read() to read a sample off the ADC
 *
 * @param v unused
 */
static void timerhandle( void * v )
{
    RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
	uint16_t r = hs_adc_read();
	sounddata[soundhead] = r>>6;
	soundhead = (soundhead+1)&(HPABUFFSIZE-1);
}

/**
 * @return true if a sample has been read from the ADC and is queued for processing
 */
bool sampleAvailable(void)
{
	return soundhead != soundtail;
}

/**
 * Get a sample from the ADC in the queue, return it, and increment the queue so
 * the next sample is returned the next time this is called
 *
 * @return the sample which was read from the ADC
 */
uint8_t getSample(void)
{
	uint8_t samp = sounddata[soundtail];
	soundtail = (soundtail + 1) % (HPABUFFSIZE);
	return samp;
}

/**
 * Initialize RTC Timer 1 to  run at 16KHz (DFREQ) and call timerhandle()
 * This timer is also used for PWM, so it can't do both PWM and ADC reading at the same time
 *
 * Calls ContinueHPATimer() to fully enable to timer and start an ADC reading with hs_adc_start()
 */
void ICACHE_FLASH_ATTR StartHPATimer()
{
	/* ???
	hw_timer_init(FRC1_SOURCE, 1);
	hw_timer_arm(62);
	hw_timer_set_func(timerhandle);
	*/

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

    ContinueHPATimer();
}

/**
 * Pause the hardware timer used to sample the ADC
 */
void ICACHE_FLASH_ATTR PauseHPATimer()
{
    TM1_EDGE_INT_DISABLE();
    ETS_FRC1_INTR_DISABLE();
	system_timer_reinit(); // TODO this is the SW timer??
	hpaRunning = false;
}

/**
 * Start the hardware timer used to sample the ADC
 */
void ICACHE_FLASH_ATTR ContinueHPATimer()
{
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
	system_timer_reinit(); // TODO this is the SW timer??
	hs_adc_start();
	hpaRunning = true;
}

/**
 * @return true if the hpa timer is running, false otherwise
 */
bool isHpaRunning(void)
{
	return hpaRunning;
}

