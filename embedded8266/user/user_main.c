#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "espconn.h"
#include "mystuff.h"
#include "ws2812.h"

#define PORT 7777
#define SERVER_TIMEOUT 1000
#define MAX_CONNS 5
#define MAX_FRAME 2000

#define procTaskPrio        0
#define procTaskQueueLen    1


static volatile os_timer_t some_timer;
static struct espconn *pUdpServer;



//Note to self.  In future consider looking at this: http://pastebin.com/6eLxSrNz








//Tasks that happen all the time.

os_event_t    procTaskQueue[procTaskQueueLen];
static void ICACHE_FLASH_ATTR
procTask(os_event_t *events)
{
	system_os_post(procTaskPrio, 0, 0 );
	if( events->sig == 0 && events->par == 0 )
	{
		//Idle Event.
	}
}


//Timer event.
static void ICACHE_FLASH_ATTR
 myTimer(void *arg)
{
	int i;
	uart0_sendStr(".");
	SetupConstants();

#define GPIO_OUTPUT_SET(gpio_no, bit_value) \
	gpio_output_set(bit_value<<gpio_no, ((~bit_value)&0x01)<<gpio_no, 1<<gpio_no,0)
	GPIO_OUTPUT_SET(GPIO_ID_PIN(WSGPIO), 0);

	WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );

	//XXX LOOK HERE!!!
	//Right now, I am not actually using real data from the inputs, but rather doing this to test performance of my algorithm. 
	//The next step is to take real ADC inpts.
	for( i = 0; i < 4000; i++ )
	{
		HandleProgressiveInt( 0xaa, 0xbb );
	}

	WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );

}


//Called when new packet comes in.
static void ICACHE_FLASH_ATTR
udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
	int i = 0;
	struct espconn *pespconn = (struct espconn *)arg;
	uint8_t buffer[MAX_FRAME];

	//Make sure watchdog is disabled.  WS2812's take a while and can mess it up.
	ets_wdt_disable();
	os_intr_lock();
	WS2812OutBuffer( pusrdata, len );
	os_intr_unlock();
	ets_sprintf( buffer, "%d\r\n", len );
	uart0_sendStr(buffer);
}

void ICACHE_FLASH_ATTR at_recvTask()
{
	//Called from UART.
}


void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	int wifiMode = wifi_get_opmode();

	uart0_sendStr("\r\nCustom Server\r\n");

	wifi_set_opmode( 2 ); //We broadcast our ESSID, wait for peopel to join.

    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	espconn_create( pUdpServer );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = 7777;
	espconn_regist_recvcb(pUdpServer, udpserver_recv);

	wifi_station_dhcpc_start();

	if( espconn_create( pUdpServer ) )
	{
		while(1) { uart0_sendStr( "\r\nFAULT\r\n" ); }
	}

	//XXX TODO figure out how to safely re-allow this.
	ets_wdt_disable();

	char outbuffer[] = { 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,0xff,0xff, 0x00,0xff,0x00 };
	WS2812OutBuffer( outbuffer, 1 ); //Initialize the output.

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	uart0_sendStr("\r\nCustom Server\r\n");
	WS2812OutBuffer( outbuffer, sizeof(outbuffer) );


	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, 1000, 1);
 
	system_os_post(procTaskPrio, 0, 0 );
}


