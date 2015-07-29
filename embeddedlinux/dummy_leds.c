//Copyright (Public Domain) 2015 <>< Charles Lohr
//This file may be used in whole or part in any way for any purpose by anyone
//without restriction.

#include <ccconfig.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>

pthread_t pt;
Display *d;
Window w;
XEvent e;
int sockfd;
struct sockaddr_in servaddr;
int wid, hei;

uint8_t mesg[NUM_LIN_LEDS*3+3];

#define LEDS_PER_ROW 15
#define LED_SIZE 15


void thread_function( void * tf )
{
	int n;
	struct sockaddr_in cliaddr;
	socklen_t len;
    XEvent exppp;

	while(1)
	{
		len = sizeof(cliaddr);
		n = recvfrom(sockfd,mesg,sizeof(mesg),0,(struct sockaddr *)&cliaddr,&len);
		//XSendEventXClearArea( d, w, 0, 0, 1, 1, 1 );
		XLockDisplay( d );
		memset(&exppp, 0, sizeof(exppp));
        exppp.type = Expose;
        exppp.xexpose.window = w;
        XSendEvent(d,w,False,ExposureMask,&exppp);
		XFlush( d );
		XUnlockDisplay( d );
	}
}


int main(void)
{
	int n;

	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(7777);

	if( bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) )
	{
		fprintf( stderr, "Error binding.\n" );
	}


	memset( mesg, 0, sizeof( mesg ) );

	pthread_create (&pt, NULL, (void *) &thread_function, (void *)0);

	int s;
	wid = LEDS_PER_ROW;
	hei = NUM_LIN_LEDS/LEDS_PER_ROW + ( (NUM_LIN_LEDS%LEDS_PER_ROW)?1:0 );

	XInitThreads();

	d = XOpenDisplay(NULL);

	if (d == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
	}

	s = DefaultScreen(d);
	w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, wid*LED_SIZE, hei*LED_SIZE, 1, BlackPixel(d, s), WhitePixel(d, s));
	XSetStandardProperties( d, w, "LED Simulaor", "LED Simulator", None, NULL, 0, NULL );
	XSelectInput(d, w, ExposureMask | KeyPressMask);
	XMapWindow(d, w);

	GC dgc = XCreateGC(d, w, 0, 0);

	while (1) {
		XNextEvent(d, &e);
		if (e.type == Expose) {
			for( n = 0; n < NUM_LIN_LEDS; n++ )
			{
				unsigned long color = ( mesg[n*3+3] << 8 ) | ( mesg[n*3+4] << 16 ) | (mesg[n*3+5] << 0);
				XSetForeground(d, dgc, color);
				int x = n % wid;
				int y = n / wid;

				XFillRectangle(d, w, dgc, x*LED_SIZE, y*LED_SIZE, LED_SIZE, LED_SIZE);
			}
		}
	}

	XCloseDisplay(d);
	return 0;
}
