

#define CNFG_IMPLEMENTATION
#include "../../colorchord2/rawdraw/rawdraw_sf.h"
#include "../../colorchord2/cnfa/CNFA_pulse.c"
#include "../../colorchord2/cnfa/CNFA.c"


#include <math.h>
#include <ctype.h>

#define ECC3_BPERO 24
#define ECC3_OCTAVES 6

#define FREQ_BASE 32.7

#include "../embeddedcc3.h"

float setRot = 1.25;

void HandleKey( int keycode, int bDown )
{
	if( !bDown ) return;
	switch( toupper( keycode ) )
	{
	case 'A': setRot += 0.1; break;
	case 'Q': setRot -= 0.1; break;
	}
}

void HandleButton( int x, int y, int button, int bDown )
{
	
}

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

		float Q = 1.0; // Higher Q = more narrow.

		// Radians.  Smaller numbers make NC sharper.  Larger, NC bigger.
		// 1.25 is selected specifically because it is the minimum to always
		// fully detect microtones.  Any smaller and there become unresolvable notes.
		float NCRotation = 1.25; 

		float tFalloff = 1-(1./Q*omegaPerSample/ECC3_BPERO);

		float rescalePhasor = ((tFalloff+1)/2); // Is this right?  It looks right.
		ecc3_constants[i] = cos( omegaPerSample ) * 4294967295 * rescalePhasor;
		ecc3_iirfalloff[i] = tFalloff *(1ULL<<32);

		// Treat ECC_BINS = unity.
		float power_falloff = pow( 0.5, ((ECC_BINS-i)/(float)ECC3_BPERO));
		ecc3_powerfalloff[i] = power_falloff*(1ULL<<32);

		printf( "%d: %f: om:%f %llu %llu\n", i, fTarget, omegaPerSample, (1ULL<<32)-ecc3_constants[i], (1ULL<<32)-ecc3_iirfalloff[i] );
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

		fx = 0;

		float BPERONCTotals[ECC3_BPERO] = { 0 };

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
			float relativeRotate = setRot;

			float rP = cos(relativeRotateP)*inrP - sin(relativeRotateP)*iniP;
			float iP = sin(relativeRotateP)*inrP + cos(relativeRotateP)*iniP;
			float rN = cos(relativeRotate)*inrN - sin(relativeRotate)*iniN;
			float iN = sin(relativeRotate)*inrN + cos(relativeRotate)*iniN;
			float v =  -(rP * rN + iP * iN);
			float ascale = ecc3_powerfalloff[i] / (float)(1ULL<<32);

if( i == 48 )
{
CNFGColor( 0xffffffff );
	printf( "Rot: %6.2f %8.0f %8.0f (%6.2f) %8.0f %8.0f (%6.2f) Rot: %f\n", setRot, inrP, iniP, atan2( iniP, inrP ), inrN, iniN, atan2( iniN, inrN ) );
}

			int negative = 0;
			if( v < 0 ) { v = -v; negative = 1; }


			v = sqrt(v);

			int noteinoctave = i%ECC3_BPERO;
			int o = i/ECC3_BPERO;
			float rpower = 1.0;
			if( o == 0 )
			{
				rpower = noteinoctave / (float)ECC3_BPERO;
			}
			else if( o == ECC3_OCTAVES-1 )
			{
				rpower = (ECC3_BPERO - noteinoctave - 1) / (float)ECC3_BPERO;
			}
			BPERONCTotals[noteinoctave] += v * rpower * (negative?0:1) * ascale;

			float scale = 0.01*ascale;
			if( negative )
			{
				uint32_t hex = ECC3toHEX( (i%ECC3_BPERO)*8, 255, 127 );
				CNFGColor( ((hex&0xff)<<16) | ((hex&0xff00)<<16) | ((hex&0xff0000)>>8) | 0xff );
				CNFGTackRectangle( fx, h*2/3, fx+fpv, h*2/3+v*scale ); 
			}
			else
			{
				uint32_t hex = ECC3toHEX( (i%ECC3_BPERO)*8, 255, 255 );
				CNFGColor( ((hex&0xff)<<16) | ((hex&0xff00)<<16) | ((hex&0xff0000)>>8) | 0xff );
				CNFGTackRectangle( fx, h*2/3-v*scale, fx+fpv, h*2/3 ); 
			}

				uint32_t hex = ECC3toHEX( (i%ECC3_BPERO)*8, 255, 255 );
				CNFGColor( ((hex&0xff)<<16) | ((hex&0xff00)<<16) | ((hex&0xff0000)>>8) | 0xff );

			float p = (rP * rP + iP * iP);
			CNFGTackRectangle( fx, h-(sqrt(p))*scale, fx+fpv, h ); 


			fx += fpv;
		}


		fx = 0;


		for( int i = 0; i < ECC3_BPERO; i++ )
		{
			float v =  BPERONCTotals[i];
			int negative = v < 0;

			float scale = 0.01;

			if( negative )
			{
				v *= -1;
				uint32_t hex = ECC3toHEX( (i%ECC3_BPERO)*8, 255, 127 );
				CNFGColor( ((hex&0xff)<<16) | ((hex&0xff00)<<16) | ((hex&0xff0000)>>8) | 0xff );
				CNFGTackRectangle( fx, h/3, fx+fpv, h/3+v*scale ); 
			}
			else
			{
				uint32_t hex = ECC3toHEX( (i%ECC3_BPERO)*8, 255, 255 );
				CNFGColor( ((hex&0xff)<<16) | ((hex&0xff00)<<16) | ((hex&0xff0000)>>8) | 0xff );
				CNFGTackRectangle( fx, h/3-v*scale, fx+fpv, h/3 ); 
			}


			fx += fpv;
		}


		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();	
		usleep(10000);	
	}
}
