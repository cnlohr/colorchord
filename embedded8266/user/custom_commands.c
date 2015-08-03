#include "commonservices.h"
#include <gpio.h>
#include <ccconfig.h>
#include <eagle_soc.h>
#include <DFT32.h>
#include <embeddednf.h>
#include <embeddedout.h>

extern volatile uint8_t sounddata[];
extern volatile uint16_t soundhead;


extern uint8_t RootNoteOffset; //Set to define what the root note is.  0 = A.
uint8_t gDFTIIR = 6;
uint8_t gFUZZ_IIR_BITS = 1;
uint8_t gFILTER_BLUR_PASSES = 2;
uint8_t gSEMIBITSPERBIN = 3;
uint8_t gMAX_JUMP_DISTANCE = 4;
uint8_t gMAX_COMBINE_DISTANCE = 7;
uint8_t gAMP_1_IIR_BITS = 4;
uint8_t gAMP_2_IIR_BITS = 2;
uint8_t gMIN_AMP_FOR_NOTE = 80;
uint8_t gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR = 64;
uint8_t gNOTE_FINAL_AMP = 12;
uint8_t gNERF_NOTE_PORP = 15;
uint8_t gUSE_NUM_LIN_LEDS = NUM_LIN_LEDS;


uint8_t * gConfigurables[] = { &RootNoteOffset, &gDFTIIR, &gFUZZ_IIR_BITS, &gFILTER_BLUR_PASSES,
	&gSEMIBITSPERBIN, &gMAX_JUMP_DISTANCE, &gMAX_COMBINE_DISTANCE, &gAMP_1_IIR_BITS,
	&gAMP_2_IIR_BITS, &gMIN_AMP_FOR_NOTE, &gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR, &gNOTE_FINAL_AMP,
	&gNERF_NOTE_PORP, &gUSE_NUM_LIN_LEDS, 0 };

char * gConfigurableNames[] = { "gROOT_NOTE_OFFSET", "gDFTIIR", "gFUZZ_IIR_BITS", "gFILTER_BLUR_PASSES",
	"gSEMIBITSPERBIN", "gMAX_JUMP_DISTANCE", "gMAX_COMBINE_DISTANCE", "gAMP_1_IIR_BITS",
	"gAMP_2_IIR_BITS", "gMIN_AMP_FOR_NOTE", "gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR", "gNOTE_FINAL_AMP",
	"gNERF_NOTE_PORP", "gUSE_NUM_LIN_LEDS", 0 };

int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;

	switch( pusrdata[1] )
	{


	case 'b': case 'B': //bins
	{
		int i;
		int whichSel = my_atoi( &pusrdata[2] );

		uint16_t * which = 0;
		switch( whichSel )
		{
		case 0:
			which = embeddedbins32; break;
		case 1:
			which = fuzzed_bins; break;
		case 2:
			which = folded_bins; break;
		default:
			buffend += ets_sprintf( buffend, "!CB" );
			return buffend-buffer;
		}

		buffend += ets_sprintf( buffend, "CB%d:%d:", whichSel, FIXBINS );
		for( i = 0; i < FIXBINS; i++ )
		{
			uint16_t samp = which[i];
			*(buffend++) = tohex1( samp>>12 );
			*(buffend++) = tohex1( samp>>8 );
			*(buffend++) = tohex1( samp>>4 );
			*(buffend++) = tohex1( samp>>0 );
		}
		return buffend-buffer;
	}


	case 'm': case 'M': //Oscilloscope
	{
		int i, it = soundhead;
		buffend += ets_sprintf( buffend, "CM:512:" );
		for( i = 0; i < 512; i++ )
		{
			uint8_t samp = sounddata[it++];
			it = it & (HPABUFFSIZE-1);
			*(buffend++) = tohex1( samp>>4 );
			*(buffend++) = tohex1( samp&0x0f );
		}
		return buffend-buffer;
	}

	case 'n': case 'N': //Notes
	{
		int i;
		buffend += ets_sprintf( buffend, "CN:%d:", MAXNOTES );
		for( i = 0; i < MAXNOTES; i++ )
		{
			uint16_t dat;
			dat = note_peak_freqs[i];
			*(buffend++) = tohex1( dat>>4 );
			*(buffend++) = tohex1( dat>>0 );
			dat = note_peak_amps[i];
			*(buffend++) = tohex1( dat>>12 );
			*(buffend++) = tohex1( dat>>8 );
			*(buffend++) = tohex1( dat>>4 );
			*(buffend++) = tohex1( dat>>0 );
			dat = note_peak_amps2[i];
			*(buffend++) = tohex1( dat>>12 );
			*(buffend++) = tohex1( dat>>8 );
			*(buffend++) = tohex1( dat>>4 );
			*(buffend++) = tohex1( dat>>0 );
			dat = note_jumped_to[i];
			*(buffend++) = tohex1( dat>>4 );
			*(buffend++) = tohex1( dat>>0 );
		}
		return buffend-buffer;
	}



	case 'v': case 'V': //ColorChord Values
	{
		if( pusrdata[2] == 'R' || pusrdata[2] == 'r' )
		{
			int i;

			buffend += ets_sprintf( buffend, "CVR:rBASE_FREQ=%d:rDFREQ=%d:rOCTAVES=%d:rFIXBPERO=%d:NOTERANGE=%d:", 
				(int)BASE_FREQ, (int)DFREQ, (int)OCTAVES, (int)FIXBPERO, (int)(NOTERANGE) );
			buffend += ets_sprintf( buffend, "CVR:rMAXNOTES=%d:rNUM_LIN_LEDS=%d:rLIN_WRAPAROUND=%d:rLIN_WRAPAROUND=%d:", 
				(int)MAXNOTES, (int)NUM_LIN_LEDS, (int)LIN_WRAPAROUND, (int)LIN_WRAPAROUND );

			i = 0;
			while( gConfigurableNames[i] )
			{
				buffend += ets_sprintf( buffend, "%s=%d:", gConfigurableNames[i], *gConfigurables[i] );
				i++;
			}

			return buffend-buffer;
		}
		else if( pusrdata[2] == 'W' || pusrdata[2] == 'w' )
		{
			char * colon = 0, * colon2 = 0;
			do
			{
				int i = 0;
				colon = (char *) ets_strstr( (char*)&pusrdata[2], ":" );
				if( !colon ) break;
				*colon = 0;
				colon++;
				colon2 = (char *) ets_strstr( (char*)colon, ":" );
				if( !colon2 ) break;
				*colon2 = 0;
				colon2++;

				while( gConfigurableNames[i] )
				{
					if( strcmp( colon, gConfigurableNames[i] ) == 0 )
					{
						*gConfigurables[i] = my_atoi(colon2);
						buffend += ets_sprintf( buffend, "CVW" );
						return buffend-buffer;
					}
					i++;
				}
			} while( 0 );

			buffend += ets_sprintf( buffend, "!CV" );
			return buffend-buffer;
		}
		else
		{
			buffend += ets_sprintf( buffend, "!CV" );
			return buffend-buffer;
		}
		
	}


	}
	return -1;
}

