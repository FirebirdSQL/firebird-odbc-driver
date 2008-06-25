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
 *  The Original Code was created by Vladimir Tsvigun for IBPhoenix.
 *
 *  Copyright (c) 2005 Vladimir Tsvigun
 *  All Rights Reserved.
 */

//  
// ListParamTransaction.h: interface for the ListParamTransaction class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_ListParamTransaction_H_)
#define _ListParamTransaction_H_

#include <string.h>
#include "Mlist.h"

#ifdef _WINDOWS
#ifndef strcasecmp

#if _MSC_VER >= 1400 // VC80 and later
#define strcasecmp		_stricmp
#define strncasecmp		_strnicmp
#else
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif // _MSC_VER >= 1400

#endif // strcasecmp
#endif

namespace IscDbcLibrary {

class CNodeParamTransaction
{
public:
	char		nameUnique[32]; // name DSN or similar
	int			lengthNameUnique;
	char		nameTransaction[32];
	int			lengthNameTransaction;
	char		*tpbBuffer;
	int			lengthTpbBuffer;
	bool		autoCommit;
	int			lockTimeout;

	CNodeParamTransaction()
	{
		tpbBuffer = NULL;
		remove();
	}
	~CNodeParamTransaction()
	{
		remove();
	}
	void setTpbBuffer( const char * tpbBuf, int lenTpbBuffer )
	{
		tpbBuffer = new char[lenTpbBuffer];
		memcpy( tpbBuffer, tpbBuf, lenTpbBuffer );
		lengthTpbBuffer = lenTpbBuffer;
	}
	void remove()
	{ 
		lengthNameUnique = 0;
		lengthNameTransaction = 0;
		delete[] tpbBuffer;
		tpbBuffer = NULL;
		lengthTpbBuffer = 0;
		autoCommit = false;
		lockTimeout = 0;
	}
	CNodeParamTransaction & operator =(const CNodeParamTransaction & src)
	{ 
		memcpy( nameUnique, src.nameUnique, src.lengthNameUnique );
		lengthNameUnique = src.lengthNameUnique;

		memcpy( nameTransaction, src.nameTransaction, src.lengthNameTransaction );
		lengthNameTransaction = src.lengthNameTransaction;

		delete[] tpbBuffer;
		lengthTpbBuffer = src.lengthTpbBuffer;
		tpbBuffer = new char[lengthTpbBuffer];
		memcpy( tpbBuffer, src.tpbBuffer, lengthTpbBuffer );
		autoCommit = src.autoCommit;
		lockTimeout = src.lockTimeout;

		return  *this;
	}
	int getMaxLengthName() { return 31; }
};

class CParamTransactionComparator
{
public:
	static int compare(const CNodeParamTransaction *a, const CNodeParamTransaction *b) 
	{
		if ( a->lengthNameUnique )
		{
			if ( a->lengthNameUnique < b->lengthNameUnique ) return -1;
			else if ( a->lengthNameUnique > b->lengthNameUnique ) return 1;
			int ret = strncasecmp ( a->nameUnique, b->nameUnique, a->lengthNameUnique );
			if ( ret )
				return ret;
		}

	    if ( a->lengthNameTransaction < b->lengthNameTransaction ) return -1;
	    else if ( a->lengthNameTransaction > b->lengthNameTransaction ) return 1;
	    return strncasecmp ( a->nameTransaction, b->nameTransaction, a->lengthNameTransaction );
	}
};

typedef MList<CNodeParamTransaction, CParamTransactionComparator> ListParamTransaction;

}; // end namespace IscDbcLibrary

#endif // !defined(_ListParamTransaction_H_)
