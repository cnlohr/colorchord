//Copyright 2015 <>< Charles Lohr under the ColorChord License.

//Output drivers, used for outputting lights, etc... However, this technique
//may be used for unrelated-to-output plugins.

#ifndef _OUTDRIVERS_H
#define _OUTDRIVERS_H

#include "os_generic.h"

#include "notefinder.h"
#include "parameters.h"


struct NoteFinder;

#define MAX_LEDS 32678

extern int force_white;
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
	void (*deconstructDriver)(void * id);
	void (*Params)(void * id);
};

extern struct OutDriverListElem ODList[MAX_OUT_DRIVERS];
extern const char OutDriverParameters[MAX_OUT_DRIVER_STRING];

void deconstructDriver(void * id);

//Pass setup "name=[driver]"
struct DriverInstances * SetupOutDriver( );
void RegOutDriver( const char * ron, struct DriverInstances * (*Init)( ) );

#define REGISTER_OUT_DRIVER( name ) \
	void __attribute__((constructor)) REGISTER##name() { RegOutDriver( #name, name ); }

#endif
