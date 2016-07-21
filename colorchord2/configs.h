#ifndef _CONFIGS_H
#define _CONFIGS_H

#define NRDEFFILES 10

extern int gargc;
extern char ** gargv;


void LoadFile( const char * filename );
void SetEnvValues( int force );
void ProcessArgs();
void SetupConfigs();

#endif

