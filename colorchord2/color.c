//Copyright (Public Domain) 2015 <>< Charles Lohr, under the NewBSD License.
//This file may be used in whole or part in any way for any purpose by anyone
//without restriction.

#include "color.h"
#include <math.h>

uint32_t CCtoHEX( float note, float sat, float value )
{
	float hue = 0.0;
	note = fmodf( note, 1.0 );
	note *= 12.0;
	if( note < 4.0 )
	{
		//Needs to be YELLOW->RED
		hue = (4.0 - note) / 24.0;
	}
	else if( note < 8.0 )
	{
		//            [4]  [8]
		//Needs to be RED->BLUE
		hue = ( 4.0 - note ) / 12.0;
	}
	else
	{
		//             [8] [12]
		//Needs to be BLUE->YELLOW
		hue = ( 12.0 - note ) / 8.0 + 1.0/6.0;
	}
	return HSVtoHEX( hue, sat, value );
}


//0/6: RED
//1/6: YELLOW
//2/6: GREEN
//3/6: CYAN
//4/6: BLUE
//5/6: PURPLE
//6/6: RED
uint32_t HSVtoHEX( float hue, float sat, float value )
{

	float pr = 0.0;
	float pg = 0.0;
	float pb = 0.0;

	short ora = 0.0;
	short og = 0.0;
	short ob = 0.0;

	float ro = fmod( hue * 6.0, 6.0 );

	float avg = 0.0;

	ro = fmod( ro + 6.0 + 1.0, 6.0 ); //Hue was 60* off...

	if( ro < 1.0 ) //yellow->red
	{
		pr = 1.0;
		pg = 1.0 - ro;
	} else if( ro < 2.0 )
	{
		pr = 1.0;
		pb = ro - 1.0;
	} else if( ro < 3.0 )
	{
		pr = 3.0 - ro;
		pb = 1.0;
	} else if( ro < 4.0 )
	{
		pb = 1.0;
		pg = ro - 3.0;
	} else if( ro < 5.0 )
	{
		pb = 5.0 - ro;
		pg = 1.0;
	} else
	{
		pg = 1.0;
		pr = ro - 5.0;
	}

	//Actually, above math is backwards, oops!
	pr *= value;
	pg *= value;
	pb *= value;

	avg += pr;
	avg += pg;
	avg += pb;

	pr = pr * sat + avg * (1.0-sat);
	pg = pg * sat + avg * (1.0-sat);
	pb = pb * sat + avg * (1.0-sat);

	ora = pr * 255;
	og = pb * 255;
	ob = pg * 255;

	if( ora < 0 ) ora = 0;
	if( ora > 255 ) ora = 255;
	if( og < 0 ) og = 0;
	if( og > 255 ) og = 255;
	if( ob < 0 ) ob = 0;
	if( ob > 255 ) ob = 255;

    // Pack bits in RGBA format
    return (ora << 24) | (og << 16) | (ob << 8) | 0xFF;
}
