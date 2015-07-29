//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _FILTER_H
#define _FILTER_H


//Perform a two-pass filter on the data, circularly.  Once right, then left.
//void FilterFoldedBinsIIRTWOPASS( float * folded, int bins, float strength );

void FilterFoldedBinsBlob( float * folded, int bins, float strength, int iter );


#endif
