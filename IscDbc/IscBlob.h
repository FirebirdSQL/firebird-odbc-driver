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

// IscBlob.h: interface for the IscBlob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCBLOB_H_)
#define _ISCBLOB_H_

#include "BinaryBlob.h"
#include "Connection.h"

namespace IscDbcLibrary {

class IscStatement;
class IscConnection;


class IscBlob : public BinaryBlob
{
public:
	void* getSegment (int pos);
	int getSegmentLength (int pos);
	char* getString();

	void bind(Statement *stmt, char * sqldata);
	void attach(char * pointBlob, bool fetched, bool clear);
	void setType(short sqlsubtype);
	void fetchBlob();
	int getSegment (int offset, int length, void *address);
	void writeBlob(char * sqldata);
	void writeStreamHexToBlob(char * sqldata);
	void writeBlob(char * sqldata, char *data, int length);
	void writeStringHexToBlob(char * sqldata, char *data, int length);
	int length();
	IscBlob();
	IscBlob(IscStatement *stmt, XSQLVAR *var);
	~IscBlob();

	void directCreateBlob( char * sqldata );
	void directOpenBlob(char * sqldata);
	bool directFetchBlob(char *data, int length, int &lengthRead);
	bool directGetSegmentToHexStr( char * bufData, int lenData, int &lenRead );
	void directWriteBlob( char *data, int length );
	void directCloseBlob();

	IscStatement	*statement;
	ISC_QUAD		blobId;
	isc_blob_handle directBlobHandle;
	bool			fetched;
	bool			directBlob;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_ISCBLOB_H_)
