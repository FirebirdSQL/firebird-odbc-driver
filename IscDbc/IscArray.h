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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2003 Vladimir Tsvigun
 *  All Rights Reserved.
 */

//  
// IscArray.h: interface for the IscArray class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_IscArray_H_)
#define _IscArray_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "BinaryBlob.h"
#include "Connection.h"

namespace IscDbcLibrary {

class IscConnection;
class IscStatement;
class Value;

struct SIscArrayData
{
	void*			arrBufData;
	int				arrBufDataSize;
	int				arrCountElement;
	int				arrSizeElement;
	int				arrTypeElement;
};

class IscArray : public BinaryBlob
{
public:

	void attach(SIscArrayData * arr, bool bClear = false);
	void detach(SIscArrayData * arr);
	void removeBufData();
	void getBytesFromArray();
	void fetchArrayToString();
	void writeArray(Value * value);

	virtual void getBytes(long pos, long length, void * address);
	virtual int length();
	virtual int getSegment (int offset, int length, void *address);
	virtual int	getLength();

	IscArray(SIscArrayData * ptArr);
	IscArray(IscConnection	*parentConnection,XSQLVAR *var);
	~IscArray();

	IscConnection	*connection;
	ISC_QUAD		arrayId;
	bool			clear;
	bool			fetched;
	bool			fetchedBinary;
	ISC_ARRAY_DESC	arrDesc;
	void*			arrBufData;
	int				arrBufDataSize;
	int				arrCountElement;
	int				arrSizeElement;
	int				arrTypeElement;
};

}; // end namespace IscDbcLibrary

#endif // !defined(_IscArray_H_)
