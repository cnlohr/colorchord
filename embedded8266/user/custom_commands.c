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


#define CONFIGURABLES 22 //(plus1)

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
uint8_t gCOLORCHORD_ACTIVE = 1;
uint8_t gCOLORCHORD_OUTPUT_DRIVER = 0;
uint8_t gCOLORCHORD_SHIFT_INTERVAL = 0;
uint8_t gCOLORCHORD_FLIP_ON_PEAK = 0;
int8_t gCOLORCHORD_SHIFT_DISTANCE = 0; //distance of shift
uint8_t gCOLORCHORD_SORT_NOTES = 0; // 0 no sort, 1 inc freq, 2 dec amps, 3 dec amps2
uint8_t gCOLORCHORD_LIN_WRAPAROUND = 0; // 0 no adjusting, else current led display has minimum deviation to prev



struct SaveLoad
{
	uint8_t configs[CONFIGURABLES];
} settings;

uint8_t gConfigDefaults[CONFIGURABLES] =  { 0, 6, 1, 2, 3, 4, 7, 4, 2, 80, 64, 12, 15, NUM_LIN_LEDS, 1, 0, 0, 0, 0, 0, 0, 0 };

uint8_t * gConfigurables[CONFIGURABLES] = { &RootNoteOffset, &gDFTIIR, &gFUZZ_IIR_BITS, &gFILTER_BLUR_PASSES,
	&gSEMIBITSPERBIN, &gMAX_JUMP_DISTANCE, &gMAX_COMBINE_DISTANCE, &gAMP_1_IIR_BITS,
	&gAMP_2_IIR_BITS, &gMIN_AMP_FOR_NOTE, &gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR, &gNOTE_FINAL_AMP,
	&gNERF_NOTE_PORP, &gUSE_NUM_LIN_LEDS, &gCOLORCHORD_ACTIVE, &gCOLORCHORD_OUTPUT_DRIVER, &gCOLORCHORD_SHIFT_INTERVAL, &gCOLORCHORD_FLIP_ON_PEAK, &gCOLORCHORD_SHIFT_DISTANCE, &gCOLORCHORD_SORT_NOTES, &gCOLORCHORD_LIN_WRAPAROUND, 0 };

char * gConfigurableNames[CONFIGURABLES] = { "gROOT_NOTE_OFFSET", "gDFTIIR", "gFUZZ_IIR_BITS", "gFILTER_BLUR_PASSES",
	"gSEMIBITSPERBIN", "gMAX_JUMP_DISTANCE", "gMAX_COMBINE_DISTANCE", "gAMP_1_IIR_BITS",
	"gAMP_2_IIR_BITS", "gMIN_AMP_FOR_NOTE", "gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR", "gNOTE_FINAL_AMP",
	"gNERF_NOTE_PORP", "gUSE_NUM_LIN_LEDS", "gCOLORCHORD_ACTIVE", "gCOLORCHORD_OUTPUT_DRIVER","gCOLORCHORD_SHIFT_INTERVAL", "gCOLORCHORD_FLIP_ON_PEAK", "gCOLORCHORD_SHIFT_DISTANCE", "gCOLORCHORD_SORT_NOTES", "gCOLORCHORD_LIN_WRAPAROUND", 0 };

void ICACHE_FLASH_ATTR CustomStart( )
{
	int i;
	spi_flash_read( 0x3D000, (uint32*)&settings, sizeof( settings ) );
	for( i = 0; i < CONFIGURABLES; i++ )
	{
		if( gConfigurables[i] )
		{
			*gConfigurables[i] = settings.configs[i];
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
		buffend += ets_sprintf( buffend, "CL\t%d\t", gUSE_NUM_LIN_LEDS );
		uint16_t toledsvals = gUSE_NUM_LIN_LEDS*3;
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

			EnterCritical();
			ets_intr_lock();
			spi_flash_erase_sector( 0x3D000/4096 );
			spi_flash_write( 0x3D000, (uint32*)&settings, ((sizeof( settings )-1)&(~0xf))+0x10 );
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

			buffend += ets_sprintf( buffend, "rBASE_FREQ=%d\trDFREQ=%d\trOCTAVES=%d\trFIXBPERO=%d\trNOTERANGE=%d\t", 
				(int)BASE_FREQ, (int)DFREQ, (int)OCTAVES, (int)FIXBPERO, (int)(NOTERANGE) );
			buffend += ets_sprintf( buffend, "rMAXNOTES=%d\trNUM_LIN_LEDS=%d\t", 
				(int)MAXNOTES, (int)NUM_LIN_LEDS );

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

