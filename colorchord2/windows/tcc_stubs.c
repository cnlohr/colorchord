#include <string.h>
#include <_mingw.h>
#include <tcclib.h>
#include "symbol_enumerator.h"

_CRTIMP int __cdecl _vscprintf(const char *_Format,va_list _ArgList);

#define REMATH(x)   double __cdecl x( double f ); float x##f(float v) { return x(v); }
#define REMATH2(x)  double __cdecl x( double f, double g ); float x##f(float v, float w) { return x(v,w); }

int SymnumCheck( const char * path, const char * name, void * location, long size )
{
	if( strncmp( name, "REGISTER", 8 ) == 0 )
	{
		typedef void (*sf)();
		sf fn = (sf)location;
		fn();
	}
	return 0;
}



void ManuallyRegisterDevices()
{
	EnumerateSymbols( SymnumCheck );
}

REMATH( acos );
REMATH( cos );
REMATH( sin );
REMATH( sqrt );
REMATH( asin );
REMATH( exp );
REMATH2( fmod );
REMATH2( pow );

double __cdecl strtod (const char* str, char** endptr);
float strtof( const char* str, char** endptr)
{
	return strtod( str, endptr );
}

double __cdecl atan2(double a, double b);
float atan2f(float a, float b)
{
	return atan2( a, b );
}

//From http://stackoverflow.com/questions/40159892/using-asprintf-on-windows
int __cdecl vsprintf_s(  
   char *buffer,  
   size_t numberOfElements,  
   const char *format,  
   va_list argptr   
);   

int asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vasprintf(strp, fmt, ap);
    va_end(ap);
    return r;
}

int vasprintf(char **strp, const char *fmt, va_list ap) {
    // _vscprintf tells you how big the buffer needs to be
    int len = _vscprintf(fmt, ap);
    if (len == -1) {
        return -1;
    }
    size_t size = (size_t)len + 1;
    char *str = (char*)malloc(size);
    if (!str) {
        return -1;
    }
    // _vsprintf_s is the "secure" version of vsprintf
    int r = vsprintf_s(str, len + 1, fmt, ap);
    if (r == -1) {
        free(str);
        return -1;
    }
    *strp = str;
    return r;
}