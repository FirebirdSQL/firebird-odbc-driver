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
 */

// AsciiBlob.cpp: implementation of the AsciiBlob class.
//
//////////////////////////////////////////////////////////////////////


#include "IscDbc.h"
#include "AsciiBlob.h"
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

AsciiBlob::AsciiBlob()
{
	useCount = 1;
	populated = true;
}


AsciiBlob::AsciiBlob(int minSegmentSize) : Stream (minSegmentSize)
{
	useCount = 1;
	populated = true;
}

AsciiBlob::AsciiBlob(Blob * blob)
{
	useCount = 1;
	populated = true;
	Stream::putSegment (blob);
}

#ifdef ENGINE
AsciiBlob::AsciiBlob(Database * db, long recNumber, long sectId)
{
	useCount = 1;
	populated = false;
	database = db;
	recordNumber = recNumber;
	sectionId = sectId;
}
#endif

AsciiBlob::~AsciiBlob()
{

}

void AsciiBlob::addRef()
{
	++useCount;
}

int AsciiBlob::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

int AsciiBlob::length()
{
	if (!populated)
		populate();

	return totalLength;
}

void AsciiBlob::getSubString(long pos, long length, char * address)
{
	if (!populated)
		populate();

	Stream::getSegment (pos, length, address);
}

void AsciiBlob::putSegment(int length, const char * data, bool copyFlag)
{
	Stream::putSegment (length, data, copyFlag);
}

int AsciiBlob::getSegmentLength(int pos)
{
	if (!populated)
		populate();

	return Stream::getSegmentLength (pos);
}

const char* AsciiBlob::getSegment(int pos)
{
	if (!populated)
		populate();

	return (const char*) Stream::getSegment (pos);
}

void AsciiBlob::populate()
{
#ifdef ENGINE
	if (database)
		database->fetchRecord (sectionId, recordNumber, this);
#endif

	populated = true;
}

void AsciiBlob::putSegment(const char *string)
{
	Stream::putSegment (string);
}
