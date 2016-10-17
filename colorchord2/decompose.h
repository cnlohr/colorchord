//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _DECOMPOSE_H
#define _DECOMPOSE_H

#include "notefinder.h"

//Decompose a histogram into a series of normal distributions.
int DecomposeHistogram( float * histogram, int bins, struct NoteDists * out_dists, int max_dists, double default_sigma, int iterations );
float CalcHistAt( float pt, int bins, struct NoteDists * out_dists, int cur_dists );


#define TURBO_DECOMPOSE

#endif

