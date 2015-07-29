//This is very noisy for some reason, don't use for now.

#ifndef _MP45DT02
#define _MP45DT02

#include "ccconfig.h"

//sampling frequency
#define FS DFREQ

#define VOLUME 10

//PDM decimation factor
#define DECIMATION 64

#define MIC_IN_BUF_SIZE ((((FS/1000)*DECIMATION)/8)/2)
#define MIC_OUT_BUF_SIZE (FS/1000)

//i2s clock is clock for mic
//clock for mic is calculated as Fs*decimation_factor
//so we have to divide with 32 (frame_length*num_channels)
//to get i2s sampling freq
#define I2S_FS ((FS*DECIMATION)/(16*2))


void InitMP45DT02();

//called from interrpt
void GotSample( int s );

#endif

