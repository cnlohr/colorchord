#ifndef _EMBEDDEDCC3_H
#define _EMBEDDEDCC3_H

/* Overall outline
 1: Try goertzels-only, but NC-mode. See if we can NC with IIR
*/

// What if we do an IIR based falloff
// but, we were very careful about what that IIR window was.

#ifndef ECC3_BPERO
	#define ECC3_BPERO 24
#endif

#ifndef ECC3_OCTAVES
	#define ECC3_OCTAVES 6
#endif

#ifndef ECC3_FRAMETYPE
	#define ECC3_FRAMETYPE int16_t
#endif


#define ECC_BINS ((ECC3_BPERO)*(ECC3_OCTAVES))

// Goertzels basically requires the coefficients to be 32-bits.
uint32_t ecc3_constants[ECC_BINS];
uint32_t ecc3_iirfalloff[ECC_BINS];

typedef struct embeddedcc3_t
{
	int32_t spreva2[ECC_BINS][2];
} embeddedcc3;

static void ResetECC3( embeddedcc3 * ecc ) { memset( ecc, 0, sizeof(*ecc) ); }

static void UpdateECC3( embeddedcc3 * ecc, ECC3_FRAMETYPE * frames, int nrframes );

static void UpdateECC3( embeddedcc3 * ecc, ECC3_FRAMETYPE * frames, int nrframes )
{
	int i;
	int32_t * sprevaptr = ecc->spreva2[0];
	for( i = 0; i < ECC_BINS; i++ )
	{
		int32_t sprev = sprevaptr[0];
		int32_t sprev2 = sprevaptr[1];
		uint32_t coeff = ecc3_constants[i];
		uint32_t iircons = ecc3_iirfalloff[i];
		for( int j = 0; j < nrframes; j++ )
		{
			ECC3_FRAMETYPE v = frames[j];

			//From https://github.com/cnlohr/cnlohr_tricks/#goertzels-dft

			// Update phasor
			int32_t s = 
				v 
				+ 2*((sprev*(uint64_t)coeff)>>32)   // Actually sprev * coeff, *2 here because >>31 would be a slower operation.  For floating-point formulations, do that here.
				- ((sprev2*(uint64_t)iircons)>>32); // Traditionally, goertzels would use sprev2*1 here over the window, but we are trying windowless.

			sprev2 = sprev;
			sprev = s;
		}
//printf( "%d %d\n", i, sprev );
		sprevaptr[0] = sprev;
		sprevaptr[1] = sprev2;
		sprevaptr += 2;
	}
}


#endif

