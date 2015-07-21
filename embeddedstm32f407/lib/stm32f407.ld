/*. Entry Point *./
ENTRY( Reset_Handler )

/* Highest address of the user mode stack .*/

_estack = 0x2000a000;    /* end of 40K RAM */

/* Generate a link error if heap and s tack dont fit int o RAM */

_Min_Heap_Size = 0;      /* required amount of heap .*/
_Min_Stack_Size = 0x200; /* required amount of stack .*/

MEMORY
{
	FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
	RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
	CCMRAM (xrw)     : ORIGIN = 0x10000000, LENGTH = 64K
	MEMORY_B1 (rx)  : ORIGIN = 0x60000000, LENGTH = 0K
	RAM_BACKUP (rw) : ORIGIN = 0x40024000, LENGTH = 4K
}

SECTIONS
{

	/* The startup code goes first into FLASH */
	.isr_vector :
	{
		. = ALIGN(4);
		KEEP(*(.isr_vector)) /* Startup code */
		. = ALIGN(4);
	} >FLASH

	/* The program code and other data goes into FLASH */
	.text :
	{
		. = ALIGN(4);
		*(.text)           /* .text sections (code) */
		*(.text*)          /* .text* sections (code) */
		*(.rodata)         /* .rodata sections (constants, strings, etc.) */
		*(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
		*(.glue_7)         /* glue arm to thumb code */
		*(.glue_7t)        /* glue thumb to arm code */
		*(.eh_frame)

		KEEP (*(.init))
		KEEP (*(.fini))

		. = ALIGN(4);
		_etext = .;        /* define a global symbols at end of code */
	} >FLASH


	.ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH

	.ARM : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >FLASH

	.preinit_array     :
	{
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array*))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	} >FLASH

	.init_array :
	{
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
	} >FLASH

	.fini_array :
	{
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array*))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} >FLASH

	/* used by the startup to initialize data */
	_sidata = LOADADDR(.data);

	/* Initialized data sections goes into RAM, load LMA copy after code */
	.data : 
	{
		. = ALIGN(4);
		_sdata = .;
		*(.data)
		*(.data*)

		. = ALIGN(4);
		_edata = .;
	} >RAM AT> FLASH

	.ccmram :
	{
		. = ALIGN(4);
		_sccmram = .; 
		*(.ccmram)
		*(.ccmram*)

		. = ALIGN(4);
		_eccmram = .; 
	} >CCMRAM AT> FLASH

	/* Uninitialized data section */
	. = ALIGN(4);
	.bss :
	{
		_sbss = .;
		__bss_start__ = _sbss;
		*(.bss)
		*(.bss*)
		*(COMMON)

		. = ALIGN(4);
		_ebss = .;
		__bss_end__ = _ebss;
	} >RAM
}


