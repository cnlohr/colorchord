//Generic C Hash table thing.
//WARNING: There is a known bug somewhere in the remove entry thing.
//I never found it...
//
// Copyright 2015 <>< Charles Lohr, under the NewBSD or MIT/x11 License.

#ifndef _CHASH_H
#define _CHASH_H


struct chashentry
{
	const char * key;
	void * value;
	unsigned long hash;
};

struct chashlist
{
	int length;
	struct chashentry items[0];
};


struct chash
{
	int entryCount;
	int allowMultiple;
	int bucketCountPlace;
	struct chashentry * buckets;
};

struct chash * GenerateHashTable( int allowMultiple );

void ** HashTableInsert( struct chash * hash, const char * key, int dontDupKey );

//returns # of entries removed.
int HashTableRemove( struct chash * hash, const char * key );

//returns 1 if entry was removed, 0 otherwise.
int HashTableRemoveSpecific( struct chash * hash, const char * key, void * value );

void HashDestroy( struct chash * hash, int deleteKeys );

//Gets a pointer to the pointer to day, allowing you to read or modify the entry.
//NOTE: This only returns one of the elements if multiple keys are allowed.
void ** HashUpdateEntry( struct chash * hash, const char * key );

//Returns the entry itself, if none found, returns 0.
void * HashGetEntry( struct chash * hash, const char * key );

//Get list of entries that match a given key.
struct chashlist * HashGetAllEntries( struct chash * hash, const char * key );

//Can tolerate an empty list.
struct chashlist * HashProduceSortedTable( struct chash * hash );


#endif
