//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

#include "commonservices.h"
#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "espconn.h"
#include "mystuff.h"
#include "ip_addr.h"
#include "http.h"
#include "spi_flash.h"
#include "esp8266_rom.h"
#include <gpio.h>
#include "flash_rewriter.h"

static struct espconn *pUdpServer;
static struct espconn *pHTTPServer;
struct espconn *pespconn;
uint16_t g_gpiooutputmask = 0;


static int need_to_switch_back_to_soft_ap = 0; //0 = no, 1 = will need to. 2 = do it now.
#define MAX_STATIONS 20
struct totalscan_t
{
	char name[32];
	char mac[18]; //string
	int8_t rssi;
	uint8_t channel;
	uint8_t encryption;
} totalscan[MAX_STATIONS];
int scanplace = 0;
static void ICACHE_FLASH_ATTR scandone(void *arg, STATUS status)
{
	scaninfo *c = arg; 
	struct bss_info *inf; 

	if( need_to_switch_back_to_soft_ap == 1 )
		need_to_switch_back_to_soft_ap = 2;

	printf("!%p\n",c->pbss);

	if (!c->pbss) {
		scanplace = -1;
		return;
	}
//		printf("!%s\n",inf->ssid);
	STAILQ_FOREACH(inf, c->pbss, next) {
		printf( "%s\n", inf->ssid );
		ets_memcpy( totalscan[scanplace].name, inf->ssid, 32 );
		ets_sprintf( totalscan[scanplace].mac, MACSTR, MAC2STR( inf->bssid ) );
		//ets_memcpy( totalscan[scanplace].mac, "not implemented", 16 );
		totalscan[scanplace].rssi = inf->rssi;
		totalscan[scanplace].channel = inf->channel;
		totalscan[scanplace].encryption = inf->authmode;
		inf = (struct bss_info *) &inf->next;
		scanplace++;
		if( scanplace == MAX_STATIONS ) break;
	}
}



