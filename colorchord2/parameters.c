//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "parameters.h"
#include "chash.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static struct chash * parameters;

//XXX TODO: Make this thread safe.
static char returnbuffer[32];

static void Init()
{
	if( !parameters )
	{
		parameters = GenerateHashTable( 0 );
	}
}


float GetParameterF( const char * name, float defa )
{
	struct Param * p = (struct Param*)HashGetEntry( parameters, name );	

	if( p )
	{
		switch( p->t )
		{
		case PAFLOAT: return *((float*)p->lp->ptr);
		case PAINT:   return *((int*)p->lp->ptr);
		case PASTRING:
		case PABUFFER: if( p->lp->ptr ) return atof( p->lp->ptr );
		default: break;
		}
	}
	printf( "U: %s = %f\n", name, defa );

	return defa;
}

int GetParameterI( const char * name, int defa )
{
	struct Param * p = (struct Param*)HashGetEntry( parameters, name );	

	if( p )
	{
		switch( p->t )
		{
		case PAFLOAT: return *((float*)p->lp->ptr);
		case PAINT:   return *((int*)p->lp->ptr);
		case PASTRING:
		case PABUFFER: if( p->lp->ptr ) return atoi( p->lp->ptr );
		default: break;
		}
	}

	printf( "U: %s = %d\n", name, defa );

	return defa;
}

const char * GetParameterS( const char * name, const char * defa )
{
	struct Param * p = (struct Param*)HashGetEntry( parameters, name );	

	if( p )
	{
		switch( p->t )
		{
		case PAFLOAT: snprintf( returnbuffer, sizeof( returnbuffer ), "%0.4f", *((float*)p->lp->ptr) ); return returnbuffer;
		case PAINT:   snprintf( returnbuffer, sizeof( returnbuffer ), "%d", *((int*)p->lp->ptr) );      return returnbuffer;
		case PASTRING:
		case PABUFFER: return p->lp->ptr;
		default: break;
		}
	}

	printf( "U: %s = %s\n", name, defa );

	return defa;
}


static int SetParameter( struct Param * p, const char * str )
{
	struct LinkedParameter * lp;
	lp = p->lp;

	switch( p->t )
	{
	case PAFLOAT:
		while( lp )
		{
			*((float*)lp->ptr) = atof( str );
			lp = lp->lp;
		}
		break;
	case PAINT:
		while( lp )
		{
			*((int*)lp->ptr) = atoi( str );
			lp = lp->lp;
		}
		break;
	case PABUFFER:
		while( lp )
		{
			strncpy( (char*)lp->ptr, str, p->size );
			if( p->size > 0 )
				((char*)lp->ptr)[p->size-1]= '\0';
			lp = lp->lp;
		}
		break;
	case PASTRING:
		while( lp )
		{
			free( lp->ptr );
			lp->ptr = strdup( str );
			lp = lp->lp;
		}
		break;
	default:
		return -1;
	}

	struct ParamCallback * cb = p->callback;
	while( cb )
	{
		cb->t( cb->v );
		cb = cb->next;
	}

	return 0;
}

void RegisterValue( const char * name, enum ParamType t, void * ptr, int size )
{
	Init();

	struct Param * p = (struct Param*)HashGetEntry( parameters, name );

	if( p )
	{
		//Entry already exists.
		if( p->orphan )
		{
			if( p->t != PASTRING )
			{
				fprintf( stderr, "Warning: Orphan parameter %s was not a PSTRING.\n", name );
			}
			char * orig = p->lp->ptr;
			p->lp->ptr = ptr;
			p->t = t;
			p->size = size;
			p->orphan = 0;
			int r = SetParameter( p, orig );
			free( orig );
			if( r )
			{
				fprintf( stderr, "Warning: Problem when setting Orphan parameter %s\n", name );
			}
		}
		else
		{
			struct LinkedParameter * lp = p->lp;
			if( size != p->size )
			{
				fprintf( stderr, "Size mismatch: Parameter %s.\n", name );
			}
			else
			{
				p->lp = malloc( sizeof( struct LinkedParameter ) );
				p->lp->lp = lp;
				p->lp->ptr = ptr;
				memcpy( p->lp->ptr, p->lp->lp->ptr, size );
			}
		}
	}
	else
	{
		struct Param ** n = (struct Param**)HashTableInsert( parameters, name, 1 );
		*n = malloc( sizeof( struct Param ) );
		(*n)->t = t;
		(*n)->lp = malloc( sizeof( struct LinkedParameter ) );
		(*n)->lp->lp = 0;
		(*n)->lp->ptr = ptr;
		(*n)->orphan = 0;
		(*n)->size = size;
		(*n)->callback = 0;
	}
}

