

#define CNFG_IMPLEMENTATION
#include "../../colorchord2/rawdraw/rawdraw_sf.h"
#include "../../colorchord2/cnfa/CNFA_pulse.c"
#include "../../colorchord2/cnfa/CNFA.c"


void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
int HandleDestroy() { return 0; }

void AudioCB( struct CNFADriver * sd, short * out, short * in, int framesp, int framesr )
{
	printf( "%d\n", framesr );
}

int main()
{
	struct CNFADriver * cnfad = 0;
	CNFGSetup( "Example App", 1024, 768 );
	cnfad = CNFAInit( 0, "ecc3", AudioCB, 0, 48000, 0, 1, 1024, 0, 0, 0 );

	while(CNFGHandleInput())
	{
		CNFGBGColor = 0x000000ff; //Dark Blue Background

		short w, h;
		CNFGClearFrame();
		CNFGGetDimensions( &w, &h );

		//Change color to white.
		CNFGColor( 0xffffffff ); 

		CNFGPenX = 1; CNFGPenY = 1;
		CNFGDrawText( "Hello, World", 2 );
		CNFGTackPixel( 30, 30 );         

		//Draw a line from 50,50 to 100,50
		CNFGTackSegment( 50, 50, 100, 50 );


		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();		
	}
}
