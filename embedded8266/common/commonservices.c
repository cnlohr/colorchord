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

int ets_str2macaddr(void *, void *);

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
		const char * colon = (const char *) ets_strstr( (char*)&pusrdata[2], "\t" );
		int nr = my_atoi( &pusrdata[2] );

		switch (pusrdata[1])
		{
		case 'e': case 'E':  //(FE#\n) <- # = sector.
		{
			if( nr < 128 )
			{
				buffend += ets_sprintf(buffend, "!FE%d\r\n", nr );
				break;
			}

			EnterCritical();
			spi_flash_erase_sector( nr );	//SPI_FLASH_SEC_SIZE      4096
			ExitCritical();

			buffend += ets_sprintf(buffend, "FE%d\r\n", nr );
			break;
		}

		case 'b': case 'B':  //(FB#\n) <- # = block.
		{
			if( nr < 8 )
			{
				buffend += ets_sprintf(buffend, "!FB%d\r\n", nr );
				break;
			}

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
		case 'w': case 'W':	//Flash Write (FW#\n) <- # = byte pos.  Reads until end-of-packet.
			if( colon )
			{
				colon++;
				const char * colon2 = (const char *) ets_strstr( (char*)colon, "\t" );
				if( colon2 && nr >= 524288)
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
		case 'x': case 'X':	//Flash Write Hex (FX#\t#\tDATTAAAAA) <- a = byte pos. b = length (in hex-pairs). Generally used for web-browser.
			if( colon )
			{
				int i;
				int siz = 0;
				colon++;
				char * colon2 = (char *) ets_strstr( (char*)colon, "\t" );
				if( colon2 )
				{
					*colon2 = 0;
					siz = my_atoi( colon );
				}
				//nr = place to write.
				//siz = size to write.
				//colon2 = data start.
				if( colon2 && nr >= 524288)
				{
					colon2++;
					int datlen = ((int)len - (colon2 - pusrdata))/2;
					if( datlen > siz ) datlen = siz;

					for( i = 0; i < datlen; i++ )
					{
						int8_t r1 = fromhex1( *(colon2++) );
						int8_t r2 = fromhex1( *(colon2++) );
						if( r1 == -1 || r2 == -1 ) goto failfx;
						buffend[i] = (r1 << 4) | r2;
					}

					//ets_memcpy( buffer, colon2, datlen );

					EnterCritical();
					spi_flash_write( nr, (uint32*)buffend, (datlen/4)*4 );
					ExitCritical();

					buffend += ets_sprintf(buffend, "FX%d\t%d\r\n", nr, siz );
					break;
				}
			}
failfx:
			buffend += ets_sprintf(buffend, "!FX\r\n" );
			break;
		case 'r': case 'R':	//Flash Read (FR#\n) <- # = sector.
			if( colon )
			{
				colon++;
				int datlen = my_atoi( colon );
				datlen = (datlen/4)*4; //Must be multiple of 4 bytes
				if( datlen <= 1280 )
				{
					buffend += ets_sprintf(buffend, "FR%08d\t%04d\t", nr, datlen ); //Caution: This string must be a multiple of 4 bytes.
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
	case 'i': case 'I': 	//Respond with device info.
	{
		buffend += ets_sprintf(buffend, "I" );
		int i;
		for( i = 0; i < 2 ;i++ )
		{
			struct ip_info ipi;
			wifi_get_ip_info( i, &ipi );
			buffend += ets_sprintf(buffend, "\t"IPSTR, IP2STR(&ipi.ip) );
		}
		buffend += ets_sprintf(buffend, "\r\n" );
		return buffend - buffer;
	}
	case 'w': case 'W':	// (W1:SSID:PASSWORD) (To connect) or (W2) to be own base station.  ...or WI, to get info... or WS to scan.
	{
		int c1l = 0;
		int c2l = 0;
		char * colon = (char *) ets_strstr( (char*)&pusrdata[2], "\t" );
		char * colon2 = (colon)?((char *)ets_strstr( (char*)(colon+1), "\t" )):0;
		char * colon3 = (colon2)?((char *)ets_strstr( (char*)(colon2+1), "\t" )):0;
		char * colon4 = (colon3)?((char *)ets_strstr( (char*)(colon3+1), "\t" )):0;
		char * extra = colon2;

		char mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
		int bssid_set = 0;
		if( colon ) { *colon = 0; colon++; }
		if( colon2 ) { *colon2 = 0; colon2++; }
		if( colon3 ) { *colon3 = 0; colon3++; }
		if( colon4 ) { *colon4 = 0; colon4++; }

		if( colon ) { c1l = ets_strlen( colon ); }
		if( colon2 ) {  c2l = ets_strlen( colon2 );  }
		if( colon3 )
		{
			bssid_set = ets_str2macaddr( mac, colon3 )?1:0;
			if( ( mac[0] == 0x00 || mac[0] == 0xff ) &&
				( mac[1] == 0x00 || mac[1] == 0xff ) &&  
				( mac[2] == 0x00 || mac[2] == 0xff ) &&  
				( mac[3] == 0x00 || mac[3] == 0xff ) &&  
				( mac[4] == 0x00 || mac[4] == 0xff ) &&  
				( mac[5] == 0x00 || mac[5] == 0xff ) ) bssid_set = 0;
		}

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
			if( colon && colon2 && ets_strlen( colon ) > 1 )
			{
				if( c1l > 31 ) c1l = 31;
				if( c2l > 63 ) c2l = 63;

				printf( "Switching to: \"%s\"/\"%s\" (%d/%d). BSSID_SET: %d [%c]\n", colon, colon2, c1l, c2l, bssid_set, pusrdata[1] );

				if( pusrdata[1] == '1' )
				{
					struct station_config stationConf;
					wifi_station_get_config(&stationConf);

					os_memcpy(&stationConf.ssid, colon, c1l);
					os_memcpy(&stationConf.password, colon2, c2l);

					stationConf.ssid[c1l] = 0;
					stationConf.password[c2l] = 0;
					stationConf.bssid_set = bssid_set;
					os_memcpy( stationConf.bssid, mac, 6 );

					printf( "-->'%s'\n",stationConf.ssid);
					printf( "-->'%s'\n",stationConf.password);

					EnterCritical();
//					wifi_station_set_config(&stationConf);
					wifi_set_opmode( 1 );
					wifi_station_set_config(&stationConf);
					wifi_station_connect();
					ExitCritical();

//					wifi_station_get_config( &stationConf );


					buffend += ets_sprintf( buffend, "W1\r\n" );
					printf( "Switching.\n" );
				}
				else
				{
					struct softap_config config;
					char macaddr[6];
					wifi_softap_get_config( &config );

					wifi_get_macaddr(SOFTAP_IF, macaddr);

					os_memcpy( &config.ssid, colon, c1l );
					os_memcpy( &config.password, colon2, c2l );
					config.ssid_len = c1l;
#if 0 //We don't support encryption.
					config.ssid[c1l] = 0;
					config.password[c2l] = 0;

					config.authmode = 0;
					if( colon3 )
					{
						int k;
						for( k = 0; enctypes[k]; k++ )
						{
							if( strcmp( colon3, enctypes[k] ) == 0 )
								config.authmode = k;
						} 
					}
					
#endif

					int chan = (colon4)?my_atoi(colon4):config.channel;
					if( chan == 0 || chan > 13 ) chan = 1;
					config.channel = chan;

//					printf( "Mode now. %s %s %d %d %d %d %d\n", config.ssid, config.password, config.ssid_len, config.channel, config.authmode, config.max_connection );
//					printf( "Mode Set. %d\n", wifi_get_opmode() );

					EnterCritical();
					wifi_softap_set_config(&config);
					wifi_set_opmode( 2 );
					ExitCritical();
					printf( "Switching SoftAP: %d %d.\n", chan, config.authmode );

					buffend += ets_sprintf( buffend, "W2\r\n" );
				}
			}
			break;
		case 'I':
			{
				char macmap[15];
				int mode = wifi_get_opmode();

				buffend += ets_sprintf( buffend, "WI%d", mode );

				if( mode == 2 )
				{
					uint8_t mac[6];
					struct softap_config ap;
					wifi_softap_get_config( &ap );
					wifi_get_macaddr( 1, mac );
					ets_sprintf( macmap, MACSTR, MAC2STR( mac ) );
					buffend += ets_sprintf( buffend, "\t%s\t%s\t%s\t%d", ap.ssid, ap.password, macmap, ap.channel );
				}
				else
				{
					struct station_config sc;
					ets_memset( &sc, 0, sizeof( sc ) );
					wifi_station_get_config( &sc );
					if( sc.bssid_set )
						ets_sprintf( macmap, MACSTR, MAC2STR( sc.bssid ) );
					else
						macmap[0] = 0;
					buffend += ets_sprintf( buffend, "\t%s\t%s\t%s\t%d", sc.ssid, sc.password, macmap, wifi_get_channel() );
				}
			}
			break;
		case 'X': case 'x':
		{
			int rssi = wifi_station_get_rssi();
			if( rssi >= 0 )
			{
				buffend += ets_sprintf( buffend, "WX-" );
			}
			else
			{
				buffend += ets_sprintf( buffend, "WX%d", wifi_station_get_rssi() );
			}
			break;
		}
		case 'S': case 's':
			{
				int i, r;
				struct scan_config sc;

				scanplace = 0;

				sc.ssid = 0;
				sc.bssid = 0;
				sc.channel = 0;
				sc.show_hidden = 1;
				EnterCritical();
				if( wifi_get_opmode() == SOFTAP_MODE )
				{
					wifi_set_opmode_current( STATION_MODE );
					need_to_switch_back_to_soft_ap = 1;
				}

				r = wifi_station_scan(&sc, scandone );
				ExitCritical();

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
			buffend += ets_sprintf( buffend, "GF%d\t%d\n", nr, on );
			break;
		}
		case 'g': case 'G':
			buffend += ets_sprintf( buffend, "GG%d\t%d\n", nr, GPIO_INPUT_GET( GPIO_ID_PIN(nr) ) );
			break;
		case 's': case 'S':
		{
			uint32_t rmask = 0;
			int i;
			for( i = 0; i < 16; i++ )
			{
				rmask |= GPIO_INPUT_GET( GPIO_ID_PIN(i) )?(1<<i):0;
			}
			buffend += ets_sprintf( buffend, "GS\t%d\t%d\n", g_gpiooutputmask, rmask );
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

void ICACHE_FLASH_ATTR CSPreInit()
{
	int opmode = wifi_get_opmode();
	printf( "Opmode: %d\n", opmode );
	if( opmode == 1 )
	{
		struct station_config sc;
		wifi_station_get_config(&sc);
		printf( "Station mode: \"%s\":\"%s\" (bssid_set:%d)\n", sc.ssid, sc.password, sc.bssid_set );
		if( sc.ssid[0] == 0 && !sc.bssid_set )
		{
			wifi_set_opmode( 2 );
			opmode = 2;
		}
		else
		{
			wifi_station_connect();
		}
	}
	if( opmode == 2 )
	{
		struct softap_config sc;
		wifi_softap_get_config(&sc);
		printf( "SoftAP mode: \"%s\":\"%s\"\n", sc.ssid, sc.password );
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
		EnterCritical();
		wifi_set_opmode_current( SOFTAP_MODE );
		ExitCritical();
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

