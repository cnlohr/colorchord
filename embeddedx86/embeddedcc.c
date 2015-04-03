//
// This is the teststrap for the Embedded ColorChord System.
// It is intended as a minimal scaffolding for testing Embedded ColorChord.
//

#include <stdio.h>
#include "embeddednf.h"

int main()
{
	int wf = 0;
	int ci;
	Init();
	while( ( ci = getchar() ) != EOF )
	{
		int cs = ci - 0x80;
#ifdef USE_32DFT
		PushSample32( ((int8_t)cs)*32 );
#else
		Push8BitIntegerSkippy( (int8_t)cs );
#endif
		//printf( "%d ", cs ); fflush( stdout );
		wf++;
		if( wf == 64 )
		{
			HandleFrameInfo();
			wf = 0;
		}
	}
	return 0;
}

