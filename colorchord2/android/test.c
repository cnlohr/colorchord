//Copyright (c) 2011-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
// NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK
// Super basic test - see rawdrawandroid's thing for a more reasonable test.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <asset_manager.h>
#include <asset_manager_jni.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/sensor.h>

#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <asm/byteorder.h>

#include "android_usb_devices.h"

#include "os_generic.h"

#define CNFG_IMPLEMENTATION
#include "CNFG.h"

#define printf( x...) LOGI( x )

unsigned frames = 0;
unsigned long iframeno = 0;

void AndroidDisplayKeyboard(int pShow);
volatile int suspended;

short screenx, screeny;
int lastmousex = 0;
int lastmousey = 0;
int lastbid = 0;
int lastmask = 0;
int lastkey, lastkeydown;

static int keyboard_up;

int mousedown;
int colormode;
double colormodechangetime;


void HandleKey( int keycode, int bDown )
{
	lastkey = keycode;
	lastkeydown = bDown;
	if( keycode == 10 && !bDown ) { keyboard_up = 0; AndroidDisplayKeyboard( keyboard_up );  }
}

void HandleButton( int x, int y, int button, int bDown )
{
	lastbid = button;
	lastmousex = x;
	lastmousey = y;

	if( bDown )  { colormode = (colormode+1)%2; }
	if( !bDown ) { colormodechangetime = OGGetAbsoluteTime(); }
	mousedown = bDown;
//	if( bDown ) { keyboard_up = !keyboard_up; AndroidDisplayKeyboard( keyboard_up ); }
}

void HandleMotion( int x, int y, int mask )
{
	lastmask = mask;
	lastmousex = x;
	lastmousey = y;
}

extern struct android_app * gapp;

int lastFDWrite = 0;

void HandleDestroy()
{
	printf( "Destroying\n" );
	exit(10);
}

void HandleSuspend()
{
	suspended = 1;
}

void HandleResume()
{
	suspended = 0;
}


#define NUM_LEDS 20
uint8_t Colorbuf[NUM_LEDS*4];

char rettext[512];
char assettext[512];

int pixelhueX = -1, pixelhueY = -1;

unsigned long HSVtoHEX( float hue, float sat, float value );
unsigned long PixelHue( int x, int y )
{
	float sat = (pixelhueY-y) / (float)pixelhueY * 2.0;
	float hue = x / (float)pixelhueX;
	if( sat < 1.0 )
	{
		return HSVtoHEX( x * 0.0012, (sat<1)?sat:1, 1.0 );
	}
	else
	{
		return HSVtoHEX( x * 0.0012, (sat<1)?sat:1, 2.0-sat );
	}
}

