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

// IscBlob.h: interface for the IscBlob class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ISCBLOB_H_)
#define _ISCBLOB_H_

#include "BinaryBlob.h"
#include "Connection.h"

class IscStatement;
class IscConnection;


class IscBlob : public BinaryBlob
{
public:
	virtual void* getSegment (int pos);
	virtual int getSegmentLength (int pos);
	virtual char* getString();

	void bind(Connection *connect, char * sqldata);
	void attach(char * pointBlob, bool fetched, bool clear);
	void setType(short sqlsubtype);
	void fetchBlob();
	virtual int getSegment (int offset, int length, void *address);
	virtual int length();
	IscBlob();
	IscBlob(IscConnection *connect, XSQLVAR *var);
	virtual ~IscBlob();

	IscConnection	*connection;
	ISC_QUAD		blobId;
	bool			fetched;
};

#endif // !defined(_ISCBLOB_H_)
