/*
 *	PROGRAM:		Schema Converter
 *	MODULE:			Hash.h
 *	DESCRIPTION:	Hash table stuff
 *
 * copyright (c) 1997 by James A. Starkey for IBPhoenix.
 */

#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Hash.h"

Hash::Hash (int size)
{
/**************************************
 *
 *		H a s h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

table = new HashEntry* [size];
tableSize = size;
memset (table, 0, sizeof (HashEntry*) * size);
}

Hash::~Hash ()
{
/**************************************
 *
 *		~ H a s h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

for (int n = 0; n < tableSize; ++n)
    {
	HashEntry *entry;
	while (entry = table [n])
		{
		table [n] = entry->next;
		delete entry;
		}
	}

delete table;
}

Hash::HashEntry::~HashEntry ()
{
/**************************************
 *
 *		~ H a s h E n t r y
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

free (string);
}

void Hash::deleteAll ()
{
/**************************************
 *
 *		d e l e t e A l l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

for (int n = 0; n < tableSize; ++n)
    {
	HashEntry *entry;
	while (entry = table [n])
		{
		table [n] = entry->next;
		delete entry->object;
		delete entry;
		}
	}
}

void Hash::finiWalk (void *ptr)
{
/**************************************
 *
 *		f i n i W a l k
 *
 **************************************
 *
 * Functional description
 *		Pick up next item.
 *
 **************************************/

delete ((HashWalk*) ptr);
}

void *Hash::getObject (void *ptr)
{
/**************************************
 *
 *		g e t O b j e c t
 *
 **************************************
 *
 * Functional description
 *		Return string of current position in table walk.
 *
 **************************************/
HashWalk *marker = (HashWalk*) ptr;

return marker->node->object;
}

const char *Hash::getString (void *ptr)
{
/**************************************
 *
 *		g e t S t r i n g
 *
 **************************************
 *
 * Functional description
 *		Return string of current position in table walk.
 *
 **************************************/
HashWalk *marker = (HashWalk*) ptr;

return marker->node->string;
}

int Hash::hash (const char *string)
{
/**************************************
 *
 *		h a s h
 *
 **************************************
 *
 * Functional description
 *		Hash function.
 *
 **************************************/
int	value = 0, c;

while (c = (unsigned) *string++)
   value = value * 11 + c;

if (value < 0)
    value = -value;

return value % tableSize;
}

void *Hash::initWalk ()
{
/**************************************
 *
 *		i n i t W a l k
 *
 **************************************
 *
 * Functional description
 *		Prepare for walk thru hash table.
 *
 **************************************/
HashWalk *marker = new HashWalk;
marker->node = NULL;
marker->slot = -1;

return marker;
}

void Hash::insert (const char *string, void *object)
{
/**************************************
 *
 *		i n s e r t
 *
 **************************************
 *
 * Functional description
 *		Insert object into table.
 *
 **************************************/
int			slot = hash (string);
HashEntry	*entry = new HashEntry;

entry->string = strdup (string);
entry->object = object;
entry->next = table [slot];
table [slot] = entry;
}

void *Hash::lookup (const char *string)
{
/**************************************
 *
 *		l o o k u p
 *
 **************************************
 *
 * Functional description
 *		Insert object into table.
 *
 **************************************/
int		slot = hash (string);

for (HashEntry *entry = table [slot]; entry; entry = entry->next)
    if (!strcmp (entry->string, string))
		return entry->object;

return NULL;
}

int Hash::next (void *ptr)
{
/**************************************
 *
 *		n e x t
 *
 **************************************
 *
 * Functional description
 *		Pick up next item.
 *
 **************************************/
HashWalk *marker = (HashWalk*) ptr;

if (marker->node && (marker->node = marker->node->next))
	return TRUE;

while (++marker->slot < tableSize)
    {
	if (marker->node = table [marker->slot])
		return TRUE;
	}

return FALSE;
}

bool Hash::isFirst(const char * string)
{
/**************************************
 *
 *		i s F i r s t
 *
 **************************************
 *
 * Functional description
 *		If this is the first reference to string in hash table, define
 *		the string and return true.
 *
 **************************************/

if (lookup (string))
    return false;

insert (string, this);

return true;
}
