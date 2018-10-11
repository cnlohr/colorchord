//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "espconn.h"
#include "esp82xxutil.h"
#include "ws2812_i2s.h"
#include "hpatimer.h"
#include "DFT32.h"
#include "ccconfig.h"
#include <embeddednf.h>
#include <embeddedout.h>
#include <commonservices.h>
#include "ets_sys.h"
#include "gpio.h"
//#define PROFILE

#define PORT 7777
#define SERVER_TIMEOUT 1500
#define MAX_CONNS 5
#define MAX_FRAME 2000

#define PROC_TASK_PRIO        0
#define PROC_TASK_QUEUE_LEN    1

struct CCSettings CCS;
static volatile os_timer_t some_timer;
static struct espconn *pUdpServer;

extern volatile uint8_t sounddata[HPABUFFSIZE];
extern volatile uint16_t soundhead;
uint16_t soundtail;

static uint8_t hpa_running = 0;
static uint8_t hpa_is_paused_for_wifi;

os_event_t    procTaskQueue[PROC_TASK_QUEUE_LEN];
uint32_t samp_iir = 0;
int wf = 0;

void EnterCritical();
void ExitCritical();
void ICACHE_FLASH_ATTR CustomStart( );

/**
 * Required. Users can call system_phy_set_rfoption to set the RF option in user_rf_pre_init,
 * or call system_deep_sleep_set_option before Deep-sleep. If RF is disabled,
 * ESP8266 Station and SoftAP will both be disabled, so the related APIs must not be
 * called. Wi-Fi radio functions and network stack management APIs are not available
 * when the radio is disabled
 */
void ICACHE_FLASH_ATTR user_rf_pre_init()
{
}


/**
 * Called from procTask() when 128 sound samples, a colorchord frame,
 * have been received and filtered.
 *
 * This updates the LEDs.
 */
static void NewFrame()
{
	if( !COLORCHORD_ACTIVE )
	{
		return;
	}

	//uint8_t led_outs[NUM_LIN_LEDS*3];
	int i;
	HandleFrameInfo();

	switch( COLORCHORD_OUTPUT_DRIVER )
	{
	case 0:
		UpdateLinearLEDs();
		break;
	case 1:
		UpdateAllSameLEDs();
		break;
	};

	//SendSPI2812( ledOut, NUM_LIN_LEDS );
	ws2812_push( ledOut, USE_NUM_LIN_LEDS * 3 );
}

/**
 * The main process that runs all the time. Registered with system_os_task(),
 * priority 0 (PROC_TASK_PRIO). Always posts an event to itself to stay running.
 *
 * This task filters samples stored in the sounddata[] queue and feeds them into PushSample32()
 *
 * When enough samples have been filtered, it'll call NewFrame() to change the LEDs.
 *
 * If no samples are queued for filtering, call CSTick()
 *
 * @param events OS signals, sent through system_os_post(). If sig an par
 *               are 0, call CSTick()
 */
static void procTask(os_event_t *events)
{
	system_os_post(PROC_TASK_PRIO, 0, 0 );

	if( COLORCHORD_ACTIVE && !hpa_running )
	{
		ExitCritical();
		hpa_running = 1;
	}

	if( !COLORCHORD_ACTIVE && hpa_running )
	{
		EnterCritical();
		hpa_running = 0;
	}
	
	//For profiling so we can see how much CPU is spent in this loop.
#ifdef PROFILE
	WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(0), 1 );
#endif
	while( soundtail != soundhead )
	{
		int32_t samp = sounddata[soundtail];
		samp_iir = samp_iir - (samp_iir>>10) + samp;
		samp = (samp - (samp_iir>>10))*16;
		samp = (samp * CCS.gINITIAL_AMP) >> 4;
		PushSample32( samp );
		soundtail = (soundtail+1)&(HPABUFFSIZE-1);

		wf++;
		if( wf == 128 )
		{
			NewFrame();
			wf = 0; 
		}
	}
#ifdef PROFILE
	WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(0), 0 );
#endif

	if( events->sig == 0 && events->par == 0 )
	{
		CSTick( 0 );
	}

}

