//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#define PARAM_BUFF 160

enum ParamType
{
	NONE,
	PAFLOAT,
	PAINT,
	PASTRING, //const char *, cannot set.
	PABUFFER,
	NUM_PARAMS,
};

typedef void (*ParamCallbackT)( void * v );

struct ParamCallback
{
	ParamCallbackT t;
	void * v;
	struct ParamCallback * next;
};

struct LinkedParameter
{
	void * ptr;
	struct LinkedParameter * lp;
};

struct Param
{
	char orphan; //If this is set, then this is something that was received from a string, but has no claimed interface.
				 //It will be claimed when RegisterValue is called.  NOTE: When orphan is set, it must be a malloc'd string.
	enum ParamType t;
	int size;
	struct LinkedParameter * lp;

	struct ParamCallback * callback;
};


//This is the preferred method for getting settings, that way changes will be propogated
void RegisterValue( const char * name, enum ParamType, void * ptr, int size );

void DumpParameters();

//Use these only if you really can't register your value.
float GetParameterF( const char * name, float defa );
int GetParameterI( const char * name, int defa );
const char * GetParameterS( const char * name, const char * defa );

//Format: parameter1=value1;parameter2=value2, OR \n instead of; ... \r's treated as whitespace.  Will trip whitespace.
void SetParametersFromString( const char * string );

void AddCallback( const char * name, ParamCallbackT t, void * v );

#define REGISTER_PARAM( parameter_name, type ) \
	void __attribute__((constructor))  REGISTER##parameter_name() { RegisterValue( #parameter_name, type, &parameter_name, sizeof( parameter_name ) ); }


#endif
