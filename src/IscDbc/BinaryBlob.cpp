/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

namespace IscDbcLibrary {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

BinaryBlob::BinaryBlob()
{
    useCount = 1;
    offset = 0;
    populated = true;
	directLength = 0;
}

BinaryBlob::BinaryBlob(int minSegmentSize) : Stream (minSegmentSize)
{
    useCount = 1;
    offset = 0;
    populated = true;
	directLength = 0;
}

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

void BinaryBlob::clear()
{
	Stream::clear();
    offset = 0;
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
void BinaryBlob::getBinary(int pos, int length, void * address)
{
	if (!populated)
		populate();

	offset += Stream::getSegmentToBinary (pos, length, address);
}

void BinaryBlob::getHexString(int pos, int length, void * address)
{
	if (!populated)
		populate();

	offset += Stream::getSegmentToHexStr (pos, length, address);
}

void BinaryBlob::getBytes(int pos, int length, void * address)
{
	if (!populated)
		populate();

	offset += Stream::getSegment (pos, length, address);
}

void BinaryBlob::getBytesW(int pos, int length, void * address)
{
	if (!populated)
		populate();

	offset += Stream::getSegmentW (pos, length, address);
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

void BinaryBlob::putLongSegment(int length, const char * data)
{
	while ( length >= DEFAULT_BLOB_BUFFER_LENGTH )
	{
		Stream::putSegment ( DEFAULT_BLOB_BUFFER_LENGTH, data, true );
		data += DEFAULT_BLOB_BUFFER_LENGTH;
		length -= DEFAULT_BLOB_BUFFER_LENGTH;
	}
	if (length)	Stream::putSegment (length, data, true);
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
	populated = true;
}

void BinaryBlob::putSegment(Blob * blob)
{
	Stream::putSegment (blob);
}

void BinaryBlob::attach(char * pointBlob, bool fetched, bool clear)
{

}

void BinaryBlob::bind(Statement *stmt, char * sqldata)
{

}

}; // end namespace IscDbcLibrary
