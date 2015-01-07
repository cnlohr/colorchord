#ifndef _PARAMETERS_H
#define _PARAMETERS_H

#define PARAM_BUFF 128

enum ParamType
{
	NONE,
	PFLOAT,
	PINT,
	PSTRING, //const char *, cannot set.
	PBUFFER,
	NUM_PARAMS,
};

typedef void (*ParamCallbackT)( void * v );

struct ParamCallback
{
	ParamCallbackT t;
	void * v;
	struct ParamCallback * next;
};

struct Param
{
	char orphan; //If this is set, then this is something that was received from a string, but has no claimed interface.
				 //It will be claimed when RegisterValue is called.  NOTE: When orphan is set, it must be a malloc'd string.
	enum ParamType t;
	void * ptr;
	int size;

	struct ParamCallback * callback;
};

void RegisterValue( const char * name, enum ParamType, void * ptr, int size );
void DumpParameters();
float GetParameterF( const char * name, float defa );
int GetParameterI( const char * name, int defa );
const char * GetParameterS( const char * name, const char * defa );

//Format: parameter1=value1;parameter2=value2, OR \n instead of; ... \r's treated as whitespace.  Will trip whitespace.
void SetParametersFromString( const char * string );

void AddCallback( const char * name, ParamCallbackT t, void * v );

#define REGISTER_PARAM( parameter_name, type ) \
	void Register##parameter_name() __attribute__((constructor(101))); \
	void Register##parameter_name() { RegisterValue( #parameter_name, type, &parameter_name, sizeof( parameter_name ) ); }


#endif
