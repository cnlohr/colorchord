

#define CNFG_IMPLEMENTATION
#include "../../colorchord2/rawdraw/rawdraw_sf.h"
#include "../../colorchord2/cnfa/CNFA_pulse.c"
#include "../../colorchord2/cnfa/CNFA.c"


#include <math.h>

#define ECC3_BPERO 24
#define ECC3_OCTAVES 6

#define FREQ_BASE 55

#include "../embeddedcc3.h"

void HandleKey( int keycode, int bDown ) { }
void HandleButton( int x, int y, int button, int bDown ) { }
void HandleMotion( int x, int y, int mask ) { }
int HandleDestroy() { return 0; }

embeddedcc3 cc3;

float maxv = 0;
float powers[ECC_BINS];
float reals[ECC_BINS];
float imags[ECC_BINS];

void AudioCB( struct CNFADriver * sd, short * out, short * in, int framesp, int framesr )
{
//	ResetECC3( &cc3 );
	UpdateECC3( &cc3, in, framesr );
//	printf( "%d\n", ecc3_constants[0] );
//	printf( "%d %d %d\n", framesr, cc3.spreva2[0], cc3.spreva2[1] );
		for( int i = 0; i < ECC_BINS; i++ )
		{
			float fTarget = FREQ_BASE * pow( 2, (i-0.5)/(float)ECC3_BPERO );
			float omegaPerSample = fTarget / 48000 * 3.1415926 * 2;


			float sprev = cc3.spreva2[i][0];
			float sprev2 = cc3.spreva2[i][1];
			float coeff = 2 * (ecc3_constants[i] / (float)(1ULL<<32));

			float power = sprev*sprev + sprev2*sprev2 - (coeff * sprev * sprev2);


			float coeff_s = 2 * sin( omegaPerSample );
			double rR = 0.5 * coeff   * sprev - sprev2;
			double rI = 0.5 * coeff_s * sprev;
			//printf ("%f %f  %f %f (%f %f)\n", sprev, sprev2, rR, rI, coeff_s, coeff );
			reals[i] = rR;
			imags[i] = rI;
			power = sqrt( power );
			powers[i] = power;
			if( power > maxv ) maxv = power;
		}
}

int main()
{
	struct CNFADriver * cnfad = 0;
	CNFGSetup( "Example App", 1024, 768 );
	cnfad = CNFAInit( 0, "ecc3", AudioCB, 0, 48000, 0, 1, 1024, 0, 0, 0 );

	ResetECC3( &cc3 );

	for( int i = 0; i < ECC_BINS; i++ )
	{
		float fTarget = FREQ_BASE * pow( 2, (i-0.5)/(float)ECC3_BPERO );
		float omegaPerSample = fTarget / 48000 * 3.1415926 * 2;

		// With omegaPerSample/4.0, it's too flat at high and too sharp at small.
		// With .99, it's too sharp on high end and flat on low end.
		// With 1.0-.001/omegaPerSample, it's too sharp on high end, lumpy on low.
		//float tFalloff = 1-(omegaPerSample/3.14159/2/12);

		float Q = 1; // 0.5 = Target 50% adjacent note.
		float tFalloff = 1-(1./Q*omegaPerSample/ECC3_BPERO);

		float rescalePhasor = ((tFalloff+1)/2); // Is this right?  It looks right.
		ecc3_constants[i] = cos( omegaPerSample ) * 4294967295 * rescalePhasor;
		ecc3_iirfalloff[i] = tFalloff *(1ULL<<32);

		// Treat ECC_BINS = unity.
		float power_falloff = pow( 0.5, ((ECC_BINS-i)/(float)ECC3_BPERO));
		ecc3_powerfalloff[i] = power_falloff*(1ULL<<32);

		printf( "%d: %f: om:%f %llu %llu\n", fTarget, i, omegaPerSample, (1ULL<<32)-ecc3_constants[i], (1ULL<<32)-ecc3_iirfalloff[i] );
	}

	while(CNFGHandleInput())
	{
		CNFGBGColor = 0x000000ff; //Dark Blue Background

		short w, h;
		CNFGClearFrame();
		CNFGGetDimensions( &w, &h );

		//Change color to white.
		CNFGColor( 0xffffffff ); 

		float fpv = (float)w / ECC_BINS;
		float fx = 0;

		for( int i = 0; i < ECC_BINS-1; i++ )
		{
			//float v = powers[i];
			//const float scale = 0.0001;
			float inrP = reals[i];
			float iniP = imags[i];
			float inrN = reals[i+1];
			float iniN = imags[i+1];

//need to rotate 
//XXX HERE TODO PICKUP
			float relativeRotateP = 0;
			float relativeRotate = -1.57;

			float rP = cos(relativeRotateP)*inrP - sin(relativeRotateP)*iniP;
			float iP = sin(relativeRotateP)*inrP + cos(relativeRotateP)*iniP;
			float rN = cos(relativeRotate)*inrN - sin(relativeRotate)*iniN;
			float iN = sin(relativeRotate)*inrN + cos(relativeRotate)*iniN;
			float v =  (rP * rN + iP * iN);
			float ascale = ecc3_powerfalloff[i] / (float)(1ULL<<32);

			uint32_t hex = ECC3toHEX( (i%ECC3_BPERO)*8, 255, 255 );
			CNFGColor( ((hex&0xff)<<16) | ((hex&0xff00)<<16) | ((hex&0xff0000)>>8) | 0xff );

if( i == 48 )
{
CNFGColor( 0xffffffff );
	printf( "%f %f (%f) %f %f (%f)\n", inrP, iniP, atan2( iniP, inrP ), inrN, iniN, atan2( iniN, inrN ) );
}

			if( v < 0 ) v = 0;
			v = sqrt(v);
			float scale = 0.001*ascale;
			CNFGTackRectangle( fx, h/2-v*scale, fx+fpv, h/2 ); 

			float p = (rP * rP + iP * iP);
			CNFGTackRectangle( fx, h-(sqrt(p))*scale, fx+fpv, h ); 


			fx += fpv;
		}


		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();	
		usleep(10000);	
	}
}
