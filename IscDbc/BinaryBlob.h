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

// BinaryBlob.h: interface for the BinaryBlob class.
//
//////////////////////////////////////////////////////////////////////

/*
 * copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.
 */


#if !defined(AFX_BINARYBLOB_H__74F68A11_3271_11D4_98E1_0000C01D2301__INCLUDED_)
#define AFX_BINARYBLOB_H__74F68A11_3271_11D4_98E1_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Blob.h"
#include "Stream.h"

namespace IscDbcLibrary
{

class Database;

class BinaryBlob : public Blob, public Stream 
{
public:
	void putSegment (Blob *blob);
#ifdef ENGINE
	 BinaryBlob (Database *db, long recordNumber, long sectId);
#endif
	virtual void* getSegment (int pos);
	virtual int	  getSegment (int offset, int length, void* address);
	virtual int getSegmentLength (int pos);
	void putSegment (int length, const char *data, bool copyFlag);
	int length();
	void getHexString(long pos, long length, void * address);
	void getBytes (long pos, long length, void *address);
	 BinaryBlob (int minSegmentSize);
	BinaryBlob();
	virtual ~BinaryBlob();
	virtual int release();
	virtual void addRef();
	void populate();

	int			useCount;
	int			offset;
	Database	*database;
	long		sectionId;
	long		recordNumber;
	bool		populated;
};

}; // end namespace IscDbcLibrary

#endif // !defined(AFX_BINARYBLOB_H__74F68A11_3271_11D4_98E1_0000C01D2301__INCLUDED_)
