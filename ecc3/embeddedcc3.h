#ifndef _EMBEDDEDCC3_H
#define _EMBEDDEDCC3_H

// TODO: Fix function names.

/* Overall outline
 1: Try goertzels-only, but NC-mode. See if we can NC with IIR
*/

// What if we do an IIR based falloff
// but, we were very careful about what that IIR window was.

// References:
//   [1] NC Method https://github.com/CaiB/ColorChord.NET/blob/master/Simulations/BinPeakLocation.m
//     (Originally) Kanatov, et. al https://www.mdpi.com/2079-9292/8/3/330
//   [2] Goertzels DFT https://github.com/cnlohr/cnlohr_tricks/#goertzels-dft
//   [3] Rotating output bins for NC-style collaboration between bins.
//		https://github.com/7ERr0r/sbsdft/blob/02b80708151af77cacfa4a8d2502139bd8ca93fe/src/spectrumapp/sbswdft.rs#L1474-L1496
//
// Useful tool I used:
//  https://audio.js.org/util/tone-generator/

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
#define SEMIBITSPERBIN 3 // 8 semitone levels.
#define ECC3_NOTERANGE ((1<<SEMIBITSPERBIN)*ECC3_BPERO)

// Goertzels basically requires the coefficients to be 32-bits.
uint32_t ecc3_constants[ECC_BINS];
uint32_t ecc3_iirfalloff[ECC_BINS];
uint32_t ecc3_powerfalloff[ECC_BINS];

uint32_t ecc3_phasorsin[ECC_BINS];
uint32_t ecc3_phasorcos[ECC_BINS];
uint32_t ecc3_ncrotateC;
uint32_t ecc3_ncrotateS;


typedef struct embeddedcc3_t
{
	int32_t spreva2[ECC_BINS][2];

	uint32_t folded_dft_nc[ECC3_BPERO];
} embeddedcc3;

static void ResetECC3( embeddedcc3 * ecc ) { memset( ecc, 0, sizeof(*ecc) ); }
static void UpdateECC3( embeddedcc3 * ecc, ECC3_FRAMETYPE * frames, int nrframes );
static void RunOutputECC3( embeddedcc3 * ecc );

// Utility
static uint32_t ECC3toHEX( uint8_t note, uint8_t sat, uint8_t val );
static uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val );
static int apsqrt_ECC3( uint32_t i );

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
			//
			// Update phasor. The way this is written, it turns into:
			//  mulhsu, mulhsu, slli, add, sub (RISC-V)
			// or
			//  (asr,umull,mla)x2, add, add, sub (ARMv7)
			// for ARM, we could re-formulate coefficients as int32_t, 
			//  but it would require more fixup.  It offers no benefit.
			//
			// Part 1: Actually sprev * coeff, *2 here because >>31
			//  would be a slower operation.  For floating-point
			//  formulations, do that here.
			//
			// Part 2: Traditionally, goertzels would use sprev2*1
			//  here over the window, but we are trying windowless.
			//  so we need to re-tune coefficient.
			int32_t s = 
				v 
				+ 2*((sprev *(uint64_t)coeff  )>>32)
				-   ((sprev2*(uint64_t)iircons)>>32);

			sprev2 = sprev;
			sprev = s;
		}
//printf( "%d %d\n", i, sprev );
		sprevaptr[0] = sprev;
		sprevaptr[1] = sprev2;
		sprevaptr += 2;
	}
}

// Debug, eventually put on stack.
int32_t realimag[ECC_BINS][2];

static void RunOutputECC3( embeddedcc3 * ecc )
{
	int i;
	for( i = 0; i < ECC_BINS; i++ )
	{
		int32_t sprev = ecc->spreva2[i][0];
		int32_t sprev2 = ecc->spreva2[i][1];

		int32_t real = ( ( sprev * (uint64_t)ecc3_phasorcos[i]) >> 32) - sprev2;
		int32_t imag = ( ( sprev * (uint64_t)ecc3_phasorsin[i]) >> 32);

		realimag[i][0] = real;
		realimag[i][1] = imag;
	}

	int n;

	i = 0;

	for( n = 0; n < ECC3_BPERO; n++ )
	{
		int32_t powervalue = 0;
		int sum = 0;
		for( int o = 0; o < ECC3_OCTAVES; o++ )
		{
			int i = n + o * ECC3_BPERO;
			int32_t rp = realimag[i][0];
			int32_t ip = realimag[i][1];
			if( i == ECC_BINS ) break;
			int32_t rn = realimag[i+1][0];
			int32_t in = realimag[i+1][1];

			int32_t rotrn = ((rn * (uint64_t)ecc3_ncrotateC)>>32) - ((in * (uint64_t)ecc3_ncrotateS)>>32);
			int32_t rotin = ((rn * (uint64_t)ecc3_ncrotateS)>>32) + ((in * (uint64_t)ecc3_ncrotateC)>>32);

			// 12 picked here arbitrarily. It's so we can handle up to value of 13M in input.

			#define INASCALE 12
			int v =  -(( (rp * (int64_t)rotrn)>>INASCALE) + ((ip * (int64_t)rotin)>>INASCALE));

			int negative = 0;
			if( v < 0 ) continue;

			uint32_t powerfalloff = ecc3_powerfalloff[i];
			v = (apsqrt_ECC3(v));
			int noteinoctave = i%ECC3_BPERO;

			// Feather start and end.
			if( o == 0 )
			{
				powerfalloff = powerfalloff / ECC3_BPERO * (n+1);
			}
			else if( o == ECC3_OCTAVES-1 )
			{
				// Taper ending to zero to exclude last element, since it is unusable.
				powerfalloff = powerfalloff / ECC3_BPERO * (ECC3_BPERO - n - 1) ;
			}
			else
			{
				// powerfalloff unaffected.
			}
			powervalue += (v * (uint64_t)powerfalloff)>>32;
		}
		ecc->folded_dft_nc[n] = powervalue;
	}
}




