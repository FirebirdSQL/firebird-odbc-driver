/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *
 *	Changes
 *
 *	2002-05-21	BinaryBlob.cpp
 *				Change release() to test useCount <=0
 *	
 *	2002-05-20	BinaryBlob.cpp
 *				Contributed by Robert Milharcic
 *				o Start with useCount of 0
 *
 */

// BinaryBlob.cpp: implementation of the BinaryBlob class.
//
//////////////////////////////////////////////////////////////////////


#include "IscDbc.h"
#include "BinaryBlob.h"

#ifdef ENGINE
#include "Database.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


BinaryBlob::BinaryBlob()
{
    useCount = 1;
    offset = 0;
    populated = true;
}

BinaryBlob::BinaryBlob(int minSegmentSize) : Stream (minSegmentSize)
{
    useCount = 1;
    offset = 0;
    populated = true;
}

#ifdef ENGINE
BinaryBlob::BinaryBlob(Database * db, long recNumber, long sectId)
{
    useCount = 1;
    offset = 0;
    populated = false;
    database = db;
    recordNumber = recNumber;
    sectionId = sectId;
}
#endif

BinaryBlob::~BinaryBlob()
{

}

void BinaryBlob::addRef()
{
	++useCount;
}

int BinaryBlob::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

/***
bool BinaryBlob::write(const char * filename)
{
	FILE *file = fopen (filename, "w");

	if (!file)
		return false;

	long offset = 0;
	int length;
	char buffer [1024];

	while (length = Stream::getSegment (offset, sizeof (buffer) - 1, buffer))
		{
		buffer [length] = 0;
		fprintf (file, "%s", buffer);
		offset += length;
		}

	fclose (file);

	return true;
}

int BinaryBlob::getLine(int length, char * buffer)
{
	int l = Stream::getSegment (offset, length - 1, buffer, '\n');
	buffer [l] = 0;
	offset += l;

	return l;
}

void BinaryBlob::rewind()
{
	offset = 0;
}

bool BinaryBlob::loadFile(const char * fileName)
{
	FILE *file = fopen (fileName, "r");

	if (!file)
		return false;

	char buffer [256];

	while (fgets (buffer, sizeof (buffer), file))
		putSegment (strlen (buffer), buffer, true);

	fclose (file);

	return true;
}
***/
void BinaryBlob::getHexString(long pos, long length, void * address)
{
	if (!populated)
		populate();

	Stream::getSegmentToHexStr (pos, length, address);
}

void BinaryBlob::getBytes(long pos, long length, void * address)
{
	if (!populated)
		populate();

	Stream::getSegment (pos, length, address);
}

int BinaryBlob::length()
{
	if (!populated)
		populate();

	return totalLength;
}

void BinaryBlob::putSegment(int length, const char * data, bool copyFlag)
{
	Stream::putSegment (length, data, copyFlag);
}


int BinaryBlob::getSegmentLength(int pos)
{
	if (!populated)
		populate();

	return Stream::getSegmentLength (pos);
}

void* BinaryBlob::getSegment(int pos)
{
	return Stream::getSegment (pos);
}

int	BinaryBlob::getSegment (int offset, int length, void* address)
{
	return Stream::getSegment (offset,length,address);
}

void BinaryBlob::populate()
{
#ifdef ENGINE
	if (database)
		database->fetchRecord (sectionId, recordNumber, this);
#endif

	populated = true;
}

void BinaryBlob::putSegment(Blob * blob)
{
	Stream::putSegment (blob);
}