/**
 * Timer handler for a software timer set to fire every 100ms, forever.
 * Calls CSTick() every 100ms.
 * If the hardware is in wifi station mode, this Enables the hardware timer
 * to sample the ADC once the IP address has been received and printed
 *
 * @param arg unused
 */
static void ICACHE_FLASH_ATTR myTimer(void *arg)
{
	CSTick( 1 );

	if( hpa_is_paused_for_wifi && printed_ip )
	{
		StartHPATimer(); //Init the high speed  ADC timer.
		hpa_running = 1;
		hpa_is_paused_for_wifi = 0; // only need to do once prevents unstable ADC
	}
//	uart0_sendStr(".");
//	printf( "%d/%d\n",soundtail,soundhead );
//	printf( "%d/%d\n",soundtail,soundhead );
//	uint8_t ledout[] = { 0x00, 0xff, 0xaa, 0x00, 0xff, 0xaa, };
//	ws2812_push( ledout, 6 );
}


/**
 * UDP Packet handler, registered with espconn_regist_recvcb().
 * It takes the UDP data and jams it right into the LEDs
 *
 * @param arg The espconn struct for this packet
 * @param pusrdata
 * @param len
 */
static void ICACHE_FLASH_ATTR udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;
//	uint8_t buffer[MAX_FRAME];
//	uint8_t ledout[] = { 0x00, 0xff, 0xaa, 0x00, 0xff, 0xaa, };
	uart0_sendStr("X");
	ws2812_push( pusrdata+3, len );
}

/**
 * UART RX handler, called by uart0_rx_intr_handler(). Currently does nothing
 *
 * @param c The char received on the UART
 */
void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}

/**
 * The main initialization function
 */
void ICACHE_FLASH_ATTR user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	int wifiMode = wifi_get_opmode();

	uart0_sendStr("\r\nColorChord\r\n");

	//Uncomment this to force a system restore.
	//	system_restore();

	// Load configurable parameters from SPI memory
	CustomStart();

#ifdef PROFILE
	GPIO_OUTPUT_SET(GPIO_ID_PIN(0), 0);
#endif

	// Common services (wifi) pre-init.
	CSPreInit();

	// Set up UDP server to receive LED data, udpserver_recv()
    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	espconn_create( pUdpServer );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = 7777;
	espconn_regist_recvcb(pUdpServer, udpserver_recv);

	if( espconn_create( pUdpServer ) )
	{
		while(1) { uart0_sendStr( "\r\nFAULT\r\n" ); }
	}

	// Common services (wifi) init. Sets up another UDP server to receive
	// commands (issue_command)and an HTTP server
	CSInit();

	// Add a process to filter queued ADC samples and output LED signals
	system_os_task(procTask, PROC_TASK_PRIO, procTaskQueue, PROC_TASK_QUEUE_LEN);

	// Start a software timer to call CSTick() every 100ms and start the hw timer eventually
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, 100, 1);

	// Set GPIO16 for Input
    WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC and rtc_gpio0 connection

    WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

    WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);	//out disable

    // Init colorchord
	InitColorChord();

	// Tricky: If we are in station mode, wait for that to get resolved before enabling the high speed timer.
	if( wifi_get_opmode() == 1 )
	{
		hpa_is_paused_for_wifi = 1;
	}
	else
	{
		// Init the high speed  ADC timer.
		StartHPATimer();
		hpa_running = 1;
	}

	// Initialize LEDs
	ws2812_init();

	// Attempt to make ADC more stable
	// https://github.com/esp8266/Arduino/issues/2070
	// see peripherals https://espressif.com/en/support/explore/faq
	//wifi_set_sleep_type(NONE_SLEEP_T); // on its own stopped wifi working
	//wifi_fpm_set_sleep_type(NONE_SLEEP_T); // with this seemed no difference

	// Post an event to procTask() to get it started
	system_os_post(PROC_TASK_PRIO, 0, 0 );
}

/**
 * If the firmware leaves a critical section, enable the hardware timer
 * used to sample the ADC. This allows the interrupt to fire.
 */
void EnterCritical()
{
	PauseHPATimer();
	//ets_intr_lock();
}

/**
 * If the firmware enters a critical section, disable the hardware timer
 * used to sample the ADC and the corresponding interrupt
 */
void ExitCritical()
{
	//ets_intr_unlock();
	ContinueHPATimer();
}


