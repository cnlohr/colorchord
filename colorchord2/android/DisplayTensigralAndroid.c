#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include "parameters.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "color.h"
#include "CNFG.h"
#include "rawdrawandroid/android_usb_devices.h"

#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <asm/byteorder.h>

extern int is_suspended;

#define MAX_LEDS_PER_NOTE 512

extern short screenx, screeny;

struct DTADriver
{
	int leds;
};

char TensigralDebugStatus[8192];

int RequestPermissionOrGetConnectionFD( char * debug_status, uint16_t vid, uint16_t pid );
void DisconnectUSB(); //Disconnect from USB

extern jobject deviceConnection;
extern int deviceConnectionFD;



static void DTAUpdate(void * id, struct NoteFinder*nf)
{
	struct DTADriver * d = (struct DTADriver*)id;
	int i;

	if( deviceConnectionFD == 0 )
	{
		RequestPermissionOrGetConnectionFD( TensigralDebugStatus, 0xabcd, 0xf410 );
	}

	if( !is_suspended )
	{
		CNFGPenX = 800;
		CNFGPenY = 800;
		CNFGColor( 0xffffff );
		CNFGDrawText( TensigralDebugStatus, 2 );
	}

	if( !deviceConnectionFD ) return;


	uint8_t packet[64];

	if( deviceConnectionFD )
	{
		//This section does the crazy wacky stuff to actually split the LEDs into HID Packets and get them out the door... Carefully.
		int byrem = d->leds*4;  //OutLEDs[i*3+1]
		int offset = 0;
		int marker = 0;
		for( i = 0; i < 2; i++ )
		{
			uint8_t sendbuf[64];
			sendbuf[0] = (byrem > 60)?15:(byrem/4);
			sendbuf[1] = offset;

//			memcpy( sendbuf + 2, Colorbuf + offset*4, sendbuf[0]*4 );
			int j;
			for( j = 0; j < sendbuf[0]; j++ )
			{
				sendbuf[j*4+3] = OutLEDs[marker++];
				sendbuf[j*4+2] = OutLEDs[marker++];
				sendbuf[j*4+4] = OutLEDs[marker++];
				sendbuf[j*4+5] = 0;
			}

			offset += sendbuf[0];
			byrem -= sendbuf[0]*4;


			if( byrem == 0 ) sendbuf[0] |= 0x80;
			int tsend = 65; //Size of payload (must be 64+1 always)

			//Ok this looks weird, because Android has a bulkTransfer function, but that has a TON of layers of misdirection before it just calls the ioctl.
			struct usbdevfs_bulktransfer  ctrl;
			memset(&ctrl, 0, sizeof(ctrl));
			ctrl.ep = 0x02; //Endpoint 0x02 is output endpoint.
			ctrl.len = 64;
			ctrl.data = sendbuf;
			ctrl.timeout = 100;
			int lastFDWrite = ioctl(deviceConnectionFD, USBDEVFS_BULK, &ctrl);
			if( lastFDWrite < 0 )
			{
				DisconnectUSB();
				break;
			}
			usleep(1000); 
		}
	}

	CNFGColor( 0xffffff );
}

static void DTAParams(void * id )
{
	struct DTADriver * d = (struct DTADriver*)id;

	d->leds = 9;		RegisterValue(  "leds", PAINT, &d->leds, sizeof( d->leds ) );
}

static struct DriverInstances * DisplayTensigralAndroid(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct DTADriver * d = ret->id = malloc( sizeof( struct DTADriver ) );
	memset( d, 0, sizeof( struct DTADriver ) );
	ret->Func = DTAUpdate;
	ret->Params = DTAParams;
	DTAParams( d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplayTensigralAndroid);