int ICACHE_FLASH_ATTR issue_command(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;
	pusrdata[len] = 0;

	switch( pusrdata[0] )
	{
	case 'e': case 'E': 	//Echo
		if( retsize > len )
		{
			ets_memcpy( buffend, pusrdata, len );
			return len;
		}
		else
		{
			return -1;
		}
	case 'f': case 'F':  //Flashing commands (F_)
	{
		flashchip->chip_size = 0x01000000;
		const char * colon = (const char *) ets_strstr( (char*)&pusrdata[2], ":" );
		int nr = my_atoi( &pusrdata[2] );

		switch (pusrdata[1])
		{
		case 'e': case 'E':  //(FE#\n) <- # = sector.
		{
			EnterCritical();
			spi_flash_erase_sector( nr );	//SPI_FLASH_SEC_SIZE      4096
			ExitCritical();

			buffend += ets_sprintf(buffend, "FE%d\r\n", nr );
			break;
		}

		case 'b': case 'B':  //(FE#\n) <- # = sector.
		{
			EnterCritical();
			SPIEraseBlock( nr );
			ExitCritical();

			buffend += ets_sprintf(buffend, "FB%d\r\n", nr );
			break;
		}

		case 'm': case 'M': //Execute the flash re-writer
			{
				int r = (*GlobalRewriteFlash)( &pusrdata[2], len-2 );
				buffend += ets_sprintf( buffend, "!FM%d\r\n", r );
				break;
			}
		case 'w': case 'W':	//Flash Write (FW#\n) <- # = byte pos.
			if( colon )
			{
				colon++;
				const char * colon2 = (const char *) ets_strstr( (char*)colon, ":" );
				if( colon2 )
				{
					colon2++;
					int datlen = (int)len - (colon2 - pusrdata);
					ets_memcpy( buffer, colon2, datlen );

					EnterCritical();
					spi_flash_write( nr, (uint32*)buffer, (datlen/4)*4 );
					ExitCritical();

					buffend += ets_sprintf(buffend, "FW%d\r\n", nr );
					break;
				}
			}
			buffend += ets_sprintf(buffend, "!FW\r\n" );
			break;
		case 'r': case 'R':	//Flash Read (FR#\n) <- # = sector.
			if( colon )
			{
				colon++;
				int datlen = my_atoi( colon );
				datlen = (datlen/4)*4; //Must be multiple of 4 bytes
				if( datlen <= 1280 )
				{
					buffend += ets_sprintf(buffend, "FR%08d:%04d:", nr, datlen ); //Caution: This string must be a multiple of 4 bytes.
					spi_flash_read( nr, (uint32*)buffend, datlen );
					break;
				}
			}
			buffend += ets_sprintf(buffend, "!FR\r\n" );
			break;
		}

		flashchip->chip_size = 0x00080000;
		return buffend - buffer;
	}
	case 'w': case 'W':	// (W1:SSID:PASSWORD) (To connect) or (W2) to be own base station.  ...or WI, to get info... or WS to scan.
	{
		char * colon = (char *) ets_strstr( (char*)&pusrdata[2], ":" );
		char * colon2 = (colon)?((char *)ets_strstr( (char*)(colon+1), ":" )):0;
		char * extra = colon2;

		if( extra )
		{
			for( ; *extra; extra++ )
			{
				if( *extra < 32 )
				{
					*extra = 0;
					break;
				}
			}
		}

		switch (pusrdata[1])
		{
		case '1': //Station mode
		case '2': //AP Mode
			if( colon && colon2 )
			{
				int c1l = 0;
				int c2l = 0;
				*colon = 0;  colon++;
				*colon2 = 0; colon2++;
				c1l = ets_strlen( colon );
				c2l = ets_strlen( colon2 );
				if( c1l > 32 ) c1l = 32;
				if( c2l > 32 ) c2l = 32;

				printf( "Switching to: \"%s\"/\"%s\".\n", colon, colon2 );

				if( pusrdata[1] == '1' )
				{
					struct station_config stationConf;
					os_bzero( &stationConf, sizeof( stationConf ) );

					os_memcpy(&stationConf.ssid, colon, c1l);
					os_memcpy(&stationConf.password, colon2, c2l);

					wifi_set_opmode( 1 );
					wifi_station_set_config(&stationConf);
					wifi_station_connect();

					buffend += ets_sprintf( buffend, "W1\r\n" );
					printf( "Switching.\n" );
				}
				else
				{
					wifi_set_opmode_current( 1 );
					wifi_set_opmode( 2 );
					buffend += ets_sprintf( buffend, "W2\r\n" );
				}
			}
			break;
		case 'I':
			{
				int mode = wifi_get_opmode();

				buffend += ets_sprintf( buffend, "WI%d", mode );

				if( mode == 2 )
				{
					struct softap_config ap;
					wifi_softap_get_config( &ap );
					buffend += ets_sprintf( buffend, "\t%s\t%s\t%s\t%d", ap.ssid, ap.password, enctypes[ap.authmode], ap.channel );
				}
				else
				{
					struct station_config sc;
					wifi_station_get_config( &sc );
					buffend += ets_sprintf( buffend, "\t%s\t%s\tna\t%d", sc.ssid, sc.password, wifi_get_channel() );
				}
			}
			break;
		case 'S': case 's':
			{
				int i, r;

				scanplace = 0;

				if( wifi_get_opmode() == SOFTAP_MODE )
				{
					wifi_set_opmode_current( STATION_MODE );
					need_to_switch_back_to_soft_ap = 1;
					r = wifi_station_scan(0, scandone );
				}
				else
				{
					r = wifi_station_scan(0, scandone );
				}

				buffend += ets_sprintf( buffend, "WS%d\n", r );
				uart0_sendStr(buffer);

				break;
			}
			break;
		case 'R': case 'r':
			{
				int i, r;

				buffend += ets_sprintf( buffend, "WR%d\n", scanplace );
				
				for( i = 0; i < scanplace && buffend - buffer < retsize - 64; i++ )
				{
					buffend += ets_sprintf( buffend, "#%s\t%s\t%d\t%d\t%s\n", 
						totalscan[i].name, totalscan[i].mac, totalscan[i].rssi, totalscan[i].channel, enctypes[totalscan[i].encryption] );
				}

				break;
			}
			break;
		}
		return buffend - buffer;
	}
	case 'G': case 'g':
	{
		static const uint32_t AFMapper[16] = {
			0, PERIPHS_IO_MUX_U0TXD_U, 0, PERIPHS_IO_MUX_U0RXD_U,
			0, 0, 1, 1,
			1, 1, 1, 1,
			PERIPHS_IO_MUX_MTDI_U, PERIPHS_IO_MUX_MTCK_U, PERIPHS_IO_MUX_MTMS_U, PERIPHS_IO_MUX_MTDO_U };

		int nr = my_atoi( &pusrdata[2] );


		if( AFMapper[nr] == 1 )
		{
			buffend += ets_sprintf( buffend, "!G%c%d\n", pusrdata[1], nr );
			return buffend - buffer;
		}
		else if( AFMapper[nr] )
		{
			PIN_FUNC_SELECT( AFMapper[nr], 3);  //Select AF pin to be GPIO.
		}

		switch( pusrdata[1] )
		{
		case '0':
		case '1':
			GPIO_OUTPUT_SET(GPIO_ID_PIN(nr), pusrdata[1]-'0' );
			buffend += ets_sprintf( buffend, "G%c%d", pusrdata[1], nr );
			g_gpiooutputmask |= (1<<nr);
			break;
		case 'i': case 'I':
			GPIO_DIS_OUTPUT(GPIO_ID_PIN(nr));
			buffend += ets_sprintf( buffend, "GI%d\n", nr );
			g_gpiooutputmask &= ~(1<<nr);
			break;
		case 'f': case 'F':
		{
			int on = GPIO_INPUT_GET( GPIO_ID_PIN(nr) );
			on = !on;
			GPIO_OUTPUT_SET(GPIO_ID_PIN(nr), on );
			g_gpiooutputmask |= (1<<nr);
			buffend += ets_sprintf( buffend, "GF%d:%d\n", nr, on );
			break;
		}
		case 'g': case 'G':
			buffend += ets_sprintf( buffend, "GG%d:%d\n", nr, GPIO_INPUT_GET( GPIO_ID_PIN(nr) ) );
			break;
		case 's': case 'S':
		{
			uint32_t rmask = 0;
			int i;
			for( i = 0; i < 16; i++ )
			{
				rmask |= GPIO_INPUT_GET( GPIO_ID_PIN(i) )?(1<<i):0;
			}
			buffend += ets_sprintf( buffend, "GS:%d:%d\n", g_gpiooutputmask, rmask );
			break;
		}
		}
		return buffend - buffer;
	}
	case 'c': case 'C':
		return CustomCommand( buffer, retsize, pusrdata, len);
	}
	return -1;
}