void SetParametersFromString( const char * string )
{
	char name[PARAM_BUFF];
	char value[PARAM_BUFF];
	char c;

	int namepos = -1; //If -1, not yet found.
	int lastnamenowhite = 0;
	int valpos = -1;
	int lastvaluenowhite = 0;
	char in_value = 0;
	char in_comment = 0;

	while( 1 )
	{
		c = *(string++);
		char is_whitespace = ( c == ' ' || c == '\t' || c == '\r' );
		char is_break = ( c == '\n' || c == ';' || c == 0 );
		char is_comment = ( c == '#' );
		char is_equal = ( c == '=' );

		if( is_comment )
		{
			in_comment = 1;
		}

		if( in_comment )
		{
			if( !is_break )
				continue;
		}

		if( is_break )
		{
			if( namepos < 0 || valpos < 0 ) 
			{
				//Can't do anything with this line.
			}
			else
			{
				name[lastnamenowhite] = 0;
				value[lastvaluenowhite] = 0;

				struct Param * p = (struct Param*)HashGetEntry( parameters, name );
				if( p )
				{
					printf( "Set: %s %s\n", name, value );
					SetParameter( p, value );
				}
				else
				{
					//p is an orphan.
//					printf( "Orp: %s %s\n", name, value );
					struct Param ** n = (struct Param **)HashTableInsert( parameters, name, 0 );
					*n = malloc( sizeof ( struct Param ) );
					(*n)->orphan = 1;
					(*n)->t = PASTRING;
					(*n)->lp = malloc( sizeof( struct LinkedParameter ) );
					(*n)->lp->lp = 0;
					(*n)->lp->ptr = strdup( value );
					(*n)->size = strlen( value ) + 1;
					(*n)->callback = 0;
				}
			}

			namepos = -1;
			lastnamenowhite = 0;
			valpos = -1;
			lastvaluenowhite = 0;
			in_value = 0;
			in_comment = 0;

			if( c )
				continue;
			else
				break;
		}

		if( is_equal )
		{
			in_value = 1;
			continue;
		}

		if( !in_value )
		{
			if( namepos == -1 )
			{
				if( !is_whitespace )
					namepos = 0;
			}

			if( namepos >= 0 && namepos < PARAM_BUFF )
			{
				name[namepos++] = c;
				if( !is_whitespace )
					lastnamenowhite = namepos;
			}
		}
		else
		{
			if( valpos == -1 )
			{
				if( !is_whitespace )
					valpos = 0;
			}

			if( valpos >= 0 && valpos < PARAM_BUFF )
			{
				value[valpos++] = c;
				if( !is_whitespace )
					lastvaluenowhite = valpos;
			}
		}
	}
}

void AddCallback( const char * name, ParamCallbackT t, void * v )
{
	struct Param * p = (struct Param*)HashGetEntry( parameters, name );	
	if( p )
	{
		struct ParamCallback ** last = &p->callback;
		struct ParamCallback * cb = p->callback;
		while( cb )
		{
			last = &cb->next;
			cb = cb->next;
		}
		cb = *last = malloc( sizeof( struct ParamCallback ) );
		cb->t = t;
		cb->v = v;
		cb->next = 0;
	}
	else
	{
		fprintf( stderr, "Warning: cannot add callback to %s\n.", name );
	}
}


void DumpParameters()
{
	int i;
	struct chashlist * l = HashProduceSortedTable( parameters );

	for( i = 0; i < l->length; i++ )
	{
		struct chashentry * e = &l->items[i];
		printf( "%s = %s\n", e->key, GetParameterS( e->key, "" ) );
	}
	printf( "\n" );

	free( l );
}


