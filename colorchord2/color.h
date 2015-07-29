//Copyright (Public Domain) 2015 <>< Charles Lohr, under the NewBSD License.
//This file may be used in whole or part in any way for any purpose by anyone
//without restriction.

#ifndef _COLOR_H
#define _COLOR_H

#include <stdint.h>

//note = 0..1 (Root is YELLOW); Val = 0..1
//NOTE: CC is _NOT_ HUE!!!
uint32_t CCtoHEX( float note, float sat, float value );
uint32_t HSVtoHEX( float hue, float sat, float value );

#endif