int main()
{
	int i, x, y;
	double ThisTime;
	double LastFPSTime = OGGetAbsoluteTime();
	double LastFrameTime = OGGetAbsoluteTime();
	double SecToWait;
	int linesegs = 0;
	uint32_t * pixelHueBackdrop = 0;

	CNFGBGColor = 0x400000;
	CNFGDialogColor = 0x444444;
	CNFGSetup( "Test Bench", 0, 0 );

	//To make text look boldish

	while(1)
	{
		int i, pos;
		float f;
		iframeno++;
		RDPoint pto[3];

		CNFGHandleInput();

		if( suspended ) { usleep(50000); continue; }

		CNFGClearFrame();
		CNFGGetDimensions( &screenx, &screeny );

		if( ( screenx != pixelhueX || screeny != pixelhueY ) && screenx > 0 && screeny > 0)
		{
			pixelhueX = screenx;
			pixelhueY = screeny;
			pixelHueBackdrop = realloc( pixelHueBackdrop, pixelhueX * pixelhueY * 4 );
			int x, y;
			for( y = 0; y < pixelhueY; y++ )
			for( x = 0; x < pixelhueX; x++ )
			{
				pixelHueBackdrop[x+y*screenx] = PixelHue( x, y );
			}
		}

		//This whole section does cool stuff with LEDs
		int allledbytes = NUM_LEDS*4;
		for( i = 0; i < allledbytes; i+=4 )
		{
			uint32_t rk;
			float sat = (OGGetAbsoluteTime() - colormodechangetime)*3.0;

			if( colormode )
			{
				rk = PixelHue( lastmousex, lastmousey );
			}
			else
			{
				rk = HSVtoHEX( i * 0.012+ iframeno* .01, (sat<1)?sat:1, 1.0 );
			}

			int white = (int)((1.-sat) * 255);
			if( white > 255 ) white = 255;
			if( white < 0 ) white = 0;

			Colorbuf[i+0] = rk>>8;
			Colorbuf[i+1] = rk;
			Colorbuf[i+2] = rk>>16;
			Colorbuf[i+3] = white;
		}


		if( pixelHueBackdrop && colormode == 1 && mousedown )
		{
			CNFGUpdateScreenWithBitmap( pixelHueBackdrop, pixelhueX, pixelhueY );
		}
		else
		{
			int led = 0;
			for( led = 0; led < NUM_LEDS; led++ )
			{
				uint32_t col = ( Colorbuf[led*4+0] << 8) | ( Colorbuf[led*4+1] ) | ( Colorbuf[led*4+2] << 16);
				CNFGColor( 0xff000000 | col );
				int sx = (led * screenx) / (NUM_LEDS);
				CNFGTackRectangle( sx, 850, sx + screenx/(NUM_LEDS)+1, screeny );
				CNFGFlushRender();
			}
		}

		if( deviceConnectionFD )
		{
			//This section does the crazy wacky stuff to actually split the LEDs into HID Packets and get them out the door... Carefully.
			int byrem = allledbytes;
			int offset = 0;
			for( i = 0; i < 2; i++ )
			{
				uint8_t sendbuf[64];
				sendbuf[0] = (byrem > 60)?15:(byrem/4);
				sendbuf[1] = offset;

				memcpy( sendbuf + 2, Colorbuf + offset*4, sendbuf[0]*4 );

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
				lastFDWrite = ioctl(deviceConnectionFD, USBDEVFS_BULK, &ctrl);
				if( lastFDWrite < 0 )
				{
					DisconnectUSB();
					break;
				}
				usleep(1000); 
			}

			{
				char * rxprintf = rettext;
				uint8_t RXbuf[64];
				//Also read-back the properties.
				struct usbdevfs_bulktransfer  ctrl;
				memset(&ctrl, 0, sizeof(ctrl));
				ctrl.ep = 0x81; //Endpoint 0x81 is input endpoint.
				ctrl.len = 64;
				ctrl.data = RXbuf;
				ctrl.timeout = 100;
				int lastfdread = ioctl(deviceConnectionFD, USBDEVFS_BULK, &ctrl);
				rxprintf += sprintf( rxprintf, "RX: %d\n", lastfdread );
				if( lastfdread == 64 )
				{
					int temperature = RXbuf[4] | (RXbuf[5]<<8);
					int adc = RXbuf[6] | (RXbuf[7]<<8);
					int voltage = RXbuf[8] | (RXbuf[9]<<8);
					rxprintf += sprintf( rxprintf, "T: %d  ADC: %d V: %d\n", temperature, adc, voltage );	

					int t;
					CNFGColor( 0xffffffff );
					for( t = 0; t < 3; t++ )
					{
						CNFGTackSegment( t * screenx / 3, RXbuf[20+t] * 50 + 1100, (t+1)*screenx/3, RXbuf[20+t] * 50 + 1100 );
					}
				}
			}
		}


		if( deviceConnectionFD == 0 )
		{
			RequestPermissionOrGetConnectionFD( assettext, 0xabcd, 0xf410 );
		}

		CNFGPenX = 20; CNFGPenY = 200;
		char st[2048];
		sprintf( st, "%dx%d %d %d %d %d - %d %d - %d\n%s\n%s", screenx, screeny, lastmousex, lastmousey, lastkey, lastkeydown, lastbid, lastmask, lastFDWrite, assettext, rettext );

		CNFGColor( 0xff000000 );
		glLineWidth( 20.0f );
		CNFGDrawText( st, 10 );
		CNFGFlushRender();

		CNFGColor( 0xFFFFFFFF );
		glLineWidth( 2.0f );
		CNFGDrawText( st, 10 );
		CNFGFlushRender();


		// Square behind text

		frames++;
		CNFGSwapBuffers();

		ThisTime = OGGetAbsoluteTime();
		if( ThisTime > LastFPSTime + 1 )
		{
			printf( "FPS: %d\n", frames );
			frames = 0;
			linesegs = 0;
			LastFPSTime+=1;
		}
	}

	return(0);
}













unsigned long HSVtoHEX( float hue, float sat, float value )
{
	float pr = 0;
	float pg = 0;
	float pb = 0;

	short ora = 0;
	short og = 0;
	short ob = 0;

	float ro = fmod( hue * 6, 6. );

	float avg = 0;

	ro = fmod( ro + 6 + 1, 6 ); //Hue was 60* off...

	if( ro < 1 ) //yellow->red
	{
		pr = 1;
		pg = 1. - ro;
	} else if( ro < 2 )
	{
		pr = 1;
		pb = ro - 1.;
	} else if( ro < 3 )
	{
		pr = 3. - ro;
		pb = 1;
	} else if( ro < 4 )
	{
		pb = 1;
		pg = ro - 3;
	} else if( ro < 5 )
	{
		pb = 5 - ro;
		pg = 1;
	} else
	{
		pg = 1;
		pr = ro - 5;
	}

	//Actually, above math is backwards, oops!
	pr *= value;
	pg *= value;
	pb *= value;

	avg += pr;
	avg += pg;
	avg += pb;

	pr = pr * sat + avg * (1.-sat);
	pg = pg * sat + avg * (1.-sat);
	pb = pb * sat + avg * (1.-sat);

	ora = pr * 255;
	og = pb * 255;
	ob = pg * 255;

	if( ora < 0 ) ora = 0;
	if( ora > 255 ) ora = 255;
	if( og < 0 ) og = 0;
	if( og > 255 ) og = 255;
	if( ob < 0 ) ob = 0;
	if( ob > 255 ) ob = 255;

	return (ob<<16) | (og<<8) | ora;
}



