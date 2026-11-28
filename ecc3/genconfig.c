#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char ** argv )
{
	if( argc < 2 )
	{
		fprintf( stderr, "Usage: genconfig [SPS] [Freq Base=32.7032] [BPERO=24] [OCTAVES=6] [Q=1.0]\n" );
		return -1;
	}

	float sps = atof(argv[1]);
	float freqbase = (argc > 2) ? atof(argv[2]) : 32.7032;
	int ECC3_BPERO = (argc > 3) ? atoi(argv[3]) : 24;
	int octaves = (argc > 4) ? atoi(argv[4]) : 6;
	float Q = (argc > 5) ? atof(argv[5]) : 1.0;

	float highestNote = freqbase * pow( 2, octaves + (1.0/ECC3_BPERO ) );
	if( highestNote * 4 > sps )
	{
		fprintf( stderr, "Error: for CC3, you must sample at at least 4x the note over the highest frequency\n" );
		return -1;
	}

	printf( "#ifndef _CC3CONFIG_H\n" );
	printf( "#define _CC3CONFIG_H\n" );
	printf( "\n" );
	printf( "#define ECC3_FREQ_BASE %f // Not used in algorithm, just for note.\n", freqbase ); 
	printf( "#define ECC3_SPS %f // Not used in algorithm, just for note.\n", sps ); 
	printf( "// Highest Note: %f Hz.\n", highestNote );
	printf( "\n" );
	printf( "#define ECC3_BPERO %d\n", ECC3_BPERO );
	printf( "#define ECC3_OCTAVES %d\n", octaves );
	printf( "#define ECC3_CONSTANTS_INCLUDED 1\n" );

	// Radians.  Smaller numbers make NC sharper.  Larger, NC bigger.
	// 1.25 is selected specifically because it is the minimum to always
	// fully detect microtones.  Any smaller and there become unresolvable notes.
	// **cannot exceed 1.57**, because of integer overflow */
	float NCRotation = 1.25; 
	uint32_t ecc3_ncrotateC = cos(NCRotation)*((1ULL<<32)-1);
	uint32_t ecc3_ncrotateS = sin(NCRotation)*((1ULL<<32)-1);

	printf( "const uint32_t ecc3_ncrotateC = %u\n", ecc3_ncrotateC );
	printf( "const uint32_t ecc3_ncrotateS = %u\n", ecc3_ncrotateS );

	uint32_t ECC3_BINS = ECC3_BPERO*octaves;

	uint32_t ecc3_constants[ECC3_BINS];
	uint32_t ecc3_iirfalloff[ECC3_BINS];
	uint32_t ecc3_powerfalloff[ECC3_BINS];
	uint32_t ecc3_phasorsin[ECC3_BINS];
	uint32_t ecc3_phasorcos[ECC3_BINS];

	for( int i = 0; i < ECC3_BINS; i++ )
	{
		float fTarget = freqbase * pow( 2, (i-0.5)/(float)ECC3_BPERO );
		float omegaPerSample = fTarget / sps * 3.1415926 * 2;

		float tFalloff = 1-(1./Q*omegaPerSample/ECC3_BPERO);

		float rescalePhasor = ((tFalloff+1)/2); // Is this right?  It looks right.
		ecc3_constants[i] = cos( omegaPerSample ) * ((1ULL<<32)-1) * rescalePhasor;
		ecc3_phasorsin[i] = sin( omegaPerSample ) * ((1ULL<<32)-1);
		ecc3_phasorcos[i] = cos( omegaPerSample ) * ((1ULL<<32)-1);
		ecc3_iirfalloff[i] = tFalloff *(1ULL<<32);

		// Treat ECC3_BINS = unity.
		float power_falloff = pow( 0.5, ((ECC3_BINS-i)/(float)ECC3_BPERO));
		ecc3_powerfalloff[i] = power_falloff*(1ULL<<32);

		//printf( "%d: %f: om:%f %llu %llu\n", i, fTarget, omegaPerSample, (1ULL<<32)-ecc3_constants[i], (1ULL<<32)-ecc3_iirfalloff[i] );
	}

	printf( "const uint32_t ecc3_constants[%d] = {\n\t", ECC3_BINS );
	for( int i = 0; i < ECC3_BINS; i++ ) printf( "%u,%s", ecc3_constants[i], ((i&0x7) == 0x7) ? "\n\t" : " " );
	printf( "};\n" );

	printf( "const uint32_t ecc3_iirfalloff[%d] = {\n\t", ECC3_BINS );
	for( int i = 0; i < ECC3_BINS; i++ ) printf( "%u,%s", ecc3_iirfalloff[i], ((i&0x7) == 0x7) ? "\n\t" : " " );
	printf( "};\n" );

	printf( "const uint32_t ecc3_powerfalloff[%d] = {\n\t", ECC3_BINS );
	for( int i = 0; i < ECC3_BINS; i++ ) printf( "%u,%s", ecc3_powerfalloff[i], ((i&0x7) == 0x7) ? "\n\t" : " " );
	printf( "};\n" );

	printf( "const uint32_t ecc3_phasorsin[%d] = {\n\t", ECC3_BINS );
	for( int i = 0; i < ECC3_BINS; i++ ) printf( "%u,%s", ecc3_phasorsin[i], ((i&0x7) == 0x7) ? "\n\t" : " " );
	printf( "};\n" );

	printf( "const uint32_t ecc3_phasorcos[%d] = {\n\t", ECC3_BINS );
	for( int i = 0; i < ECC3_BINS; i++ ) printf( "%u,%s", ecc3_phasorcos[i], ((i&0x7) == 0x7) ? "\n\t" : " " );
	printf( "};\n" );

	printf( "\n" );
	printf( "#endif\n" );

	return 0;
}

