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
#include "flash_rewriter.h"

static struct espconn *pUdpServer;
static struct espconn *pHTTPServer;
struct espconn *pespconn;

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



void ICACHE_FLASH_ATTR issue_command(void *arg, char *pusrdata, unsigned short len)
{
	pespconn = (struct espconn *)arg;
	//We accept commands on this.

	switch( pusrdata[0] )
	{
	case 'f': case 'F':  //Flashing commands (F_)
	{
		flashchip->chip_size = 0x01000000;
//		ets_printf( "PC%p\n", &pusrdata[2] );
		const char * colon = (const char *) ets_strstr( (char*)&pusrdata[2], ":" );
		int nr = my_atoi( &pusrdata[2] );

		switch (pusrdata[1])
		{
		case 'e': case 'E':  //(FE#\n) <- # = sector.
		{
			char  __attribute__ ((aligned (16))) buffer[50];
			char * buffend;
			buffend = buffer;

			EnterCritical();
			spi_flash_erase_sector( nr );	//SPI_FLASH_SEC_SIZE      4096
			ExitCritical();

			buffend += ets_sprintf(buffend, "FE%d\r\n", nr );
			espconn_sent( pespconn, buffer, ets_strlen( buffer ) );
			break;
		}

		case 'b': case 'B':  //(FE#\n) <- # = sector.
		{
			char  __attribute__ ((aligned (16))) buffer[50];
			char * buffend;
			buffend = buffer;
			//spi_flash_erase_sector( nr );	//SPI_FLASH_SEC_SIZE      4096

			EnterCritical();
			SPIEraseBlock( nr );
			ExitCritical();

			buffend += ets_sprintf(buffend, "FB%d\r\n", nr );
			espconn_sent( pespconn, buffer, ets_strlen( buffer ) );
			break;
		}

		case 'm': case 'M': //Execute the flash re-writer
			{
				char ret[128];
				int n = ets_sprintf( ret, "FM" );
				espconn_sent( pespconn, ret, n );
				int r = (*GlobalRewriteFlash)( &pusrdata[2], len-2 );
				n = ets_sprintf( ret, "!FM%d", r );
				espconn_sent( pespconn, ret, n );
			}
		case 'w': case 'W':	//Flash Write (FW#\n) <- # = byte pos.
			if( colon )
			{
				char  __attribute__ ((aligned (32))) buffer[1300];
				char * buffend;
				buffend = buffer;
				buffer[0] = 0;
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

					//SPIWrite( nr, (uint32_t*)buffer, (datlen/4)*4 );

					buffend = buffer;
					buffend += ets_sprintf(buffend, "FW%d\r\n", nr );
					if( ets_strlen( buffer ) )
						espconn_sent( pespconn, buffer, ets_strlen( buffer ) );
				}
			}
			break;
		case 'r': case 'R':	//Flash Read (FR#\n) <- # = sector.
			if( colon )
			{
				char  __attribute__ ((aligned (16))) buffer[1300];
				char * buffend;
				buffend = buffer;
				buffer[0] = 0;
				colon++;
				int datlen = my_atoi( colon );
				if( datlen <= 1280 )
				{
					buffend += ets_sprintf(buffend, "FR%08d:%04d:", nr, datlen );
					int frontlen = buffend - buffer;
					spi_flash_read( nr, (uint32*)buffend, (datlen/4)*4 );
					espconn_sent( pespconn, buffer, frontlen + datlen );
					buffer[0] = 0;
					if( ets_strlen( buffer ) )
						espconn_sent( pespconn, buffer, ets_strlen( buffer ) );
				}
			}
			break;
		}

		flashchip->chip_size = 0x00080000;
		break;
	}
	case 'w': case 'W':	// (W1:SSID:PASSWORD) (To connect) or (W2) to be own base station.  ...or WI, to get info... or WS to scan.
	{
		char * colon = (char *) ets_strstr( (char*)&pusrdata[2], ":" );
		char * colon2 = (colon)?((char *)ets_strstr( (char*)(colon+1), ":" )):0;
		char * extra = colon2;
		char  __attribute__ ((aligned (16))) buffer[1300];
		char * buffend;
		buffend = buffer;
		buffer[0] = 0;

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
			if( colon && colon2 )
			{
				int c1l = 0;
				int c2l = 0;
				struct station_config stationConf;
				*colon = 0;  colon++;
				*colon2 = 0; colon2++;
				c1l = ets_strlen( colon );
				c2l = ets_strlen( colon2 );
				if( c1l > 32 ) c1l = 32;
				if( c2l > 32 ) c2l = 32;
				os_bzero( &stationConf, sizeof( stationConf ) );

				printf( "Switching to: \"%s\"/\"%s\".\n", colon, colon2 );

				os_memcpy(&stationConf.ssid, colon, c1l);
				os_memcpy(&stationConf.password, colon2, c2l);

				wifi_set_opmode( 1 );
				wifi_station_set_config(&stationConf);
				wifi_station_connect();
				espconn_sent( pespconn, "W1\r\n", 4 );

				printf( "Switching.\n" );
			}
			break;
		case '2':
			{
				wifi_set_opmode_current( 1 );
				wifi_set_opmode( 2 );
				espconn_sent( pespconn, "W2\r\n", 4 );
			}
			break;
		case 'I':
			{
				struct station_config sc;
				int mode = wifi_get_opmode();
				char buffer[200];
				char * buffend = &buffer[0];

				buffend += ets_sprintf( buffend, "WI%d", mode );

				wifi_station_get_config( &sc );

				buffend += ets_sprintf( buffend, ":%s:%s", sc.ssid, sc.password );

				espconn_sent( pespconn, buffer, buffend - buffer );
			}
			break;
		case 'S': case 's':
			{
				char buffer[1024];
				char * buffend = &buffer[0];
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

				espconn_sent( pespconn, buffer, buffend - buffer );

			}
			break;
		case 'R': case 'r':
			{
				char buffer[1024];
				char * buffend = &buffer[0];
				int i, r;

				buffend += ets_sprintf( buffend, "WR%d\n", scanplace );
				
				for( i = 0; i < scanplace; i++ )
				{
					buffend += ets_sprintf( buffend, "#%s\t%s\t%d\t%d\t%s\n", 
						totalscan[i].name, totalscan[i].mac, totalscan[i].rssi, totalscan[i].channel, enctypes[totalscan[i].encryption] );
				}

				espconn_sent( pespconn, buffer, buffend - buffer );

			}
			break;
		}
		break;
	}
	default:
		break;
	}
}

void ICACHE_FLASH_ATTR CSInit()
{
    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = 7878;
	espconn_regist_recvcb(pUdpServer, issue_command);
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

