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
 */


// Blob.cpp: implementation of the Blob class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "Blob.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/***
Blob::Blob()
{
	useCount = 1;
	minSegment = 0;
	offset = 0;
}

Blob::Blob(int minSegmentSize) : Stream (minSegmentSize)
{
	useCount = 1;
	minSegment = 0;
	offset = 0;
}

Blob::~Blob()
{

}

void Blob::addRef()
{
	++useCount;
}

void Blob::release()
{
	ASSERT (useCount > 0);

	if (--useCount == 0)
		delete this;
}


bool Blob::write(const char * filename)
{
	FILE *file = fopen (filename, "w");

	if (!file)
		return false;

	long offset = 0;
	int length;
	char buffer [1024];

	while (length = getSegment (offset, sizeof (buffer) - 1, buffer))
		{
		buffer [length] = 0;
		fprintf (file, "%s", buffer);
		offset += length;
		}

	fclose (file);

	return true;
}

int Blob::getLine(int length, char * buffer)
{
	int l = getSegment (offset, length - 1, buffer, '\n');
	buffer [l] = 0;
	offset += l;

	return l;
}

void Blob::rewind()
{
	offset = 0;
}

bool Blob::loadFile(const char * fileName)
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
