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

// AsciiBlob.h: interface for the AsciiBlob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ASCIIBLOB_H__74F68A12_3271_11D4_98E1_0000C01D2301__INCLUDED_)
#define AFX_ASCIIBLOB_H__74F68A12_3271_11D4_98E1_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Blob.h"
#include "Stream.h"

class Database;


class AsciiBlob : public Clob, public Stream
{
public:
	virtual void putSegment (const char *string);
	AsciiBlob (int minSegmentSize);
	void populate();
	 AsciiBlob(Database * db, long recNumber, long sectId);
	virtual const char* getSegment (int pos);
	virtual int getSegmentLength (int pos);
	AsciiBlob (Blob *blob);
	virtual void putSegment (int length, const char *data, bool copyFlag);
	virtual void getSubString (long pos, long length, char *buffer);
	virtual int length();
	virtual int release();
	virtual void addRef();
	AsciiBlob();
	virtual ~AsciiBlob();

	int		useCount;
	Database	*database;
	long		sectionId;
	long		recordNumber;
	bool		populated;
};

#endif // !defined(AFX_ASCIIBLOB_H__74F68A12_3271_11D4_98E1_0000C01D2301__INCLUDED_)