void ICACHE_FLASH_ATTR issue_command_udp(void *arg, char *pusrdata, unsigned short len)
{
	char  __attribute__ ((aligned (32))) retbuf[1300];
	int r = issue_command( retbuf, 1300, pusrdata, len );
	if( r > 0 )
	{
		espconn_sent( (struct espconn *)arg, retbuf, r );
	}
}

void ICACHE_FLASH_ATTR CSInit()
{
    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = 7878;
	espconn_regist_recvcb(pUdpServer, issue_command_udp);
	espconn_create( pUdpServer );

	pHTTPServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pHTTPServer, 0, sizeof( struct espconn ) );
	espconn_create( pHTTPServer );
	pHTTPServer->type = ESPCONN_TCP;
    pHTTPServer->state = ESPCONN_NONE;
	pHTTPServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	pHTTPServer->proto.tcp->local_port = 80;
    espconn_regist_connectcb(pHTTPServer, httpserver_connectcb);
    espconn_accept(pHTTPServer);
    espconn_regist_time(pHTTPServer, 15, 0); //timeout
}

void CSTick( int slowtick )
{
	static uint8_t tick_flag = 0;

	if( slowtick )
	{
		tick_flag = 1;
		return;
	}

	if(	need_to_switch_back_to_soft_ap == 2 )
	{
		need_to_switch_back_to_soft_ap = 0;
		wifi_set_opmode_current( SOFTAP_MODE );
	}

	HTTPTick(0);

	//TRICKY! If we use the timer to initiate this, connecting to people's networks
	//won't work!  I don't know why, so I do it here.  this does mean sometimes it'll
	//pause, though.
	if( tick_flag )
	{
		tick_flag = 0;
		HTTPTick(1);
	}
}

