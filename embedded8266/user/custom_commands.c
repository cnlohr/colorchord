#include "commonservices.h"
#include <gpio.h>
#include <ccconfig.h>
#include <eagle_soc.h>
#include "esp82xxutil.h"
#include <DFT32.h>
#include <embeddednf.h>
#include <embeddedout.h>
#include "custom_commands.h"

extern volatile uint8_t sounddata[];
extern volatile uint16_t soundhead;


#define CONFIGURABLES (sizeof(struct CCSettings) / sizeof(uint16_t)) //(plus1)

#define SAVE_LOAD_KEY 0xAA

struct SaveLoad
{
	uint16_t configs[CONFIGURABLES];
	uint16_t SaveLoadKey; //Must be SAVE_LOAD_KEY to be valid.
} settings;

struct CCSettings CCS;

typedef struct {
	uint16_t defaultVal;
	char * name;
	uint16_t * val;
} configurable_t;

configurable_t gConfigs[CONFIGURABLES] =
{
	{
		.defaultVal = 0,
		.name = "gROOT_NOTE_OFFSET",
		.val = &CCS.gROOT_NOTE_OFFSET
	},
	{
		.defaultVal = 6,
		.name = "gDFTIIR",
		.val = &CCS.gDFTIIR
	},
	{
		.defaultVal = 1,
		.name = "gFUZZ_IIR_BITS",
		.val = &CCS.gFUZZ_IIR_BITS
	},
	{
		.defaultVal = 2,
		.name = "gFILTER_BLUR_PASSES",
		.val = &CCS.gFILTER_BLUR_PASSES
	},
	{
		.defaultVal = 3,
		.name = "gSEMIBITSPERBIN",
		.val = &CCS.gSEMIBITSPERBIN
	},
	{
		.defaultVal = 4,
		.name = "gMAX_JUMP_DISTANCE",
		.val = &CCS.gMAX_JUMP_DISTANCE
	},
	{
		.defaultVal = 7,
		.name = "gMAX_COMBINE_DISTANCE",
		.val = &CCS.gMAX_COMBINE_DISTANCE
	},
	{
		.defaultVal = 4,
		.name = "gAMP_1_IIR_BITS",
		.val = &CCS.gAMP_1_IIR_BITS
	},
	{
		.defaultVal = 2,
		.name = "gAMP_2_IIR_BITS",
		.val = &CCS.gAMP_2_IIR_BITS
	},
	{
		.defaultVal = 80,
		.name = "gMIN_AMP_FOR_NOTE",
		.val = &CCS.gMIN_AMP_FOR_NOTE
	},
	{
		.defaultVal = 64,
		.name = "gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR",
		.val = &CCS.gMINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR
	},
	{
		.defaultVal = 12,
		.name = "gNOTE_FINAL_AMP",
		.val = &CCS.gNOTE_FINAL_AMP
	},
	{
		.defaultVal = 15,
		.name = "gNERF_NOTE_PORP",
		.val = &CCS.gNERF_NOTE_PORP
	},
	{
		.defaultVal = NUM_LIN_LEDS,
		.name = "gUSE_NUM_LIN_LEDS",
		.val = &CCS.gUSE_NUM_LIN_LEDS
	},
	{
		.defaultVal = 1,
		.name = "gCOLORCHORD_ACTIVE",
		.val = &CCS.gCOLORCHORD_ACTIVE
	},
	{
		.defaultVal = 0,
		.name = "gCOLORCHORD_OUTPUT_DRIVER",
		.val = &CCS.gCOLORCHORD_OUTPUT_DRIVER
	},
	{
		.defaultVal = 16,
		.name = "gINITIAL_AMP",
		.val = &CCS.gINITIAL_AMP
	},
	{
		.defaultVal = 0,
		.name = "gLED_DRIVER_MODE",
		.val = &CCS.gLED_DRIVER_MODE
	},
	{
		.defaultVal = 0,
		.name = 0,
		.val = 0
	}
};

/**
 * Initialization for settings, called by user_init().
 * Reads settings from SPI flash, uses defaults if a key value isn't present
 */
void ICACHE_FLASH_ATTR CustomStart( )
{
	int i;
	spi_flash_read( CCCONFIG_ADDRESS, (uint32*)&settings, sizeof( settings ) );
	if( settings.SaveLoadKey == SAVE_LOAD_KEY )
	{
		for( i = 0; i < CONFIGURABLES; i++ )
		{
			if( gConfigs[i].val )
			{
				*gConfigs[i].val = settings.configs[i];
			}
		}
	}
	else
	{
		for( i = 0; i < CONFIGURABLES; i++ )
		{
			if( gConfigs[i].val )
			{
				*gConfigs[i].val = gConfigs[i].defaultVal;
			}
		}
	}
}

/**
 * Receives custom UDP commands on BACKEND_PORT. The UDP server is set up by CSInit() via user_init()
 * Custom UDP commands start with the letter 'C' or 'c'
 * Individual commands are documented in README.md
 *
 * @param buffer   The buffer to fill with data to return
 * @param retsize  The length of the buffer to fill with data to return
 * @param pusrdata The received data, starting with 'C' or 'c'
 * @param len      The length of the received data
 * @return The length of the return buffer filled with data
 */
int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;

	// Start with pusrdata[1] because pusrdata[0] is 'C' or 'c'
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
				if( gConfigs[i].val )
					*gConfigs[i].val = gConfigs[i].defaultVal;
			}
			buffend += ets_sprintf( buffend, "CD" );
			return buffend-buffer;
		}

		case 'r': case 'R':
		{
			int i;
			for( i = 0; i < CONFIGURABLES-1; i++ )
			{
				if( gConfigs[i].val )
					*gConfigs[i].val = settings.configs[i];
			}
			buffend += ets_sprintf( buffend, "CSR" );
			return buffend-buffer;
		}

		case 's': case 'S':
		{
			int i;

			for( i = 0; i < CONFIGURABLES-1; i++ )
			{
				if( gConfigs[i].val )
					settings.configs[i] = *gConfigs[i].val;
			}
			settings.SaveLoadKey = SAVE_LOAD_KEY;

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
			while( gConfigs[i].name )
			{
				buffend += ets_sprintf( buffend, "%s=%d\t", gConfigs[i].name, *gConfigs[i].val );
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
				while( gConfigs[i].name )
				{
					if( strcmp( name, gConfigs[i].name ) == 0 )
					{
						*gConfigs[i].val = val;
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

