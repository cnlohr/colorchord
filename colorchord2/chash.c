//Generic C Hash table thing.
//WARNING: There is a known bug somewhere in the remove entry thing.
//I never found it...
//
// Copyright 2015 <>< Charles Lohr, under the NewBSD or MIT/x11 License.

#include "chash.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#define I_AM_LITTLE (((union { unsigned x; unsigned char c; }){1}).c)

static unsigned long GetStrHash( const char * c )
{
	int place = 0;
	unsigned int hashSoFar = 0;

	if( !c ) return 0;

	do
	{
		unsigned char cc = (*c);
		unsigned long this = cc;
		this <<= (((place++)&3)<<3);
		hashSoFar += this;
	} while( *(c++) );

	return hashSoFar;
}


static unsigned long GeneralUsePrimes[] = { 7, 13, 37, 73, 131, 229, 337, 821,
	2477, 4594, 8941, 14797, 24953, 39041, 60811, 104729, 151909,
	259339, 435637,	699817, 988829, 1299827 };

struct chash * GenerateHashTable( int allowMultiple )
{
	int bucketplace = 0;
	int count = GeneralUsePrimes[bucketplace];
	struct chash * ret = malloc( sizeof( struct chash ) );
	ret->entryCount = 0;
	ret->allowMultiple = allowMultiple;
	ret->bucketCountPlace = bucketplace;
	ret->buckets = malloc( sizeof( struct chashentry ) * count );
	memset( ret->buckets, 0, sizeof( struct chashentry ) * count );
	return ret;
}

void ** HashTableInsert( struct chash * hash, const char * key, int dontDupKey )
{
	int buckets = GeneralUsePrimes[hash->bucketCountPlace];
	int thisHash = GetStrHash( key ) % buckets;
	int multi = hash->allowMultiple;
	struct chashentry * thisEntry;
	struct chashentry * overextndEntry = &hash->buckets[buckets];
	int i;

	//Cannot insert null!
	if( !key )
	{
		return 0;
	}

	//Half full?
	if( hash->entryCount > (buckets>>1) )
	{
		//Resize!
		int newbucketcount = GeneralUsePrimes[hash->bucketCountPlace + 1];
		struct chashentry * newbuckets = malloc( sizeof( struct chashentry ) * newbucketcount );
		struct chashentry * oldbuckets = hash->buckets;
		overextndEntry = &newbuckets[newbucketcount];

		memset( newbuckets, 0, sizeof( struct chashentry ) * newbucketcount );

		for( i = 0; i < buckets; i++ )
		{
			struct chashentry * oe = &oldbuckets[i];
			int newhash;
			struct chashentry * ne;

			//Empty? Continue!
			if( !oe->key ) continue;

			newhash = GetStrHash( oe->key ) % newbucketcount;
			ne = &newbuckets[newhash];
			while( ne->key )
			{
				ne++;
				if( ne == overextndEntry ) ne = &newbuckets[0];
			}

			ne->key = oe->key;
			ne->value = oe->value;
		}

		//Replacing done, now update all.

		hash->buckets = newbuckets;
		free( oldbuckets );
		hash->bucketCountPlace++;
		buckets = newbucketcount;
		thisHash = GetStrHash( key ) % buckets;
	}

	thisEntry = &hash->buckets[thisHash];

	while( thisEntry->key )
	{

		//If we aren't allowing multiple entries, say so!
		if( !multi && strcmp( thisEntry->key, key ) == 0 )
		{
			return &thisEntry->value;
		}

		thisEntry++;
		if( thisEntry == overextndEntry ) thisEntry = &hash->buckets[0];
	}

	thisEntry->value = 0;
	if( dontDupKey )
	{
		thisEntry->key = key;
	}
	else
	{
		thisEntry->key = strdup( key );
	}
	thisEntry->hash = thisHash;

	hash->entryCount++;

	return &thisEntry->value;
}

