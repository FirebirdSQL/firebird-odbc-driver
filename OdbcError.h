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

// OdbcError.h: interface for the OdbcError class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ODBCERROR_H__C19738BE_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ODBCERROR_H__C19738BE_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscDbc/JString.h"

class OdbcError  
{
public:
	void setColumnNumber (int column, int row);
	void setRowNumber (int number);
	RETCODE sqlGetDiagField (int diagId, SQLPOINTER ptr, int bufferLength, SQLSMALLINT *stringLength);
	RETCODE sqlGetDiagRec (UCHAR *stateBuffer, SDWORD *nativeCode, UCHAR *msgBuffer, int msgBufferLength, SWORD *msgLength);
	OdbcError(int code, const char *state, JString errorMsg);
	virtual ~OdbcError();

	OdbcError		*next;
	char			sqlState [128];
	JString			msg;
	int				nativeCode;
	int				rowNumber;
	int				columnNumber;
};

#endif // !defined(AFX_ODBCERROR_H__C19738BE_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
