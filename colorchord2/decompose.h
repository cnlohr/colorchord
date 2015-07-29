//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _DECOMPOSE_H
#define _DECOMPOSE_H

//Decompose a histogram into a series of normal distributions.
int DecomposeHistogram( float * histogram, int bins, float * out_means, float * out_amps, float * out_sigmas, int max_dists, double default_sigma, int iterations );
float CalcHistAt( float pt, int bins, float * out_means, float * out_amps, float * out_sigmas, int cur_dists );


#define TURBO_DECOMPOSE

#endif