//Re-process a range; populated is the number of elements we expect to run into.
//If we removed some, it will be less length by that much.
//NOTE: This function doesn't range check diddily.
static void RedoHashRange( struct chash * hash, int start, int length, int populated )
{
	int i;
	int buckets = GeneralUsePrimes[hash->bucketCountPlace];
	struct chashentry * thisEntry = &hash->buckets[start];
	struct chashentry * overEntry = &hash->buckets[buckets];
	struct chashentry * copyEntry;

	struct chashlist * resort = alloca( sizeof( struct chashlist ) + sizeof( struct chashentry ) * (populated) );
	resort->length = populated;
	copyEntry = &resort->items[0];

	for( i = 0; i < length; i++ )
	{
		if( thisEntry->key )
		{
			copyEntry->key = thisEntry->key;
			copyEntry->value = thisEntry->value;
			copyEntry->hash = thisEntry->hash;
			copyEntry++;
			thisEntry->key = 0;
		}

		thisEntry++;
		if( thisEntry == overEntry ) thisEntry = &hash->buckets[0];
	}

	hash->entryCount -= populated;

	copyEntry = &resort->items[0];

	for( i = 0; i < populated; i++ )
	{
		*HashTableInsert( hash, copyEntry->key, 1 ) = copyEntry->value;
		copyEntry++;
	}

}



int HashTableRemove( struct chash * hash, const char * key )
{
	int buckets = GeneralUsePrimes[hash->bucketCountPlace];
	int thisHash = GetStrHash( key ) % buckets;
	int multi = hash->allowMultiple;
	struct chashentry * thisEntry = &hash->buckets[thisHash];
	struct chashentry * overEntry = &hash->buckets[buckets];
	int startentry = thisHash;
	int entriesSearched = 0;
	int removedEntries = 0;

	//Search the list for matches, search until we have an empty spot.
	while( thisEntry->key )
	{
		if( strcmp( thisEntry->key, key ) == 0 )
		{
			free( (char*)thisEntry->key );
			thisEntry->key = 0;
			removedEntries++;
			if( !multi )
			{
				break;
			}
		}

		thisEntry++;
		if( thisEntry == overEntry ) thisEntry = &hash->buckets[0];
		entriesSearched++;
	}

	if( removedEntries == 0 ) 
		return 0;

	hash->entryCount -= removedEntries;

	RedoHashRange( hash, startentry, entriesSearched, entriesSearched-removedEntries );

	return removedEntries;
}

int HashTableRemoveSpecific( struct chash * hash, const char * key, void * value )
{
	int buckets = GeneralUsePrimes[hash->bucketCountPlace];
	int thisHash = GetStrHash( key ) % buckets;
	int multi = hash->allowMultiple;
	struct chashentry * thisEntry = &hash->buckets[thisHash];
	struct chashentry * overEntry = &hash->buckets[buckets];
	int startentry = thisHash;
	int entriesSearched = 0;
	int removedEntries = 0;

	//Search the list for matches, search until we have an empty spot.
	while( thisEntry->key )
	{
		if( strcmp( thisEntry->key, key ) == 0 && thisEntry->value == value )
		{
			free( (char*)thisEntry->key );
			thisEntry->key = 0;
			removedEntries++;
			if( !multi )
			{
				break;
			}
		}

		thisEntry++;
		if( thisEntry == overEntry ) thisEntry = &hash->buckets[0];
		entriesSearched++;
	}

	if( removedEntries == 0 ) 
		return 0;

	hash->entryCount -= removedEntries;

	RedoHashRange( hash, startentry, entriesSearched, entriesSearched-removedEntries );

	return removedEntries;
}


void HashDestroy( struct chash * hash, int deleteKeys )
{
	int buckets = GeneralUsePrimes[hash->bucketCountPlace];
	int i;

	if( deleteKeys )
	{
		struct chashentry * start = &hash->buckets[0];
		for( i = 0; i < buckets; i++ )
		{
			free( (char*) (start++)->key );
		}
	}

	free( hash->buckets );

	free( hash );
}