static uint32_t EHSVtoHEX( uint8_t hue, uint8_t sat, uint8_t val )
{
	#define SIXTH1 43
	#define SIXTH2 85
	#define SIXTH3 128
	#define SIXTH4 171
	#define SIXTH5 213

	uint16_t or = 0, og = 0, ob = 0;

	hue -= SIXTH1; //Off by 60 degrees.

	//TODO: There are colors that overlap here, consider 
	//tweaking this to make the best use of the colorspace.

	if( hue < SIXTH1 ) //Ok: Yellow->Red.
	{
		or = 255;
		og = 255 - ((uint16_t)hue * 255) / (SIXTH1);
	}
	else if( hue < SIXTH2 ) //Ok: Red->Purple
	{
		or = 255;
		ob = (uint16_t)hue*255 / SIXTH1 - 255;
	}
	else if( hue < SIXTH3 )  //Ok: Purple->Blue
	{
		ob = 255;
		or = ((SIXTH3-hue) * 255) / (SIXTH1);
	}
	else if( hue < SIXTH4 ) //Ok: Blue->Cyan
	{
		ob = 255;
		og = (hue - SIXTH3)*255 / SIXTH1;
	}
	else if( hue < SIXTH5 ) //Ok: Cyan->Green.
	{
		og = 255;
		ob = ((SIXTH5-hue)*255) / SIXTH1;
	}
	else //Green->Yellow
	{
		og = 255;
		or = (hue - SIXTH5) * 255 / SIXTH1;
	}

	uint16_t rv = val;
	if( rv > 128 ) rv++;
	uint16_t rs = sat;
	if( rs > 128 ) rs++;

	//or, og, ob range from 0...255 now.
	//Need to apply saturation and value.

	or = (or * val)>>8;
	og = (og * val)>>8;
	ob = (ob * val)>>8;

	//OR..OB == 0..65025
	or = or * rs + 255 * (256-rs);
	og = og * rs + 255 * (256-rs);
	ob = ob * rs + 255 * (256-rs);
//printf( "__%d %d %d =-> %d\n", or, og, ob, rs );

	or >>= 8;
	og >>= 8;
	ob >>= 8;

	return or | (og<<8) | ((uint32_t)ob<<16);
}


static uint32_t ECC3toHEX( uint8_t note, uint8_t sat, uint8_t val )
{
	uint16_t hue = 0;
	uint16_t third = 65535/3;
	uint16_t scalednote = note;
	uint32_t renote = ((uint32_t)note * 65536) / ECC3_NOTERANGE;

	//Note is expected to be a vale from 0..(ECC3_NOTERANGE-1)
	//renote goes from 0 to the next one under 65536.


	if( renote < third )
	{
		//Yellow to Red.
		hue = (third - renote) >> 1;
	}
	else if( renote < (third<<1) )
	{
		//Red to Blue
		hue = (third-renote);
	}
	else
	{
		//hue = ((((65535-renote)>>8) * (uint32_t)(third>>8)) >> 1) + (third<<1);
		hue = (uint16_t)(((uint32_t)(65536-renote)<<16) / (third<<1)) + (third>>1); // ((((65535-renote)>>8) * (uint32_t)(third>>8)) >> 1) + (third<<1);
	}
	hue >>= 8;

	return EHSVtoHEX( hue, sat, val );
}

static int apsqrt_ECC3( uint32_t i )
{
	// Fast approximate square root, accurate to about 1-2 code values across integer space.
	if( i == 0 ) return 0;
	int x = 1<<( ( 32 - __builtin_clz(i) )/2);
	x = (x + i/x)/2;
	x = (x + i/x)/2;
	x = (x + i/x)/2;
	return x;
}


#endif


