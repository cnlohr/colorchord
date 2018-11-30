#include "commonservices.h"
#include <gpio.h>
#include <ccconfig.h>
#include <eagle_soc.h>
#include "esp82xxutil.h"
#include <DFT32.h>
#include <embeddednf.h>
#include <embeddedout.h>

extern volatile uint8_t sounddata[];
extern volatile uint16_t soundhead;


#define CONFIGURABLES 19 //(plus1)


struct SaveLoad
{
	uint16_t configs[CONFIGURABLES];
	uint16_t SaveLoadKey; //Must be 0xaa to be valid.
} settings;

struct CCSettings CCS;

uint16_t gConfigDefaults[CONFIGURABLES] =  { 0, 6, 1, 2, 3, 4, 7, 4, 2, 80, 64, 12, 15, NUM_LIN_LEDS, 1, 0, 16, 0, 0 };

uint16_t * gConfigurables[CONFIGURABLES] = { &CCS.gROOT_NOTE_OFFSET, &CCS.gDFTIIR, &CCS.gFUZZ_IIR_BITS, &CCS.gFILTER_BLUR_PASSES,
	&CCS.gSEMIBITSPERBIN, &CCS.gMAX_JUMP_DISTANCE, &CCS.gMAX_COMBINE_DISTANCE, &CCS.gAMP_1_IIR_BITS,
	&CCS.gAMP_2_IIR_BITS, &CCS.gMIN_AMP_FOR_NOTE, &CCS.gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR, &CCS.gNOTE_FINAL_AMP,
	&CCS.gNERF_NOTE_PORP, &CCS.gUSE_NUM_LIN_LEDS, &CCS.gCOLORCHORD_ACTIVE, &CCS.gCOLORCHORD_OUTPUT_DRIVER, &CCS.gINITIAL_AMP, &CCS.gLED_DRIVER_MODE, 0 };

char * gConfigurableNames[CONFIGURABLES] = { "gROOT_NOTE_OFFSET", "gDFTIIR", "gFUZZ_IIR_BITS", "gFILTER_BLUR_PASSES",
	"gSEMIBITSPERBIN", "gMAX_JUMP_DISTANCE", "gMAX_COMBINE_DISTANCE", "gAMP_1_IIR_BITS",
	"gAMP_2_IIR_BITS", "gMIN_AMP_FOR_NOTE", "gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR", "gNOTE_FINAL_AMP",
	"gNERF_NOTE_PORP", "gUSE_NUM_LIN_LEDS", "gCOLORCHORD_ACTIVE", "gCOLORCHORD_OUTPUT_DRIVER", "gINITIAL_AMP", "gLED_DRIVER_MODE", 0 };

void ICACHE_FLASH_ATTR CustomStart( )
{
	int i;
	spi_flash_read( CCCONFIG_ADDRESS, (uint32*)&settings, sizeof( settings ) );
	if( settings.SaveLoadKey == 0xaa )
	{
		for( i = 0; i < CONFIGURABLES; i++ )
		{
			if( gConfigurables[i] )
			{
				*gConfigurables[i] = settings.configs[i];
			}
		}
	}
	else
	{
		for( i = 0; i < CONFIGURABLES; i++ )
		{
			if( gConfigurables[i] )
			{
				*gConfigurables[i] = gConfigDefaults[i];
			}
		}
	}
}