void ** HashUpdateEntry( struct chash * hash, const char * key )
{
	int buckets = GeneralUsePrimes[hash->bucketCountPlace];
	int thisHash = GetStrHash( key ) % buckets;
	struct chashentry * thisEntry = &hash->buckets[thisHash];
	struct chashentry * overEntry = &hash->buckets[buckets];

	while( thisEntry->key )
	{
		//We allow null keys.
		if( !key && !thisEntry->key )
		{
			return &thisEntry->value;
		}

		if( key && thisEntry->key && strcmp( thisEntry->key, key ) == 0 )
		{
			return &thisEntry->value;
		}

		thisEntry++;
		if( thisEntry == overEntry ) thisEntry = &hash->buckets[0];
	}

	return 0;
}

void * HashGetEntry( struct chash * hash, const char * key )
{
	void ** v = HashUpdateEntry( hash, key );
	if( v )
		return *v;
	return 0;
}


struct chashlist * HashGetAllEntries( struct chash * hash, const char * key )
{
	int retreser = 3;
	int buckets = GeneralUsePrimes[hash->bucketCountPlace];
	int thisHash = GetStrHash( key ) % buckets;
	int multi = hash->allowMultiple;
	struct chashentry * thisEntry = &hash->buckets[thisHash];
	struct chashentry * overEntry = &hash->buckets[buckets];
	struct chashlist * ret = malloc( sizeof( struct chashlist ) + sizeof( struct chashentry ) * retreser );

	ret->length = 0;

	while( thisEntry->key )
	{
		if( strcmp( thisEntry->key, key ) == 0 )
		{
			ret->items[ret->length].key = thisEntry->key;
			ret->items[ret->length].value = thisEntry->value;
			ret->items[ret->length].hash = thisEntry->hash;
			ret->length++;

			if( !multi )
			{
				return ret;
			}

			if( ret->length == retreser )
			{
				retreser *= 2;
				ret = realloc( ret, sizeof( struct chashlist ) + sizeof( struct chashentry ) * retreser );
			}
		}

		thisEntry++;
		if( thisEntry == overEntry ) thisEntry = &hash->buckets[0];
	}

	if( ret->length == 0 )
	{
		free( ret );
		return 0;
	}

	return ret;
}


static void merge( struct chashentry ** tmpitems, int start, int middle, int end )
{
	int aptr = start;
	int bptr = middle;
	int i = 0;
	int count = end-start+1;
	struct chashentry * ret[count];

	if( count == 1 ) return;

	while( i < count )
	{
		if( bptr == end+1 || ( aptr != middle && strcmp( tmpitems[aptr]->key, tmpitems[bptr]->key ) < 0 ) )
		{
			ret[i++] = tmpitems[aptr];
			aptr++;
		}
		else
		{
			ret[i++] = tmpitems[bptr];
			bptr++;
		}
	}

	for( i = 0; i < count; i++ )
	{
		tmpitems[i+start] = ret[i];
	}
}


static void merge_sort( struct chashentry ** tmpitems, int start, int end )
{
	int middle = (end+start)/2;

	if( end <= start )
		return;

	merge_sort( tmpitems, start, middle );
	merge_sort( tmpitems, middle+1, end );
	merge( tmpitems, start, middle+1, end );
}


struct chashlist * HashProduceSortedTable( struct chash * hash )
{
	struct chashentry ** tmpitems = alloca( sizeof( struct chashlist * ) * hash->entryCount ); //temp list of pointers.
	struct chashentry * thisEntry = &hash->buckets[0];
	struct chashlist * ret = malloc( sizeof( struct chashlist ) + sizeof( struct chashentry ) * hash->entryCount );
	int i;
	int index = 0;
	int buckets = GeneralUsePrimes[hash->bucketCountPlace];

	ret->length = hash->entryCount;

	if( hash->entryCount == 0 )
	{
		return ret;
	}

	//Move the table into tmp.
	for( i = 0; i < buckets; i++, thisEntry++ )
	{
		if( !thisEntry->key ) continue;

		tmpitems[index++] = thisEntry;
	}

	//Sort tmpitems
	merge_sort( tmpitems, 0, index-1 );

	for( i = 0; i < hash->entryCount; i++ )
	{
		ret->items[i].key = (tmpitems[i])->key;
		ret->items[i].value = (tmpitems[i])->value;
		ret->items[i].hash = (tmpitems[i])->hash;
	}

	return ret;
}


