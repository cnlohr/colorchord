#ifndef _SORT_H
#define _SORT_H

//XXX WARNING: This is a TERRIBLE TERRIBLE O(N^2) SORTING ALGORITHM
//You should probably fix this!!!

//Sort the indices of an array of floating point numbers
//Highest->Lowest
void SortFloats( int * indexouts, float * array, int count );

//Move the floats around according to the new index.
void RemapFloats( int * indexouts, float * array, int count );

#endif


