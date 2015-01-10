#ifndef _OUTDRIVERS_H
#define _OUTDRIVERS_H

#include "os_generic.h"

struct NoteFinder;

#define MAX_LEDS 32678


extern unsigned char	OutLEDs[MAX_LEDS*3];
extern int				UsedLEDs;


#define MAX_OUT_DRIVERS 64
#define MAX_OUT_DRIVER_STRING 1024

struct OutDriverListElem
{
	const char * Name;
	struct DriverInstances * (*Init)();
};

struct DriverInstances
{
	void * id;
	void (*Func)(void * id, struct NoteFinder* nf );
	void (*Params)(void * id);
};

extern struct OutDriverListElem ODList[MAX_OUT_DRIVERS];
extern const char OutDriverParameters[MAX_OUT_DRIVER_STRING];

//Pass setup "name=[driver]"
struct DriverInstances * SetupOutDriver( );
void RegOutDriver( const char * ron, struct DriverInstances * (*Init)( ) );

#define REGISTER_OUT_DRIVER( name ) \
	EXECUTE_AT_BOOT( r##name, RegOutDriver( #name, name ) );

#endif