int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;

	switch( pusrdata[1] )
	{


	case 'b': case 'B': //bins
	{
		int i;
		int whichSel = ParamCaptureAndAdvanceInt( );

		uint16_t * which = 0;
		uint16_t qty = FIXBINS;
		switch( whichSel )
		{
		case 0:
			which = embeddedbins32; break;
		case 1:
			which = fuzzed_bins; break;
		case 2:
			qty = FIXBPERO;
			which = folded_bins; break;
		default:
			buffend += ets_sprintf( buffend, "!CB" );
			return buffend-buffer;
		}

		buffend += ets_sprintf( buffend, "CB%d\t%d\t", whichSel, qty );
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


	case 'l': case 'L': //LEDs
	{
		int i, it = 0;
		buffend += ets_sprintf( buffend, "CL\t%d\t", USE_NUM_LIN_LEDS );
		uint16_t toledsvals = USE_NUM_LIN_LEDS*3;
		if( toledsvals > 600 ) toledsvals = 600;
		for( i = 0; i < toledsvals; i++ )
		{
			uint8_t samp = ledOut[it++];
			*(buffend++) = tohex1( samp>>4 );
			*(buffend++) = tohex1( samp&0x0f );
		}
		return buffend-buffer;
	}


	case 'm': case 'M': //Oscilloscope
	{
		int i, it = soundhead;
		buffend += ets_sprintf( buffend, "CM\t512\t" );
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
		buffend += ets_sprintf( buffend, "CN\t%d\t", MAXNOTES );
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


	case 's': case 'S':
	{
		switch (pusrdata[2] )
		{

		case 'd': case 'D':
		{
			int i;
			for( i = 0; i < CONFIGURABLES-1; i++ )
			{
				if( gConfigurables[i] )
					*gConfigurables[i] = gConfigDefaults[i];
			}
			buffend += ets_sprintf( buffend, "CD" );
			return buffend-buffer;
		}

		case 'r': case 'R':
		{
			int i;
			for( i = 0; i < CONFIGURABLES-1; i++ )
			{
				if( gConfigurables[i] )
					*gConfigurables[i] = settings.configs[i];
			}
			buffend += ets_sprintf( buffend, "CSR" );
			return buffend-buffer;
		}

		case 's': case 'S':
		{
			int i;

			for( i = 0; i < CONFIGURABLES-1; i++ )
			{
				if( gConfigurables[i] )
					settings.configs[i] = *gConfigurables[i];
			}
			settings.SaveLoadKey = 0xAA;

			EnterCritical();
			ets_intr_lock();
			spi_flash_erase_sector( CCCONFIG_ADDRESS/4096 );
			spi_flash_write( CCCONFIG_ADDRESS, (uint32*)&settings, ((sizeof( settings )-1)&(~0xf))+0x10 );
			ets_intr_unlock();
			ExitCritical();

			buffend += ets_sprintf( buffend, "CS" );
			return buffend-buffer;
		}
		}
		buffend += ets_sprintf( buffend, "!CS" );
		return buffend-buffer;
	}



	case 'v': case 'V': //ColorChord Values
	{
		if( pusrdata[2] == 'R' || pusrdata[2] == 'r' )
		{
			int i;

			buffend += ets_sprintf( buffend, "CVR\t" );

			i = 0;
			while( gConfigurableNames[i] )
			{
				buffend += ets_sprintf( buffend, "%s=%d\t", gConfigurableNames[i], *gConfigurables[i] );
				i++;
			}

			buffend += ets_sprintf( buffend, "rBASE_FREQ=%d\trDFREQ=%d\trOCTAVES=%d\trFIXBPERO=%d\trNOTERANGE=%d\trSORT_NOTES=%d\t", 
				(int)BASE_FREQ, (int)DFREQ, (int)OCTAVES, (int)FIXBPERO, (int)(NOTERANGE),(int)SORT_NOTES );
			buffend += ets_sprintf( buffend, "rMAXNOTES=%d\trNUM_LIN_LEDS=%d\trLIN_WRAPAROUND=%d\trLIN_WRAPAROUND=%d\t", 
				(int)MAXNOTES, (int)NUM_LIN_LEDS, (int)LIN_WRAPAROUND, (int)LIN_WRAPAROUND );

			return buffend-buffer;
		}
		else if( pusrdata[2] == 'W' || pusrdata[2] == 'w' )
		{
			parameters+=2;
			char * name = ParamCaptureAndAdvance();
			int val = ParamCaptureAndAdvanceInt();
			int i = 0;

			do
			{
				while( gConfigurableNames[i] )
				{
					if( strcmp( name, gConfigurableNames[i] ) == 0 )
					{
						*gConfigurables[i] = val;
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

