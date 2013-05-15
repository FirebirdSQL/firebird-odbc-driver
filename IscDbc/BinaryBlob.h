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

// BinaryBlob.h: interface for the BinaryBlob class.
//
//////////////////////////////////////////////////////////////////////

/*
 * copyright (c) 1999 - 2000 by James A. Starkey for IBPhoenix.
 */


#if !defined(_BINARYBLOB_H_)
#define _BINARYBLOB_H_

#include "Blob.h"
#include "Stream.h"

namespace IscDbcLibrary {

class Database;

class BinaryBlob : public Blob, public Stream 
{
public:
	void putSegment (Blob *blob);
	virtual void* getSegment (int pos);
	virtual int	  getSegment (int offset, int length, void* address);
	virtual int	  getSegmentLength (int pos);
	virtual void  writeBlob(char * sqldata) {};
	virtual void  writeStreamHexToBlob(char * sqldata) {};
	virtual void  writeBlob(char * sqldata, char *data, int length) {};
	virtual void  writeStringHexToBlob(char * sqldata, char *data, int length) {};
	virtual void  directCreateBlob( char * sqldata ) {};
	virtual void  directOpenBlob(char * sqldata ) {};
	virtual bool  directFetchBlob(char *data, int length, int &lengthRead) { return false; }
	virtual bool  directGetSegmentToHexStr( char * bufData, int lenData, int &lenRead ) { return false; }
	virtual void  directWriteBlob( char *data, int length ) {};
	virtual void  directCloseBlob() {};
	virtual int	  getOffset() { return offset; }
	void putSegment (int length, const char *data, bool copyFlag);
	void putLongSegment(int length, const char * data);
	int length();
	void getHexString(int pos, int length, void * address);
	void getBytes (int pos, int length, void *address);
	void getBytesW (int pos, int length, void *address);
	void getBinary (int pos, int length, void * address);
	BinaryBlob (int minSegmentSize);
	BinaryBlob();
	virtual ~BinaryBlob();
	virtual int release();
	virtual void clear();
	virtual void addRef();
	void populate();
	virtual void bind(Statement *stmt, char * sqldata);
	void attach(char * pointBlob, bool fetched, bool clear);
	bool isBlob(){ return enType == enTypeBlob; }
	bool isClob(){ return enType == enTypeClob; }
	bool isArray(){ return enType == enTypeArray; }

	int			useCount;
	int			offset;
	Database	*database;
	int			sectionId;
	int			recordNumber;
	bool		populated;
	int			directLength;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_BINARYBLOB_H_)
