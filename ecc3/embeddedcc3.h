#ifndef _EMBEDDEDCC3_H
#define _EMBEDDEDCC3_H

/* Overall outline
 1: Try goertzels-only, but NC-mode. See if we can NC with IIR
*/

// What if we do an IIR based falloff
// but, we were very careful about what that IIR window was.

#ifndef ECC3_BERPO
	#define ECC3_BPERO 24
#endif

#ifndef ECC3_OCTAVES
	#define ECC3_OCTAVES 6
#endif

#ifndef ECC3_FRAMETYPE
	#define ECC3_FRAMETYPE int16_t
#endif

// Goertzels basically requires the coefficients to be 32-bits.
const uint32_t ecc3_constants[ECC3_BPERO*ECC3_OCTAVES];

struct embeddedcc3_t
{
	int32_t spreva2[ECC3_BPERO*ECC3_OCTAVES][2];
} embeddedcc3;

static void UpdateECC3( embeddedcc3 * ecc, ECC3_FRAMETYPE * frames, int nrframes );



static void UpdateECC3( embeddedcc3 * ecc, ECC3_FRAMETYPE * frames, int nrframes )
{
	int i;
	for( i = 0; i < ECC3_BPERO; i++ )
	{
		
	}
}


#endif

